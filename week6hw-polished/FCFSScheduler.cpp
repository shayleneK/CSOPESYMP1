#include "FCFSScheduler.h"
#include "Process.h"
#include "ProcessFactory.h"
#include "Command.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

FCFSScheduler::FCFSScheduler(int num_cores, int min_ins, int max_ins)
    : Scheduler(num_cores, min_ins, max_ins)
{
    std::cout << "[FCFS DEBUG] Constructor received min_ins=" << min_ins
              << ", max_ins=" << max_ins << std::endl;
}

FCFSScheduler::~FCFSScheduler() { stop_scheduler(); }

void FCFSScheduler::start()
{
    if (generating_processes.load())
        return;

    generating_processes.store(true);
    running = true;

    generator_thread = std::thread([this]()
                                   {
        int cycle_counter = 0;
        const int batch_process_frequency = 100;

        while (generating_processes.load() && running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (++cycle_counter >= batch_process_frequency)
            {
                std::ostringstream oss;
                oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
                std::string name = oss.str();

                auto process = ProcessFactory::generate_dummy_process(name, min_instructions, max_instructions);
                process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));
                add_process(process);

                cycle_counter = 0;
            }
        } });
}

void FCFSScheduler::start_process_generator()
{
    start(); // Same behavior now
}

void FCFSScheduler::stop_scheduler()
{
    generating_processes = false;
    if (generator_thread.joinable())
    {
        generator_thread.join();
    }
}

bool FCFSScheduler::is_scheduler_running() const
{
    return generating_processes;
}

void FCFSScheduler::run_core(int core_id)
{
    while (running)
    {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_condition.wait(lock, [this]
                                 { return !running || !ready_queue.empty(); });

            if (!running)
                break;

            if (!ready_queue.empty())
            {
                process = ready_queue.front();
                ready_queue.pop();
                core_available[core_id] = false;
            }
            else
            {
                continue;
            }
        }

        if (process)
        {
            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes[core_id] = process;
            }

            while (!process->is_finished)
            {
                auto start = std::chrono::high_resolution_clock::now();
                process->execute(core_id);
                auto end = std::chrono::high_resolution_clock::now();

                int duration = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    core_util_time[core_id] += duration;
                    core_process_count[core_id]++;
                    total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);
                }
            }

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                core_available[core_id] = true;
            }

            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes.erase(core_id);
                std::cout << "[FCFS] Process " << process->name << " finished on core " << core_id << std::endl;
            }
        }
    }
}

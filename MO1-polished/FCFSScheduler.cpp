#include "FCFSScheduler.h"
#include "Process.h"
#include "ProcessFactory.h"
#include "ConsoleManager.h"
#include "ScreenConsole.h"
#include "Command.h"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

FCFSScheduler::FCFSScheduler(int num_cores, int min_ins, int max_ins)
    : Scheduler(num_cores, min_ins, max_ins)
{
    //std::cout << "[FCFS DEBUG] Constructor received min_ins=" << min_ins
      //        << ", max_ins=" << max_ins << std::endl;
}

FCFSScheduler::~FCFSScheduler()
{
    shutdown();
}

void FCFSScheduler::start()
{
    generating_processes.store(true);
    running = true;
   // std::cout << "[FCFS DEBUG] Scheduler started (CPU ticks will drive generation)\n";
}

void FCFSScheduler::start_process_generator()
{
    start();
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
    return generating_processes.load();
}

void FCFSScheduler::run_core(int core_id)
{
    //std::cout << "[FCFS][Core " << core_id << "] Core thread started.\n";

    while (running)
    {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_condition.wait(lock, [this]
                                 { return !running || !ready_queue.empty(); });

            if (!running)
            {
               // std::cout << "[FCFS][Core " << core_id << "] Immediate shutdown triggered.\n";
                break;
            }

            if (!ready_queue.empty())
            {
                process = ready_queue.front();
                ready_queue.pop();
                core_available[core_id] = false;

                //std::cout << "[FCFS][Core " << core_id << "] Picked process "
                    //      << process->getName() << " from ready queue.\n";
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
                process_to_core[process] = core_id;
            }

            auto start = std::chrono::high_resolution_clock::now();

            while (!process->isFinished())
            {
                if (!running)
                {
                   // std::cout << "[FCFS][Core " << core_id << "] Immediate shutdown triggered inside loop.\n";
                    break;
                }

                if (process->can_execute())
                {
                    process->execute(core_id);
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            int duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                core_util_time[core_id] += duration_ms;
                core_process_count[core_id]++;
                total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);
                core_available[core_id] = true;
            }

            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes.erase(core_id);
                process_to_core.erase(process);
               // std::cout << "[FCFS][Core " << core_id << "] Process " << process->getName()
                      //    << " finished and removed from running list.\n";
            }
        }
    }

    // std::cout << "[FCFS][Core " << core_id << "] Core thread exiting.\n";
}

void FCFSScheduler::on_cpu_cycle(uint64_t cycle_number)
{
    if (!generating_processes.load())
        return;

    if (cycle_number % batch_process_freq == 0)
    {
        // std::cout << "[FCFS DEBUG] on_cpu_cycle(" << cycle_number << ") -> generating process\n";
        generate_new_process();
    }
}

void FCFSScheduler::start_core_threads()
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        cpu_cores.emplace_back(&FCFSScheduler::run_core, this, i);
    }
}

void FCFSScheduler::generate_new_process()
{
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
    std::string name = oss.str();

    auto process = ProcessFactory::generate_dummy_process(name, min_instructions, max_instructions);
    process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));

    add_process(process);

    ConsoleManager *cm = ConsoleManager::getInstance();
    if (!cm->hasConsole(name))
    {
        cm->createConsole("screen", name);
    }

    auto screen = std::dynamic_pointer_cast<ScreenConsole>(cm->getConsoleByName(name));
    if (screen)
    {
        screen->attachProcess(process);
    }
    else
    {
       // std::cout << "[FCFS DEBUG] Failed to attach process to screen console: " << name << "\n";
    }

    //std::cout << "[FCFS DEBUG] New process " << name << " created at tick "
          //    << ConsoleManager::getCpuCycles() << "\n";
}

#include "RRScheduler.h"
#include "Process.h"
#include "ProcessFactory.h"
#include "ConsoleManager.h"
#include "ScreenConsole.h"
#include "Command.h"
#include "ConsoleManager.h"

#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

RRScheduler::RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins, int delay_per_exec)
    : Scheduler(num_cores, min_ins, max_ins), time_quantum(quantum_ms)
{
}

RRScheduler::~RRScheduler()
{
    shutdown();
}

void RRScheduler::start()
{
    if (generating_processes.load())
        return;

    generating_processes.store(true);
    running = true;

    generator_thread = std::thread([this]()
                                   {
        int cycle_counter = 0;

        while (generating_processes.load() && running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (++cycle_counter >= batch_process_freq)
            {
                generate_new_process();
                cycle_counter = 0;
            }
        } });
}

void RRScheduler::generate_new_process()
{
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
    std::string name = oss.str();

    auto process = ProcessFactory::generate_dummy_process(name, min_instructions, max_instructions);
    process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));
    add_process(process);

    ConsoleManager::getInstance()->createConsole("screen", name);

    auto screen = std::dynamic_pointer_cast<ScreenConsole>(
        ConsoleManager::getInstance()->getConsoleByName(name));
    if (screen)
    {
        screen->attachProcess(process);
    }
}

void RRScheduler::start_process_generator()
{
    start(); // Same behavior now
}

void RRScheduler::start_core_threads()
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        cpu_cores.emplace_back(&RRScheduler::run_core, this, i);
    }
}

void RRScheduler::stop_scheduler()
{
    generating_processes = false;
    if (generator_thread.joinable())
    {
        generator_thread.join();
    }
}

bool RRScheduler::is_scheduler_running() const
{
    return generating_processes;
}

void RRScheduler::run_core(int core_id)
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

                // std::cout << "[RR][Core " << core_id << "] Picked process " << process->getName()
                //     << " from ready queue.\n";
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

            int cpu_ticks_exec = 0;
            int max_cpu_ticks = time_quantum;

            // std::cout << "[RR][Core " << core_id << "] Executing up to " << max_cpu_ticks
            //       << " commands for process " << process->getName() << ".\n";

            auto start = std::chrono::high_resolution_clock::now();

            while (cpu_ticks_exec < max_cpu_ticks && !process->isFinished())
            {
                if (!running)
                {
                    // std::cout << "[RR][Core " << core_id << "] Immediate shutdown triggered.\n";
                    break;
                }

                if (process->can_execute())
                {
                    process->execute(core_id);
                    cpu_ticks_exec++;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            int duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                core_util_time[core_id] += duration_ms;
                core_process_count[core_id]++;
                total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);

                if (!process->isFinished())
                {
                    process_to_core.erase(process);
                    ready_queue.push(process);
                    // std::cout << "[RR][Core " << core_id << "] Preempting process " << process->getName()
                    //        << " after " << cpu_ticks_exec << " CPU ticks.\n";
                }

                core_available[core_id] = true;
            }

            {
                std::unique_lock<std::mutex> lock(running_mutex);
                if (process->isFinished())
                {
                    current_processes.erase(core_id);
                    process_to_core.erase(process);
                    // std::cout << "[RR][Core " << core_id << "] Process " << process->getName()
                    //     << " finished and removed from running list.\n";
                }
            }
        }
    }

    // std::cout << "[RR][Core " << core_id << "] Core thread exiting.\n";
}

std::vector<std::shared_ptr<Process>> RRScheduler::get_running_processes()
{
    std::vector<std::shared_ptr<Process>> result;

    std::unique_lock<std::mutex> lock(running_mutex);
    for (const auto &entry : current_processes)
    {
        result.push_back(entry.second);
    }

    return result;
}

void RRScheduler::on_cpu_cycle(uint64_t cycle_number)
{
    if (!generating_processes.load())
        return;

    if (cycle_number % batch_process_freq == 0)
    {
        generate_new_process();
    }
}
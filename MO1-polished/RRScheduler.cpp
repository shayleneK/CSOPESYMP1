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
#include <fstream>

RRScheduler::RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins, int delay_per_exec, MemoryManager *memory_manager, int mem_per_proc)
    : Scheduler(num_cores, min_ins, max_ins, memory_manager),
      time_quantum(quantum_ms),
      delay_per_execution(delay_per_exec),
      mem_per_process(mem_per_proc)
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
        int quantum_counter = 0;

        while (generating_processes.load() && running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (++cycle_counter >= batch_process_freq)
            {
                generate_new_process();
                cycle_counter = 0;
                quantum_counter ++;
                static std::atomic<uint64_t> snapshot_id{1};
                save_memory_snapshot(snapshot_id++);
            }
        } });
}

void RRScheduler::generate_new_process()
{
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
    std::string name = oss.str();

    auto process = ProcessFactory::generate_dummy_process(name, min_instructions, max_instructions, mem_per_process);
    std::cout << "[RR] Created process: " << name
              << " (requires " << mem_per_process << " KB memory)\n";

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
                // std::cout << "[RR][Core " << core_id << "] Executing process "
                //           << process->getName() << " at command index "
                //           << process->getCurrentCommandIndex() << std::endl;

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

                    if (memory_manager_)
                    {
                        memory_manager_->deallocate(process->getName());
                    }
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

void RRScheduler::save_memory_snapshot(uint64_t batch_number)
{
    if (!memory_manager_)
    {
        std::cerr << "[RR] Cannot save memory snapshot: memory manager is null.\n";
        return;
    }

    // Step 1: Generate the filename
    std::ostringstream filename;
    filename << "memory_stamp_" << batch_number << ".txt";

    // Step 2: Open the file for writing
    std::ofstream file(filename.str());
    if (!file.is_open())
    {
        std::cerr << "[RR] Failed to open " << filename.str() << " for writing.\n";
        return;
    }

    // Step 3: Write the timestamp in the desired format
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);
    file << "Timestamp: (";
    file << std::put_time(local_time, "%m/%d/%Y %I:%M:%S%p");
    file << ")\n";

    // Step 4: Write the number of processes in memory (you could track this yourself or ask the memory manager)
    size_t in_memory = memory_manager_->countAllocatedProcesses(); // <- implement this in MemoryManager
    file << "Number of processes in memory: " << in_memory << "\n";

    // Step 5: Write the total external fragmentation
    file << "Total external fragmentation in KB: " << memory_manager_->getExternalFragmentation() << "\n";

    // Step 6: Write the memory layout
    file << memory_manager_->printMemoryLayout() << "\n";

    // Step 7: Close the file and log the success
    file.close();
    std::cout << "[RR] Saved memory snapshot to " << filename.str() << std::endl;
}

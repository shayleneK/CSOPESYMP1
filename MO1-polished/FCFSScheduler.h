#include "RRScheduler.h"
#include "Process.h"
#include "ProcessFactory.h"
#include "ConsoleManager.h"
#include "ScreenConsole.h"
#include "Command.h"

#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <algorithm>

/*
RRScheduler::RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins,
                         int delay_per_exec, int mem_per_proc, MemoryManager &mem_mgr)
    : Scheduler(num_cores, min_ins, max_ins, mem_per_proc),
      time_quantum(quantum_ms),
      memory_manager_(mem_mgr)
{
    process_memory_map_.clear();
}
*/
RRScheduler::RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins, int delay_per_exec)
    : Scheduler(num_cores, min_ins, max_ins)  
{
    this->delay_per_exec = delay_per_exec;
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
        int batch_counter = 0;

        while (generating_processes.load() && running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (++cycle_counter >= batch_process_freq)
            {
                generate_new_process();
                cycle_counter = 0;
                save_memory_snapshot(batch_counter);
                batch_counter++;

            }
        } });
}

/*
void RRScheduler::generate_new_process()
{
    std::ostringstream oss;
    std::cout << "[RR] Creating process with mem_per_proc = " << mem_per_proc << "\n";

    oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
    std::string name = oss.str();

    size_t mem_required = mem_per_proc;

    auto process = ProcessFactory::generate_dummy_process(name, mem_required, min_instructions, max_instructions);
    process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));

    int start_address = memory_manager_.allocate(mem_required, process->name);

    if (start_address != -1)
    {
        process->loadToMemory(start_address);
        add_process(process);
        process_memory_map_.push_back(process->name);

        ConsoleManager::getInstance()->createConsole("screen", name);

        auto screen = std::dynamic_pointer_cast<ScreenConsole>(
            ConsoleManager::getInstance()->getConsoleByName(name));
        if (screen)
        {
            screen->attachProcess(process);
        }

        std::cout << "[RR] Process " << name << " allocated at ["
                  << start_address << "-" << start_address + mem_required - 1 << "]" << std::endl;
    }
    else
    {
        std::cout << "[RR] Process " << name << " could not be loaded into memory. Re-queued." << std::endl;
    }
}
*/
void RRScheduler::add_process(std::shared_ptr<Process> proc)
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    ready_queue.push(proc);
    all_processes.push_back(proc);
    queue_condition.notify_one();
}

void RRScheduler::generate_new_process() {
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
    std::string name = oss.str();

    size_t mem_required = mem_per_proc;

    auto process = ProcessFactory::generate_dummy_process(name, mem_required, min_instructions, max_instructions);
    process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));

    if (memoryManager && memoryManager->allocate(mem_required, name)) {
        add_process(process);

        ConsoleManager::getInstance()->createConsole("screen", name);
        auto screen = std::dynamic_pointer_cast<ScreenConsole>(
            ConsoleManager::getInstance()->getConsoleByName(name));
        if (screen) {
            screen->attachProcess(process);
        }

        std::cout << "[RR] Process " << name << " loaded into memory.\n";
    } else {
        std::cout << "[RR] Process " << name << " could not be allocated — re-queued.\n";
    }
}

void RRScheduler::start_process_generator()
{
    start();
}

/*
void RRScheduler::start_core_threads()
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        cpu_cores.emplace_back(&RRScheduler::run_core, this, i);
    }
}
*/

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
/*
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

            auto start = std::chrono::high_resolution_clock::now();

            while (cpu_ticks_exec < max_cpu_ticks && !process->isFinished())
            {
                if (!running)
                    break;

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
                }

                core_available[core_id] = true;
            }

            {
                std::unique_lock<std::mutex> lock(running_mutex);
                if (process->isFinished())
                {
                    current_processes.erase(core_id);
                    process_to_core.erase(process);

                    memory_manager_.deallocate(process->name);
                    process_memory_map_.erase(
                        std::remove(process_memory_map_.begin(), process_memory_map_.end(), process->name),
                        process_memory_map_.end());

                    std::cout << "[RR][Core " << core_id << "] Process " << process->getName()
                              << " finished and memory released." << std::endl;
                }
            }
        }
    }
}
*/

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

//void RRScheduler::on_cpu_cycle(uint64_t cycle_number)
//{
    // if (!generating_processes.load())
    //     return;

    // if (cycle_number % batch_process_freq == 0)
    // {
    //     generate_new_process();

    //     static uint64_t cycle_counter = 0;
    //     if (++cycle_counter % time_quantum == 0)
    //     {
    //         save_memory_snapshot(cycle_counter);
    //     }
    // }
//}
void RRScheduler::on_cpu_cycle(uint64_t cycle_number)
{
    if (!generating_processes.load())
        return;

    // Generate new process every batch_process_freq cycles
    if (cycle_number % batch_process_freq == 0)
    {
        generate_new_process();
    }

    // Save memory snapshot every quantum cycle
    if (cycle_number % time_quantum == 0)
    {
        static uint64_t snapshot_counter = 0;
        save_memory_snapshot(snapshot_counter++);
    }
}
/*
void RRScheduler::save_memory_snapshot(uint64_t batch_number)
{
    // Step 1: Generate the filename
    std::ostringstream filename;
    filename << "memory_stamp_" << batch_number << ".txt";

    // Step 2: Open the file for writing
    std::ofstream file(filename.str());
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << filename.str() << std::endl;
        return;
    }

    // Step 3: Write the timestamp in the desired format
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);
    file << "Timestamp: (";
    file << std::put_time(local_time, "%m/%d/%Y %I:%M:%S%p");
    file << ")\n";

    // Step 4: Write the number of processes in memory
    file << "Number of processes in memory: " << process_memory_map_.size() << "\n";

    
    // Step 5: Write the total external fragmentation
    file << "Total external fragmentation in KB: " << memory_manager_.getExternalFragmentation() << "\n";

    // Step 6: Write the memory layout
    file << memory_manager_.printMemoryLayout() << "\n";
    
   if (memoryManager) {
        file << "Total external fragmentation in KB: " << memoryManager->getExternalFragmentation() << "\n";
        file << memoryManager->printMemoryLayout() << "\n";
    } else {
        file << "MemoryManager not initialized.\n";
    }

    // Step 7: Close the file and log the success
    file.close();
    std::cout << "[RR] Saved memory snapshot to " << filename.str() << std::endl;
}
*/
void RRScheduler::save_memory_snapshot(uint64_t batch_number) {
    std::ostringstream filename;
    filename << "memory_stamp_" << batch_number << ".txt";

    std::ofstream file(filename.str());
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename.str() << std::endl;
        return;
    }

    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);
    file << "Timestamp: (" << std::put_time(local_time, "%m/%d/%Y %I:%M:%S%p") << ")\n";

    file << "Number of processes in memory: ";
    if (memoryManager) {
        // You can add a method to count allocated processes
        int count = 0;
        // Or leave it as placeholder
        file << "N/A\n";
    } else {
        file << "0\n";
    }

    if (memoryManager) {
        file << "Total external fragmentation in KB: " << memoryManager->getExternalFragmentation() << "\n";
        file << memoryManager->printMemoryLayout() << "\n";
    } else {
        file << "MemoryManager not initialized.\n";
    }

    file.close();
    std::cout << "[RR] Saved memory snapshot to " << filename.str() << std::endl;
}

void RRScheduler::notify_process_started(int core_id, std::shared_ptr<Process> process)
{
    // You can add logging or bookkeeping here
    std::cout << "[RR] Process " << process->getName() << " started on core " << core_id << "\n";
}

void RRScheduler::notify_process_finished(int core_id, std::shared_ptr<Process> process, int duration_ms)
{
    // Add any cleanup or logging you need
    std::cout << "[RR] Process " << process->getName() << " finished on core " << core_id
              << " (duration: " << duration_ms << " ms)\n";
}

int RRScheduler::get_quantum() const
{
    return time_quantum; // assuming you have a member `quantum` in RRScheduler
}
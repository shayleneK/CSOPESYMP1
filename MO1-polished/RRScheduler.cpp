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

RRScheduler::RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins,
                         int delay_per_exec, int mem_per_proc, MemoryManager &mem_mgr)
    : Scheduler(num_cores, min_ins, max_ins, mem_per_proc),
      time_quantum(quantum_ms),
      memory_manager_(mem_mgr),
      cpuCycleManager([this](uint64_t cycle) { this->on_cpu_cycle(cycle); }, 1000)
{
    process_memory_map_.clear();
}

RRScheduler::~RRScheduler()
{
    shutdown();
}

void RRScheduler::start() {
    if (generating_processes.load()) return;

    generating_processes.store(true);
    running = true;
    cpuCycleManager.start();  // 🔁 use new manager
}

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

void RRScheduler::start_process_generator()
{
    start();
}

void RRScheduler::start_core_threads()
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        cpu_cores.emplace_back(&RRScheduler::run_core, this, i);
    }
}

void RRScheduler::shutdown() {
    generating_processes = false;
    running = false;
    cpuCycleManager.stop();   // Stop the CPU cycle thread

    stop_scheduler(); // Custom cleanup logic (if any)

    queue_condition.notify_all(); // Wake up any waiting threads

    for (auto& t : cpu_cores) {
        if (t.joinable())
            t.join(); // Wait for threads to exit cleanly
    }
}

void RRScheduler::stop_scheduler()
{
    generating_processes = false;
    cpuCycleManager.stop();  // stop periodic generation
}

bool RRScheduler::is_scheduler_running() const
{
    return generating_processes;
}

void RRScheduler::run_core(int core_id) {
    while (ConsoleManager::getInstance()->isRunning()) {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if (!ready_queue.empty()) {
            auto process = ready_queue.front();
            ready_queue.pop();
            lock.unlock(); // Unlock during execution

            if (!process->isInMemory()) {
                size_t addr = memory_manager_.allocate(process->getMemoryRequired(), process->getName());
                if (addr != SIZE_MAX && addr != static_cast<size_t>(-1)) {
                    process->loadToMemory(addr);
                } else {
                    process->log_execution(core_id, "Memory full. Re-queueing process: " + process->getName());
                    lock.lock();
                    ready_queue.push(process);
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
            }

            process->execute(core_id);

            if (process->isFinished()) {
                memory_manager_.deallocate(process->getName());
                process->releaseFromMemory();
                notify_process_finished(core_id, process, time_quantum);
            } else {
                lock.lock();
                ready_queue.push(process);
                lock.unlock();
            }
        } else {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
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
}

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

    // Step 7: Close the file and log the success
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
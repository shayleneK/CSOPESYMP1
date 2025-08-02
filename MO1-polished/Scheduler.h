#pragma once

#include "Process.h"
#include "CPUCore.h"  // Ensure CPUCore.h is included
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

// Forward declaration
class MemoryManager;

class Scheduler
{
public:
    // Constructor
    Scheduler(int num_cores, int min_instructions, int max_instructions);
    virtual ~Scheduler();

    // Core methods
    //virtual void run_core(int core_id);
    virtual void start_core_threads();
    virtual void start();
    virtual void shutdown();

    // Process management
    virtual std::vector<std::shared_ptr<Process>> get_running_processes();
    std::vector<std::shared_ptr<Process>> get_finished_processes();
    std::vector<std::shared_ptr<Process>> get_all_processes();
    int get_core_of_process(const std::shared_ptr<Process>& p);
    virtual void start_process_generator();
    std::map<int, std::map<std::string, float>> get_cpu_stats();

    // Getters
    int get_min_instructions() const { return min_instructions; }
    int get_max_instructions() const { return max_instructions; }

    bool is_done();
    std::shared_ptr<Process> find_process_by_name(const std::string& name);

    // Pure virtual (must be implemented by derived classes)
    virtual void on_cpu_cycle(uint64_t cycle_number) = 0;
    virtual void set_batch_frequency(int freq) { batch_process_freq = freq; }
    virtual bool is_scheduler_running() const = 0;
    virtual void stop_scheduler();

    // CPUCore objects
    std::vector<std::shared_ptr<CPUCore>> cpu_core_objects;

    // Memory manager
    void set_memory_manager(MemoryManager* mem) { this->memoryManager = mem; }

    // Add process (pure virtual)
    virtual void add_process(std::shared_ptr<Process> proc) = 0;
    void set_mem_per_proc(size_t size) { this->mem_per_proc = size; }
protected:
    std::vector<bool> core_available;
    std::queue<std::shared_ptr<Process>> ready_queue;
    std::vector<std::shared_ptr<Process>> all_processes;
    std::map<int, std::shared_ptr<Process>> current_processes;
    std::map<std::shared_ptr<Process>, int> process_to_core;
    std::mutex running_mutex;
    int next_pid = 0;

    std::map<int, int> core_process_count;
    std::map<int, int> core_util_time;
    int total_cpu_time = 0;
    bool running = true;
    std::map<int, uint64_t> total_ticks_per_core;
    std::map<int, uint64_t> busy_ticks_per_core;

    std::mutex queue_mutex;
    std::condition_variable queue_condition;

    std::thread generator_thread;
    std::atomic<bool> generating_processes{false};
    std::atomic<bool> global_shutdown{false};

    // Config
    int min_instructions;
    int max_instructions;
    int batch_process_freq;

    std::atomic<uint64_t> cpu_cycles;

    // Memory
    MemoryManager* memoryManager = nullptr;
    size_t mem_per_proc = 4096; // Default 4KB

    int delay_per_exec = 0;
};
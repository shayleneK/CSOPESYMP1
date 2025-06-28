#pragma once

#include "Process.h"
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class Scheduler
{
public:
    // === Lifecycle & Initialization ===
    Scheduler(int num_cores, int min_instructions, int max_instructions);
    virtual ~Scheduler();

    // === Core Execution Methods ===
    virtual void run_core(int core_id);
    virtual void start_core_threads();
    virtual void start();

    // === Process Management ===
    void add_process(std::shared_ptr<Process> process);

    virtual std::vector<std::shared_ptr<Process>> get_running_processes();
    std::vector<std::shared_ptr<Process>> get_finished_processes();
    std::vector<std::shared_ptr<Process>> get_all_processes();

    // === Scheduler Control ===
    virtual void start_process_generator();
    virtual void stop_scheduler();
    virtual bool is_scheduler_running() const = 0;
    void shutdown();

    // === CPU Stats / Utilization ===
    std::map<int, std::map<std::string, float>> get_cpu_stats();

    // === Configuration Accessors ===
    int get_min_instructions() const { return min_instructions; }
    int get_max_instructions() const { return max_instructions; }
    virtual void set_batch_frequency(int freq) { batch_process_freq = freq; }

    // === Utility & Query Methods ===
    bool is_done();
    std::shared_ptr<Process> find_process_by_name(const std::string &name);

    // === Pure Virtual Interface (To Be Implemented by Subclasses) ===
    virtual void on_cpu_cycle(uint64_t cycle_number) = 0;

protected:
    // === Core Resources ===
    std::vector<bool> core_available;
    std::queue<std::shared_ptr<Process>> ready_queue;
    std::vector<std::shared_ptr<Process>> all_processes;
    std::map<int, std::shared_ptr<Process>> current_processes; // core_id -> Process

    // === Process Generation ===
    std::thread generator_thread;
    std::atomic<bool> generating_processes{false};

    // === Synchronization Primitives ===
    mutable std::mutex running_mutex;
    std::mutex queue_mutex;
    std::condition_variable queue_condition;

    // === Process IDs & Counters ===
    int next_pid = 0;

    // === CPU Statistics Tracking ===
    std::map<int, int> core_process_count;
    std::map<int, int> core_util_time;
    std::map<int, uint64_t> total_ticks_per_core;
    std::map<int, uint64_t> busy_ticks_per_core;
    int total_cpu_time = 0;

    // === Global State ===
    bool running = true;
    std::atomic<uint64_t> cpu_cycles; // Shared CPU cycle counter
    std::atomic<bool> global_shutdown{false};

    // === Configurable Parameters ===
    int min_instructions;
    int max_instructions;
    int batch_process_freq;

    // === Core Threads ===
    std::vector<std::thread> cpu_cores;
};
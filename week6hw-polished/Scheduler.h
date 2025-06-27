// Scheduler.h
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
    Scheduler(int num_cores, int min_instructions, int max_instructions);
    virtual ~Scheduler();

    virtual void run_core(int core_id);

    virtual void start_core_threads();
    virtual void start();

    void add_process(std::shared_ptr<Process> process);
    void shutdown();

    virtual std::vector<std::shared_ptr<Process>> get_running_processes();
    std::vector<std::shared_ptr<Process>> get_finished_processes();
    std::vector<std::shared_ptr<Process>> get_all_processes();
    virtual void start_process_generator();
    std::map<int, std::map<std::string, float>> get_cpu_stats();

    int get_min_instructions() const { return min_instructions; }
    int get_max_instructions() const { return max_instructions; }

    bool is_done();
    std::shared_ptr<Process> find_process_by_name(const std::string &name);

    virtual void on_cpu_cycle(uint64_t cycle_number) = 0;
    virtual void set_batch_frequency(int freq) { batch_process_freq = freq; }

    virtual bool is_scheduler_running() const = 0;

    virtual void stop_scheduler();

protected:
    std::vector<bool> core_available;
    std::queue<std::shared_ptr<Process>> ready_queue;
    std::vector<std::shared_ptr<Process>> all_processes;
    std::map<int, std::shared_ptr<Process>> current_processes; // core_id -> Process
    std::mutex running_mutex;
    int next_pid = 0;

    std::map<int, int> core_process_count;
    std::map<int, int> core_util_time;
    int total_cpu_time = 0;
    bool running = true;

    std::mutex queue_mutex;
    std::condition_variable queue_condition;

    std::thread generator_thread;
    std::atomic<bool> generating_processes{false};

    // config
    std::vector<std::thread> cpu_cores;
    int min_instructions;
    int max_instructions;

    std::atomic<uint64_t> cpu_cycles; // Shared CPU cycle counter
    int batch_process_freq;
};

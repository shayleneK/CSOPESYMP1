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

class Scheduler
{
public:
    Scheduler(int num_cores);
    virtual ~Scheduler();

    virtual void run_core(int core_id); // ✅ virtual

    virtual void start_core_threads(); // ✅ optional virtual if subclass overrides

    void add_process(std::shared_ptr<Process> process);
    void shutdown();

    virtual std::vector<std::shared_ptr<Process>> get_running_processes();
    std::vector<std::shared_ptr<Process>> get_finished_processes();
    std::vector<std::shared_ptr<Process>> get_all_processes();
    std::map<int, std::map<std::string, float>> get_cpu_stats();
    virtual void start_process_generator();

    bool is_done();

protected:
    std::vector<std::thread> cpu_cores;
    std::vector<bool> core_available;
    std::queue<std::shared_ptr<Process>> ready_queue;
    std::vector<std::shared_ptr<Process>> all_processes;
    std::map<int, int> core_process_count;
    std::map<int, int> core_util_time;
    int total_cpu_time = 0;
    bool running = true;

    std::mutex queue_mutex;
    std::condition_variable queue_condition;
};

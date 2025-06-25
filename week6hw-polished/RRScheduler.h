// RRScheduler.h
#pragma once

#include "Scheduler.h"
#include <atomic>
#include <thread>

class RRScheduler : public Scheduler
{
public:
    RRScheduler(int num_cores, int quantum_ms);
    ~RRScheduler();

    void run_core(int core_id) override; // ✅ override clearly
    void start_core_threads() override;

    void start();
    void stop_scheduler();
    bool is_scheduler_running() const;
    void add_dummy_process();
    void start_process_generator() override;
    std::vector<std::shared_ptr<Process>> get_running_processes() override;
    // std::vector<std::shared_ptr<Process>> get_finished_processes() override;

protected:
    int time_quantum;
    std::atomic<bool> generating_processes{false};
    std::thread generator_thread;

    std::mutex running_mutex;
    std::map<int, std::shared_ptr<Process>> current_processes;
    std::atomic<int> next_pid{1};
};

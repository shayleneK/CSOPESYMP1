// FCFSScheduler.h
#pragma once
#include "Scheduler.h"
#include <thread>
#include <atomic>
#include <mutex>

class FCFSScheduler : public Scheduler
{
public:
    FCFSScheduler(int num_cores, int min_ins, int max_ins);
    ~FCFSScheduler();

    void start() override;
    void start_process_generator();
    void stop_scheduler();
    bool is_scheduler_running() const;
    void run_core(int core_id);
    void on_cpu_cycle(uint64_t cycle_number) override;
    void generate_new_process();

private:
    std::atomic<bool> generating_processes{false};
    std::thread generator_thread;
    int next_pid = 1;
};
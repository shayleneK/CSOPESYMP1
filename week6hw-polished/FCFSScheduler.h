#pragma once

#include "Scheduler.h"
#include <thread>
#include <atomic>
#include <memory>

class FCFSScheduler : public Scheduler
{
public:
    FCFSScheduler(int num_cores, int min_ins, int max_ins);
    ~FCFSScheduler();

    void start() override;
    void shutdown();
    void stop_scheduler() override;
    bool is_scheduler_running() const override;
    void start_core_threads() override;

    void start_process_generator() override;
    void on_cpu_cycle(uint64_t cycle_number) override;

protected:
    void run_core(int core_id) override;
    void generate_new_process();

private:
    std::atomic<bool> generating_processes{false};
    std::thread generator_thread;
};

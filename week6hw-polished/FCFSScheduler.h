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

private:
    int next_pid = 1;
};
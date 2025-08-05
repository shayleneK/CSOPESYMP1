#pragma once

#include "Scheduler.h"
#include "CPUCore.h"
#include <thread>
#include <atomic>
#include <memory>

class FCFSScheduler : public Scheduler
{
public:
    FCFSScheduler(int num_cores, int min_ins, int max_ins, MemoryManager &memory_manager);
    ~FCFSScheduler();

    void start() override;
    // void stop_scheduler() override;
    bool is_scheduler_running() const override;
    void start_core_threads() override;

    void on_cpu_cycle(uint64_t cycle_number) override;
    int get_min_instructions() const override { return min_instructions; }
    int get_max_instructions() const override { return max_instructions; }
    int get_num_cores() const override;
    double getCpuUtilization() const override;

    void notify_process_started(int core_id, std::shared_ptr<Process> process) override;
    void notify_process_finished(int core_id, std::shared_ptr<Process> process, int duration_ms) override;

protected:
    void run_core(int core_id) override;
    void generate_new_process();

private:
    // std::atomic<bool> generating_processes{false};
    //  std::thread generator_thread;
    int min_instructions;
    int max_instructions;
    std::condition_variable cv;
    std::mutex cv_m;

    int num_cores;
    MemoryManager &memory_manager_;
};

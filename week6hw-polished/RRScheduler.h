// RRScheduler.h
#pragma once

#include "Scheduler.h"
#include <atomic>
#include <thread>

class RRScheduler : public Scheduler
{
public:
    RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins);
    ~RRScheduler();

    void run_core(int core_id) override;
    void start_core_threads() override;

    void start() override;
    void stop_scheduler();
    bool is_scheduler_running() const;
    void start_process_generator() override;
    void set_batch_frequency(int freq) { batch_process_freq = freq; }
    void on_cpu_cycle(uint64_t cycle_number) override;
    void generate_new_process();

    std::vector<std::shared_ptr<Process>> get_running_processes() override;

protected:
    int time_quantum;
    int min_ins;
    int max_ins;
    std::atomic<bool> generating_processes{false};
    std::thread generator_thread;

    std::mutex running_mutex;
    std::map<int, std::shared_ptr<Process>> current_processes;
    std::atomic<int> next_pid{1};
};

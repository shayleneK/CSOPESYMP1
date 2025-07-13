#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H

#include "Scheduler.h"
#include "MemoryManager.h"
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>

class RRScheduler : public Scheduler
{
public:
    RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins, int delay_per_exec, int mem_per_proc, MemoryManager &mem_mgr);
    ~RRScheduler();

    void start();
    void stop_scheduler();
    bool is_scheduler_running() const;

    void start_core_threads();
    void start_process_generator();
    void on_cpu_cycle(uint64_t cycle_number);
    std::vector<std::shared_ptr<Process>> get_running_processes();

private:
    void generate_new_process();
    void run_core(int core_id);
    void save_memory_snapshot(uint64_t cycle_number);

    int time_quantum;
    std::atomic<bool> generating_processes{false};
    bool running = false;

    std::thread generator_thread;
    std::vector<std::thread> cpu_cores;

    MemoryManager memory_manager_;

    // Tracks process names currently occupying memory
    std::vector<std::string> process_memory_map_;
};

#endif // RR_SCHEDULER_H

#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H
#include <atomic>

#include "Scheduler.h"

class RRScheduler : public Scheduler
{
private:
    int time_quantum; // Time slice in milliseconds (must be a multiple of 50ms)
    std::atomic<bool> generating_processes;
    std::thread generator_thread;
    int next_pid = 1;

public:
    RRScheduler(int num_cores, int quantum_ms);
    void run_core(int core_id) override;
    void start();
    void stop_scheduler();
    bool is_scheduler_running() const;
    void add_dummy_process();
    virtual ~RRScheduler();
};

#endif // RR_SCHEDULER_H
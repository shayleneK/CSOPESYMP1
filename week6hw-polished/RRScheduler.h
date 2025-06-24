#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H

#include "Scheduler.h"

class RRScheduler : public Scheduler {
private:
    int time_quantum; // Time slice in milliseconds (must be a multiple of 50ms)

public:
    RRScheduler(int num_cores, int quantum_ms);
    void run_core(int core_id) override;
};

#endif // RR_SCHEDULER_H
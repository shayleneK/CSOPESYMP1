#include "CPUCycleManager.h"
#include "Scheduler.h"
#include <iostream>
#include <thread>
#include <chrono>

CPUCycleManager::CPUCycleManager()
    : running_(false),
      cpu_cycles_(0),
      total_cycles_(0),
      busy_cycles_(0),
      idle_cycles_(0),
      callback_(nullptr),
      interval_ms_(1000) {}

CPUCycleManager::~CPUCycleManager()
{
    stop();
}

CPUCycleManager &CPUCycleManager::getInstance()
{
    static CPUCycleManager instance;
    return instance;
}

void CPUCycleManager::start()
{
    if (running_)
        return;
    running_ = true;
    cpu_thread_ = std::thread(&CPUCycleManager::cpuLoop, this);
}

void CPUCycleManager::stop()
{
    running_ = false;
    if (cpu_thread_.joinable())
        cpu_thread_.join();
}

void CPUCycleManager::set_callback(Callback new_callback)
{
    callback_ = new_callback;
}

uint64_t CPUCycleManager::getCpuCycles() const
{
    return cpu_cycles_.load();
}

uint64_t CPUCycleManager::getIdleCycles() const
{
    return idle_cycles_.load();
}

uint64_t CPUCycleManager::getBusyCycles() const
{
    return busy_cycles_.load();
}

uint64_t CPUCycleManager::getTotalCycles() const
{
    return total_cycles_.load();
}

double CPUCycleManager::getUtilization() const
{
    uint64_t total = total_cycles_.load();
    if (total == 0)
        return 0.0;
    return 100.0 * busy_cycles_.load() / total;
}

void CPUCycleManager::cpuLoop()
{
    while (running_)
    {
        cpu_cycles_++;
        total_cycles_++;

        Scheduler *scheduler = Scheduler::getInstance();
        if (scheduler)
        {
            if (scheduler->is_scheduler_running() && !scheduler->get_running_processes().empty())
            {
                busy_cycles_++;
            }
            else
            {
                idle_cycles_++;
            }
            scheduler->on_cpu_cycle(cpu_cycles_);
        }
        else
        {
            idle_cycles_++; // Count as idle if no scheduler
        }

        if (callback_)
        {
            callback_(cpu_cycles_);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

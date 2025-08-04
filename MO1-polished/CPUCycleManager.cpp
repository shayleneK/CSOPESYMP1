#include "CPUCycleManager.h"
#include "Scheduler.h"
#include <chrono>
#include <thread>
#include <iostream>

CPUCycleManager::CPUCycleManager(Callback callback, int interval_ms)
    : callback_(callback),
      interval_ms_(interval_ms),
      running_(false),
      cycle_count_(0) {}

CPUCycleManager::~CPUCycleManager() {
    stop();
}

CPUCycleManager& CPUCycleManager::getInstance() {
    static CPUCycleManager instance;
    return instance;
}

void CPUCycleManager::start() {
    if (running) return;
    running = true;
    cpu_thread = std::thread(&CPUCycleManager::cpuLoop, this);
}

void CPUCycleManager::stop() {
    running = false;
    if (cpu_thread.joinable())
        cpu_thread.join();
}

uint64_t CPUCycleManager::getCpuCycles() const {
    return cpu_cycles.load();
}

double CPUCycleManager::getUtilization() const {
    uint64_t total = total_cycles.load();
    if (total == 0) return 0.0;
    return 100.0 * busy_cycles.load() / total;
}

void CPUCycleManager::run() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
        callback_(cycle_count_++);
    }
}

void CPUCycleManager::set_callback(Callback new_callback) {
    callback_ = new_callback;
}

void CPUCycleManager::cpuLoop() {
    while (running) {
        cpu_cycles++;
        total_cycles++;

        Scheduler* scheduler = Scheduler::getInstance();
        if (scheduler && scheduler->is_scheduler_running()) {
            if (!scheduler->get_running_processes().empty()) {
                busy_cycles++;
            }
            scheduler->on_cpu_cycle(cpu_cycles);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

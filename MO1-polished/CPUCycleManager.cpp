#include "CPUCycleManager.h"
#include <chrono>
#include <thread>

CPUCycleManager::CPUCycleManager(std::function<void(uint64_t)> cycle_callback, int cycle_interval_ms)
    : callback(cycle_callback), running(false), interval_ms(cycle_interval_ms), cycle_count(0) {}

CPUCycleManager::~CPUCycleManager() {
    stop();
}

void CPUCycleManager::start() {
    if (running) return;

    running = true;
    worker = std::thread(&CPUCycleManager::run, this);
}

void CPUCycleManager::stop() {
    running = false;
    if (worker.joinable()) {
        worker.join();
    }
}

void CPUCycleManager::run() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        if (running && callback) {
            callback(++cycle_count);
        }
    }
}

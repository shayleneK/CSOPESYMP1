#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <cstdint>

class CPUCycleManager {
public:
    CPUCycleManager(std::function<void(uint64_t)> cycle_callback, int cycle_interval_ms = 1000);
    ~CPUCycleManager();

    void start();
    void stop();

private:
    std::function<void(uint64_t)> callback;
    std::atomic<bool> running;
    int interval_ms;
    uint64_t cycle_count;
    std::thread worker;

    void run();
};

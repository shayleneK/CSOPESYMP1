#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

class CPUCycleManager {
public:
    using Callback = std::function<void(uint64_t)>;
    CPUCycleManager(Callback callback, int interval_ms = 1000);
    static CPUCycleManager& getInstance();

    void start();
    void stop();
    uint64_t getCpuCycles() const;
    double getUtilization() const;
    void set_callback(Callback new_callback);

private:
    CPUCycleManager();
    void cpuLoop();
    void run();

    std::atomic<bool> running;
    std::atomic<uint64_t> cpu_cycles;
    std::atomic<uint64_t> total_cycles;
    std::atomic<uint64_t> busy_cycles;

    std::thread cpu_thread;
    Callback callback_;
    int interval_ms_;
    std::thread worker_;
    std::atomic<bool> running_;
    uint64_t cycle_count_;
};

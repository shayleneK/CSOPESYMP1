#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

class CPUCycleManager {
public:
    using Callback = std::function<void(uint64_t)>;

    // Singleton accessor
    static CPUCycleManager& getInstance();

    // Start and stop the CPU ticking loop
    void start();
    void stop();

    // Set external callback to be called every tick
    void set_callback(Callback new_callback);

    // Get CPU statistics
    uint64_t getCpuCycles() const;
    double getUtilization() const;

    ~CPUCycleManager();

private:
    CPUCycleManager();                          // Private constructor for singleton
    CPUCycleManager(const CPUCycleManager&) = delete;

    CPUCycleManager& operator=(const CPUCycleManager&) = delete;

    void cpuLoop();                         // Internal ticking loop

    std::atomic<bool> running_;
    std::atomic<uint64_t> cpu_cycles_;
    std::atomic<uint64_t> total_cycles_;
    std::atomic<uint64_t> busy_cycles_;
    Callback callback_;
    std::thread cpu_thread_;
    int interval_ms_;                       // Tick interval (default 1000ms)
};

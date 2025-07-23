#pragma once

#include "Process.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>

class CPUCore {
public:
    CPUCore(int id, std::function<std::shared_ptr<Process>(int)> fetch_process_fn);
    ~CPUCore();

    void start();
    void stop();
    bool is_idle() const;

    int get_id() const { return core_id; }
    uint64_t get_busy_time_ms() const;
    uint64_t get_process_count() const;

private:
    void run();

    int core_id;
    std::function<std::shared_ptr<Process>(int)> fetch_process;
    std::atomic<bool> running;
    std::thread worker;

    mutable std::mutex stats_mutex;
    uint64_t busy_time_ms = 0;
    uint64_t process_count = 0;
    bool idle = true;
};
#include "CPUCore.h"
#include <chrono>

CPUCore::CPUCore(int id, std::function<std::shared_ptr<Process>(int)> fetch_process_fn)
    : core_id(id), fetch_process(fetch_process_fn), running(false) {}

CPUCore::~CPUCore() {
    stop();
}

void CPUCore::start() {
    running = true;
    worker = std::thread(&CPUCore::run, this);
}

void CPUCore::stop() {
    running = false;
    if (worker.joinable())
        worker.join();
}

bool CPUCore::is_idle() const {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return idle;
}

uint64_t CPUCore::get_busy_time_ms() const {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return busy_time_ms;
}

uint64_t CPUCore::get_process_count() const {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return process_count;
}

void CPUCore::run() {
    while (running) {
        auto process = fetch_process(core_id);

        if (process) {
            {
                std::lock_guard<std::mutex> lock(stats_mutex);
                idle = false;
            }

            auto start = std::chrono::high_resolution_clock::now();
            process->execute(core_id);
            auto end = std::chrono::high_resolution_clock::now();

            std::lock_guard<std::mutex> lock(stats_mutex);
            busy_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            process_count++;
            idle = true;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // idle wait
        }
    }
}

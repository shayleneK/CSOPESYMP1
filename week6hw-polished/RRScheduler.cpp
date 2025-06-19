#include "RRScheduler.h"
#include "Process.h"
#include <iostream>
#include <thread>
#include <chrono>

RRScheduler::RRScheduler(int num_cores, int quantum_ms)
    : Scheduler(num_cores), time_quantum(quantum_ms) {
    // Ensure quantum is a multiple of 50ms (since each command takes 50ms)
    if (quantum_ms % 50 != 0) {
        std::cerr << "[WARNING] Time quantum should be a multiple of 50ms for compatibility.\n";
    }
}

void RRScheduler::run_core(int core_id) {
    while (running) {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_condition.wait(lock, [this] {
                return !running || !ready_queue.empty();
            });

            if (!running) break;

            if (!ready_queue.empty()) {
                process = ready_queue.front();
                ready_queue.pop();
                core_available[core_id] = false;
            } else {
                continue;
            }
        }

        if (process) {
            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes[core_id] = process;
            }

            // Calculate how many commands can be executed in this quantum
            int commands_per_quantum = time_quantum / 50;

            for (int i = 0; i < commands_per_quantum && !process->is_finished; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                process->execute(core_id);  // Executes one command
                auto end = std::chrono::high_resolution_clock::now();
                int duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    core_util_time[core_id] += duration_ms;
                    core_process_count[core_id]++;
                    total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);
                }
            }

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (!process->is_finished) {
                    ready_queue.push(process); // Requeue unfinished process
                }
                core_available[core_id] = true;
            }

            {
                std::unique_lock<std::mutex> lock(running_mutex);
                if (process->is_finished) {
                    current_processes.erase(core_id);
                    std::cout << "Process " << process->name << " finished on core " << core_id << std::endl;
                }
            }
        }
    }
}
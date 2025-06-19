#include "Scheduler.h"
#include "Process.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <algorithm>

Scheduler::Scheduler(int num_cores)
{
    for (int i = 0; i < num_cores; ++i)
    {
        core_available.push_back(true);
        core_process_count[i] = 0;
        core_util_time[i] = 0;
    }
}

Scheduler::~Scheduler()
{
    shutdown();
}

void Scheduler::add_process(std::shared_ptr<Process> process)
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    ready_queue.push(process);
    all_processes.push_back(process);
    lock.unlock();
    queue_condition.notify_one();
}

void Scheduler::start()
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        cpu_cores.emplace_back(&Scheduler::run_core, this, i);
    }
}

void Scheduler::shutdown()
{
    running = false;
    queue_condition.notify_all();
    for (auto &core : cpu_cores)
    {
        if (core.joinable())
        {
            core.join();
        }
    }
}

void Scheduler::run_core(int core_id)
{
    while (running)
    {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_condition.wait(lock, [this]
                                 { return !running || !ready_queue.empty(); });

            if (!running)
                break;

            if (!ready_queue.empty())
            {
                process = ready_queue.front();
                ready_queue.pop();
                core_available[core_id] = false;
            }
            else
            {
                continue;
            }
        }

        if (process)
        {
            // Add to currently running processes
            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes[core_id] = process;
            }

            // Execute the process
            auto start_exec_time = std::chrono::high_resolution_clock::now();
            process->execute(core_id);
            auto end_exec_time = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::milli> elapsed_ms = end_exec_time - start_exec_time;
            int duration_ms = static_cast<int>(elapsed_ms.count());

            core_util_time[core_id] += duration_ms;
            core_process_count[core_id]++;
            total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);

            core_available[core_id] = true;

            // Remove from running processes
            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes.erase(core_id);
            }
        }
    }
}

std::vector<std::shared_ptr<Process>> Scheduler::get_running_processes()
{
    std::unique_lock<std::mutex> lock(running_mutex);
    std::vector<std::shared_ptr<Process>> running_procs;
    for (const auto& pair : current_processes)
    {
        running_procs.push_back(pair.second);
    }
    return running_procs;
}
std::vector<std::shared_ptr<Process>> Scheduler::get_finished_processes()
{
    std::vector<std::shared_ptr<Process>> finished_procs;
    for (const auto &p : all_processes)
    {
        if (p->is_finished)
        {
            finished_procs.push_back(p);
        }
    }
    return finished_procs;
}

std::vector<std::shared_ptr<Process>> Scheduler::get_all_processes()
{
    return all_processes;
}

bool Scheduler::is_done()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    return ready_queue.empty() && std::all_of(core_available.begin(), core_available.end(), [](bool avail)
                                               { return avail; });
}

std::map<int, std::map<std::string, float>> Scheduler::get_cpu_stats()
{
    std::map<int, std::map<std::string, float>> stats;
    std::unique_lock<std::mutex> lock(queue_mutex);

    for (size_t core_id = 0; core_id < core_available.size(); ++core_id)
    {
        float util = 0.0f;
        if (total_cpu_time > 0)
        {
            util = (static_cast<float>(core_util_time[core_id]) / total_cpu_time) * 100.0f;
        }

        int queue_size = ready_queue.size();

        stats[core_id]["util"] = util;
        stats[core_id]["queue_size"] = static_cast<float>(queue_size);
    }

    return stats;
}

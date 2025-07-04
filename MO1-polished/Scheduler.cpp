// Scheduler.cpp
#include "Scheduler.h"
#include <chrono>
#include <algorithm>
#include <iostream>

Scheduler::Scheduler(int num_cores, int min_ins, int max_ins)
    : min_instructions(min_ins), max_instructions(max_ins)
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
    queue_condition.notify_one();
}

void Scheduler::start_core_threads() // IMPORTANT: check where should u run
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        cpu_cores.emplace_back(&Scheduler::run_core, this, i);
    }
}

void Scheduler::shutdown()
{
    global_shutdown = true;
    running = false;
    stop_scheduler();

    queue_condition.notify_all();

    for (auto &t : cpu_cores)
    {
        if (t.joinable())
            t.join();
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
            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes[core_id] = process;
            }
            auto start_time = std::chrono::high_resolution_clock::now();
            process->execute(core_id);
            auto end_time = std::chrono::high_resolution_clock::now();

            int duration_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());

            core_util_time[core_id] += duration_ms;
            core_process_count[core_id]++;
            total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);

            core_available[core_id] = true;
            if (process->is_finished)
            {
                current_processes.erase(core_id);
            }
        }
    }
}
void Scheduler::start()
{
    start_core_threads();
    start_process_generator();
}

std::vector<std::shared_ptr<Process>> Scheduler::get_running_processes()
{
    std::unique_lock<std::mutex> lock(running_mutex);
    std::vector<std::shared_ptr<Process>> running_procs;
    for (const auto &pair : current_processes)
    {
        if (!pair.second->is_finished)
        {
            running_procs.push_back(pair.second);
        }
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

int Scheduler::get_core_of_process(const std::shared_ptr<Process>& p) {
    std::unique_lock<std::mutex> lock(running_mutex);
    auto it = process_to_core.find(p);
    if (it != process_to_core.end()) {
        return it->second;
    }
    return -1; // Not found
}

bool Scheduler::is_done()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    return ready_queue.empty() && std::all_of(core_available.begin(), core_available.end(), [](bool avail)
                                              { return avail; });
}

void Scheduler::start_process_generator()
{
    std::cout << "[Scheduler] process gen()." << std::endl;
}

std::map<int, std::map<std::string, float>> Scheduler::get_cpu_stats()
{
    std::map<int, std::map<std::string, float>> stats;
    std::unique_lock<std::mutex> lock(queue_mutex);
    uint64_t safe_total_time = total_cpu_time > 0 ? total_cpu_time : 1;
    for (size_t core_id = 0; core_id < core_available.size(); ++core_id)
    {
        float util_percent = (static_cast<float>(core_util_time[core_id]) / safe_total_time) * 100.0f;

        stats[static_cast<int>(core_id)]["util"] = util_percent;
        stats[static_cast<int>(core_id)]["busy_time_ms"] = static_cast<float>(core_util_time[core_id]);
        stats[static_cast<int>(core_id)]["process_count"] = static_cast<float>(core_process_count[core_id]);
        stats[static_cast<int>(core_id)]["available"] = core_available[core_id] ? 1.0f : 0.0f;
    }

    return stats;
}

void Scheduler::stop_scheduler()
{
    generating_processes = false;
    

    if (generator_thread.joinable())
    {
        generator_thread.join();
    }
}

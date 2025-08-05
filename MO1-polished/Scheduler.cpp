// Scheduler.cpp
#include "Scheduler.h"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <functional>

Scheduler::Scheduler(int num_cores)
    : core_available(num_cores, true)
{
    for (int i = 0; i < num_cores; ++i)
    {
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

void Scheduler::start_core_threads()
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        core_available[i] = true;

        auto core = std::make_shared<CPUCore>(i, [this](int core_id) -> std::shared_ptr<Process>
                                              {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (!ready_queue.empty()) {
                auto process = ready_queue.front();
                ready_queue.pop();
                core_available[core_id] = false;

                {
                    std::lock_guard<std::mutex> lock2(running_mutex);
                    current_processes[core_id] = process;
                    process_to_core[process] = core_id;
                }

                return process;
            }

            core_available[core_id] = true;
            return nullptr; });

        cpu_core_objects.push_back(core);
        core->start();
    }
}

void Scheduler::shutdown()
{
    global_shutdown = true;
    running = false;
    stop_scheduler();

    queue_condition.notify_all();

    for (auto &core : cpu_core_objects)
    {
        core->stop();
    }

    for (auto &t : cpu_cores)
    {
        if (t.joinable())
            t.join();
    }
}
/*
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
*/
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
        if (!pair.second->isFinished())
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
        if (p->isFinished())
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

int Scheduler::get_core_of_process(const std::shared_ptr<Process> &p)
{
    std::unique_lock<std::mutex> lock(running_mutex);
    auto it = process_to_core.find(p);
    if (it != process_to_core.end())
    {
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
    {
        std::lock_guard<std::mutex> lock(generator_mutex);
        generating_processes = true;
    }
    generator_thread = std::thread([this]
                                   { generatorLoop(); });
}

std::map<int, std::map<std::string, float>> Scheduler::get_cpu_stats()
{
    std::map<int, std::map<std::string, float>> stats;

    for (size_t i = 0; i < cpu_core_objects.size(); ++i)
    {
        auto &core = cpu_core_objects[i];
        float busy = static_cast<float>(core->get_busy_time_ms());
        float count = static_cast<float>(core->get_process_count());
        float total = busy + 1; // prevent div-by-zero

        stats[i]["util"] = (busy / total) * 100.0f;
        stats[i]["busy_time_ms"] = busy;
        stats[i]["process_count"] = count;
        stats[i]["available"] = core->is_idle() ? 1.0f : 0.0f;
    }

    return stats;
}

std::shared_ptr<Process> Scheduler::getProcessByName(const std::string &name)
{
    for (auto &p : all_processes)
    { // assuming you store processes in a vector
        if (p->getName() == name)
            return p;
    }
    return nullptr;
}

void Scheduler::stop_scheduler()
{
    {
        std::lock_guard<std::mutex> lock(generator_mutex);
        generating_processes = false;
    }
    generator_cv.notify_all(); // Wake up thread if sleeping
    if (generator_thread.joinable())
        generator_thread.join();
}

void Scheduler::generatorLoop()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(generator_mutex);
        if (!generating_processes)
            break;
        generator_cv.wait_for(lock, std::chrono::seconds(1)); // Wake periodically or on stop
        if (!generating_processes)
            break;

        lock.unlock(); // Unlock before generating
        generate_new_process();
    }
}

void Scheduler::start_scheduler()
{
    {
        std::lock_guard<std::mutex> lock(generator_mutex);
        generating_processes = true;
    }
    generator_thread = std::thread([this]
                                   { this->generatorLoop(); });
}

int Scheduler::get_num_cores() const
{
    return num_cores;
}
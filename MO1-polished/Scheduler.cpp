
#include "Scheduler.h"
#include "CPUCycleManager.h"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <functional>

Scheduler* Scheduler::instance_ = nullptr;
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

void Scheduler::shutdown()
{
    global_shutdown = true;
    running = false;
    stop_scheduler();

    queue_condition.notify_all();

    CPUCycleManager::getInstance().stop();
}

void Scheduler::on_cpu_cycle(uint64_t cycle) {
    std::unique_lock<std::mutex> lock(queue_mutex);

    if (!ready_queue.empty()) {
        // Pick next process to run
        auto process = ready_queue.front();
        ready_queue.pop();

        // Run it
        process->execute(0);  // 0 as dummy core_id, since cores are abstracted

        if (!process->isFinished()) {
            ready_queue.push(process);  // Re-enqueue if not done
        } else {
            std::lock_guard<std::mutex> lock2(running_mutex);
            current_processes.erase(0);
        }

        {
            std::lock_guard<std::mutex> lock2(running_mutex);
            current_processes[0] = process;
        }
    } else {
        std::lock_guard<std::mutex> lock2(running_mutex);
        current_processes.erase(0);
    }
}

void start_core_threads(){
    CPUCycleManager::getInstance().start();
}

void Scheduler::start()
{
    running = true;
    CPUCycleManager::getInstance().start();
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
Scheduler *Scheduler::getInstance()
{
    return instance_;
}

void Scheduler::setInstance(Scheduler *scheduler)
{
    instance_ = scheduler;
}
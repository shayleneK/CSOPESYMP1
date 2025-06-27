// RRScheduler.cpp
#include "RRScheduler.h"
#include "Process.h"
#include "Command.h"
#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

RRScheduler::RRScheduler(int num_cores, int quantum_ms, int min_ins, int max_ins)
    : Scheduler(num_cores, min_ins, max_ins), time_quantum(quantum_ms)
{
    if (quantum_ms % 50 != 0)
    {
        std::cerr << "[WARNING] Time quantum should be a multiple of 50ms.\n";
    }
}

RRScheduler::~RRScheduler()
{
    stop_scheduler();
}

void RRScheduler::start()
{
    if (generating_processes)
        return;

    running = true;
    generating_processes = true;

    // Start the CPU core threads
    start_core_threads();

    // Start the generator thread
    generator_thread = std::thread([this]()
                                   {
        int cycle_counter = 0;
        int batch_process_frequency = 100;
        while (generating_processes && running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (++cycle_counter >= batch_process_frequency)
            {
                add_dummy_process();
                cycle_counter = 0;
            }
        } });
}

// RRScheduler.cpp
void RRScheduler::start_process_generator()
{
    if (generating_processes.load())
        return;

    generating_processes.store(true);
    generator_thread = std::thread([this]()
                                   {
        int batch_process_freq = 100;
        int cycle_counter = 0;

        while (generating_processes.load() && running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            cycle_counter++;
            if (cycle_counter >= batch_process_freq) {
                add_dummy_process();
                add_dummy_process();
                add_dummy_process();
                add_dummy_process();
                cycle_counter = 0;
            }
        } });
}

void RRScheduler::start_core_threads()
{
    for (int i = 0; i < static_cast<int>(core_available.size()); ++i)
    {
        cpu_cores.emplace_back(&RRScheduler::run_core, this, i);
    }
}

void RRScheduler::add_dummy_process()
{
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
    std::string name = oss.str();

    auto process = std::make_shared<Process>(name);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ins_dist(min_ins, max_ins); // setting
    std::uniform_int_distribution<> op_dist(0, 5);

    int instruction_count = ins_dist(gen);
    for (int i = 0; i < instruction_count; ++i)
    {
        int op = op_dist(gen);
        switch (op)
        {
        case 0:
            process->add_command(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));
            break;
        case 4:
            process->add_command(std::make_shared<SleepCommand>(1));
            break;
        default:
            process->add_command(std::make_shared<PrintCommand>("\"Dummy command from " + name + "\""));
            break;
        }
    }

    process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));
    add_process(process);
}

void RRScheduler::stop_scheduler()
{
    generating_processes = false;
    if (generator_thread.joinable())
    {
        generator_thread.join();
    }
}

bool RRScheduler::is_scheduler_running() const
{
    return generating_processes;
}

void RRScheduler::run_core(int core_id)
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

            int commands_per_quantum = time_quantum / 50;
            for (int i = 0; i < commands_per_quantum && !process->is_finished; ++i)
            {
                auto start = std::chrono::high_resolution_clock::now();
                process->execute(core_id);
                auto end = std::chrono::high_resolution_clock::now();

                int duration = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    core_util_time[core_id] += duration;
                    core_process_count[core_id]++;
                    total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);
                }
            }

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (!process->is_finished)
                {
                    ready_queue.push(process);
                }
                core_available[core_id] = true;
            }

            {
                std::unique_lock<std::mutex> lock(running_mutex);
                if (process->is_finished)
                {
                    current_processes.erase(core_id);
                    std::cout << "Process " << process->name << " finished on core " << core_id << std::endl;
                }
            }
        }
    }
}

// RRScheduler.cpp
std::vector<std::shared_ptr<Process>> RRScheduler::get_running_processes()
{
    std::vector<std::shared_ptr<Process>> result;

    // 1. Add ready queue processes
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        std::queue<std::shared_ptr<Process>> temp = ready_queue;
        while (!temp.empty())
        {
            result.push_back(temp.front());
            temp.pop();
        }
    }

    // 2. Add currently running processes on cores
    {
        std::unique_lock<std::mutex> lock(running_mutex);
        for (const auto &entry : current_processes)
        {
            result.push_back(entry.second);
        }
    }

    return result;
}

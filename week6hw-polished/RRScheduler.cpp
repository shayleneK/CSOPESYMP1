#include "RRScheduler.h"
#include "Process.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <random>
#include <iomanip>
#include "Command.h"

RRScheduler::RRScheduler(int num_cores, int quantum_ms)
    : Scheduler(num_cores), time_quantum(quantum_ms)
{
    // Ensure quantum is a multiple of 50ms (since each command takes 50ms)
    if (quantum_ms % 50 != 0)
    {
        std::cerr << "[WARNING] Time quantum should be a multiple of 50ms for compatibility.\n";
    }
}

RRScheduler::~RRScheduler() {}

void RRScheduler::start()
{
    if (generating_processes)
        return;

    generating_processes = true;
    generator_thread = std::thread([this]()
                                   {
        int cycle_counter = 0;
        int batch_process_frequency = 100; // ** IMPORTANT: DUMMY VALUE FOR NOW W/O CONFIG
        while (generating_processes && running) {
            cycle_counter++;

            if (cycle_counter >= batch_process_frequency) {
                add_dummy_process();
                cycle_counter = 0;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // simulate one CPU tick
        } });
}

#include <random>
#include <sstream>

void RRScheduler::add_dummy_process()
{
    // IMPORTANT : DUMMY VALS FOR NOW
    int min_instructions = 10;
    int max_instructions = 20;
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << next_pid++;
    std::string name = oss.str();

    auto process = std::make_shared<Process>(name);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ins_count_dist(min_instructions, max_instructions);
    std::uniform_int_distribution<> op_dist(0, 5); // 6 types of ops

    int instruction_count = ins_count_dist(gen);

    for (int i = 0; i < instruction_count; ++i)
    {
        int op = op_dist(gen);
        switch (op)
        {
        case 0:
        { // PRINT
            process->add_command(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));
            break;
        }
        case 1:
        { // DECLARE
            char var = 'a' + (i % 26);
            process->add_command(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));

            // process->add_command(std::make_shared<DeclareCommand>(std::string(1, var), "0"));
            break;
        }
        case 2:
        { // ADD
            char dst = 'a' + ((i + 0) % 26);
            char src1 = 'a' + ((i + 1) % 26);
            char src2 = 'a' + ((i + 2) % 26);
            process->add_command(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));

            /* process->add_command(std::make_shared<AddCommand>(
                std::string(1, dst),
                std::string(1, src1),
                std::string(1, src2))); */
            break;
        }
        case 3:
        { // SUBTRACT
            char dst = 'a' + ((i + 0) % 26);
            char src1 = 'a' + ((i + 1) % 26);
            char src2 = 'a' + ((i + 2) % 26);
            process->add_command(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));

            /* process->add_command(std::make_shared<PrintCommand>(
                std::string(1, dst),
                std::string(1, src1),
                std::string(1, src2))); */
            break;
        }
        case 4:
        { // SLEEP
            std::uniform_int_distribution<> sleep_dist(1, 10);
            process->add_command(std::make_shared<SleepCommand>(sleep_dist(gen)));
            break;
        }
        case 5:
        { // FOR loop
            std::uniform_int_distribution<> repeat_dist(1, 3);
            int repeat = repeat_dist(gen);

            std::vector<std::shared_ptr<Command>> body;
            for (int j = 0; j < 5 && i + j < instruction_count; ++j)
            {
                int inner_op = op_dist(gen);
                if (inner_op == 0)
                {
                    body.push_back(std::make_shared<PrintCommand>("\"Loop iteration!\""));
                }
                else if (inner_op == 1)
                {
                    char v = 'a' + ((i + j) % 26);

                    // body.push_back(std::make_shared<PrintCommand>(std::string(1, v), "0"));
                    body.push_back(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));
                }
                else if (inner_op == 2)
                {
                    // body.push_back(std::make_shared<PrintCommand>("x", "x", "1"));
                    body.push_back(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));
                }
                else if (inner_op == 3)
                {
                    // body.push_back(std::make_shared<PrintCommand>("x", "x", "1"));
                    body.push_back(std::make_shared<PrintCommand>("\"Hello world from " + name + "!\""));
                }
                else if (inner_op == 4)
                {
                    body.push_back(std::make_shared<SleepCommand>(1));
                }
            }

            // process->add_command(std::make_shared<PrintCommand>(repeat, body));
            break;
        }
        }
    }

    // Final completion message
    process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));

    add_process(process); // Defined in Scheduler base class
}

void RRScheduler::start_process_generator()
{
    // IMPORTANT : DUMMY VALS FOR NOW
    if (generating_processes.load())
        return;

    generating_processes.store(true);
    generator_thread = std::thread([this]()
                                   {
        int batch_process_freq = 100;
        int cycle_counter = 0;
        while (generating_processes.load() && running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 1 tick = 50ms
            cycle_counter++;

            if (cycle_counter >= batch_process_freq) {
                add_dummy_process();
                cycle_counter = 0;
            }
        } });
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

            // Calculate how many commands can be executed in this quantum
            int commands_per_quantum = time_quantum / 50;

            for (int i = 0; i < commands_per_quantum && !process->is_finished; ++i)
            {
                auto start = std::chrono::high_resolution_clock::now();
                process->execute(core_id); // Executes one command
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
                if (!process->is_finished)
                {
                    ready_queue.push(process); // Requeue unfinished process
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
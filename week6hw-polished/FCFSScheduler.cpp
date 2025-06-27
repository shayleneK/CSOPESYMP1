#include "FCFSScheduler.h"
#include "Process.h"
#include "Command.h"
#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

FCFSScheduler::FCFSScheduler(int num_cores, int min_ins, int max_ins)
    : Scheduler(num_cores, min_ins, max_ins) {}

FCFSScheduler::~FCFSScheduler() { stop_scheduler(); }

void FCFSScheduler::start()
{
    if (generating_processes.load())
        return;

    generating_processes.store(true);
    running = true;

    generator_thread = std::thread([this]()
                                   {
        int cycle_counter = 0;
        const int batch_process_frequency = 100;

        while (generating_processes.load() && running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (++cycle_counter >= batch_process_frequency)
            {
                add_dummy_process();
                cycle_counter = 0;
            }
        } });
}

void FCFSScheduler::start_process_generator()
{
    if (generating_processes.load())
        return;

    generating_processes.store(true);
    generator_thread = std::thread([this]()
                                   {
        int batch_process_freq = 100;
        int cycle_counter = 0;
        while (generating_processes.load() && running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            cycle_counter++;
            if (cycle_counter >= batch_process_freq)
            {
                add_dummy_process();
                cycle_counter = 0;
            }
        } });
}

void FCFSScheduler::add_dummy_process()
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
        {
            std::uniform_int_distribution<> msg_type(0, 1); // 0: static, 1: with variable
            std::vector<std::string> vars = {"x", "y", "result", "temp", "value"};

            std::string message;
            if (msg_type(gen) == 0)
            {
                message = "Hello world from " + name + "!";
            }
            else
            {
                std::string var = vars[gen() % vars.size()];
                message = "\"Value from: \" + " + var;
            }

            process->add_command(std::make_shared<PrintCommand>(message));
            break;
        }
        case 1:
        {
            std::vector<std::string> var_names = {"a", "b", "c", "temp", "sum"};

            std::string target = var_names[gen() % var_names.size()];
            std::string op1 = var_names[gen() % var_names.size()];
            std::string op2 = var_names[gen() % var_names.size()];

            std::uniform_int_distribution<> bool_dist(0, 1);
            bool op1_is_var = bool_dist(gen);
            bool op2_is_var = bool_dist(gen);

            std::uniform_int_distribution<uint16_t> value_dist(1, 500);
            uint16_t val1 = value_dist(gen);
            uint16_t val2 = value_dist(gen);

            process->add_command(std::make_shared<AddCommand>(
                target,
                op1, op2,
                op1_is_var, op2_is_var,
                val1, val2));
            break;
        }

        case 2:
        {
            std::vector<std::string> var_names = {"a", "b", "c", "temp", "diff"};

            std::string target = var_names[gen() % var_names.size()];
            std::string op1 = var_names[gen() % var_names.size()];
            std::string op2 = var_names[gen() % var_names.size()];

            std::uniform_int_distribution<> bool_dist(0, 1);
            bool op1_is_var = bool_dist(gen);
            bool op2_is_var = bool_dist(gen);

            std::uniform_int_distribution<uint16_t> value_dist(1, 500);
            uint16_t val1 = value_dist(gen);
            uint16_t val2 = value_dist(gen);

            process->add_command(std::make_shared<SubtractCommand>(
                target,
                op1, op2,
                op1_is_var, op2_is_var,
                val1, val2));
            break;
        }

        case 3:

            // NOT TOO SURE ABOUT THIS -JUL
            {
                // Generate some variable names on the fly
                std::vector<std::string> vars;
                for (int i = 1; i <= 5; ++i)
                {
                    vars.push_back("a" + std::to_string(i));
                    vars.push_back("b" + std::to_string(i));
                    vars.push_back("c" + std::to_string(i));
                }

                // Pick a variable name at random
                std::string var_name = vars[gen() % vars.size()];

                // Assign a random value
                std::uniform_int_distribution<uint16_t> val_dist(0, 1000);
                uint16_t value = val_dist(gen);

                process->add_command(std::make_shared<DeclareCommand>(var_name, value));
                break;
            }

        case 4:
        {
            std::uniform_int_distribution<> rep_dist(1, 3); // loop 2–5 times
            int repeats = rep_dist(gen);

            std::vector<std::shared_ptr<Command>> nested_cmds;
            std::uniform_int_distribution<> nested_len_dist(1, 3); // 2–4 inner instructions
            int nested_instruction_count = nested_len_dist(gen);

            std::uniform_int_distribution<> inner_op_dist(0, 3); // limit to simpler types for nesting

            for (int j = 0; j < nested_instruction_count; ++j)
            {
                int inner_op = inner_op_dist(gen);
                switch (inner_op)
                {
                case 0:
                {
                    std::string message = "\"Looped message " + std::to_string(j) + "\"";
                    nested_cmds.push_back(std::make_shared<PrintCommand>(message));
                    break;
                }
                case 1:
                {
                    std::string target = "loopVar" + std::to_string(j);
                    std::string op1 = "a" + std::to_string(j);
                    std::string op2 = "b" + std::to_string(j);
                    bool op1_is_var = true;
                    bool op2_is_var = false;
                    std::uniform_int_distribution<uint16_t> val_dist(1, 50);
                    uint16_t val2 = val_dist(gen);

                    nested_cmds.push_back(std::make_shared<AddCommand>(
                        target, op1, op2,
                        op1_is_var, op2_is_var,
                        0, val2));
                    break;
                }
                case 2:
                {
                    std::string var = "loopD" + std::to_string(j);
                    std::uniform_int_distribution<uint16_t> val_dist(0, 100);
                    uint16_t val = val_dist(gen);
                    nested_cmds.push_back(std::make_shared<DeclareCommand>(var, val));
                    break;
                }
                case 3:
                {
                    nested_cmds.push_back(std::make_shared<SleepCommand>(1));
                    break;
                }
                }
            }

            process->add_command(std::make_shared<ForCommand>(nested_cmds, repeats));
            break;
        }

        case 5:
        {
            std::uniform_int_distribution<uint8_t> sleep_ticks(10, 100); // sleep for 10–100ms
            uint8_t ticks = sleep_ticks(gen);

            process->add_command(std::make_shared<SleepCommand>(ticks));
            break;
        }
        default:
            process->add_command(std::make_shared<PrintCommand>("\"Dummy command from " + name + "\""));
            break;
        }
    }

    process->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));
    add_process(process);
}

void FCFSScheduler::stop_scheduler()
{
    generating_processes = false;
    if (generator_thread.joinable())
    {
        generator_thread.join();
    }
}

bool FCFSScheduler::is_scheduler_running() const
{
    return generating_processes;
}

void FCFSScheduler::run_core(int core_id)
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

            while (!process->is_finished)
            {
                auto start = std::chrono::high_resolution_clock::now();
                process->execute(core_id);
                auto end = std::chrono::high_resolution_clock::now();

                int duration = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    core_util_time[core_id] += duration;
                    core_process_count[core_id]++;
                    total_cpu_time = std::max(total_cpu_time, core_util_time[core_id]);
                }
            }

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                core_available[core_id] = true;
            }

            {
                std::unique_lock<std::mutex> lock(running_mutex);
                current_processes.erase(core_id);
                std::cout << "[FCFS] Process " << process->name << " finished on core " << core_id << std::endl;
            }
        }
    }
}

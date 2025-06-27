#include "Command.h"
#include "Process.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <sstream>

Process *current_process = nullptr;

PrintCommand::PrintCommand(const std::string &msg) : message(msg) {}

void PrintCommand::execute(Process *proc, int core_id, std::ofstream &output_file, const std::string &process_name)
{
    std::string output;

    // Handle case: "prefix" + var
    size_t plus_pos = message.find('+');
    if (plus_pos != std::string::npos)
    {
        std::string prefix = message.substr(0, plus_pos);
        std::string var_name = message.substr(plus_pos + 1);

        // Clean quotes and whitespace
        prefix.erase(remove_if(prefix.begin(), prefix.end(), [](char c)
                               { return c == '"' || isspace(c); }),
                     prefix.end());
        var_name.erase(remove_if(var_name.begin(), var_name.end(), ::isspace), var_name.end());

        uint16_t val = proc->get_var(var_name);
        output = prefix + std::to_string(val);
    }
    else
    {
        // Remove quotes if present
        if (!message.empty() && message.front() == '"' && message.back() == '"')
            output = message.substr(1, message.size() - 2);
        else
            output = message;
    }

    // Add Log Entry
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);

    std::ostringstream log_entry;
    log_entry << "(" << std::put_time(std::localtime(&time_now), "%Y-%m-%d %H:%M:%S") << ") "
              << "Core:" << core_id << " - PRINT(\"" << output << "\")";

    proc->logs.push_back(log_entry.str());
}

SleepCommand::SleepCommand(int duration) : duration_ms(duration) {}

void SleepCommand::execute(Process *proc, int core_id, std::ofstream &output_file, const std::string &process_name)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
}

DeclareCommand::DeclareCommand(const std::string &var, uint16_t val)
    : var_name(var), value(val) {}

void DeclareCommand::execute(Process *proc, int core_id, std::ofstream &output_file, const std::string &process_name)
{
    proc->set_var(var_name, value);
}

AddCommand::AddCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
                       bool o1_var, bool o2_var, uint16_t v1, uint16_t v2)
    : target(tgt), op1(o1), op2(o2), op1_is_var(o1_var), op2_is_var(o2_var),
      val1(v1), val2(v2) {}

void AddCommand::execute(Process *proc, int core_id, std::ofstream &output_file, const std::string &process_name)
{
    uint16_t a = op1_is_var ? proc->get_var(op1) : val1;
    uint16_t b = op2_is_var ? proc->get_var(op2) : val2;
    uint32_t result = a + b;
    if (result > 65535)
        result = 65535;
    proc->set_var(target, static_cast<uint16_t>(result));
}

SubtractCommand::SubtractCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
                                 bool o1_var, bool o2_var, uint16_t v1, uint16_t v2)
    : target(tgt), op1(o1), op2(o2), op1_is_var(o1_var), op2_is_var(o2_var),
      val1(v1), val2(v2) {}

void SubtractCommand::execute(Process *proc, int core_id, std::ofstream &output_file, const std::string &process_name)
{
    int32_t a = op1_is_var ? proc->get_var(op1) : val1;
    int32_t b = op2_is_var ? proc->get_var(op2) : val2;
    int32_t result = a - b;
    if (result < 0)
        result = 0;
    proc->set_var(target, static_cast<uint16_t>(result));
}

ForCommand::ForCommand(const std::vector<std::shared_ptr<Command>> &cmds, int reps)
    : instructions(cmds), repeat(reps) {}

void ForCommand::execute(Process *proc, int core_id, std::ofstream &output_file, const std::string &process_name)
{
    for (int i = 0; i < repeat; ++i)
    {
        for (auto &cmd : instructions)
        {
            cmd->execute(proc, core_id, output_file, process_name);
        }
    }
}
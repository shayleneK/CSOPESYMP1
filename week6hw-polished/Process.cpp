#include "Process.h"
#include "Command.h"
#include <iostream>
#include <thread>
#include <chrono>

Process::Process(const std::string &name) : name(name)
{
    // output_filename = name + "_log.txt";
    // output_file.open(output_filename, std::ios::out | std::ios::trunc);
    // if (!output_file.is_open())
    // {
    //     std::cerr << "[ERROR] Could not open log file for process: " << name
    //               << " at " << output_filename << std::endl;
    // }
}

Process::~Process()
{
    if (output_file.is_open())
    {
        output_file.close();
    }
}

void Process::add_command(std::shared_ptr<Command> cmd)
{
    commands.push_back(cmd);
}

void Process::execute(int core_id)
{
    has_started = true;
    start_time = std::chrono::system_clock::now();
    current_core = core_id;

    // Execute all commands, passing core ID and output file stream
    for (size_t i = 0; i < commands.size(); ++i)
    {
        current_command_index = i;
        commands[i]->execute(this, core_id, output_file, name);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    is_finished = true;
    finish_time = std::chrono::system_clock::now();
}

uint16_t Process::get_var(const std::string &var_name)
{
    return variables[var_name]; // defaults to 0 if not found
}

void Process::set_var(const std::string &var_name, uint16_t value)
{
    if (value > std::numeric_limits<uint16_t>::max())
    {
        value = std::numeric_limits<uint16_t>::max();
    }
    // No need to check for < 0 since uint16_t can't be negative
    variables[var_name] = value;
}
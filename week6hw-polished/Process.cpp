#include "Process.h"
#include "Command.h"
#include <iostream>
#include <thread>
#include <chrono>

Process::Process(const std::string &name) : name(name)
{
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
    // Set start time on first execution
    if (!has_started)
    {
        has_started = true;
        start_time = std::chrono::system_clock::now();
    }

    current_core = core_id;

    // Execute only one command per call
    if (current_command_index < commands.size())
    {
        commands[current_command_index]->execute(core_id, output_file, name);
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulated command duration

        current_command_index++;
    }

    // Check if all commands are done
    if (current_command_index >= commands.size())
    {
        is_finished = true;
        finish_time = std::chrono::system_clock::now();
    }
}

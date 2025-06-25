#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <fstream> // Required for std::ofstream
#include <unordered_map>

class Command; // Forward declaration

class Process
{
public:
    std::string name;
    std::vector<std::shared_ptr<Command>> commands;
    bool has_started = false;
    bool is_finished = false;

    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point finish_time;

    std::string output_filename; // Log file name for this process
    std::ofstream output_file;   // Stream to the log file

    std::unordered_map<std::string, uint16_t> variables; //storing variables

    Process(const std::string &name);
    ~Process(); // Closes the output_file

    void add_command(std::shared_ptr<Command> cmd);
    void execute(int core_id); // Executes commands, logs to output_file

    int current_core = -1;         // Core ID where this process is running
    int current_command_index = 0; // Index of current command being executed

    uint16_t get_var(const std::string& var_name);
    void set_var(const std::string& var_name, uint16_t value);
};

#endif // PROCESS_H

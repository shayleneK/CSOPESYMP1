#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

class Command;

class Process
{
public:
    // === Constructors ===
    Process(const std::string &name, int core_id = -1);

    // === Public Data Members ===
    std::string name;
    bool is_finished = false;
    bool has_started = false;
    int current_command_index = 0;
    int current_core = -1; // Default unless set via constructor

    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point finish_time;

    // === Command Management ===
    std::vector<std::shared_ptr<Command>> commands;
    void add_command(std::shared_ptr<Command> cmd);
    size_t get_instruction_count() const;

    // === Execution Logic ===
    void execute(int core_id);
    bool can_execute();

    // === Variable Management ===
    uint16_t get_var(const std::string &var_name);
    void set_var(const std::string &var_name, uint16_t value);

    // === Logging ===
    std::vector<std::string> logs;
    void log_execution(int core_id, const std::string &message);

    // === Getters ===
    std::string getName() const;
    bool isFinished() const;
    bool hasStarted() const;
    int getCurrentCore() const;
    int getCurrentCommandIndex() const;
    std::chrono::system_clock::time_point getStartTime() const;
    std::chrono::system_clock::time_point getFinishTime() const;
    const std::vector<std::string> &getLogs() const;

private:
    // === Internal State ===
    int delay_per_exec = 0;
    int delay_counter = 0;
    std::map<std::string, uint16_t> variables;
};
#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <memory>
#include <iostream>
#include <vector>
#include <stdint.h>

class Process;      
class Command {
public:
    virtual ~Command() = default;
    virtual void execute(int core_id, std::ofstream& output_file, const std::string& process_name) = 0;
};

class PrintCommand : public Command {
private:
    std::string message;

public:
    PrintCommand(const std::string& msg);
    // Executes print, writes to output_file with timestamp and core ID.
    void execute(int core_id, std::ofstream& output_file, const std::string& process_name) override;
};

class SleepCommand : public Command {
private:
    int duration_ms;

public:
    SleepCommand(int duration);
    // Simulates work by pausing the current thread.
    void execute(int core_id, std::ofstream& output_file, const std::string& process_name) override;
};

class DeclareCommand : public Command {
private:
    std::string var_name;
    uint16_t value;

public:
    DeclareCommand(const std::string& var, uint16_t val);
    void execute(int core_id, std::ofstream& output_file, const std::string& process_name) override;
};

class AddCommand : public Command {
private:
    std::string target, op1, op2;
    bool op1_is_var, op2_is_var;
    uint16_t val1, val2;

public:
    AddCommand(const std::string& tgt, const std::string& o1, const std::string& o2,
               bool o1_var = true, bool o2_var = true,
               uint16_t v1 = 0, uint16_t v2 = 0);
    void execute(int core_id, std::ofstream& output_file, const std::string& process_name) override;
};

class SubtractCommand : public Command {
private:
    std::string target, op1, op2;
    bool op1_is_var, op2_is_var;
    uint16_t val1, val2;

public:
    SubtractCommand(const std::string& tgt, const std::string& o1, const std::string& o2,
                    bool o1_var = true, bool o2_var = true,
                    uint16_t v1 = 0, uint16_t v2 = 0);
    void execute(int core_id, std::ofstream& output_file, const std::string& process_name) override;
};

class ForCommand : public Command {
private:
    std::vector<std::shared_ptr<Command>> instructions;
    int repeat;

public:
    ForCommand(const std::vector<std::shared_ptr<Command>>& cmds, int reps);
    void execute(int core_id, std::ofstream& output_file, const std::string& process_name) override;
};

#endif // COMMAND_H

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

class Process;

class Command
{
public:
    virtual ~Command() = default;
    virtual void execute(Process *proc, int core_id, const std::string &process_name) = 0;
};

class PrintCommand : public Command
{
private:
    std::string message;

public:
    PrintCommand(const std::string &msg);
    void execute(Process *proc, int core_id, const std::string &process_name) override;
};

class SleepCommand : public Command
{
private:
    int duration_ms;

public:
    SleepCommand(int duration);
    void execute(Process *proc, int core_id, const std::string &process_name) override;
};

class DeclareCommand : public Command
{
private:
    std::string var_name;
    uint16_t value;

public:
    DeclareCommand(const std::string &var, uint16_t val);
    void execute(Process *proc, int core_id, const std::string &process_name) override;
};

class AddCommand : public Command
{
private:
    std::string target, op1, op2;
    bool op1_is_var, op2_is_var;
    uint16_t val1, val2;

public:
    AddCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
               bool o1_var, bool o2_var, uint16_t v1, uint16_t v2);
    void execute(Process *proc, int core_id, const std::string &process_name) override;
};

class SubtractCommand : public Command
{
private:
    std::string target, op1, op2;
    bool op1_is_var, op2_is_var;
    uint16_t val1, val2;

public:
    SubtractCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
                    bool o1_var, bool o2_var, uint16_t v1, uint16_t v2);
    void execute(Process *proc, int core_id, const std::string &process_name) override;
};

class ForCommand : public Command
{
private:
    std::vector<std::shared_ptr<Command>> instructions;
    int repeat;

public:
    ForCommand(const std::vector<std::shared_ptr<Command>> &cmds, int reps);
    void execute(Process *proc, int core_id, const std::string &process_name) override;
};

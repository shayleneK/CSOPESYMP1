// Command.h
#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

class Process; // Forward declaration

// --- Base Command ---
class Command
{
public:
    virtual ~Command() = default;
    virtual void execute(Process *proc, int coreId, const std::string &processName) = 0;
};

// --- PrintCommand ---
struct PrintSegment
{
    bool isVar;
    std::string value;
};

class PrintCommand : public Command
{
    std::vector<PrintSegment> segments;

public:
    PrintCommand(const std::vector<PrintSegment> &segment);
    PrintCommand(const std::string &msg);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

// --- SleepCommand ---
class SleepCommand : public Command
{
private:
    int durationMs;

public:
    explicit SleepCommand(int duration);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

// --- DeclareCommand ---
class DeclareCommand : public Command
{
private:
    std::string varName;
    uint16_t value;

public:
    DeclareCommand(const std::string &var, uint16_t val);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

// --- AddCommand ---
class AddCommand : public Command
{
private:
    std::string target;
    std::string op1, op2;
    bool op1IsVar, op2IsVar;
    uint16_t val1, val2;

public:
    AddCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
               bool o1Var, bool o2Var, uint16_t v1, uint16_t v2);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

// --- SubtractCommand ---
class SubtractCommand : public Command
{
private:
    std::string target;
    std::string op1, op2;
    bool op1IsVar, op2IsVar;
    uint16_t val1, val2;

public:
    SubtractCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
                    bool o1Var, bool o2Var, uint16_t v1, uint16_t v2);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

// --- ForCommand ---
class ForCommand : public Command
{
private:
    std::vector<std::shared_ptr<Command>> instructions;
    int repeat;

public:
    ForCommand(const std::vector<std::shared_ptr<Command>> &cmds, int reps);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

// --- ReadCommand ---
class ReadCommand : public Command
{
private:
    std::string varName;
    uint32_t address;

public:
    ReadCommand(const std::string &var, uint32_t addr);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

// --- WriteCommand ---
class WriteCommand : public Command
{
private:
    uint32_t address;
    uint16_t value;

public:
    WriteCommand(uint32_t addr, uint16_t val);
    void execute(Process *proc, int coreId, const std::string &processName) override;
};

#endif // COMMAND_H

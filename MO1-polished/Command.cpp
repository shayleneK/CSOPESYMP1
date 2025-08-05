// Command.cpp
#include "Command.h"
#include "Process.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

// --- PrintCommand ---

PrintCommand::PrintCommand(const std::vector<PrintSegment> &segments)
    : segments(segments) {}

PrintCommand::PrintCommand(const std::string &msg)
{
    segments.push_back({false, msg});
}

void PrintCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    std::string output;

    for (const auto &seg : segments)
    {
        if (seg.isVar)
        {
            output += std::to_string(proc->getVar(seg.value));
        }
        else
        {
            output += seg.value;
        }
    }

    // Log with timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    std::ostringstream logEntry;
#ifdef _WIN32
    std::tm localTime;
    localtime_s(&localTime, &timeNow);
    logEntry << "(" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << ") ";
#else
    logEntry << "(" << std::put_time(std::localtime(&timeNow), "%Y-%m-%d %H:%M:%S") << ") ";
#endif
    logEntry << "Core:" << coreId << " - PRINT(\"" << output << "\")";

    std::cout << logEntry.str() << std::endl;
    proc->logExecution(coreId, "PRINT(\"" + output + "\")");
}

// --- SleepCommand ---
SleepCommand::SleepCommand(int duration) : durationMs(duration) {}

void SleepCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    proc->logExecution(coreId, "SLEEP(" + std::to_string(durationMs) + "ms)");
}

// --- DeclareCommand ---
DeclareCommand::DeclareCommand(const std::string &var, uint16_t val)
    : varName(var), value(val) {}

void DeclareCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    bool success = proc->declareVar(varName, value);
    if (!success)
    {
        std::ostringstream oss;
        oss << "Failed to declare '" << varName << "': symbol table full (32 variables max)";
        proc->logExecution(coreId, oss.str());
    }
    else
    {
        proc->logExecution(coreId, "DECLARE " + varName + " = " + std::to_string(value));
    }
}

// --- AddCommand ---
AddCommand::AddCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
                       bool o1Var, bool o2Var, uint16_t v1, uint16_t v2)
    : target(tgt), op1(o1), op2(o2), op1IsVar(o1Var), op2IsVar(o2Var),
      val1(v1), val2(v2) {}

void AddCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    uint16_t a = op1IsVar ? proc->getVar(op1) : val1;
    uint16_t b = op2IsVar ? proc->getVar(op2) : val2;
    uint32_t result = a + b;
    if (result > 65535)
        result = 65535;
    proc->declareVar(target, static_cast<uint16_t>(result));
    proc->logExecution(coreId, "ADD " + target + " = " + std::to_string(a) + " + " + std::to_string(b));
}

// --- SubtractCommand ---
SubtractCommand::SubtractCommand(const std::string &tgt, const std::string &o1, const std::string &o2,
                                 bool o1Var, bool o2Var, uint16_t v1, uint16_t v2)
    : target(tgt), op1(o1), op2(o2), op1IsVar(o1Var), op2IsVar(o2Var),
      val1(v1), val2(v2) {}

void SubtractCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    int32_t a = op1IsVar ? proc->getVar(op1) : val1;
    int32_t b = op2IsVar ? proc->getVar(op2) : val2;
    int32_t result = a - b;
    if (result < 0)
        result = 0;
    proc->declareVar(target, static_cast<uint16_t>(result));
    proc->logExecution(coreId, "SUB " + target + " = " + std::to_string(a) + " - " + std::to_string(b));
}

// --- ForCommand ---
ForCommand::ForCommand(const std::vector<std::shared_ptr<Command>> &cmds, int reps)
    : instructions(cmds), repeat(reps) {}

void ForCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    for (int i = 0; i < repeat; ++i)
    {
        for (const auto &cmd : instructions)
        {
            cmd->execute(proc, coreId, processName);
        }
    }
    proc->logExecution(coreId, "FOR x" + std::to_string(repeat));
}

// --- ReadCommand ---
ReadCommand::ReadCommand(const std::string &var, uint32_t addr)
    : varName(var), address(addr) {}

void ReadCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    uint16_t value = proc->readMemory(address);
    proc->declareVar(varName, value);
    proc->logExecution(coreId, "READ " + varName + " <- MEM[" + std::to_string(address) + "]");
}

WriteCommand::WriteCommand(uint32_t addr, uint16_t val, bool isVar, const std::string &var)
    : address(addr), value(val), isVariable(isVar), varName(var) {}

void WriteCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    uint16_t toWrite = value;
    if (isVariable)
    {
        if (!proc->hasVar(varName))
        {
            std::cerr << "[" << processName << "] Error: Variable '" << varName << "' not declared.\n";
            return;
        }
        toWrite = proc->getVar(varName);
    }

    proc->writeMemory(address, toWrite);
}

InvalidAccessCommand::InvalidAccessCommand(uint32_t addr) : address(addr) {}

void InvalidAccessCommand::execute(Process *process, int coreId, const std::string &procName)
{
    process->readMemory(address); // or writeMemory(address, 0)
}
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
/*
PrintCommand::PrintCommand(const std::string &msg) : message(msg) {}

void PrintCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    std::string output;

    // Handle case: "prefix" + var
    size_t plusPos = message.find('+');
    if (plusPos != std::string::npos)
    {
        std::string prefix = message.substr(0, plusPos);
        std::string varName = message.substr(plusPos + 1);

        // Clean quotes and whitespace
        prefix.erase(std::remove_if(prefix.begin(), prefix.end(),
                                    [](char c)
                                    { return c == '"' || std::isspace(c); }),
                     prefix.end());
        varName.erase(std::remove_if(varName.begin(), varName.end(), ::isspace), varName.end());

        uint16_t val = proc->getVar(varName);
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

    // Get timestamp
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
*/
PrintCommand::PrintCommand(const std::string &msg) : message(msg)
{}

void PrintCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    std::string output;

    // Tokenize by '+' and process each part
    std::istringstream iss(message);
    std::string token;
    std::vector<std::string> tokens;

    // Split by '+' (manually, since std::getline with '+' as delimiter skips whitespace)
    size_t start = 0;
    size_t pos = message.find('+');

    while (pos != std::string::npos)
    {
        tokens.push_back(message.substr(start, pos - start));
        start = pos + 1;
        pos = message.find('+', start);
    }
    tokens.push_back(message.substr(start)); // Last token

    // Process each token
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::string t = tokens[i];
        // Trim whitespace
        t.erase(t.begin(), std::find_if(t.begin(), t.end(), [](int ch) { return !std::isspace(ch); }));
        t.erase(std::find_if(t.rbegin(), t.rend(), [](int ch) { return !std::isspace(ch); }).base(), t.end());

        if (t.empty()) continue;

        // Check if it's a quoted string
        if (t.front() == '"' && t.back() == '"')
        {
            t = t.substr(1, t.size() - 2); // Remove quotes
            output += t;
        }
        else if (t.front() == '"' && t.length() > 1)
        {
            t = t.substr(1);
            output += t;
        }
        else if (t.back() == '"' && t.length() > 1)
        {
            t = t.substr(0, t.size() - 1);
            output += t;
        }
        else
        {
            // It's a variable
            uint16_t val = proc->getVar(t);
            output += std::to_string(val);
        }
    }

    // Get timestamp
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

    /*
void ReadCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    uint16_t value = proc->readMemory(address);
    proc->declareVar(varName, value);
    proc->logExecution(coreId, "READ " + varName + " <- MEM[" + std::to_string(address) + "]");
}
    */
void ReadCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    if (address >= proc->getMemorySize())
    {
        proc->markAsError(address);
        return;
    }

    uint16_t value = proc->readMemory(address);

    bool success = proc->declareVar(varName, value);
    if (!success)
    {
        proc->logExecution(coreId, "Failed to declare " + varName + " (symbol table full)");
        return;
    }

    std::ostringstream oss;
    oss << "READ " << varName << " <- MEM[0x" << std::hex << address << std::dec << "] = " << value;
    proc->logExecution(coreId, oss.str());
}

// --- WriteCommand ---
WriteCommand::WriteCommand(uint32_t addr, uint16_t val)
    : address(addr), value(val) {}

/*
void WriteCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    proc->writeMemory(address, value);
    proc->logExecution(coreId, "WRITE MEM[" + std::to_string(address) + "] = " + std::to_string(value));
}
*/
void WriteCommand::execute(Process *proc, int coreId, const std::string &processName)
{
    if (address >= proc->getMemorySize())
    {
        proc->markAsError(address);
        return;
    }

    proc->writeMemory(address, value);

    std::ostringstream oss;
    oss << "WRITE MEM[0x" << std::hex << address << std::dec << "] = " << value;
    proc->logExecution(coreId, oss.str());
}
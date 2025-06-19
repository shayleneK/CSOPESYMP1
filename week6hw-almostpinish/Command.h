#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <memory>
#include <iostream>

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

#endif // COMMAND_H

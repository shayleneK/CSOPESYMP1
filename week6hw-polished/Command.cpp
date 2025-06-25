#include "Command.h"
#include <iostream>
#include <thread>

PrintCommand::PrintCommand(const std::string &msg) : message(msg) {}

void PrintCommand::execute(int core_id, std::ofstream &output_file, const std::string &process_name)
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif

    // if (output_file.is_open()) {
    //     output_file << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "."
    //                 << std::setfill('0') << std::setw(3) << ms.count() << "] ";
    //     output_file << "CPU Core " << core_id << ": " << message << std::endl;
    // } else {
    //     std::cerr << "[ERROR] PrintCommand for process " << process_name
    //               << ": Output file not open, message not logged: " << message << std::endl;
    // }
}

SleepCommand::SleepCommand(int duration) : duration_ms(duration) {}

void SleepCommand::execute(int core_id, std::ofstream &output_file, const std::string &process_name)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
}

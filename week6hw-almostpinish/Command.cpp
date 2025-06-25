#include "Command.h"
#include "Process.h"
#include <algorithm>
#include <iostream>
#include <thread>

Process* current_process = nullptr;

PrintCommand::PrintCommand(const std::string& msg) : message(msg) {}

void PrintCommand::execute(int core_id, std::ofstream& output_file, const std::string& process_name) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif

    if (output_file.is_open()) {
        output_file << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "."
                    << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        output_file << "CPU Core " << core_id << ": " << message << std::endl;
    } else {
        std::cerr << "[ERROR] PrintCommand for process " << process_name
                  << ": Output file not open, message not logged: " << message << std::endl;
    }
}

SleepCommand::SleepCommand(int duration) : duration_ms(duration) {}

void SleepCommand::execute(int core_id, std::ofstream& output_file, const std::string& process_name) {
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
}

DeclareCommand::DeclareCommand(const std::string& var, uint16_t val)
    : var_name(var), value(val) {}

void DeclareCommand::execute(int core_id, std::ofstream& output_file, const std::string& process_name) {
    current_process->set_var(var_name, value);
}

AddCommand::AddCommand(const std::string& tgt, const std::string& o1, const std::string& o2,
                       bool o1_var, bool o2_var, uint16_t v1, uint16_t v2)
    : target(tgt), op1(o1), op2(o2), op1_is_var(o1_var), op2_is_var(o2_var),
      val1(v1), val2(v2) {}

void AddCommand::execute(int core_id, std::ofstream& output_file, const std::string& process_name) {
    uint16_t a = op1_is_var ? current_process->get_var(op1) : val1;
    uint16_t b = op2_is_var ? current_process->get_var(op2) : val2;
    uint32_t result = a + b;
    if (result > 65535) result = 65535;
    current_process->set_var(target, static_cast<uint16_t>(result));
}

SubtractCommand::SubtractCommand(const std::string& tgt, const std::string& o1, const std::string& o2,
                                 bool o1_var, bool o2_var, uint16_t v1, uint16_t v2)
    : target(tgt), op1(o1), op2(o2), op1_is_var(o1_var), op2_is_var(o2_var),
      val1(v1), val2(v2) {}

void SubtractCommand::execute(int core_id, std::ofstream& output_file, const std::string& process_name) {
    int32_t a = op1_is_var ? current_process->get_var(op1) : val1;
    int32_t b = op2_is_var ? current_process->get_var(op2) : val2;
    int32_t result = a - b;
    if (result < 0) result = 0;
    current_process->set_var(target, static_cast<uint16_t>(result));
}

ForCommand::ForCommand(const std::vector<std::shared_ptr<Command>>& cmds, int reps)
    : instructions(cmds), repeat(reps) {}

void ForCommand::execute(int core_id, std::ofstream& output_file, const std::string& process_name) {
    for (int i = 0; i < repeat; ++i) {
        for (auto& cmd : instructions) {
            cmd->execute(core_id, output_file, process_name);
        }
    }
}
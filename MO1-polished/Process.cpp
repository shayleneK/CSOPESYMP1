#include "Process.h"
#include "Command.h"
#include "ConsoleManager.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

Process::Process(const std::string &name, int core_id)
    : name(name), current_core(core_id), delay_per_exec(0), delay_counter(0) {}

void Process::add_command(std::shared_ptr<Command> cmd)
{
    commands.push_back(cmd);
}

void Process::execute(int core_id)
{
    if (is_finished)
        return;

    current_core = core_id;
    if (!ConsoleManager::getInstance()->isRunning()) // or custom global flag
        return;

    if (!has_started)
    {
        has_started = true;
        start_time = std::chrono::system_clock::now();
    }

    if (delay_counter > 0)
    {
        delay_counter--;
        return;
    }

    if (current_command_index >= commands.size())
    {
        is_finished = true;
        finish_time = std::chrono::system_clock::now();
        log_execution(core_id, "Process " + name + " has completed all its commands.");
        return;
    }

    commands[current_command_index]->execute(this, core_id, name);
    current_command_index++;
    delay_counter = delay_per_exec;
}

uint16_t Process::get_var(const std::string &var_name)
{
    auto it = variables.find(var_name);
    if (it == variables.end())
    {
        return 0;
    }
    return it->second;
}

void Process::set_var(const std::string &var_name, uint16_t value)
{
    variables[var_name] = value;
}

bool Process::can_execute()
{
    if (delay_counter == 0)
    {
        return true;
    }
    else
    {
        delay_counter--;
        return false;
    }
}

size_t Process::get_instruction_count() const
{
    return commands.size();
}

void Process::log_execution(int core_id, const std::string &message)
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_c);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

    std::ostringstream oss;
    oss << "(" << timestamp << ") Core:" << core_id << " - " << message;

    logs.push_back(oss.str());
}

// --- Getters ---
std::string Process::getName() const { return name; }
bool Process::isFinished() const { return is_finished; }
bool Process::hasStarted() const { return has_started; }
int Process::getCurrentCore() const { return current_core; }
int Process::getCurrentCommandIndex() const { return current_command_index; }
std::chrono::system_clock::time_point Process::getStartTime() const { return start_time; }
std::chrono::system_clock::time_point Process::getFinishTime() const { return finish_time; }
const std::vector<std::string> &Process::getLogs() const { return logs; }

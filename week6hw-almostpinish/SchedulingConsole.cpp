#include "SchedulingConsole.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include "Process.h"

SchedulingConsole::SchedulingConsole(Scheduler *sched)
    : AConsole("Scheduling Console"), scheduler(sched) {}

void SchedulingConsole::onEnabled()
{
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "| Scheduling Console is now active.                |\n";
    std::cout << "+--------------------------------------------------+\n";
}

void SchedulingConsole::display()
{
    render_header();
    render_running_processes(scheduler->get_running_processes());
    render_finished_processes(scheduler->get_finished_processes());

    auto cpu_data = scheduler->get_cpu_stats();
    if (!cpu_data.empty())
    {
        render_cpu_utilization(cpu_data);
    }

    render_footer();
}

void SchedulingConsole::process(std::string &command) // handling non-drawing logic
{
    // Optional background logic here
}

void SchedulingConsole::render_header()
{
    const int width = 80;
    std::string title = "CSOPESY Operating System Emulator - FCFS Scheduler";
    std::string padding((width - static_cast<int>(title.length())) / 2, ' ');

    std::cout << std::string(width, '-') << "\n";
    std::cout << padding << title << "\n";
    std::cout << std::string(width, '-') << "\n\n";
}

void SchedulingConsole::render_footer()
{
    std::cout << std::endl;
    std::cout << "Type \"screen -ls\" to view processes or \"cpu-util\" for CPU stats." << std::endl;
    std::cout << "Type \"exit\" to quit the emulator." << std::endl;
}

void SchedulingConsole::render_running_processes(const std::vector<std::shared_ptr<Process>> &processes)
{
    std::cout << "Running Processes:\n";
    if (processes.empty())
    {
        std::cout << "  (None)\n";
        return;
    }

    for (const auto &p : processes)
    {
        std::ostringstream oss;
        auto start_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                 p->start_time.time_since_epoch())
                                 .count();
        oss << " - " << p->name << " (Started: " << start_seconds << ")";
        std::cout << oss.str() << "\n";
    }
    std::cout << std::endl;
}

void SchedulingConsole::render_finished_processes(const std::vector<std::shared_ptr<Process>> &processes)
{
    std::cout << "Finished Processes:\n";
    if (processes.empty())
    {
        std::cout << "  (None)\n";
        return;
    }

    for (const auto &p : processes)
    {
        if (p->is_finished)
        {
            auto finish_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                      p->finish_time.time_since_epoch())
                                      .count();
            std::cout << " - " << p->name << " (Finished: " << finish_seconds << ")\n";
        }
    }
    std::cout << std::endl;
}

void SchedulingConsole::render_cpu_utilization(const std::map<int, std::map<std::string, float>> &stats)
{
    std::cout << "CPU Utilization:\n";
    for (const auto &[core_id, data] : stats)
    {
        float util = data.at("util");
        int queue_size = static_cast<int>(data.at("queue_size"));

        std::cout << "  CPU " << core_id << ": ";
        std::cout << "| Util(%): " << std::setw(3) << util;
        std::cout << " | Num Procs: " << queue_size << " |\n";
    }

    std::cout << std::endl;
}

bool SchedulingConsole::isRunning() const
{
    return true; // Always running unless closed by user
}
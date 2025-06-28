#include "SchedulingConsole.h"
#include "Scheduler.h"
#include "Process.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

SchedulingConsole::SchedulingConsole(Scheduler *sched)
    : AConsole("Scheduling Console"), scheduler(sched) {}

void SchedulingConsole::onEnabled()
{
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "| Scheduling Console is now active.                |\n";
    std::cout << "+--------------------------------------------------+\n";
}

// void SchedulingConsole::display()
// {
//     // Clear screen
//     // Render header
//     render_header();

//     // Display running processes
//     render_running_processes(scheduler->get_running_processes());

//     // Display finished processes
//     render_finished_processes(scheduler->get_finished_processes());

//     // Display CPU Utilization (if applicable)
//     /* if (!scheduler->get_cpu_stats().empty())
//     {
//         render_cpu_utilization(scheduler->get_cpu_stats());
//     } */

//     // Footer
//     render_footer();
// }

void SchedulingConsole::process(std::string &command) // handling non-drawing logic
{
    // Optional: Add background logic here if needed
    // E.g., periodic updates, logging, etc.
}

void SchedulingConsole::clear_screen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void SchedulingConsole::render_header()
{
    const int width = 80;

    std::string schedulerType = "";
    if (dynamic_cast<FCFSScheduler *>(scheduler))
    {
        schedulerType = "FCFS Scheduler";
    }
    else if (dynamic_cast<RRScheduler *>(scheduler))
    {
        schedulerType = "RR Scheduler";
    }

    std::string title = "CSOPESY Operating System Emulator - " + schedulerType;
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
    //std::cout << "[DEBUG] Rendering Running Processes " << "\n";

    std::cout << "Running Processes:\n";
    if (processes.empty())
    {
        std::cout << "  (None)\n";
        return;
    }

    for (const auto &p : processes)
    {
        if (!p->has_started)
        {
            std::cout << " - " << p->name << " (Scheduled, not started)\n";
            continue;
        }

        std::time_t start_time_t = std::chrono::system_clock::to_time_t(p->start_time);
        std::tm *start_tm = std::localtime(&start_time_t);

        std::ostringstream oss;
        oss << " - " << p->name
            << "   (" << std::put_time(start_tm, "%Y-%m-%d %H:%M:%S") << ")"
            << "  Core: " << p->current_core
            << ", " << p->current_command_index << " / 100";

        std::cout << oss.str() << "\n";
    }
    std::cout << std::endl;
}

bool SchedulingConsole::isRunning() const
{
    // Dummy implementation, update if needed
    return false;
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
            std::time_t finish_time_t = std::chrono::system_clock::to_time_t(p->finish_time);
            std::tm *finish_tm = std::localtime(&finish_time_t);

            std::ostringstream oss;
            oss << " - " << p->name
                << "   (" << std::put_time(finish_tm, "%Y-%m-%d %H:%M:%S") << ")"
                << " Finished "
                << " 100 / 100";
            std::cout << oss.str() << "\n";
        }
    }
    std::cout << std::endl;
}

/* void SchedulingConsole::render_cpu_utilization(const std::map<int, std::map<std::string, float>> &stats)
{
    std::cout << "CPU Utilization:\n";

    for (const auto &[core_id, data] : stats)
    {
        float util = data.at("util");
        int queue_size = static_cast<int>(data.at("queue_size"));

        std::cout << "  Core " << core_id << ": ";
        std::cout << "| Util(%): " << std::setw(3) << util;
        std::cout << " | Num Procs: " << queue_size << " |\n";
    }

    std::cout << std::endl;
} */
#include "SchedulingConsole.h"
#include "Scheduler.h"
#include "Process.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

SchedulingConsole::SchedulingConsole(Scheduler* sched)
    : AConsole("Scheduling Console"), scheduler(sched) {}

void SchedulingConsole::onEnabled()
{
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "| Scheduling Console is now active.                |\n";
    std::cout << "+--------------------------------------------------+\n";
}

void SchedulingConsole::display()
{
    if (!scheduler) {
        std::cout << "[ERROR] No scheduler initialized.\n";
        return;
    }

    render_header();
    render_running_processes(scheduler->get_running_processes());
    render_finished_processes(scheduler->get_finished_processes());
    render_footer();
}

void SchedulingConsole::process(std::string& command)
{
}

bool SchedulingConsole::isRunning() const
{
    return scheduler != nullptr;
}

void SchedulingConsole::render_header()
{
    const int width = 80;

    std::string schedulerType = "";
    if (dynamic_cast<FCFSScheduler*>(scheduler)) {
        schedulerType = "FCFS Scheduler";
    } else if (dynamic_cast<RRScheduler*>(scheduler)) {
        schedulerType = "RR Scheduler";
    }

    std::string title = "CSOPESY Operating System Emulator - " + schedulerType;
    std::string padding((width - static_cast<int>(title.length())) / 2, ' ');

    std::cout << std::string(width, '-') << "\n";
    std::cout << padding << title << "\n";
    std::cout << std::string(width, '-') << "\n";
}

void SchedulingConsole::render_running_processes(const std::vector<std::shared_ptr<Process>>& processes)
{
    std::cout << "Running Processes:\n";
    if (processes.empty()) {
        std::cout << " (None)\n";
        return;
    }

    for (const auto& p : processes) {
        std::ostringstream oss;
        oss << " - " << p->getName();

        if (p->hasStarted()) {
            auto start = std::chrono::system_clock::to_time_t(p->getStartTime());
            oss << " (" << std::put_time(std::localtime(&start), "%Y-%m-%d %H:%M:%S") << ")";
        }

        oss << " " << p->getCurrentCommandIndex() << " / " << p->get_instruction_count();

        try {
            int core_id = scheduler->get_core_of_process(p);
            if (core_id != -1) {
                oss << " (Core: " << core_id << ")";
            }
        } catch (...) {
            oss << " (Core: ?)";
        }

        std::cout << oss.str() << "\n";
    }
    std::cout << std::endl;
}

void SchedulingConsole::render_finished_processes(const std::vector<std::shared_ptr<Process>>& processes)
{
    std::cout << "Finished Processes:\n";
    if (processes.empty()) {
        std::cout << " (None)\n";
        return;
    }

    for (const auto& p : processes) {
        if (p->isFinished()) {
            auto finish = std::chrono::system_clock::to_time_t(p->getFinishTime());
            std::cout << " - " << p->getName()
                      << " (" << std::put_time(std::localtime(&finish), "%Y-%m-%d %H:%M:%S") << ")"
                      << " 100 / 100\n";
        }
    }
    std::cout << std::endl;
}

void SchedulingConsole::render_footer()
{
    std::cout << "Type \"screen -ls\" to view processes or \"exit\" to quit.\n";
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
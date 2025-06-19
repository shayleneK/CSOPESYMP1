#include "MainConsole.h"
#include "SchedulingConsole.h"
#include "Process.h"
#include "Scheduler.h"
#include "Command.h"
#include "ConsoleManager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>

void printHeader()
{
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    std::cout << "~   ___     ___     ___      ___    ___     ___   __   __~\n";
    std::cout << "~  / __|   / __|   / _ \\    | _ \\  | __|   / __|  \\ \\ / /~\n";
    std::cout << "~ | (__    \\__ \\  | (_) |   |  _/  | _|    \\__ \\   \\ V / ~\n";
    std::cout << "~  \\___|   |___/   \\___/   _|_|_   |___|   |___/   _|_|_ ~\n";
    std::cout << "~_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_| \"\"\" |_|\"\"\"\"\"|_|\"\"\"\"\"|_| \"\"\" |~\n";
    std::cout << "~\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'~\n";
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
}

MainConsole::MainConsole(Scheduler *sched)
    : AConsole("Main Console"), scheduler(sched) {}

void MainConsole::onEnabled()
{
    printHeader();
    std::cout << "| Type 'initialize', 'cpu-util', or 'screen -ls'        |\n";
    std::cout << "+--------------------------------------------------+\n\n";
}

void MainConsole::display()
{
    clear_screen();
    onEnabled();
}

void MainConsole::process()
{
    std::string command;

    while (running)
    {
        std::cout << "> ";
        std::getline(std::cin, command);

        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "exit")
        {
            std::cout << "[CMD] Exiting emulator...\n";
            running = false;
        }
        else if (command == "clear")
        {
            clear_screen();
            onEnabled();
        }
        else if (command == "screen -ls")
        {
            auto running_procs = scheduler->get_running_processes();
            auto finished_procs = scheduler->get_finished_processes();

            std::cout << "\nRunning Processes:\n";
            if (running_procs.empty())
            {
                std::cout << "  (None)\n";
            }
            else
            {
                for (const auto &p : running_procs)
                {
                    std::cout << " - " << p->name << " (Started: "
                              << std::chrono::duration_cast<std::chrono::seconds>(
                                     p->start_time.time_since_epoch())
                                     .count()
                              << ")\n";
                }
            }

            std::cout << "\nFinished Processes:\n";
            if (finished_procs.empty())
            {
                std::cout << "  (None)\n";
            }
            else
            {
                for (const auto &p : finished_procs)
                {
                    std::cout << " - " << p->name << " (Finished: "
                              << std::chrono::duration_cast<std::chrono::seconds>(
                                     p->finish_time.time_since_epoch())
                                     .count()
                              << ")\n";
                }
            }

            std::cout << std::endl;
        }
        else if (command == "cpu-util")
        {
            auto stats = scheduler->get_cpu_stats();
            std::cout << "\nCPU Utilization:\n";

            for (const auto &[core_id, data] : stats)
            {
                float util = data.at("util");
                int queue_size = static_cast<int>(data.at("queue_size"));

                std::cout << "  CPU " << core_id << ": | Util(%): " << util
                          << " | Num Procs: " << queue_size << " |\n";
            }

            std::cout << std::endl;
        }
        else if (command == "initialize")
        {
            std::cout << "[CMD] Initializing test processes...\n";

            for (int i = 0; i < 10; ++i)
            {
                auto p = std::make_shared<Process>("Process" + std::to_string(i));
                for (int j = 0; j < 100; ++j)
                {
                    p->add_command(std::make_shared<PrintCommand>("Hello from " + p->name + "!"));
                }
                scheduler->add_process(p);
            }
        }
        else if (command == "scheduler-test")
        {
            std::cout << "[CMD] Starting scheduler...\n";
            scheduler->start();
        }
        else if (command == "scheduler-stop")
        {
            std::cout << "[CMD] Stopping scheduler...\n";
            scheduler->shutdown();
        }
        else if (command == "report-util")
        {
            ConsoleManager::getInstance()->drawConsole(); // Refresh UI
        }
        else if (command.empty())
        {
            continue;
        }
        else
        {
            std::cout << "[ERROR] Unknown command: " << command << "\n";
        }
    }
}

bool MainConsole::isRunning() const
{
    return running;
}

void MainConsole::clear_screen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
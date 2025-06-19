#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>

#include "Scheduler.h"
#include "SchedulingConsole.h"
#include "Process.h"
#include "Command.h"
#include "ConsoleManager.h"
#include "GlobalState.h"
#include "RRScheduler.h"

// Global memory and symbol table definitions
std::unordered_map<std::string, uint16_t> global_symbol_table;
std::unordered_map<int, uint16_t> memory_heap;

int main()
{
    std::cout << "[INFO] Initializing OS emulator...\n";

    // Initialize scheduler with 4 CPU cores
    //Scheduler scheduler(4);

    RRScheduler scheduler(1, 50);

    std::cout << "[INFO] Creating 10 test processes, each with 100 print commands...\n";
    for (int i = 0; i < 10; ++i)
    {
        auto process = std::make_shared<Process>("Process" + std::to_string(i));

        // Add 100 print commands and sleep commands
        for (int j = 0; j < 100; ++j)
        {
            process->add_command(std::make_shared<PrintCommand>("\"Hello World from " + process->name + "\""));
            // process->add_command(std::make_shared<SleepCommand>(1)); // Simulate 1ms work
        }
        process->add_command(std::make_shared<PrintCommand>("Process " + process->name + " has completed all its commands."));

        scheduler.add_process(process);
    }

    std::cout << "[INFO] Starting scheduler...\n";
    scheduler.start();

    std::cout << "[INFO] Launching console interface...\n";
    ConsoleManager::getInstance()->initialize();
    auto console = std::make_shared<SchedulingConsole>(&scheduler);
    ConsoleManager::getInstance()->addConsole(console);

    bool running = true;
    std::string input;

    std::cout << "\nType 'screen -ls' to view processes or 'cpu-util' for stats.\n";
    std::cout << "Type 'exit' to quit the emulator.\n\n";
    std::cout << "Enter command: ";

    while (running)
    {
        ConsoleManager::getInstance()->process();
        ConsoleManager::getInstance()->drawConsole();

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // UI refresh rate

        if (std::cin >> input)
        {
            if (input == "exit")
            {
                std::cout << "[INFO] Shutting down emulator...\n";
                running = false;
            }
            else if (input == "screen")
            {
                std::string sub_command;
                if (std::cin >> sub_command && sub_command == "-ls")
                {
                    std::cout << "[CMD] Displaying current processes (console will refresh shortly)....\n";
                }
                else
                {
                    std::cout << "[ERROR] Unknown 'screen' subcommand: " << sub_command << "\n";
                }
            }
            else if (input == "cpu-util")
            {
                std::cout << "[CMD] Displaying CPU utilization...\n";
                auto stats = scheduler.get_cpu_stats();
                for (const auto &[core_id, data] : stats)
                {
                    float util = data.at("util");
                    int queue_size = static_cast<int>(data.at("queue_size"));

                    std::cout << "CPU " << core_id << ": | Util(%): " << std::fixed << std::setprecision(2) << util
                              << " | Num Procs (Ready Queue): " << queue_size << " |\n";
                }
            }
            else
            {
                std::cout << "[ERROR] Unknown command: " << input << "\n";
            }
        }
        else
        {
            // Handle EOF or input errors
        }

        // Check if all processes are finished
        if (!scheduler.get_all_processes().empty() &&
            scheduler.get_finished_processes().size() == scheduler.get_all_processes().size())
        {
            std::cout << "[INFO] All processes have finished execution. Type 'exit' to quit.\n";
        }
        std::cout << "Enter command: "; // Re-prompt for input
    }

    ConsoleManager::destroy();
    scheduler.shutdown();

    std::cout << "[INFO] Emulator exited successfully.\n";
    return 0;
}

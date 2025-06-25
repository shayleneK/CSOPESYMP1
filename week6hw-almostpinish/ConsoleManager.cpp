#include "ConsoleManager.h"
#include "AConsole.h"
#include "MainConsole.h"
#include "ScreenConsole.h"
#include "SchedulingConsole.h"
#include "ConfigManager.h"
#include "RRScheduler.h"
// #include "MarqueeConsole.h"
// #include "MemorySimulationConsole.h" 

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>

ConsoleManager *ConsoleManager::instance = nullptr;

ConsoleManager::ConsoleManager(Scheduler *scheduler)
{
    consoleTable[MAIN_CONSOLE] = std::make_shared<MainConsole>(scheduler);
    consoleTable[SCHEDULING_CONSOLE] = std::make_shared<SchedulingConsole>(scheduler);
    // consoleTable[MARQUEE_CONSOLE] = std::make_shared<MarqueeConsole>(scheduler); 
    // consoleTable[MEMORY_CONSOLE] = std::make_shared<MemorySimulationConsole>(scheduler); 
    this->currentConsole = MAIN_CONSOLE;
    this->running = true;
}

void ConsoleManager::initializeConsoles()
{
    for (auto &[type, console] : consoleTable)
    {
        console->onEnabled();
    }
}

ConsoleManager *ConsoleManager::getInstance(Scheduler *scheduler)
{
    if (!instance && scheduler != nullptr)
    {
        instance = new ConsoleManager(scheduler);
    }
    return instance;
}

void ConsoleManager::drawConsole()
{
    auto it = consoleTable.find(currentConsole);
    if (it != consoleTable.end())
    {
        it->second->display();
    }
}

void ConsoleManager::switchConsole(ConsoleType type)
{
    if (consoleTable.find(type) != consoleTable.end())
    {
        currentConsole = type;
        drawConsole();
    }
    else
    {
        std::cout << "[ERROR] Console type not supported.\n";
    }
}

void ConsoleManager::setRunning(bool running)
{
    this->running = running;
}

bool ConsoleManager::isRunning() const
{
    return running;
}

void ConsoleManager::processInput()
{
    std::string command;
    std::cout << "> ";
    std::getline(std::cin, command);

    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    if (command == "exit")
    {
        setRunning(false);
    }
    else if (command == "clear")
    {
        drawConsole();
    }
    else if (command == "initialize")
    {
        // Load from config.txt
        ConfigManager cfg;
        if (!cfg.load("config.txt")) {
            std::cout << "[ERROR] config.txt missing or invalid.\n";
            return;
        }

        std::string scheduler_type = cfg.getString("scheduler_type", "rr");
        int cores = cfg.getInt("num_cores", 4);
        int processes = cfg.getInt("num_processes", 10);
        int commands = cfg.getInt("num_commands", 100);
        int quantum = cfg.getInt("quantum", 100);

        if (scheduler_type == "rr") {
            scheduler = std::make_unique<RRScheduler>(cores, quantum);
        } else {
            scheduler = std::make_unique<Scheduler>(cores); // FCFS fallback
        }

        consoleTable[SCHEDULING_CONSOLE] = std::make_shared<SchedulingConsole>(scheduler.get());

        for (int i = 0; i < processes; ++i) {
            auto p = std::make_shared<Process>("Process" + std::to_string(i));
            for (int j = 0; j < commands; ++j) {
                p->add_command(std::make_shared<PrintCommand>("Hello from " + p->name));
            }
            scheduler->add_process(p);
        }

        std::cout << "[INFO] System initialized with " << processes << " processes.\n";
    }
    else if (command == "report-util")
    {
        if (!scheduler) {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
        std::ofstream report("csopesy-log.txt");
        auto stats = scheduler->get_cpu_stats();

        report << "[CPU UTILIZATION REPORT]\n";
        for (const auto& [core_id, data] : stats) {
            report << "CPU " << core_id << ": Util(%) = " << data.at("util")
                   << ", Queue Size = " << static_cast<int>(data.at("queue_size")) << "\n";
        }
        std::cout << "[INFO] CPU report saved to csopesy-log.txt\n";
    }
    else if (command == "scheduler-start")
    {
        clearScreen();
        switchConsole(SCHEDULING_CONSOLE);
        scheduler->start_core_threads();
        if (auto *rrsched = dynamic_cast<RRScheduler *>(scheduler.get()))
        {
            rrsched->start();
        }
    }
    else if (command == "marquee")
    {
        // switchConsole(MARQUEE_CONSOLE); // Uncomment if MarqueeConsole is implemented
    }
    else
    {
        std::cout << "[ERROR] Unknown command: " << command << "\n";
    }
}

void ConsoleManager::clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void ConsoleManager::destroy()
{
    delete instance;
    instance = nullptr;
}
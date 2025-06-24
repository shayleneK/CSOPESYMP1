#include "ConsoleManager.h"
#include "AConsole.h"
#include "MainConsole.h"
// #include "MarqueeConsole.h"
#include "ScreenConsole.h"
#include "SchedulingConsole.h"
// #include "MemorySimulationConsole.h"
#include <map>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "RRScheduler.h"

ConsoleManager *ConsoleManager::instance = nullptr;

ConsoleManager::ConsoleManager(Scheduler *scheduler)
{
    // Initialize consoles
    consoleTable[MAIN_CONSOLE] = std::make_shared<MainConsole>();
    // consoleTable[MARQUEE_CONSOLE] = std::make_shared<MarqueeConsole>(scheduler);
    consoleTable[SCHEDULING_CONSOLE] = std::make_shared<SchedulingConsole>(scheduler);
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
        // clear_screen();
        drawConsole();
    }
    else if (command == "initialize")
    {
        // IMPORTANT DUMMY VALUES

        /*  if (!read_config())
         {
             std::cout << "Error: Failed to read config.txt\n";
             continue;
         } */

        if (true) // scheduler_type == "rr"
        {
            scheduler = std::make_unique<RRScheduler>(4, 100);
        }
        /* else if (scheduler_type == "fcfs")
        {
            scheduler = std::make_unique<FcfsScheduler>(num_cores);
        } */
        else
        {
            std::cout << "Error: Unknown scheduler type in config.txt\n";
        }

        scheduler_initialized = true;
        std::cout << "System initialized successfully.\n";
    }
    else if (command == "screen -ls")
    {
        consoleTable[SCHEDULING_CONSOLE]->display();
    }
    else if (command == "marquee")
    {
        switchConsole(MARQUEE_CONSOLE);
    }
    else if (command == "scheduler-start")
    {
        clearScreen();
        switchConsole(SCHEDULING_CONSOLE);
        scheduler->start_core_threads();
        if (auto *rrsched = dynamic_cast<RRScheduler *>(scheduler.get()))
        {
            rrsched->start_process_generator(); // starts add_dummy_process()
        }
    }
    else if (command == "help")
    {
        std::cout << "\nAvailable Commands:\n";
        std::cout << " - initialize      : Switch to Main Console\n";
        std::cout << " - screen -s       : Switch to Main Console\n";
        std::cout << " - initialize      : Switch to Main Console\n";
        std::cout << " - scheduler-start : Start Scheduler\n";
        std::cout << " - marquee          : Switch to Marquee Console\n";
        std::cout << " - exit             : Quit the emulator\n\n";
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

// screens
void ConsoleManager::createScreen(const std::string &name)
{
    if (m_consoleTable.find(name) != m_consoleTable.end())
    {
        std::cout << "Screen \"" << name << "\" already exists.\n";
        return;
    }

    auto newConsole = std::make_shared<ScreenConsole>(name); // make sure ScreenConsole exists
    m_consoleTable[name] = newConsole;
    // m_previousConsole = m_activeConsole;
    // m_activeConsole = newConsole;

    std::cout << "Created and switched to screen: " << name << "\n";
}

void ConsoleManager::switchToScreen(const std::string &name)
{
    auto it = m_consoleTable.find(name);
    if (it != m_consoleTable.end())
    {
        // m_previousConsole = m_activeConsole;
        // m_activeConsole = it->second;
        std::cout << "Switched to screen: " << name << "\n";
    }
    else
    {
        std::cout << "Screen \"" << name << "\" not found.\n";
    }
}

void ConsoleManager::listScreens() const
{
    std::cout << "Available screens:\n";
    for (const auto &[name, _] : m_consoleTable)
    {
        std::cout << " - " << name << "\n";
    }
}

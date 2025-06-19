#include "ConsoleManager.h"
#include "AConsole.h"
#include "MainConsole.h"
// #include "MarqueeConsole.h"
#include "SchedulingConsole.h"
// #include "MemorySimulationConsole.h"

#include <iostream>
#include <algorithm>
#include <cctype>

ConsoleManager *ConsoleManager::instance = nullptr;

ConsoleManager::ConsoleManager(Scheduler *scheduler)
{
    // Initialize consoles
    consoleTable[MAIN_CONSOLE] = std::make_shared<MainConsole>(scheduler);
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
        // switchConsole(MAIN_CONSOLE);
        // TO DO: FIX LOGIC
    }
    else if (command == "marquee")
    {
        switchConsole(MARQUEE_CONSOLE);
    }
    else if (command == "scheduler-start")
    {
        clearScreen();
        switchConsole(SCHEDULING_CONSOLE);
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
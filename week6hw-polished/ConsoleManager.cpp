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
#include "FCFSScheduler.h"

ConsoleManager *ConsoleManager::instance = nullptr;

ConsoleManager::ConsoleManager(Scheduler *scheduler)
{
    // Initialize consoles
    consoleTable[MAIN_CONSOLE] = std::make_shared<MainConsole>();
    // consoleTable[MARQUEE_CONSOLE] = std::make_shared<MarqueeConsole>(scheduler);
    // consoleTable[SCHEDULING_CONSOLE] = std::make_shared<SchedulingConsole>(scheduler);

    this->currentConsole = MAIN_CONSOLE;
    this->running = true;
}

std::shared_ptr<AConsole> ConsoleManager::getActiveConsole() const
{
    return m_activeConsole;
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
        if (getActiveConsole() == consoleTable.at(MAIN_CONSOLE))
        {
            std::cout << "Exiting emulator.\n";
            setRunning(false);
        }
        else
        {
            std::shared_ptr<AConsole> target = consoleTable.at(MAIN_CONSOLE);
            m_activeConsole = target;
            m_previousConsole = nullptr;
            drawConsole();
            std::cout << "Switched to Main Console\n";
        }
    }

    else if (command == "clear")
    {
        clearScreen();
        drawConsole();
    }
    else if (command == "initialize")
    {
        // Simulate config-based initialization
        scheduler = std::make_unique<FCFSScheduler>(4);
        scheduler->start_core_threads();
        consoleTable[SCHEDULING_CONSOLE] = std::make_shared<SchedulingConsole>(scheduler.get());

        scheduler_initialized = true;
        std::cout << "System initialized successfully.\n";
    }
    else if (command.rfind("screen -s ", 0) == 0)
    {
        std::string name = command.substr(10); // extracts name after "screen -s "
        if (!name.empty())
        {
            createConsole("screen", name);
        }
        else
        {
            std::cout << "[ERROR] Screen name cannot be empty.\n";
        }
    }
    else if (command.rfind("screen -r ", 0) == 0)
    {
        std::string name = command.substr(10); // after "screen -r "
        if (!name.empty())
        {
            auto it = m_consoleTable.find(name);
            if (it != m_consoleTable.end())
            {
                if (m_activeConsole && m_activeConsole != consoleTable.at(MAIN_CONSOLE))
                {
                    m_previousConsole = m_activeConsole;
                }

                m_activeConsole = it->second;
                switchConsole(name);
            }
            else
            {
                std::cout << "Console \"" << name << "\" not found.\n";
            }
        }
        else
        {
            std::cout << "[ERROR] Console name required.\n";
        }
    }
    else if (command == "screen -ls")
    {
        listScreens();
    }
    else if (command == "marquee")
    {
        switchConsole(MARQUEE_CONSOLE);
    }
    else if (command == "scheduler-start")
    {
        clearScreen();
        scheduler->start_process_generator();
        switchConsole(SCHEDULING_CONSOLE);
        if (auto *rrsched = dynamic_cast<RRScheduler *>(scheduler.get()))
        {
            rrsched->start();
        }
    }
    else if (command == "help")
    {
        std::cout << "\nAvailable Commands:\n";
        std::cout << " - initialize        : Initialize the scheduler\n";
        std::cout << " - screen -s <name>  : Create and switch to a new screen\n";
        std::cout << " - screen -r <name>  : Resume a named screen\n";
        std::cout << " - screen -ls        : List all created screens\n";
        std::cout << " - scheduler-start   : Start the scheduler process loop\n";
        std::cout << " - marquee           : Switch to Marquee Console\n";
        std::cout << " - exit              : Return to previous screen or quit\n";
        std::cout << " - clear             : Clear screen\n";
    }
    else
    {
        // Fallback: pass command to active console
        if (m_activeConsole)
        {
            m_activeConsole->process(command);
        }
        else
        {
            std::cout << "[ERROR] No active console to handle command.\n";
        }
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
void ConsoleManager::createConsole(const std::string &type, const std::string &name)
{
    if (m_consoleTable.find(name) != m_consoleTable.end())
    {
        std::cout << "Console \"" << name << "\" already exists.\n";
        return;
    }

    std::shared_ptr<AConsole> newConsole;

    if (type == "screen")
    {
        newConsole = std::make_shared<ScreenConsole>(name);
    }
    else
    {
        std::cout << "Unknown console type: " << type << "\n";
        return;
    }

    m_previousConsole = m_activeConsole;
    m_activeConsole = newConsole;
    m_consoleTable[name] = newConsole;

    std::cout << "Created and switched to console: " << name << "\n";
    m_activeConsole->display(); // Add this
}

void ConsoleManager::switchConsole(const std::string &name)
{
    auto it = m_consoleTable.find(name);
    if (it != m_consoleTable.end())
    {
        m_previousConsole = m_activeConsole;
        m_activeConsole = it->second;
        std::cout << "Switched to console: " << name << "\n";
        m_activeConsole->display(); // Add this
    }
    else
    {
        std::cout << "Console \"" << name << "\" not found.\n";
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

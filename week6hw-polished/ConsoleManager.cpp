#include "ConsoleManager.h"
#include <iostream>
#include <algorithm>

// Singleton instance
ConsoleManager *ConsoleManager::instance = nullptr;

// Private constructor
ConsoleManager::ConsoleManager() {}

// Singleton access method
ConsoleManager *ConsoleManager::getInstance()
{
    if (!instance)
    {
        instance = new ConsoleManager();
    }
    return instance;
}

void ConsoleManager::clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Singleton cleanup
void ConsoleManager::destroy()
{
    delete instance;
    instance = nullptr;
}

void ConsoleManager::initialize()
{
    // Add default console windows here
}

void ConsoleManager::process()
{
    for (const auto &console : consoles)
    {
        console->process();
    }
}

void ConsoleManager::drawConsole()
{
    clearScreen();
    for (const auto &console : consoles)
    {
        console->display();
    }
}

bool ConsoleManager::isRunning()
{
    // Determine if any console wants to continue running
    for (const auto &console : consoles)
    {
        if (console->isRunning())
        {
            return true;
        }
    }

    return false;
}

void ConsoleManager::addConsole(std::shared_ptr<AConsole> console)
{
    consoles.push_back(console);
}

void ConsoleManager::removeConsole(const std::string &name)
{
    consoles.erase(
        std::remove_if(consoles.begin(), consoles.end(),
                       [&](const std::shared_ptr<AConsole> &c) -> bool
                       {
                           return c->getName() == name;
                       }),
        consoles.end());
}
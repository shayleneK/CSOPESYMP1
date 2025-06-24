#include "MainConsole.h"
#include "ConsoleManager.h"
// #include "ScreenConsole.h" // Ensure this exists
#include <iostream>
#include <algorithm>
#include <cstdlib>

MainConsole::MainConsole()
    : AConsole("Main Console") {} // just pass the name to AConsole

void MainConsole::printHeader() const
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

void MainConsole::display()
{
    clearConsole();
    printHeader();
}

void MainConsole::clearConsole() const
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void MainConsole::onEnabled()
{
    // your logic here
}

bool MainConsole::isRunning() const
{
    // your logic here
    return false;
}

void MainConsole::process(std::string &command)
{
    ConsoleManager &manager = *ConsoleManager::getInstance();

    if (command.rfind("screen -s ", 0) == 0)
    {
        std::string name = command.substr(10);
        if (!name.empty())
            manager.createScreen(name);
        // return true;
    }
    else if (command.rfind("screen -r ", 0) == 0)
    {
        std::string name = command.substr(10);
        manager.switchToScreen(name);
        // return true;
    }
    else if (command == "screen -l")
    {
        manager.listScreens();
        // return false;
    }
    else if (command == "clear")
    {
        clearConsole();
        // return false;
    }
    else if (command == "exit")
    {
        std::cout << "Exiting CLI. Goodbye!\n";
        manager.setRunning(false);
        // return false;
    }
    else
    {
        std::cout << "Unknown command: " << command << "\n";
        // return false;
    }

    // return false;
}

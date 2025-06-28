#include "ConsoleManager.h"
#include "Scheduler.h"

int main()
{
    ConsoleManager *consoleManager = ConsoleManager::getInstance();
    consoleManager->drawConsole();

    while (consoleManager->isRunning())
    {
        consoleManager->processInput();
    }

    ConsoleManager::destroy();
    return 0;
}
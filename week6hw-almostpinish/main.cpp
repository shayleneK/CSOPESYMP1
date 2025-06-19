#include "ConsoleManager.h"
#include "Scheduler.h"

int main()
{
    Scheduler scheduler(4); // 4 CPU cores
    ConsoleManager *consoleManager = ConsoleManager::getInstance(&scheduler);
    consoleManager->drawConsole();

    while (consoleManager->isRunning())
    {
        consoleManager->processInput();
    }

    ConsoleManager::destroy();
    return 0;
}
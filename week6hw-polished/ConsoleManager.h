#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

#include <vector>
#include <memory>
#include "AConsole.h"

class ConsoleManager
{
private:
    static ConsoleManager *instance; // Singleton instance
    std::vector<std::shared_ptr<AConsole>> consoles;

    // Private constructor to enforce singleton
    ConsoleManager();
    void clearScreen();

public:
    // Singleton access method
    static ConsoleManager *getInstance();

    // Initialize the console manager
    void initialize();

    // Main loop methods
    void process();
    void drawConsole();
    bool isRunning();

    // Console window management
    void addConsole(std::shared_ptr<AConsole> console);
    void removeConsole(const std::string &name);

    // Cleanup
    static void destroy();
};

#endif // CONSOLEMANAGER_H
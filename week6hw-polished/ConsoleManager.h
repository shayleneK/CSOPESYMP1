#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

#include "AConsole.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <map>

// Console types
enum ConsoleType
{
    MAIN_CONSOLE,
    MARQUEE_CONSOLE,
    SCHEDULING_CONSOLE,
    MEMORY_CONSOLE
};

class Scheduler; // Forward declaration

class ConsoleManager
{
private:
    static ConsoleManager *instance;
    std::unordered_map<ConsoleType, std::shared_ptr<AConsole>> consoleTable;
    ConsoleType currentConsole;
    bool running;

    ConsoleManager(Scheduler *scheduler);
    void initializeConsoles();
    std::map<std::string, std::shared_ptr<AConsole>> m_consoleTable;
    std::unique_ptr<Scheduler> scheduler;
    bool scheduler_initialized = false;

public:
    static ConsoleManager *getInstance(Scheduler *scheduler = nullptr);

    void drawConsole();
    void processInput();

    void switchConsole(ConsoleType type);
    void setRunning(bool running);
    void initialize();                                  // <- Add this
    void addConsole(std::shared_ptr<AConsole> console); // <- Add this
    void process();

    bool isRunning() const;
    void clearScreen();

    static void destroy();

    void createScreen(const std::string &name);
    void switchToScreen(const std::string &name);
    void listScreens() const;
};

#endif // CONSOLEMANAGER_H
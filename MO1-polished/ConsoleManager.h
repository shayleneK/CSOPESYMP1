#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

#include "AConsole.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <atomic>

#include "Scheduler.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"
#include "MarqueeConsole.h"

// Console types
enum ConsoleType
{
    MAIN_CONSOLE,
    SCHEDULING_CONSOLE,
    MEMORY_CONSOLE,
    MARQUEE_CONSOLE
};

class Scheduler; // Forward declaration

class ConsoleManager
{

private:
    static ConsoleManager *instance;
    std::unordered_map<ConsoleType, std::shared_ptr<AConsole>> consoleTable;
    ConsoleType currentConsole;
    bool running;
    std::shared_ptr<AConsole> m_activeConsole;
    std::shared_ptr<AConsole> m_previousConsole;

    std::atomic<bool> cpu_loop_running = false;
    std::atomic<int> global_tick = 0;
    ConsoleManager();
    void initializeConsoles();
    std::map<std::string, std::shared_ptr<AConsole>> m_consoleTable;
    std::unique_ptr<Scheduler> scheduler;
    bool scheduler_initialized = false;
    std::shared_ptr<AConsole> getActiveConsole() const;

    static std::atomic<uint64_t> cpu_cycles; // Shared CPU counter
    std::thread cpuThread;
    bool runningCpuLoop = false;

    void cpuCycleLoop(); // The actual loop function
    std::atomic<uint64_t> total_cycles{0};
    std::atomic<uint64_t> busy_cycles{0};

public:
    static ConsoleManager *getInstance();
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

    std::shared_ptr<AConsole> getConsoleByName(const std::string &name) const;

    void createConsole(const std::string &type, const std::string &name);
    void switchConsole(const std::string &name);
    // void listScreens() const;
    bool hasConsole(const std::string &name) const;

    void render_header(std::ostream &out);
    void render_footer(std::ostream &out);
    void render_running_processes(const std::vector<std::shared_ptr<Process>> &processes, std::ostream &out);
    void render_finished_processes(const std::vector<std::shared_ptr<Process>> &processes, std::ostream &out);

    void startCpuLoop();
    void stopCpuLoop();
    static uint64_t getCpuCycles();
    double getCpuUtilization() const;

    bool start_flag = false; 
};

#endif // CONSOLEMANAGER_H
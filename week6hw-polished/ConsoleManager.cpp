#include "ConsoleManager.h"
#include "ConfigManager.h"
#include "Command.h"
#include "AConsole.h"
#include "MainConsole.h"
#include "Process.h"
#include "ProcessFactory.h"
#include "ScreenConsole.h"
#include "SchedulingConsole.h"
#include "RRScheduler.h"
#include "FCFSScheduler.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>

ConsoleManager *ConsoleManager::instance = nullptr;
std::atomic<uint64_t> ConsoleManager::cpu_cycles(0);

ConsoleManager::ConsoleManager()
{
    consoleTable[MAIN_CONSOLE] = std::make_shared<MainConsole>();
    consoleTable[MARQUEE_CONSOLE] = std::make_shared<MarqueeConsole>();

    currentConsole = MAIN_CONSOLE;
    m_activeConsole = consoleTable[MAIN_CONSOLE];
    running = true;
}

ConsoleManager *ConsoleManager::getInstance()
{
    if (!instance)
        instance = new ConsoleManager();
    return instance;
}

void ConsoleManager::initializeConsoles()
{
    for (auto &[type, console] : consoleTable)
    {
        console->onEnabled();
    }
}

void ConsoleManager::drawConsole()
{
    if (consoleTable.count(currentConsole))
        consoleTable[currentConsole]->display();
}

void ConsoleManager::switchConsole(ConsoleType type)
{
    if (consoleTable.count(type))
    {
        currentConsole = type;
        drawConsole();
    }
    else
    {
        std::cout << "[ERROR] Console type not supported.\n";
    }
}

void ConsoleManager::switchConsole(const std::string &name)
{
    if (m_consoleTable.count(name))
    {
        m_previousConsole = m_activeConsole;
        m_activeConsole = m_consoleTable[name];
        std::cout << "Switched to console: " << name << "\n";
        m_activeConsole->display();
    }
    else
    {
        std::cout << "Console \"" << name << "\" not found.\n";
    }
}

std::shared_ptr<AConsole> ConsoleManager::getActiveConsole() const
{
    return m_activeConsole;
}

std::shared_ptr<AConsole> ConsoleManager::getConsoleByName(const std::string &name) const
{
    if (m_consoleTable.count(name))
        return m_consoleTable.at(name);
    return nullptr;
}

void ConsoleManager::createConsole(const std::string &type, const std::string &name)
{
    if (m_consoleTable.count(name))
    {
        std::cout << "Console \"" << name << "\" already exists.\n";
        return;
    }

    if (type == "screen")
    {
        m_consoleTable[name] = std::make_shared<ScreenConsole>(name);
        std::cout << "Created screen: " << name << "\n";
    }
    else
    {
        std::cout << "Unknown console type: " << type << "\n";
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

void ConsoleManager::setRunning(bool value)
{
    running = value;
}

bool ConsoleManager::isRunning() const
{
    return running;
}

bool ConsoleManager::hasConsole(const std::string &name) const
{
    return m_consoleTable.count(name);
}

uint64_t ConsoleManager::getCpuCycles()
{
    return cpu_cycles.load();
}

void ConsoleManager::startCpuLoop()
{
    if (runningCpuLoop)
        return;

    runningCpuLoop = true;
    cpuThread = std::thread(&ConsoleManager::cpuCycleLoop, this);
}

void ConsoleManager::cpuCycleLoop()
{
    while (runningCpuLoop && isRunning())
    {
        cpu_cycles.fetch_add(1);

        if (scheduler && scheduler_initialized)
            scheduler->on_cpu_cycle(cpu_cycles.load());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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
            try
            {
                if (scheduler)
                    scheduler->shutdown(); // Might be triggering termination
            }
            catch (const std::exception &e)
            {
                std::cerr << "[ERROR] Exception during scheduler shutdown: " << e.what() << "\n";
            }
            catch (...)
            {
                std::cerr << "[ERROR] Unknown exception during scheduler shutdown.\n";
            }

            setRunning(false); // Tells main loop to break
        }
        else
        {
            m_activeConsole = consoleTable[MAIN_CONSOLE];
            drawConsole();
        }
    }
    else if (command == "clear")
    {
        clearScreen();
        drawConsole();
    }
    else if (command == "initialize")
    {
        ConfigManager cfg;
        if (!cfg.load("config.txt"))
        {
            std::cout << "[ERROR] config.txt missing or invalid.\n";
            return;
        }

        int num_cpu = cfg.getInt("num-cpu", 2);
        std::string scheduler_type = cfg.getString("scheduler", "rr");
        int quantum = cfg.getInt("quantum-cycles", 5);
        int batch_freq = cfg.getInt("batch-process-freq", 1);
        int min_ins = cfg.getInt("min-ins", 1000);
        int max_ins = cfg.getInt("max-ins", 2000);
        int delay_per_exec = cfg.getInt("delay-per-exec", 100);

        if (scheduler_type == "rr")
            scheduler = std::make_unique<RRScheduler>(num_cpu, quantum, min_ins, max_ins, delay_per_exec);
        else
            scheduler = std::make_unique<FCFSScheduler>(num_cpu, min_ins, max_ins);

        scheduler->set_batch_frequency(batch_freq);
        scheduler->start_core_threads();
        startCpuLoop();
        scheduler_initialized = true;
    }
    else if (command.rfind("screen -s ", 0) == 0)
    {
        std::string name = command.substr(10);
        createConsole("screen", name);

        auto proc = ProcessFactory::generate_dummy_process(name, scheduler->get_min_instructions(), scheduler->get_max_instructions());
        proc->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));
        scheduler->add_process(proc);

        auto screen = std::dynamic_pointer_cast<ScreenConsole>(m_consoleTable[name]);
        if (screen)
            screen->attachProcess(proc);

        switchConsole(name);
        std::cout << "[screen] Process \"" << name << "\" created and added.\n";
    }
    else if (command.rfind("screen -r ", 0) == 0)
    {
        std::string name = command.substr(10);
        switchConsole(name);
    }
    else if (command == "screen -ls")
    {
        render_header();
        render_running_processes(scheduler->get_running_processes());
        render_finished_processes(scheduler->get_finished_processes());
        render_footer();
    }
    else if (command == "marquee")
    {
        switchConsole(MARQUEE_CONSOLE);
    }
    else if (command == "scheduler-start")
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
        scheduler->start();
        std::cout << "[INFO] Scheduler started.\n";
    }
    else if (command == "process-smi")
    {
        auto screen = std::dynamic_pointer_cast<ScreenConsole>(getActiveConsole());
        if (!screen)
        {
            std::cout << "[ERROR] 'process-smi' must be used in a screen console.\n";
            return;
        }

        auto proc = screen->getAttachedProcess();
        if (!proc)
        {
            std::cout << "[ERROR] No process attached.\n";
            return;
        }

        std::cout << "\n[Process Info]\n";
        std::cout << "Name: " << proc->getName() << "\n";
        std::cout << "Status: " << (proc->isFinished() ? "Finished" : "Running") << "\n";

        if (proc->hasStarted())
        {
            auto st = std::chrono::system_clock::to_time_t(proc->getStartTime());
            std::cout << "Start Time: " << std::put_time(std::localtime(&st), "%Y-%m-%d %H:%M:%S") << "\n";
        }

        if (proc->isFinished())
        {
            auto ft = std::chrono::system_clock::to_time_t(proc->getFinishTime());
            std::cout << "Finish Time: " << std::put_time(std::localtime(&ft), "%Y-%m-%d %H:%M:%S") << "\n";
        }

        std::cout << "Instructions Executed: " << proc->getCurrentCommandIndex() << " / " << proc->get_instruction_count() << "\n";

        std::cout << "Logs:\n";
        for (const auto &log : proc->getLogs())
            std::cout << log << "\n";
    }
    else
    {
        if (m_activeConsole)
            m_activeConsole->process(command);
        else
            std::cout << "[ERROR] No active console.\n";
    }
}

void ConsoleManager::render_header()
{
    std::string type = dynamic_cast<FCFSScheduler *>(scheduler.get()) ? "FCFS Scheduler" : dynamic_cast<RRScheduler *>(scheduler.get()) ? "RR Scheduler"
                                                                                                                                        : "Unknown";

    std::string title = "CSOPESY Operating System Emulator - " + type;
    std::string padding((80 - title.length()) / 2, ' ');

    std::cout << std::string(80, '-') << "\n";
    std::cout << padding << title << "\n";
    std::cout << std::string(80, '-') << "\n\n";
}

void ConsoleManager::render_footer()
{
    std::cout << "\nType \"screen -ls\" to view processes or \"exit\" to quit.\n";
}

void ConsoleManager::render_running_processes(const std::vector<std::shared_ptr<Process>> &list)
{
    std::cout << "Running Processes:\n";
    if (list.empty())
    {
        std::cout << "  (None)\n";
        return;
    }

    for (const auto &p : list)
    {
        std::ostringstream oss;
        oss << " - " << p->getName();
        if (p->hasStarted())
        {
            auto start = std::chrono::system_clock::to_time_t(p->getStartTime());
            oss << " (" << std::put_time(std::localtime(&start), "%Y-%m-%d %H:%M:%S") << ")";
        }
        oss << "  " << p->getCurrentCommandIndex() << " / " << p->get_instruction_count();
        std::cout << oss.str() << "\n";
    }
}

void ConsoleManager::render_finished_processes(const std::vector<std::shared_ptr<Process>> &list)
{
    std::cout << "Finished Processes:\n";
    if (list.empty())
    {
        std::cout << "  (None)\n";
        return;
    }

    for (const auto &p : list)
    {
        if (!p->isFinished())
            continue;

        auto finish = std::chrono::system_clock::to_time_t(p->getFinishTime());
        std::cout << " - " << p->getName()
                  << " (" << std::put_time(std::localtime(&finish), "%Y-%m-%d %H:%M:%S") << ")\n";
    }
}

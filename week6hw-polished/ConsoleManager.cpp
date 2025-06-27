#include "ConsoleManager.h"
#include "ConfigManager.h"
#include "Command.h"
#include "AConsole.h"
#include "MainConsole.h"
#include "Process.h"
#include "ProcessFactory.h"
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

#include <iomanip>
#include <sstream>

ConsoleManager *ConsoleManager::instance = nullptr;
ConsoleManager::ConsoleManager()
{
    // Initialize consoles
    consoleTable[MAIN_CONSOLE] = std::make_shared<MainConsole>();
    consoleTable[MARQUEE_CONSOLE] = std::make_shared<MarqueeConsole>();
    // consoleTable[MARQUEE_CONSOLE] = std::make_shared<MarqueeConsole>(scheduler);
    // consoleTable[SCHEDULING_CONSOLE] = std::make_shared<SchedulingConsole>(scheduler);

    this->currentConsole = MAIN_CONSOLE;
    this->m_activeConsole = consoleTable[MAIN_CONSOLE];
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

ConsoleManager *ConsoleManager::getInstance()
{
    if (!instance)
        instance = new ConsoleManager();
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
    // else if (command == "process-smi")
    // {
    //     if (!process)
    //     {
    //         std::cout << "[ERROR] No process attached to this screen.\n";
    //         return;
    //     }

    //     std::cout << "\n[Process Info]\n";
    //     std::cout << "Name: " << process->name << "\n";
    //     std::cout << "Finished: " << (process->is_finished ? "Yes" : "No") << "\n";
    //     std::cout << "Progress: " << process->current_command_index << " / "
    //               << process->get_instruction_count() << "\n";
    //     std::cout << "Running on Core: " << process->current_core << "\n";

    // }
    else if (command == "initialize")
    {
        ConfigManager cfg;
        if (!cfg.load("config.txt"))
        {
            std::cout << "[ERROR] config.txt missing or invalid.\n";
            return;
        }

        int num_cpu = cfg.getInt("num-cpu", 2);
        std::string scheduler_type = cfg.getString("scheduler", "rr"); // holds "rr" or "fcfs"
        int quantum_cycles = cfg.getInt("quantum-cycles", 5);
        int batch_process_freq = cfg.getInt("batch-process-freq", 1);
        int min_ins = cfg.getInt("min-ins", 1000);
        int max_ins = cfg.getInt("max-ins", 2000);
        int delay_per_exec = cfg.getInt("delay-per-exec", 0);

        if (scheduler_type == "rr")
        {
            scheduler = std::make_unique<RRScheduler>(num_cpu, quantum_cycles, min_ins, max_ins);
        }
        else
        {
            scheduler = std::make_unique<FCFSScheduler>(num_cpu, min_ins, max_ins);
        }

        scheduler->start_core_threads();
        scheduler_initialized = true;
    }

    else if (command.rfind("screen -s ", 0) == 0)
    {
        std::string name = command.substr(10); // Extract name

        if (!name.empty())
        {
            createConsole("screen", name);
            auto proc = ProcessFactory::generate_dummy_process(name, scheduler->get_min_instructions(), scheduler->get_max_instructions());
            proc->add_command(std::make_shared<PrintCommand>("Process " + name + " has completed all its commands."));
            scheduler->add_process(proc);

            std::cout << "[screen] Process \"" << name << "\" created and added to the ready queue.\n";
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

        render_header();

        // Display running processes
        render_running_processes(scheduler->get_running_processes());

        // Display finished processes
        render_finished_processes(scheduler->get_finished_processes());

        // Display CPU Utilization (if applicable)
        /* if (!scheduler->get_cpu_stats().empty())
        {
            render_cpu_utilization(scheduler->get_cpu_stats());
        } */

        // Footer
        render_footer();
        // listScreens();
        // consoleTable[SCHEDULING_CONSOLE]->display();
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

    /* else if (command == "report-util")
    {
        if (!scheduler) {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
        std::ofstream report("csopesy-log.txt");

        report << "[CPU UTILIZATION REPORT]\n";
        auto stats = scheduler->get_cpu_stats();
        for (const auto& [core_id, data] : stats) {
            report << "CPU " << core_id << ": Util(%) = " << data.at("util")
                   << ", Queue Size = " << static_cast<int>(data.at("queue_size")) << "\n";
        }
        std::cout << "[INFO] CPU report saved to csopesy-log.txt\n";
    } */

    else if (command == "report-util")
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }

        std::ofstream report("csopesy-log.txt");
        if (!report.is_open())
        {
            std::cout << "[ERROR] Failed to open csopesy-log.txt for writing.\n";
            return;
        }

        // Render header
        const int width = 80;
        std::string schedulerType;
        if (dynamic_cast<FCFSScheduler *>(scheduler.get()))
        {
            schedulerType = "FCFS Scheduler";
        }
        else if (dynamic_cast<RRScheduler *>(scheduler.get()))
        {
            schedulerType = "RR Scheduler";
        }
        else
        {
            schedulerType = "Unknown Scheduler";
        }
        std::string title = "CSOPESY Operating System Emulator - " + schedulerType;
        std::string padding((width - static_cast<int>(title.length())) / 2, ' ');
        report << std::string(width, '-') << "\n";
        report << padding << title << "\n";
        report << std::string(width, '-') << "\n\n";

        // Running Processes
        auto running = scheduler->get_running_processes();
        report << "Running Processes:\n";
        if (running.empty())
        {
            report << "  (None)\n";
        }
        else
        {
            for (const auto &p : running)
            {
                if (!p->has_started)
                {
                    report << " - " << p->name << " (Scheduled, not started)\n";
                }
                else
                {
                    std::time_t start_time_t = std::chrono::system_clock::to_time_t(p->start_time);
                    std::tm *start_tm = std::localtime(&start_time_t);
                    report << " - " << p->name << "   ("
                           << std::put_time(start_tm, "%Y-%m-%d %H:%M:%S") << ")"
                           << "  Core: " << p->current_core
                           << ", " << p->current_command_index << " / 100\n";
                }
            }
        }

        report << "\n";

        // Finished Processes
        auto finished = scheduler->get_finished_processes();
        report << "Finished Processes:\n";
        if (finished.empty())
        {
            report << "  (None)\n";
        }
        else
        {
            for (const auto &p : finished)
            {
                if (p->is_finished)
                {
                    std::time_t finish_time_t = std::chrono::system_clock::to_time_t(p->finish_time);
                    std::tm *finish_tm = std::localtime(&finish_time_t);
                    report << " - " << p->name << "   ("
                           << std::put_time(finish_tm, "%Y-%m-%d %H:%M:%S") << ")"
                           << " Finished  100 / 100\n";
                }
            }
        }

        report << "\n";

        // Footer
        report << "Type \"screen -ls\" to view processes or \"cpu-util\" for CPU stats.\n";
        report << "Type \"exit\" to quit the emulator.\n";

        report.close();
        std::cout << "[INFO] screen -ls output saved to csopesy-log.txt\n";
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

void ConsoleManager::render_header()
{
    const int width = 80;

    std::string schedulerType = "";
    if (dynamic_cast<FCFSScheduler *>(scheduler.get()))
    {
        schedulerType = "FCFS Scheduler";
    }
    else if (dynamic_cast<RRScheduler *>(scheduler.get()))
    {
        schedulerType = "RR Scheduler";
    }
    else
    {
        schedulerType = "Unknown Scheduler";
    }

    std::string title = "CSOPESY Operating System Emulator - " + schedulerType;
    std::string padding((width - static_cast<int>(title.length())) / 2, ' ');

    std::cout << std::string(width, '-') << "\n";
    std::cout << padding << title << "\n";
    std::cout << std::string(width, '-') << "\n\n";
}

void ConsoleManager::render_footer()
{
    std::cout << std::endl;
    std::cout << "Type \"screen -ls\" to view processes or \"cpu-util\" for CPU stats." << std::endl;
    std::cout << "Type \"exit\" to quit the emulator." << std::endl;
}

void ConsoleManager::render_running_processes(const std::vector<std::shared_ptr<Process>> &processes)
{
    std::cout << "[DEBUG] Rendering Running Processes " << "\n";

    std::cout << "Running Processes:\n";
    if (processes.empty())
    {
        std::cout << "  (None)\n";
        return;
    }

    for (const auto &p : processes)
    {
        if (!p->has_started)
        {
            std::cout << " - " << p->name << " (Scheduled, not started)\n";
            continue;
        }

        std::time_t start_time_t = std::chrono::system_clock::to_time_t(p->start_time);
        std::tm *start_tm = std::localtime(&start_time_t);

        std::ostringstream oss;
        oss << " - " << p->name
            << "   (" << std::put_time(start_tm, "%Y-%m-%d %H:%M:%S") << ")"
            << "  Core: " << p->current_core
            << ", " << p->current_command_index << " / " << p->get_instruction_count();
        ;

        std::cout << oss.str() << "\n";
    }
    std::cout << std::endl;
}

void ConsoleManager::render_finished_processes(const std::vector<std::shared_ptr<Process>> &processes)
{
    std::cout << "Finished Processes:\n";
    if (processes.empty())
    {
        std::cout << "  (None)\n";
        return;
    }

    for (const auto &p : processes)
    {
        if (p->is_finished)
        {
            std::time_t finish_time_t = std::chrono::system_clock::to_time_t(p->finish_time);
            std::tm *finish_tm = std::localtime(&finish_time_t);

            std::ostringstream oss;
            oss << " - " << p->name
                << "   (" << std::put_time(finish_tm, "%Y-%m-%d %H:%M:%S") << ")"
                << " Finished "
                << p->get_instruction_count() << " / " << p->get_instruction_count();

            std::cout << oss.str() << "\n";
        }
    }
    std::cout << std::endl;
}

#include "ConsoleManager.h"
#include "ConfigManager.h"
#include "Command.h"
#include "AConsole.h"
#include "MainConsole.h"
#include "Process.h"
#include "ProcessFactory.h"
#include "ScreenConsole.h"
#include "RRScheduler.h"
#include "FCFSScheduler.h"
#include "Scheduler.h"

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

void ConsoleManager::initialize(const ConfigManager &cfg)
{
    int num_cpu = cfg.getInt("num-cpu", 2);
    std::string scheduler_type = cfg.getString("scheduler", "rr");
    int quantum = cfg.getInt("quantum-cycles", 5);
    int batch_freq = cfg.getInt("batch-process-freq", 1);
    int min_ins = cfg.getInt("min-ins", 1000);
    int max_ins = cfg.getInt("max-ins", 2000);
    int delay_per_exec = cfg.getInt("delay-per-exec", 100);
    int max_overall_mem = cfg.getInt("max-overall-mem", 100000);
    int mem_per_frame = cfg.getInt("mem-per-frame", 1000);
    min_mem_per_proc = cfg.getInt("min-mem-per-proc", 512);
    max_mem_per_proc = cfg.getInt("max-mem-per-proc", 2048);

    std::cout << "[Config] num-cpu: " << num_cpu << "\n"
              << "[Config] scheduler: " << scheduler_type << "\n"
              << "[Config] quantum-cycles: " << quantum << "\n"
              << "[Config] batch-process-freq: " << batch_freq << "\n"
              << "[Config] min-ins: " << min_ins << "\n"
              << "[Config] max-ins: " << max_ins << "\n"
              << "[Config] delay-per-exec: " << delay_per_exec << "\n"
              << "[Config] max-overall-mem: " << max_overall_mem << "\n"
              << "[Config] mem-per-frame: " << mem_per_frame << "\n"
              << "[Config] min-mem-per-proc: " << min_mem_per_proc << "\n"
              << "[Config] max-mem-per-proc: " << max_mem_per_proc << "\n";
    std::cout << "----------------------------------------\n";

    // Create MemoryManager
    memoryManager = std::make_unique<MemoryManager>(max_overall_mem, mem_per_frame);

    // Create ProcessFactory
    processFactory = std::make_unique<ProcessFactory>(mem_per_frame, memoryManager.get());

    // Create Scheduler
    if (scheduler_type == "rr")
        scheduler = std::make_unique<RRScheduler>(
            num_cpu,
            quantum,
            min_ins,
            max_ins,
            delay_per_exec,
            *memoryManager);
    else
        scheduler = std::make_unique<FCFSScheduler>(num_cpu, min_ins, max_ins);

    scheduler->set_batch_frequency(batch_freq);
    scheduler->start_core_threads();
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

    if (type == "screen")
    {
        m_consoleTable[name] = std::make_shared<ScreenConsole>(name);
        // std::cout << "Created screen: " << name << "\n";
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

void ConsoleManager::stopCpuLoop()
{
    // std::cerr << "[DEBUG] stopCpuLoop called\n";
    runningCpuLoop = false;
    if (cpuThread.joinable())
        cpuThread.join();
    // std::cerr << "[DEBUG] cpuThread.join() done\n";
}

void ConsoleManager::cpuCycleLoop()
{
    while (runningCpuLoop && isRunning())
    {
        cpu_cycles.fetch_add(1);
        total_cycles.fetch_add(1);

        if (scheduler && scheduler_initialized)
        {
            auto running = scheduler->get_running_processes();
            if (!running.empty())
                busy_cycles.fetch_add(1);

            scheduler->on_cpu_cycle(cpu_cycles.load());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

double ConsoleManager::getCpuUtilization() const
{
    uint64_t total = total_cycles.load();
    if (total == 0)
        return 0.0;

    return 100.0 * busy_cycles.load() / total;
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
                    ConsoleManager::getInstance()->stopCpuLoop(); // STOP THIS FIRST
                std::cerr << "CPU stopped " << "\n";

                if (scheduler)
                    scheduler->shutdown(); // Then shut down scheduler logic

                setRunning(false); // breaks main loop
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
        if (scheduler_initialized)
        {
            std::cout << "[ERROR] Already initialized.\n";
            return;
        }

        ConfigManager cfg;
        if (!cfg.load("config.txt"))
        {
            std::cout << "[ERROR] config.txt missing or invalid.\n";
            return;
        }

        ConsoleManager::getInstance()->initialize(cfg);
        startCpuLoop();
        scheduler_initialized = true;
    }
    else if (command.rfind("screen -s ", 0) == 0)
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }

        std::istringstream iss(command.substr(10)); // after "screen -s "
        std::string name;
        std::string mem_str;

        iss >> name >> mem_str;

        if (name.empty() || mem_str.empty())
        {
            std::cout << "[ERROR] Usage: screen -s <process_name> <memory_size>\n";
            return;
        }

        // Convert memory size to integer
        size_t mem_size;
        try
        {
            mem_size = std::stoul(mem_str);
        }
        catch (...)
        {
            std::cout << "[ERROR] Invalid memory size format.\n";
            return;
        }

        // Validate memory size: power of 2, between 64 and 65536
        if (mem_size < 64 || mem_size > 65536 || (mem_size & (mem_size - 1)) != 0)
        {
            std::cout << "[ERROR] Invalid memory allocation. Must be power of 2 between 64 and 65536.\n";
            return;
        }

        if (hasConsole(name))
        {
            std::cout << "[ERROR] A screen with this name already exists.\n";
            return;
        }

        createConsole("screen", name);

        std::cout << "[INFO] min instructions: " << scheduler->get_min_instructions() << "\n"
                  << "[INFO] max instructions: " << scheduler->get_max_instructions() << "\n";

        // Use memory manager from ConsoleManager
        auto &memory_manager = ConsoleManager::getInstance()->getMemoryManager();
        size_t page_size = memory_manager.getPageSize();

        // --- Align and clamp memory like RR ---
        if (mem_size < page_size)
        {
            std::cout << "[screen] Requested " << mem_size << " B is below one page (" << page_size << " B). Clamping.\n";
            mem_size = page_size;
        }
        if (mem_size % page_size != 0)
        {
            size_t aligned = ((mem_size + page_size - 1) / page_size) * page_size;
            std::cout << "[screen] Adjusting allocation size from " << mem_size << " B to page-aligned " << aligned << " B.\n";
            mem_size = aligned;
        }

        size_t max_alloc = memory_manager.getTotalMemory();
        if (mem_size > max_alloc)
        {
            std::cout << "[screen] Requested " << mem_size << " B exceeds total memory (" << max_alloc << " B). Clamping.\n";
            mem_size = max_alloc;
        }

        // --- Create process ---
        auto proc = ConsoleManager::getInstance()
                        ->getProcessFactory()
                        ->generate_dummy_process(name, mem_size,
                                                 scheduler->get_min_instructions(),
                                                 scheduler->get_max_instructions());

        proc->addCommand(std::make_shared<PrintCommand>(
            "Process " + name + " has completed all its commands."));

        // --- Allocate memory & trigger page faults ---
        int start_address = -1;
        try
        {
            start_address = memory_manager.allocate(mem_size, proc->getName());

            int num_pages = (mem_size + page_size - 1) / page_size;
            for (int i = 0; i < num_pages; ++i)
            {
                proc->triggerPageFault(i);
            }
        }
        catch (const std::invalid_argument &e)
        {
            std::cout << "[ERROR] Failed to allocate memory for " << name << ": " << e.what() << "\n";
            return; // Stop here if allocation fails
        }

        if (start_address != -1)
        {
            proc->readMemory(start_address);
            scheduler->add_process(proc);

            auto screen = std::dynamic_pointer_cast<ScreenConsole>(m_consoleTable[name]);
            if (screen)
                screen->attachProcess(proc);

            std::cout << "[screen] Process \"" << name << "\" allocated at ["
                      << start_address << "-" << start_address + mem_size - 1
                      << "] with " << mem_size << " bytes.\n";
        }
        else
        {
            std::cout << "[screen] Process \"" << name << "\" could not be loaded into memory.\n";
        }
    }
else if (command.rfind("screen -c ", 0) == 0)
{
    if (!scheduler)
    {
        std::cout << "[ERROR] Scheduler not initialized.\n";
        return;
    }

    std::istringstream iss(command.substr(10));
    std::string name;
    std::string instructions_str;

    // read process name
    if (!(iss >> name))
    {
        std::cout << "[ERROR] Invalid syntax. Use: screen -c <name> \"<instructions>\"\n";
        return;
    }

    // read quoted instructions
    std::getline(iss >> std::ws, instructions_str);
    if (instructions_str.empty() || instructions_str.front() != '"' || instructions_str.back() != '"')
    {
        std::cout << "[ERROR] No instructions provided (must be in quotes).\n";
        return;
    }
    instructions_str = instructions_str.substr(1, instructions_str.size() - 2); // remove quotes

    if (hasConsole(name))
    {
        std::cout << "[ERROR] A screen with this name already exists.\n";
        return;
    }

    createConsole("screen", name);

    std::cout << "[INFO] min instructions: " << scheduler->get_min_instructions() << "\n"
              << "[INFO] max instructions: " << scheduler->get_max_instructions() << "\n";

    auto& memory_manager = ConsoleManager::getInstance()->getMemoryManager();
    size_t page_size = memory_manager.getPageSize();

    // --- Use default memory size ---
    size_t mem_size = min_mem_per_proc;

    // --- Align and clamp memory like RR ---
    if (mem_size < page_size)
    {
        std::cout << "[screen] Requested " << mem_size << " B is below one page (" << page_size << " B). Clamping.\n";
        mem_size = page_size;
    }
    if (mem_size % page_size != 0)
    {
        size_t aligned = ((mem_size + page_size - 1) / page_size) * page_size;
        std::cout << "[screen] Adjusting allocation size from " << mem_size << " B to page-aligned " << aligned << " B.\n";
        mem_size = aligned;
    }

    size_t max_alloc = memory_manager.getTotalMemory();
    if (mem_size > max_alloc)
    {
        std::cout << "[screen] Requested " << mem_size << " B exceeds total memory (" << max_alloc << " B). Clamping.\n";
        mem_size = max_alloc;
    }

    // --- Create process with given instructions ---
    auto proc = ConsoleManager::getInstance()
                    ->getProcessFactory()
                    ->generate_custom_process(name, mem_size, instructions_str);

    proc->addCommand(std::make_shared<PrintCommand>(
        "Process " + name + " has completed all its commands."));

    int start_address = -1;
    try
    {
        start_address = memory_manager.allocate(mem_size, proc->getName());

        int num_pages = (mem_size + page_size - 1) / page_size;
        for (int i = 0; i < num_pages; ++i)
        {
            proc->triggerPageFault(i);
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << "[ERROR] Failed to allocate memory for " << name << ": " << e.what() << "\n";
        return;
    }

    if (start_address != -1)
    {
        proc->readMemory(start_address);
        scheduler->add_process(proc);

        auto screen = std::dynamic_pointer_cast<ScreenConsole>(m_consoleTable[name]);
        if (screen)
            screen->attachProcess(proc);

        std::cout << "[screen] Process \"" << name << "\" allocated at ["
                  << start_address << "-" << start_address + mem_size - 1
                  << "] with " << mem_size << " bytes.\n";
    }
    else
    {
        std::cout << "[screen] Process \"" << name << "\" could not be loaded into memory.\n";
    }
}

    

    else if (command.rfind("screen -r ", 0) == 0)
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }

        std::string name = command.substr(10);

        // switchConsole(name);
        auto proc = scheduler->getProcessByName(name);
        if (!proc)
        {
            std::cout << "Process " << name << " not found.\n";
        }
        else if (proc->hasError())
        { // Crashed due to memory access violation
            std::cout << "Process " << name
                      << " shut down due to memory access violation error that occurred at "
                      << proc->getErrorTime() << ". 0x"
                      << std::hex << std::uppercase << proc->getErrorAddress()
                      << " invalid.\n";
        }
        else if (proc->isFinished())
        {
            std::cout << "Process " << name << " finished execution.\n";
        }
    }

    else if (command == "screen -ls")
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
        render_header(std::cout);
        render_running_processes(scheduler->get_running_processes(), std::cout);
        render_finished_processes(scheduler->get_finished_processes(), std::cout);
        render_footer(std::cout);
    }
    else if (command == "report-util")
    {

        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }

        std::ofstream log_file("csopesy-log.txt", std::ios::app);
        if (!log_file)
        {
            std::cerr << "[ERROR] Unable to open csopesy-log.txt for writing.\n";
            return;
        }

        render_header(log_file);
        render_running_processes(scheduler->get_running_processes(), log_file);
        render_finished_processes(scheduler->get_finished_processes(), log_file);
        render_footer(log_file);
        // log_file << "Type \"screen -ls\" to view processes or \"exit\" to quit.\n";
        log_file << std::string(80, '=') << "\n\n";
        log_file.close();
    }
    else if (command == "marquee")
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
        switchConsole(MARQUEE_CONSOLE);
    }
    else if (command == "scheduler-start")
    {
        if (start_flag)
        {
            std::cout << "[ERROR] Already started Scheduler\n";
            return;
        }

        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
        scheduler->start();
        start_flag = true;
        std::cout << "[INFO] Scheduler started.\n";
    }
    else if (command == "scheduler-stop")
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
        scheduler->stop_scheduler();
        std::cout << "[INFO] Scheduler stopped.\n";
    }
    else if (command == "process-smi")
    {
        if (!scheduler)
        {
            std::cout << "[ERROR] Scheduler not initialized.\n";
            return;
        }
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

        std::cout << "Instructions Executed: " << proc->getCurrentCommandIndex() << " / " << proc->getCurrentCommandIndex() << "\n";

        std::cout << "Logs:\n";
        for (const auto &log : proc->getLogs())
            std::cout << log << "\n";
    }
    else if (command.rfind("screen -c ", 0) == 0)
{
    if (!scheduler)
    {
        std::cout << "[ERROR] Scheduler not initialized.\n";
        return;
    }

    std::istringstream iss(command.substr(10));
    std::string name;
    std::string instructions_str;

    // read process name
    if (!(iss >> name))
    {
        std::cout << "[ERROR] Invalid syntax. Use: screen -c <name> \"<instructions>\"\n";
        return;
    }

    // read quoted instructions
    std::getline(iss >> std::ws, instructions_str);
    if (instructions_str.empty() || instructions_str.front() != '"' || instructions_str.back() != '"')
    {
        std::cout << "[ERROR] No instructions provided (must be in quotes).\n";
        return;
    }
    instructions_str = instructions_str.substr(1, instructions_str.size() - 2); // remove quotes

    if (hasConsole(name))
    {
        std::cout << "[ERROR] A screen with this name already exists.\n";
        return;
    }

    createConsole("screen", name);

    std::cout << "[INFO] min instructions: " << scheduler->get_min_instructions() << "\n"
              << "[INFO] max instructions: " << scheduler->get_max_instructions() << "\n";

    auto& memory_manager = ConsoleManager::getInstance()->getMemoryManager();
    size_t page_size = memory_manager.getPageSize();

    // --- Use default memory size ---
    size_t mem_size = min_mem_per_proc;

    // --- Align and clamp memory like RR ---
    if (mem_size < page_size)
    {
        std::cout << "[screen] Requested " << mem_size << " B is below one page (" << page_size << " B). Clamping.\n";
        mem_size = page_size;
    }
    if (mem_size % page_size != 0)
    {
        size_t aligned = ((mem_size + page_size - 1) / page_size) * page_size;
        std::cout << "[screen] Adjusting allocation size from " << mem_size << " B to page-aligned " << aligned << " B.\n";
        mem_size = aligned;
    }

    size_t max_alloc = memory_manager.getTotalMemory();
    if (mem_size > max_alloc)
    {
        std::cout << "[screen] Requested " << mem_size << " B exceeds total memory (" << max_alloc << " B). Clamping.\n";
        mem_size = max_alloc;
    }

    // --- Create process with given instructions ---
    auto proc = ConsoleManager::getInstance()
                    ->getProcessFactory()
                    ->generate_custom_process(name, mem_size, instructions_str);

    proc->addCommand(std::make_shared<PrintCommand>(
        "Process " + name + " has completed all its commands."));

    int start_address = -1;
    try
    {
        start_address = memory_manager.allocate(mem_size, proc->getName());

        int num_pages = (mem_size + page_size - 1) / page_size;
        for (int i = 0; i < num_pages; ++i)
        {
            proc->triggerPageFault(i);
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << "[ERROR] Failed to allocate memory for " << name << ": " << e.what() << "\n";
        return;
    }

    if (start_address != -1)
    {
        proc->readMemory(start_address);
        scheduler->add_process(proc);

        auto screen = std::dynamic_pointer_cast<ScreenConsole>(m_consoleTable[name]);
        if (screen)
            screen->attachProcess(proc);

        std::cout << "[screen] Process \"" << name << "\" allocated at ["
                  << start_address << "-" << start_address + mem_size - 1
                  << "] with " << mem_size << " bytes.\n";
    }
    else
    {
        std::cout << "[screen] Process \"" << name << "\" could not be loaded into memory.\n";
    }
}

    else
    {
        if (m_activeConsole)
            m_activeConsole->process(command);
        else
            std::cout << "[ERROR] No active console.\n";
    }
}

void ConsoleManager::render_header(std::ostream &out)
{
    if (!scheduler)
    {
        out << "No scheduler initialized.\n";
        out << std::string(80, '-') << "\n\n";
        return;
    }

    auto stats = scheduler->get_cpu_stats();
    std::string type = dynamic_cast<FCFSScheduler *>(scheduler.get()) ? "FCFS Scheduler"
                       : dynamic_cast<RRScheduler *>(scheduler.get()) ? "RR Scheduler"
                                                                      : "Unknown";

    std::string title = "CSOPESY Operating System Emulator - " + type;

    out << title << "\n";
    out << std::string(80, '-') << "\n\n";

    int total_cores = stats.size();
    int used = 0;
    int running_processes = scheduler->get_running_processes().size();
    float current_util = 0.0f;

    if (total_cores > 0)
        current_util = (100.0f * running_processes) / total_cores;

    out << "Cores Used: " << running_processes << " / " << total_cores << "\n";
    out << "Cores Available: " << (total_cores - running_processes) << "\n";
    out << "Current CPU Utilization: " << std::fixed << std::setprecision(1) << current_util << " %\n";

    out << std::string(80, '-') << "\n\n";
}

void ConsoleManager::render_footer(std::ostream &out)
{
    out << "\nType \"screen -ls\" to view processes or \"exit\" to quit.\n";
}

void ConsoleManager::render_running_processes(const std::vector<std::shared_ptr<Process>> &list, std::ostream &out)
{
    out << "Running Processes:\n";
    if (list.empty())
    {
        out << "  (None)\n";
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
        oss << "  " << p->getCurrentCommandIndex() << " / " << p->getTotalInstructions();

        try
        {
            int core_id = scheduler->get_core_of_process(p);
            if (core_id != -1)
                oss << " (Core: " << core_id << ")";
            else
                oss << " (Core: N/A)";
        }
        catch (const std::exception &e)
        {
            oss << " (Core: ?? - error)";
            std::cerr << "[Error] Failed to get core of process: " << e.what() << "\n";
        }
        catch (...)
        {
            oss << " (Core: ?? - unknown error)";
            std::cerr << "[Error] Unknown exception in get_core_of_process()\n";
        }

        out << oss.str() << "\n";
    }
}

void ConsoleManager::render_finished_processes(const std::vector<std::shared_ptr<Process>> &list, std::ostream &out)
{
    out << "Finished Processes:\n";
    if (list.empty())
    {
        out << "  (None)\n";
        return;
    }

    for (const auto &p : list)
    {
        if (!p->isFinished())
            continue;

        auto finish = std::chrono::system_clock::to_time_t(p->getFinishTime());
        out << " - " << p->getName()
            << " (" << std::put_time(std::localtime(&finish), "%Y-%m-%d %H:%M:%S") << ")\n";
    }
}

size_t ConsoleManager::getRandomMemSize() const
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(min_mem_per_proc, max_mem_per_proc);
    return dist(gen);
}
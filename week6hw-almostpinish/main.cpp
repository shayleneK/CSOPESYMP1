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
/*
============================================
CONFIG-BASED INITIALIZATION (disabled for now cuz i dont want it to clash with other branches)
this also includes jul's code in the separate branch)
Uncomment below when merging with config branch.
Requires:
    - #include "ConfigManager.h"
    - config.txt file in working directory
============================================

#include "ConfigManager.h"
#include "RRScheduler.h"
#include "ScreenConsole.h"
#include "Process.h"
#include "Command.h"
#include "GlobalState.h"

int main() {
    ConfigManager config;
    if (!config.load("config.txt")) {
        std::cerr << "[FATAL] Could not load config.txt\n";
        return 1;
    }

    int num_cpu = config.getInt("num-cpu", 4);
    std::string scheduler_type = config.getString("scheduler", "fcfs");
    int quantum = config.getInt("quantum-cycles", 50);
    int batch_freq = config.getInt("batch-process-freq", 1);
    int min_ins = config.getInt("min-ins", 1000);
    int max_ins = config.getInt("max-ins", 2000);
    int delay = config.getInt("delay-per-exec", 0);

    std::shared_ptr<Scheduler> scheduler;
    if (scheduler_type == "rr") {
        scheduler = std::make_shared<RRScheduler>(num_cpu, quantum);
    } else {
        scheduler = std::make_shared<Scheduler>(num_cpu);
    }

    // Auto-generate test processes
    for (int i = 0; i < 10; ++i) {
        auto p = std::make_shared<Process>("Process" + std::to_string(i));
        int ins_count = min_ins + (rand() % (max_ins - min_ins + 1));

        for (int j = 0; j < ins_count; ++j) {
            p->add_command(std::make_shared<PrintCommand>("Hello from " + p->name));
            if (delay > 0) {
                p->add_command(std::make_shared<SleepCommand>(delay));
            }
        }

        scheduler->add_process(p);
    }

    scheduler->start();

    // ConsoleManager usage remains unchanged
    ConsoleManager *consoleManager = ConsoleManager::getInstance(scheduler.get());
    consoleManager->drawConsole();
    while (consoleManager->isRunning()) {
        consoleManager->processInput();
    }

    scheduler->shutdown();
    ConsoleManager::destroy();
    return 0;
}
*/
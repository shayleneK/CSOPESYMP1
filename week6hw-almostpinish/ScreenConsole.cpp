#include "ScreenConsole.h"
#include "ConsoleManager.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

ScreenConsole::ScreenConsole(std::shared_ptr<Process> p)
    : AConsole(p->name), process(p) {}

void ScreenConsole::onEnabled() {
    std::cout << "[INFO] Attached to process screen: " << process->name << "\n";
}

void ScreenConsole::display() {
    std::cout << "\n--- " << process->name << " Screen ---\n";
    std::cout << "Commands: process-smi, exit\n";
    std::cout << "Waiting for input...\n";
}

void ScreenConsole::process() {
    std::string cmd;
    std::getline(std::cin, cmd);

    if (cmd == "exit") {
        ConsoleManager::getInstance()->removeConsole(getName());
    } else if (cmd == "process-smi") {
        std::cout << "\n[Process Info]\n";
        std::cout << "Name: " << process->name << "\n";
        std::cout << "Finished: " << (process->is_finished ? "Yes" : "No") << "\n";
        std::cout << "Progress: " << process->current_command_index << " / "
                  << process->commands.size() << "\n";
        std::cout << "Running on Core: " << process->current_core << "\n";

        std::ifstream file(process->output_filename);
        if (file.is_open()) {
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }

            int start = std::max(0, static_cast<int>(lines.size()) - 10);
            std::cout << "--- Last 10 Log Lines ---\n";
            for (int i = start; i < lines.size(); ++i) {
                std::cout << lines[i] << "\n";
            }
        } else {
            std::cout << "Log file not found.\n";
        }
    } else {
        std::cout << "[ERROR] Unknown command inside screen.\n";
    }
}
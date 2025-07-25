#include "CommandParser.h"
#include "Command.h"
#include "Process.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <memory>

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

bool is_number(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

std::vector<std::shared_ptr<Command>> CommandParser::parse_instructions(const std::string &instruction_str) {
    std::vector<std::shared_ptr<Command>> commands;

    std::istringstream iss(instruction_str);
    std::string token;

    while (std::getline(iss, token, ';')) {
        std::istringstream line(token);
        std::string cmd;
        if (line >> cmd) {
            if (cmd == "DECLARE") {
                std::string var;
                uint16_t val;
                if (line >> var >> val) {
                    commands.push_back(std::make_shared<DeclareCommand>(var, val));
                }
            }
            else if (cmd == "ADD") {
                std::string target, op1, op2;
                bool op1_is_var = false, op2_is_var = false;
                uint16_t val1 = 0, val2 = 0;

                if (line >> target >> op1 >> op2) {
                    op1_is_var = !is_number(op1);
                    op2_is_var = !is_number(op2);
                    if (!op1_is_var) val1 = std::stoul(op1);
                    if (!op2_is_var) val2 = std::stoul(op2);

                    commands.push_back(std::make_shared<AddCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
            }
            else if (cmd == "SUBTRACT") {
                std::string target, op1, op2;
                bool op1_is_var = false, op2_is_var = false;
                uint16_t val1 = 0, val2 = 0;

                if (line >> target >> op1 >> op2) {
                    op1_is_var = !is_number(op1);
                    op2_is_var = !is_number(op2);
                    if (!op1_is_var) val1 = std::stoul(op1);
                    if (!op2_is_var) val2 = std::stoul(op2);

                    commands.push_back(std::make_shared<SubtractCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
            }
            else if (cmd == "PRINT") {
                std::string msg;
                std::getline(line >> std::ws, msg); // Read full message
                commands.push_back(std::make_shared<PrintCommand>(msg));
            }
            else if (cmd == "READ") {
                std::string var_name;
                std::string addr_str;
                if (line >> var_name >> addr_str) {
                    uint16_t address = std::stoul(addr_str, nullptr, 16); // hex
                    commands.push_back(std::make_shared<ReadCommand>(var_name, address));
                }
            }
            else if (cmd == "WRITE") {
                std::string addr_str, val_str;
                if (line >> addr_str >> val_str) {
                    uint16_t address = std::stoul(addr_str, nullptr, 16); // hex
                    uint16_t value = std::stoul(val_str);
                    commands.push_back(std::make_shared<WriteCommand>(address, value));
                }
            }
            else if (cmd == "SLEEP") {
                uint8_t ticks;
                if (line >> ticks) {
                    commands.push_back(std::make_shared<SleepCommand>(ticks));
                }
            }
            else if (cmd == "FOR") {
                std::cerr << "[ERROR] FOR loops not yet supported in user-defined processes.\n";
            }
            else {
                std::cerr << "[ERROR] Unknown instruction: " << cmd << "\n";
            }
        }
    }

    return commands;
}
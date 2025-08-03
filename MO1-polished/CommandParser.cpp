#include "CommandParser.h"
#include "Command.h"
#include "Process.h"
#include "ProcessFactory.h" 
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <memory>

std::vector<std::shared_ptr<Command>> CommandParser::parse_instructions(const std::string &instruction_str) {
    std::vector<std::shared_ptr<Command>> commands;

    std::istringstream iss(instruction_str);
    std::string token;

    while (std::getline(iss, token, ';')) {
        std::string trimmed = trim(token);
        if (trimmed.empty()) continue;

        std::istringstream line(trimmed);
        std::string cmd;
        if (line >> cmd) {
            // Convert to uppercase
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

            if (cmd == "DECLARE") {
                std::string var;
                std::string val_str;
                if (line >> var >> val_str) {
                    try {
                        uint16_t val = static_cast<uint16_t>(std::stoul(val_str));
                        commands.push_back(std::make_shared<DeclareCommand>(var, val));
                    }
                    catch (...) {
                        std::cerr << "[ERROR] Invalid value in DECLARE: " << val_str << "\n";
                    }
                }
            }
            else if (cmd == "ADD") {
                std::string target, op1, op2;
                bool op1_is_var = false, op2_is_var = false;
                uint16_t val1 = 0, val2 = 0;

                if (line >> target >> op1 >> op2) {
                    op1_is_var = !is_number(op1);
                    op2_is_var = !is_number(op2);
                    if (!op1_is_var) val1 = static_cast<uint16_t>(std::stoul(op1));
                    if (!op2_is_var) val2 = static_cast<uint16_t>(std::stoul(op2));

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
                    if (!op1_is_var) val1 = static_cast<uint16_t>(std::stoul(op1));
                    if (!op2_is_var) val2 = static_cast<uint16_t>(std::stoul(op2));

                    commands.push_back(std::make_shared<SubtractCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
            }
            else if (cmd == "PRINT") {
                std::string msg;
                std::getline(line >> std::ws, msg);
                commands.push_back(std::make_shared<PrintCommand>(msg));
            }
            else if (cmd == "READ") {
                std::string var_name;
                std::string addr_str;
                if (line >> var_name >> addr_str) {
                    try {
                        uint16_t address = parse_hex_address(addr_str);
                        commands.push_back(std::make_shared<ReadCommand>(var_name, address));
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Invalid address in READ: " << addr_str << "\n";
                    }
                }
            }
            else if (cmd == "WRITE") {
                std::string addr_str, val_str;
                if (line >> addr_str >> val_str) {
                    try {
                        uint16_t address = parse_hex_address(addr_str);
                        uint16_t value = static_cast<uint16_t>(std::stoul(val_str));
                        commands.push_back(std::make_shared<WriteCommand>(address, value));
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Invalid address/value in WRITE: " << addr_str << " " << val_str << "\n";
                    }
                }
            }
            else if (cmd == "SLEEP") {
                std::string ticks_str;
                if (line >> ticks_str) {
                    try {
                        uint8_t ticks = static_cast<uint8_t>(std::stoul(ticks_str));
                        commands.push_back(std::make_shared<SleepCommand>(ticks));
                    }
                    catch (...) {
                        std::cerr << "[ERROR] Invalid ticks in SLEEP: " << ticks_str << "\n";
                    }
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
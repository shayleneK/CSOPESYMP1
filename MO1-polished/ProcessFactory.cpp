#include "ProcessFactory.h"
#include "Process.h"
#include "Command.h"
#include "CommandParser.h"

#include <random>
#include <sstream>
#include <iostream>
#include <algorithm>

static int global_pid_counter = 0;

std::shared_ptr<Process> ProcessFactory::generate_dummy_process(
    const std::string &name,
    size_t mem_required,
    int min_ins,
    int max_ins)
{
    auto process = std::make_shared<Process>(name, mem_required, global_pid_counter++);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ins_dist(min_ins, max_ins);
    std::uniform_int_distribution<> op_dist(0, 5);

    int instruction_count = ins_dist(gen);

    for (int i = 0; i < instruction_count; ++i)
    {
        int op = op_dist(gen);
        switch (op)
        {
        case 0:
        {
            std::uniform_int_distribution<> msg_type(0, 1);
            std::vector<std::string> vars = {"x", "y", "result", "temp", "value"};
            std::string message = (msg_type(gen) == 0)
                                      ? "Hello world from " + name + "!"
                                      : "\"Value from: \" + " + vars[gen() % vars.size()];
            process->addCommand(std::make_shared<PrintCommand>(message));
            break;
        }
        case 1:
        case 2:
        {
            std::vector<std::string> vars = {"a", "b", "c", "temp", "sum", "diff"};
            std::string target = vars[gen() % vars.size()];
            std::string op1 = vars[gen() % vars.size()];
            std::string op2 = vars[gen() % vars.size()];
            bool op1_is_var = gen() % 2;
            bool op2_is_var = gen() % 2;
            std::uniform_int_distribution<uint16_t> val_dist(1, 500);
            uint16_t val1 = val_dist(gen), val2 = val_dist(gen);
            if (op == 1)
                process->addCommand(std::make_shared<AddCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
            else
                process->addCommand(std::make_shared<SubtractCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
            break;
        }
        case 3:
        {
            std::vector<std::string> vars = {"a1", "b1", "c1", "temp1", "varX", "tempVar"};
            std::string var = vars[gen() % vars.size()];
            std::uniform_int_distribution<uint16_t> val_dist(0, 1000);
            uint16_t value = val_dist(gen);
            process->addCommand(std::make_shared<DeclareCommand>(var, value));
            break;
        }
        case 4:
        {
            std::uniform_int_distribution<> rep_dist(2, 5);
            int repeats = rep_dist(gen);
            std::vector<std::shared_ptr<Command>> nested_cmds;
            std::uniform_int_distribution<> nested_len_dist(1, 3);
            int nested_instruction_count = nested_len_dist(gen);
            std::uniform_int_distribution<> nested_op_dist(0, 3);
            for (int j = 0; j < nested_instruction_count; ++j)
            {
                int inner_op = nested_op_dist(gen);
                switch (inner_op)
                {
                case 0:
                    nested_cmds.push_back(std::make_shared<PrintCommand>("\"Loop message " + std::to_string(j) + "\""));
                    break;
                case 1:
                {
                    std::string target = "loop" + std::to_string(j);
                    std::string op1 = "a" + std::to_string(j);
                    std::string op2 = "b" + std::to_string(j);
                    uint16_t val2 = gen() % 50 + 1;
                    nested_cmds.push_back(std::make_shared<AddCommand>(target, op1, op2, true, false, 0, val2));
                    break;
                }
                case 2:
                {
                    std::string var = "d" + std::to_string(j);
                    uint16_t val = gen() % 100;
                    nested_cmds.push_back(std::make_shared<DeclareCommand>(var, val));
                    break;
                }
                case 3:
                    nested_cmds.push_back(std::make_shared<SleepCommand>(1));
                    break;
                }
            }
            process->addCommand(std::make_shared<ForCommand>(nested_cmds, repeats));
            break;
        }
        case 5:
        {
            std::uniform_int_distribution<uint8_t> sleep_ticks(10, 100);
            uint8_t ticks = sleep_ticks(gen);
            process->addCommand(std::make_shared<SleepCommand>(ticks));
            break;
        }
        default:
            process->addCommand(std::make_shared<PrintCommand>("\"Unknown operation fallback from " + name + "\""));
            break;
        }
    }

    return process;
}

// Used by "screen -c" to parse and generate processes with user-defined instructions
/*std::shared_ptr<Process> ProcessFactory::generate_custom_process(
    const std::string &name,
    size_t mem_required,
    const std::string &instructions_str)
{
    auto process = std::make_shared<Process>(name, mem_required, global_pid_counter++);

    std::istringstream iss(instructions_str);
    std::string token;

    while (std::getline(iss, token, ';'))
    {
        std::string trimmed = trim(token);
        if (trimmed.empty()) continue;

        std::istringstream line(trimmed);
        std::string cmd;
        if (line >> cmd)
        {
            // Convert to uppercase
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

            if (cmd == "DECLARE")
            {
                std::string var;
                std::string val_str;
                if (line >> var >> val_str)
                {
                    try {
                        uint16_t val = static_cast<uint16_t>(std::stoul(val_str));
                        process->addCommand(std::make_shared<DeclareCommand>(var, val));
                    }
                    catch (...) {
                        std::cerr << "[ERROR] Invalid value in DECLARE: " << val_str << "\n";
                    }
                }
            }
            else if (cmd == "ADD")
            {
                std::string target, op1, op2;
                bool op1_is_var = false, op2_is_var = false;
                uint16_t val1 = 0, val2 = 0;

                if (line >> target >> op1 >> op2)
                {
                    op1_is_var = !is_number(op1);
                    op2_is_var = !is_number(op2);
                    if (!op1_is_var) val1 = static_cast<uint16_t>(std::stoul(op1));
                    if (!op2_is_var) val2 = static_cast<uint16_t>(std::stoul(op2));

                    process->addCommand(std::make_shared<AddCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
            }
            else if (cmd == "SUBTRACT")
            {
                std::string target, op1, op2;
                bool op1_is_var = false, op2_is_var = false;
                uint16_t val1 = 0, val2 = 0;

                if (line >> target >> op1 >> op2)
                {
                    op1_is_var = !is_number(op1);
                    op2_is_var = !is_number(op2);
                    if (!op1_is_var) val1 = static_cast<uint16_t>(std::stoul(op1));
                    if (!op2_is_var) val2 = static_cast<uint16_t>(std::stoul(op2));

                    process->addCommand(std::make_shared<SubtractCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
            }
            else if (cmd == "PRINT")
            {
                std::string msg;
                std::getline(line >> std::ws, msg);
                process->addCommand(std::make_shared<PrintCommand>(msg));
            }
            else if (cmd == "READ")
            {
                std::string var_name;
                std::string addr_str;
                if (line >> var_name >> addr_str)
                {
                    try {
                        uint16_t address = parse_hex_address(addr_str);
                        process->addCommand(std::make_shared<ReadCommand>(var_name, address));
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Invalid address in READ: " << addr_str << "\n";
                    }
                }
            }
            else if (cmd == "WRITE")
            {
                std::string addr_str, val_str;
                if (line >> addr_str >> val_str)
                {
                    try {
                        uint16_t address = parse_hex_address(addr_str);
                        uint16_t value = static_cast<uint16_t>(std::stoul(val_str));
                        process->addCommand(std::make_shared<WriteCommand>(address, value));
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Invalid address/value in WRITE: " << addr_str << " " << val_str << "\n";
                    }
                }
            }
            else if (cmd == "SLEEP")
            {
                std::string ticks_str;
                if (line >> ticks_str)
                {
                    try {
                        uint8_t ticks = static_cast<uint8_t>(std::stoul(ticks_str));
                        process->addCommand(std::make_shared<SleepCommand>(ticks));
                    }
                    catch (...) {
                        std::cerr << "[ERROR] Invalid ticks in SLEEP: " << ticks_str << "\n";
                    }
                }
            }
            else
            {
                std::cerr << "[ERROR] Unknown instruction: " << cmd << "\n";
            }
        }
    }

    return process;
}
*/
std::shared_ptr<Process> ProcessFactory::generate_custom_process(
    const std::string &name,
    size_t mem_required,
    const std::string &instructions_str)
{
    auto process = std::make_shared<Process>(name, mem_required, global_pid_counter++);
    auto commands = CommandParser::parse_instructions(instructions_str);
    for (const auto &cmd : commands)
    {
        process->addCommand(cmd);
    }

    return process;
}

std::shared_ptr<Process> ProcessFactory::generate_background_process(
    const std::string &name,
    size_t mem_required)
{
    return generate_dummy_process(name, mem_required, 100, 100);
}
//helper functions

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

bool is_number(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

uint16_t parse_hex_address(const std::string& str) {
    std::string s = trim(str);
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.empty()) throw std::invalid_argument("Empty hex address");
    return static_cast<uint16_t>(std::stoul(s, nullptr, 16));
}
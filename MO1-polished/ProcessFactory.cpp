#include "ProcessFactory.h"
#include "Process.h"
#include "Command.h"

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
    auto process = std::make_shared<Process>(name, mem_required, global_pid_counter++, frameSize, memoryManager);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ins_dist(min_ins, max_ins);
    std::uniform_int_distribution<> op_dist(0, 5);

    int instruction_count = ins_dist(gen);

    std::cout << "Generating process " << name << " with " << instruction_count << " instructions" << std::endl;

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

    process->addCommand(
        std::make_shared<PrintCommand>("\"Process " + name + " has completed all its commands.\""));

    return process;
}

// Used by "screen -c" to parse and generate processes with user-defined instructions
std::shared_ptr<Process> ProcessFactory::generate_custom_process(
    const std::string &name,
    size_t mem_required,
    const std::string &instructions_str)
{
    std::cout << "Generating process " << name << " with custom instructions" << std::endl;
    auto process = std::make_shared<Process>(name, mem_required, global_pid_counter++, frameSize, memoryManager);

    // Step 1: Preprocess to remove newlines (convert them to spaces)
    std::string cleaned = instructions_str;
    std::replace(cleaned.begin(), cleaned.end(), '\n', ' ');
    std::replace(cleaned.begin(), cleaned.end(), '\r', ' ');

    // Step 2: Split by ';'
    std::istringstream iss(cleaned);
    std::string token;

    while (std::getline(iss, token, ';'))
    {
        // Trim leading/trailing whitespace
        auto start = token.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            continue; // empty
        auto end = token.find_last_not_of(" \t\r\n");
        token = token.substr(start, end - start + 1);

        if (token.empty())
            continue;

        std::istringstream line(token);
        std::string cmd;

        // Extract the first word (instruction)
        if (!(line >> cmd))
            continue;

        // Strip command at first non-alphabetic character
        cmd.erase(std::find_if(cmd.begin(), cmd.end(), [](int c)
                               { return !std::isalpha(c); }),
                  cmd.end());

        // Convert to uppercase
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        try
        {
            if (cmd == "DECLARE")
            {
                std::string var;
                uint16_t val;
                if (line >> var >> val)
                {
                    process->addCommand(std::make_shared<DeclareCommand>(var, val));
                }
                else
                {
                    std::cerr << "[ERROR] Invalid DECLARE syntax. Expected: DECLARE <var> <value>\n";
                }
            }
            else if (cmd == "ADD")
            {
                std::string target, op1, op2;
                if (line >> target >> op1 >> op2)
                {
                    bool op1_is_var = (op1.find_first_not_of("0123456789") != std::string::npos);
                    bool op2_is_var = (op2.find_first_not_of("0123456789") != std::string::npos);
                    uint16_t val1 = op1_is_var ? 0 : static_cast<uint16_t>(std::stoi(op1));
                    uint16_t val2 = op2_is_var ? 0 : static_cast<uint16_t>(std::stoi(op2));
                    process->addCommand(std::make_shared<AddCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
                else
                {
                    std::cerr << "[ERROR] Invalid ADD syntax. Expected: ADD <target> <op1> <op2>\n";
                }
            }
            else if (cmd == "SUBTRACT")
            {
                std::string target, op1, op2;
                if (line >> target >> op1 >> op2)
                {
                    bool op1_is_var = (op1.find_first_not_of("0123456789") != std::string::npos);
                    bool op2_is_var = (op2.find_first_not_of("0123456789") != std::string::npos);
                    uint16_t val1 = op1_is_var ? 0 : static_cast<uint16_t>(std::stoi(op1));
                    uint16_t val2 = op2_is_var ? 0 : static_cast<uint16_t>(std::stoi(op2));
                    process->addCommand(std::make_shared<SubtractCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
                else
                {
                    std::cerr << "[ERROR] Invalid SUBTRACT syntax. Expected: SUBTRACT <target> <op1> <op2>\n";
                }
            }
            else if (cmd == "PRINT")
            {
                std::string raw;
                std::getline(line, raw);
                raw.erase(0, raw.find_first_not_of(" \t"));
                if (!raw.empty() && raw.front() == '(' && raw.back() == ')')
                {
                    raw = raw.substr(1, raw.size() - 2);
                    raw.erase(0, raw.find_first_not_of(" \t"));
                }
                if (raw.empty())
                {
                    std::cerr << "[ERROR] PRINT requires an argument.\n";
                    continue;
                }

                std::vector<PrintSegment> segments;
                std::string current;
                bool inQuotes = false;

                for (char c : raw)
                {
                    if (c == '"')
                    {
                        inQuotes = !inQuotes;
                        current += c;
                    }
                    else if (c == '+' && !inQuotes)
                    {
                        if (!current.empty())
                        {
                            std::string segment = current;
                            segment.erase(0, segment.find_first_not_of(" \t"));
                            segment.erase(segment.find_last_not_of(" \t") + 1);
                            if (segment.size() >= 2 && segment.front() == '"' && segment.back() == '"')
                                segments.push_back({false, segment.substr(1, segment.size() - 2)});
                            else if (!segment.empty())
                                segments.push_back({true, segment});
                        }
                        current.clear();
                    }
                    else
                    {
                        current += c;
                    }
                }

                if (!current.empty())
                {
                    std::string segment = current;
                    segment.erase(0, segment.find_first_not_of(" \t"));
                    segment.erase(segment.find_last_not_of(" \t") + 1);
                    if (segment.size() >= 2 && segment.front() == '"' && segment.back() == '"')
                        segments.push_back({false, segment.substr(1, segment.size() - 2)});
                    else if (!segment.empty())
                        segments.push_back({true, segment});
                }

                if (!segments.empty())
                    process->addCommand(std::make_shared<PrintCommand>(segments));
                else
                    std::cerr << "[ERROR] No valid segments in PRINT statement.\n";
            }
            else if (cmd == "READ")
            {
                std::string var_name, addr_str;
                if (line >> var_name >> addr_str)
                {
                    try
                    {
                        uint16_t address = static_cast<uint16_t>(std::stoul(addr_str, nullptr, 16));
                        process->addCommand(std::make_shared<ReadCommand>(var_name, address));
                    }
                    catch (...)
                    {
                        std::cerr << "[ERROR] Invalid address in READ: " << addr_str << "\n";
                    }
                }
                else
                {
                    std::cerr << "[ERROR] Invalid READ syntax. Expected: READ <var> <hex_address>\n";
                }
            }
            else if (cmd == "WRITE")
            {
                std::string addr_str, val_str;
                if (line >> addr_str >> val_str)
                {
                    try
                    {
                        uint32_t address = std::stoul(addr_str, nullptr, 16);
                        bool is_var = (val_str.find_first_not_of("0123456789") != std::string::npos);
                        uint16_t value = is_var ? 0 : static_cast<uint16_t>(std::stoi(val_str));
                        process->addCommand(std::make_shared<WriteCommand>(address, value, is_var, val_str));
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[ERROR] Invalid WRITE operand: " << e.what() << "\n";
                        continue;
                    }
                }
                else
                {
                    std::cerr << "[ERROR] Invalid WRITE syntax. Expected: WRITE <hex_address> <value_or_var>\n";
                }
            }
            else if (cmd == "SLEEP")
            {
                uint8_t ticks;
                if (line >> ticks)
                    process->addCommand(std::make_shared<SleepCommand>(ticks));
                else
                    std::cerr << "[ERROR] Invalid SLEEP syntax. Expected: SLEEP <ticks>\n";
            }
            else
            {
                std::cerr << "[ERROR] Unknown instruction: " << cmd << "\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] Exception parsing instruction '" << cmd << "': " << e.what() << "\n";
        }
    }

    return process;
}

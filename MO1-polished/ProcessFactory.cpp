#include "ProcessFactory.h"
#include "Process.h"
#include "Command.h"
#include <random>
#include <sstream>
#include <iostream>
#include <algorithm>

// Global PID counter to ensure each process gets a unique ID
static int global_pid_counter = 0;

/**
 * Generates a dummy process with randomly generated instructions.
 * Useful for testing or simulation without user input.
 *
 * @param name Name of the process
 * @param mem_required Memory (in bytes) required by the process
 * @param min_ins Minimum number of instructions to generate
 * @param max_ins Maximum number of instructions to generate
 * @return Shared pointer to the newly created Process object
 */
std::shared_ptr<Process> ProcessFactory::generate_dummy_process(
    const std::string &name,
    size_t mem_required,
    int min_ins,
    int max_ins)
{
    // Create a new process with the given parameters
    auto process = std::make_shared<Process>(name, mem_required, global_pid_counter++, frameSize, memoryManager);

    // Special case: if the process name contains "crash", generate a crash-inducing command
    if (name.find("crash") != std::string::npos)
    {
        // Add an invalid memory access command (will cause a segfault/crash)
        process->addCommand(std::make_shared<InvalidAccessCommand>(0xFFFF));
        // This print command should never execute due to the crash above
        process->addCommand(std::make_shared<PrintCommand>("\"This should never run because the process will crash.\""));
        return process; // Return early
    }

    // Set up random number generation
    std::random_device rd;                                      // Seed source
    std::mt19937 gen(rd());                                     // Mersenne Twister random generator
    std::uniform_int_distribution<> ins_dist(min_ins, max_ins); // For instruction count
    std::uniform_int_distribution<> op_dist(0, 5);              // For choosing operation types

    // Randomly determine how many instructions this process will have
    int instruction_count = ins_dist(gen);
    std::cout << "Generating process " << name << " with " << instruction_count << " instructions" << std::endl;

    // Generate the specified number of random instructions
    for (int i = 0; i < instruction_count; ++i)
    {
        int op = op_dist(gen); // Choose a random operation type (0–5)

        switch (op)
        {
        case 0:
        {
            // PRINT command: either a fixed message or one that includes a variable
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
            // ADD or SUBTRACT command
            std::vector<std::string> vars = {"a", "b", "c", "temp", "sum", "diff"};
            std::string target = vars[gen() % vars.size()]; // Destination variable
            std::string op1 = vars[gen() % vars.size()];    // First operand
            std::string op2 = vars[gen() % vars.size()];    // Second operand

            // Randomly decide if operands are variables or literals
            bool op1_is_var = gen() % 2;
            bool op2_is_var = gen() % 2;

            // Generate random literal values (if needed)
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
            // DECLARE command: declare a variable with a random value
            std::vector<std::string> vars = {"a1", "b1", "c1", "temp1", "varX", "tempVar"};
            std::string var = vars[gen() % vars.size()];
            std::uniform_int_distribution<uint16_t> val_dist(0, 1000);
            uint16_t value = val_dist(gen);
            process->addCommand(std::make_shared<DeclareCommand>(var, value));
            break;
        }
        case 4:
        {
            // FOR loop command: repeats a block of instructions
            std::uniform_int_distribution<> rep_dist(2, 5);
            int repeats = rep_dist(gen); // Number of times to repeat

            std::vector<std::shared_ptr<Command>> nested_cmds; // Commands inside the loop
            std::uniform_int_distribution<> nested_len_dist(1, 3);
            int nested_instruction_count = nested_len_dist(gen); // 1–3 inner instructions
            std::uniform_int_distribution<> nested_op_dist(0, 3);

            // Generate random inner commands
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
                    // Add command inside loop
                    std::string target = "loop" + std::to_string(j);
                    std::string op1 = "a" + std::to_string(j);
                    std::string op2 = "b" + std::to_string(j);
                    uint16_t val2 = gen() % 50 + 1;
                    nested_cmds.push_back(std::make_shared<AddCommand>(target, op1, op2, true, false, 0, val2));
                    break;
                }
                case 2:
                {
                    // Declare command inside loop
                    std::string var = "d" + std::to_string(j);
                    uint16_t val = gen() % 100;
                    nested_cmds.push_back(std::make_shared<DeclareCommand>(var, val));
                    break;
                }
                case 3:
                    // Sleep command inside loop
                    nested_cmds.push_back(std::make_shared<SleepCommand>(1));
                    break;
                }
            }

            // Wrap the inner commands in a ForCommand and add to process
            process->addCommand(std::make_shared<ForCommand>(nested_cmds, repeats));
            break;
        }
        case 5:
        {
            // SLEEP command: pause execution for a random number of ticks
            std::uniform_int_distribution<uint8_t> sleep_ticks(10, 100);
            uint8_t ticks = sleep_ticks(gen);
            process->addCommand(std::make_shared<SleepCommand>(ticks));
            break;
        }
        default:
            // Fallback for unknown operations
            process->addCommand(std::make_shared<PrintCommand>("\"Unknown operation fallback from " + name + "\""));
            break;
        }
    }

    // Add a final print command indicating the process has finished
    process->addCommand(
        std::make_shared<PrintCommand>("\"Process " + name + " has completed all its commands.\""));

    return process;
}

/**
 * Generates a custom process from a string of user-defined instructions.
 * Each instruction is separated by a semicolon and parsed accordingly.
 *
 * @param name Name of the process
 * @param mem_required Memory (in bytes) required by the process
 * @param instructions_str String containing semicolon-separated instructions
 * @return Shared pointer to the newly created Process object
 */
std::shared_ptr<Process> ProcessFactory::generate_custom_process(
    const std::string &name,
    size_t mem_required,
    const std::string &instructions_str)
{
    std::cout << "Generating process " << name << " with custom instructions" << std::endl;

    // Create a new process
    auto process = std::make_shared<Process>(name, mem_required, global_pid_counter++, frameSize, memoryManager);

    // Preprocess: Replace newlines and carriage returns with spaces
    std::string cleaned = instructions_str;
    std::replace(cleaned.begin(), cleaned.end(), '\n', ' ');
    std::replace(cleaned.begin(), cleaned.end(), '\r', ' ');

    // Use stringstream to parse the cleaned instruction string
    std::istringstream iss(cleaned);
    std::string token;

    // Parse each instruction (separated by semicolons)
    while (std::getline(iss, token, ';'))
    {
        // Trim leading/trailing whitespace
        auto start = token.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            continue; // Skip empty lines
        auto end = token.find_last_not_of(" \t\r\n");
        token = token.substr(start, end - start + 1);
        if (token.empty())
            continue;

        std::cout << "[TOKENIZER] Trimmed token: \"" << token << "\"" << std::endl;

        std::istringstream line(token);
        std::string cmd;

        // Extract the first word (the command name)
        if (!(line >> cmd))
            continue;

        // Convert command to uppercase for case-insensitive matching
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        try
        {
            if (cmd == "DECLARE")
            {
                std::string var;
                uint16_t val;
                if (line >> var >> val)
                    process->addCommand(std::make_shared<DeclareCommand>(var, val));
                else
                    std::cerr << "[ERROR] Invalid DECLARE syntax.\n";
            }
            else if (cmd == "ADD")
            {
                std::string target, op1, op2;
                if (line >> target >> op1 >> op2)
                {
                    // Determine if operand is a variable (contains non-digit characters)
                    bool op1_is_var = (op1.find_first_not_of("0123456789") != std::string::npos);
                    bool op2_is_var = (op2.find_first_not_of("0123456789") != std::string::npos);

                    // If operand is a literal, convert to integer; otherwise, ignore value
                    uint16_t val1 = op1_is_var ? 0 : static_cast<uint16_t>(std::stoi(op1));
                    uint16_t val2 = op2_is_var ? 0 : static_cast<uint16_t>(std::stoi(op2));

                    process->addCommand(std::make_shared<AddCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
                }
                else
                    std::cerr << "[ERROR] Invalid ADD syntax.\n";
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
                    std::cerr << "[ERROR] Invalid SUBTRACT syntax.\n";
            }
            else if (cmd == "PRINT")
            {
                std::string raw;
                std::getline(line, raw);                    // Get the rest of the line
                raw.erase(0, raw.find_first_not_of(" \t")); // Trim leading whitespace

                // Unescape escaped quotes: \" -> "
                size_t pos = 0;
                while ((pos = raw.find("\\\"", pos)) != std::string::npos)
                    raw.replace(pos, 2, "\"");

                // Remove surrounding parentheses if present
                if (!raw.empty() && raw.front() == '(' && raw.back() == ')')
                {
                    raw = raw.substr(1, raw.size() - 2);
                    raw.erase(0, raw.find_first_not_of(" \t"));
                    raw.erase(raw.find_last_not_of(" \t") + 1);
                }

                if (raw.empty())
                {
                    std::cerr << "[ERROR] PRINT requires an argument.\n";
                    continue;
                }

                // Parse the print string into segments (text or variables)
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
                        // '+' outside quotes separates segments
                        std::string segment = current;
                        current.clear();

                        // Trim whitespace
                        segment.erase(0, segment.find_first_not_of(" \t"));
                        segment.erase(segment.find_last_not_of(" \t") + 1);

                        if (!segment.empty())
                        {
                            if (segment.front() == '"' && segment.back() == '"')
                                segments.push_back({false, segment.substr(1, segment.size() - 2)}); // String literal
                            else
                                segments.push_back({true, segment}); // Variable
                        }
                    }
                    else
                    {
                        current += c;
                    }
                }

                // Handle the last segment
                if (!current.empty())
                {
                    std::string segment = current;
                    segment.erase(0, segment.find_first_not_of(" \t"));
                    segment.erase(segment.find_last_not_of(" \t") + 1);
                    if (!segment.empty())
                    {
                        if (segment.front() == '"' && segment.back() == '"')
                            segments.push_back({false, segment.substr(1, segment.size() - 2)});
                        else
                            segments.push_back({true, segment});
                    }
                }

                // Debug: show parsed segments
                for (auto &seg : segments)
                    std::cout << "[DEBUG] Parsed PRINT segment: " << (seg.isVar ? "Var" : "Str") << " = '" << seg.value << "'\n";

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
                        // Parse address as hexadecimal (e.g., 0x1A2B)
                        uint16_t address = static_cast<uint16_t>(std::stoul(addr_str, nullptr, 0));
                        process->addCommand(std::make_shared<ReadCommand>(var_name, address));
                    }
                    catch (...)
                    {
                        std::cerr << "[ERROR] Invalid address in READ: " << addr_str << "\n";
                    }
                }
                else
                    std::cerr << "[ERROR] Invalid READ syntax.\n";
            }
            else if (cmd == "WRITE")
            {
                std::cout << "WRITE RECOGNIZED \n";
                std::string addr_str, val_str;
                if (line >> addr_str >> val_str)
                {
                    try
                    {
                        // Parse address (supports decimal, hex like 0x123)
                        uint32_t address = std::stoul(addr_str, nullptr, 0);
                        // Check if value is a variable (contains non-digits)
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
                    std::cerr << "[ERROR] Invalid WRITE syntax.\n";
            }
            else if (cmd == "SLEEP")
            {
                uint8_t ticks;
                if (line >> ticks)
                    process->addCommand(std::make_shared<SleepCommand>(ticks));
                else
                    std::cerr << "[ERROR] Invalid SLEEP syntax.\n";
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
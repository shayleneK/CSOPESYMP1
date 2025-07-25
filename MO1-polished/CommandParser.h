#pragma once

#include <vector>
#include <memory>
#include <string>

class Command;
class Process;

class CommandParser {
public:
    static std::vector<std::shared_ptr<Command>> parse_instructions(const std::string &instruction_str);

    static std::shared_ptr<Process> parse_process(
        const std::string &name,
        size_t mem_size,
        const std::string &instruction_str);
};
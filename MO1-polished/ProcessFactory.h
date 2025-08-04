#ifndef PROCESS_FACTORY_H
#define PROCESS_FACTORY_H

#include <memory>
#include <string>

class Process;

// Helper functions for instruction parsing
std::string trim(const std::string &str);
bool is_number(const std::string &s);
uint16_t parse_hex_address(const std::string& str);

class ProcessFactory
{
public:
    // For dummy processes
    static std::shared_ptr<Process> generate_dummy_process(
        const std::string &name,
        size_t mem_required,
        int min_ins,
        int max_ins);

    // For user-defined processes
    static std::shared_ptr<Process> generate_custom_process(
        const std::string &name,
        size_t mem_required,
        const std::string &instructions_str);
    
    static std::shared_ptr<Process> generate_background_process(
        const std::string &name,
        size_t mem_required);
};

#endif // PROCESS_FACTORY_H

#ifndef PROCESS_FACTORY_H
#define PROCESS_FACTORY_H

#include <memory>
#include <string>

class Process;

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

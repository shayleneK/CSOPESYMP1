#ifndef PROCESS_FACTORY_H
#define PROCESS_FACTORY_H

#include <memory>
#include <string>

class Process;

class ProcessFactory
{
public:
    static std::shared_ptr<Process> generate_dummy_process(
        const std::string &name,
        size_t mem_required,
        int min_ins,
        int max_ins);
};

#endif // PROCESS_FACTORY_H

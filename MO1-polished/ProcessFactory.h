#ifndef PROCESS_FACTORY_H
#define PROCESS_FACTORY_H

#include <memory>
#include <string>
#include "MemoryManager.h"

class Process;

class ProcessFactory
{
public:
    ProcessFactory(size_t frameSize, MemoryManager *memMgr)
        : frameSize(frameSize), memoryManager(memMgr) {}

    std::shared_ptr<Process> createProcess(const std::string &name, size_t mem_required)
    {
        return std::make_shared<Process>(name, mem_required, global_pid_counter++, frameSize, memoryManager);
    }
    // For dummy processes
    std::shared_ptr<Process> generate_dummy_process(
        const std::string &name,
        size_t mem_required,
        int min_ins,
        int max_ins);

    // For user-defined processes
    std::shared_ptr<Process> generate_custom_process(
        const std::string &name,
        size_t mem_required,
        const std::string &instructions_str);

private:
    size_t frameSize;
    MemoryManager *memoryManager;
    int global_pid_counter = 0;
};

#endif // PROCESS_FACTORY_H

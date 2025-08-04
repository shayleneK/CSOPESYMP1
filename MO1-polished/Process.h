#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <cstdint>
#include "MemoryManager.h"

// Forward declaration
class Command;

struct FrameEntry
{
    int frameNumber = -1;
    bool isValid = false;
    bool isDirty = false;
    long long lastUsed = 0; // for LRU
};

class Process
{
public:
    // Constructor
    Process(const std::string &name, size_t memSize, int pid, size_t frameSize, MemoryManager *memMgr);

    // Core execution
    void execute(int coreId);
    void addCommand(std::shared_ptr<Command> cmd);

    // Memory operations
    uint32_t readMemory(uint32_t virtualAddr);
    void writeMemory(uint32_t virtualAddr, uint32_t value);

    // Variable access (symbol table)
    uint32_t getVar(const std::string &name);
    bool declareVar(const std::string &name, uint32_t value);

    // Lifecycle & state
    bool canExecute() const;
    bool isFinished() const { return is_finished; }
    bool hasStarted() const { return has_started; }
    void markAsError(uint32_t addr); // for memory violation

    // Getters
    std::string getName() const { return name; }
    int getPid() const { return pid; }
    size_t getMemorySize() const { return memorySize; }
    int getCurrentCore() const { return current_core; }
    int getCurrentCommandIndex() const { return current_command_index; }
    int getTotalInstructions() const { return static_cast<int>(commands.size()); }

    std::chrono::system_clock::time_point getStartTime() const { return start_time; }
    std::chrono::system_clock::time_point getFinishTime() const { return finish_time; }
    const std::vector<std::string> &getLogs() const { return logs; }
    bool hasError() const { return has_error; }
    uint16_t getErrorAddress() const { return invalid_address; }
    std::string getErrorTime() const;

    void setMemoryManager(MemoryManager *mm) { memory_manager = mm; }

    void triggerPageFault(int virtualPage);

    // Symbol table info
    size_t getSymbolTableUsage() const { return symbol_table.size() * 2; }

    // Page table access
    int getPageTableSize() const { return numPages; }
    bool isPageValid(int page) const;
    void setPageDirty(int page);

    // For backing store
    int getSymbolTablePageNum() const { return 0; } // Page 0 = symbol table

    void logExecution(int coreId, const std::string &message);
    void markPageValid(int page);

private:
    // Helpers
    int getVirtualPageNumber(uint32_t addr) const;
    int getOffset(uint32_t addr) const;
    bool isAddressValid(uint32_t addr) const;
    MemoryManager *memory_manager = nullptr;

    // Process identity
    std::string name;
    int pid;

    // State
    bool has_started = false;
    bool is_finished = false;
    bool has_error = false;
    uint16_t invalid_address = 0;

    // Timing
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point finish_time;

    // Execution
    int current_core = -1;
    int current_command_index = 0;
    int delay_counter = 0;
    static const int delay_per_exec = 1; // from config.txt

    // Memory
    size_t memorySize;                            // total memory allocated (power of 2, ≥64)
    int numPages;                                 // memorySize / frameSize
    std::map<std::string, uint32_t> symbol_table; // symbol table (max 32 vars)
    std::vector<std::shared_ptr<Command>> commands;
    size_t frameSize;             //  size of each frame
    MemoryManager *memoryManager; // pointer to the memory manager

    // Page table: virtual page → frame metadata
    std::vector<FrameEntry> pageTable;

    // Logs
    std::vector<std::string> logs;
};

#endif // PROCESS_H
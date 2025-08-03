#include "Process.h"
#include "Command.h"
#include "ConsoleManager.h"
#include "MemoryManager.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <iostream>

// --- CONSTRUCTOR: Initialize a new process ---
Process::Process(const std::string &name, size_t memSize, int pid, size_t frameSize, MemoryManager *memMgr)
    : name(name),
      pid(pid),
      memorySize(memSize),
      frameSize(frameSize), // store frame/page size
      memoryManager(memMgr) // store memory manager pointer
{
    // Validate memory allocation per MO2 spec:
    if (memSize < 64)
    {
        throw std::invalid_argument("invalid memory allocation"); // Requirement: min 64 bytes
    }
    if ((memSize & (memSize - 1)) != 0)
    {
        throw std::invalid_argument("invalid memory allocation"); // Not power of 2 → invalid
    }

    // Compute how many virtual pages this process needs
    numPages = memorySize / frameSize; // e.g., 1024 bytes / 256 = 4 pages

    // Resize page table to hold one entry per virtual page
    pageTable.resize(numPages); // default FrameEntry: isValid=false, isDirty=false
}

// --- Add a command (e.g., DECLARE, PRINT, WRITE) to this process ---
void Process::addCommand(std::shared_ptr<Command> cmd)
{
    // MO2 Requirement: A process can have 1 to 50 instructions
    if (commands.size() < 50)
    {
        commands.push_back(cmd); // Add command to instruction list
    }
    // If more than 50, ignore — caller should validate before calling
}

// --- Execute one instruction of the process ---
void Process::execute(int coreId)
{
    // If process is already finished or crashed, do nothing
    if (is_finished || has_error)
        return;

    // Assign current CPU core for logging and execution context
    current_core = coreId;

    // Safety check: if global console is not running, stop
    if (!ConsoleManager::getInstance()->isRunning())
    {
        return;
    }

    // Mark start time on first execution (simulate process start)
    if (!has_started)
    {
        has_started = true;
        start_time = std::chrono::system_clock::now();
    }

    // Handle delay between instructions (from config.txt: delays-per-exec)
    if (delay_counter > 0)
    {
        delay_counter--; // Wait one cycle
        return;          // Don't execute instruction yet
    }

    // Check if all instructions are done
    if (current_command_index >= commands.size())
    {
        is_finished = true;
        finish_time = std::chrono::system_clock::now();
        logExecution(coreId, "Process completed all commands.");
        return;
    }

    // --- Execute current instruction ---
    // Example: PRINT, DECLARE, WRITE, etc.
    // The Command object knows how to interact with this Process
    commands[current_command_index]->execute(this, coreId, name);

    // Move to next instruction
    current_command_index++;

    // Reset delay counter so next instruction waits
    delay_counter = delay_per_exec; // From config.txt
}

// --- Can this process execute now? ---
bool Process::canExecute() const
{
    // Only if no delay is pending
    return delay_counter == 0;
}

// --- READ from virtual memory ---
uint32_t Process::readMemory(uint32_t virtualAddr)
{
    // Step 1: Check if address is within this process's allocated memory
    if (!isAddressValid(virtualAddr))
    {
        markAsError(virtualAddr); // Invalid access → kill process
        return 0;
    }

    // Step 2: Break address into page number and offset within page
    int page = getVirtualPageNumber(virtualAddr); // e.g., 0x500 → page 2 (if 256-byte pages)
    int offset = getOffset(virtualAddr);          // e.g., 0x500 → offset 0

    // Step 3: Check if the required page is currently in physical memory
    if (!isPageValid(page))
    {
        // Page is not in RAM → trigger a page fault
        triggerPageFault(page);

        // After page fault handling, check again
        // (In real OS, instruction restarts automatically)
        if (!isPageValid(page))
        {
            // Still not loaded? Something went wrong → crash
            markAsError(virtualAddr);
            return 0;
        }
    }

    // Step 4: Simulate reading a 16-bit value from memory
    // In real system: access physical frame + offset
    // Here: return dummy data based on address (for demo)
    return static_cast<uint32_t>(virtualAddr ^ 0xABCD); // Dummy value
}

// --- WRITE to virtual memory ---
void Process::writeMemory(uint32_t virtualAddr, uint32_t value)
{
    // Step 1: Validate memory bounds
    if (!isAddressValid(virtualAddr))
    {
        markAsError(virtualAddr); // Out of bounds → crash
        return;
    }

    // Step 2: Get page number
    int page = getVirtualPageNumber(virtualAddr);
    int offset = getOffset(virtualAddr);

    // Step 3: Ensure page is in memory
    if (!isPageValid(page))
    {
        triggerPageFault(page);
        if (!isPageValid(page))
        {
            markAsError(virtualAddr);
            return;
        }
    }

    // Step 4: Mark page as DIRTY because we're modifying it
    // This means when this page is evicted, it must be saved to backing store
    setPageDirty(page);

    // Step 5: Simulate write operation
    // In full emulator: write 'value' to physical frame + offset
    // Here: just log it
    std::ostringstream oss;
    oss << "Wrote 0x" << std::hex << value << " to virtual address 0x" << virtualAddr;
    logExecution(current_core, oss.str());
}

// --- Get variable value from symbol table ---
uint32_t Process::getVar(const std::string &name)
{
    // Only one check needed: is Page 0 (symbol table) valid?
    if (!isPageValid(0))
    {
        triggerPageFault(0);
        if (!isPageValid(0))
        {
            return 0;
        }
    }

    auto it = symbol_table.find(name);
    return (it != symbol_table.end()) ? it->second : 0;
}

// --- Declare a new variable ---
bool Process::declareVar(const std::string &name, uint32_t value)
{
    // MO2 Requirement: Max 32 variables (64 bytes total, 2 bytes each)
    if (symbol_table.size() >= 32)
    {
        // Limit reached → ignore new declarations
        return false;
    }

    // Store variable
    symbol_table[name] = value;

    // Symbol table is stored in Page 0 → mark it dirty
    setPageDirty(0);

    return true;
}

// --- HELPER: Get virtual page number from address ---
int Process::getVirtualPageNumber(uint32_t addr) const
{
    return addr / frameSize; // Integer division gives page #
}

// --- HELPER: Get offset within a page ---
int Process::getOffset(uint32_t addr) const
{
    return addr % frameSize; // Remainder gives offset
}

// --- HELPER: Is this virtual address valid? ---
bool Process::isAddressValid(uint32_t addr) const
{
    // Address must be less than total memory allocated to this process
    return addr < memorySize;
}

// --- HELPER: Is a given virtual page currently in physical memory? ---
bool Process::isPageValid(int page) const
{
    if (page < 0 || page >= numPages)
        return false;
    return pageTable[page].isValid;
}

// --- HELPER: Mark a page as modified (dirty) ---
void Process::setPageDirty(int page)
{
    if (page >= 0 && page < numPages)
    {
        pageTable[page].isDirty = true;
    }
}

// --- Trigger a page fault (when accessing invalid page) ---
void Process::triggerPageFault(int virtualPage)
{
    if (virtualPage < 0 || virtualPage >= numPages)
        return;

    if (memoryManager)
    {
        std::cout << "[" << name << "] Page fault: accessing invalid page " << virtualPage << "\n";

        memoryManager->loadPage(name, virtualPage); // use the process name

        pageTable[virtualPage].isValid = true; // Mark this page as loaded into memory
    }
    else
    {
        std::ostringstream oss;
        oss << "Page fault failed: no memory manager (page " << virtualPage << ")";
        logExecution(current_core, oss.str());
        markAsError(0);
    }
}

// --- Mark process as crashed due to invalid memory access ---
void Process::markAsError(uint32_t addr)
{
    has_error = true;
    is_finished = true;
    invalid_address = addr;
    finish_time = std::chrono::system_clock::now();

    // Log the violation
    std::ostringstream oss;
    oss << "Memory access violation at 0x" << std::hex << addr;
    logExecution(current_core, oss.str());
}

// --- Get formatted error time (HH:MM:SS) ---
std::string Process::getErrorTime() const
{
    if (!has_error)
        return "";
    auto now_c = std::chrono::system_clock::to_time_t(finish_time);
    std::tm tm = *std::localtime(&now_c);
    char buf[10];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return std::string(buf); // e.g., "14:22:30"
}

// --- Log an event with timestamp and core ID ---
void Process::logExecution(int coreId, const std::string &message)
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_c);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

    std::ostringstream oss;
    oss << "(" << timestamp << ") Core:" << coreId << " - " << message;

    logs.push_back(oss.str());
}

void Process::markPageValid(int page)
{
    if (page >= 0 && page < numPages)
        pageTable[page].isValid = true;
}

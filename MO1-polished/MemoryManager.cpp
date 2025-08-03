#include "MemoryManager.h"
#include <iostream>
#define GREEN "\033[32m"

#include <sstream>

//remove to test/run
//MemoryManager g_MemoryManager(16384); 
//int memPerFrame = 16;

MemoryManager::MemoryManager(size_t MAX_MEMORY)
{
    memory_blocks_.push_back(MemoryBlock{0, MAX_MEMORY - 1, false, ""});
}

int MemoryManager::allocate(size_t size, std::string process_id)
{
    for (auto it = memory_blocks_.begin(); it != memory_blocks_.end(); ++it)
    {
        if (!it->allocated && it->size() >= size)
        {
            MemoryBlock original = *it;
            memory_blocks_.erase(it);
            it = memory_blocks_.insert(it, {original.start, original.start + static_cast<int>(size) - 1, true, process_id});
            ++it;
            if (original.start + size <= original.end)
            {
                memory_blocks_.insert(it, {original.start + static_cast<int>(size), original.end, false, ""});
            }
            return original.start;
        }
    }
    return -1;
}

void MemoryManager::deallocate(const std::string &process_id)
{
    for (auto &block : memory_blocks_)
    {
        if (block.process_id == process_id)
        {
            block.allocated = false;
            block.process_id = "";
        }
    }
    defragment();
}

double MemoryManager::getExternalFragmentation()
{
    size_t total_free = 0;
    for (const auto &block : memory_blocks_)
    {
        if (!block.allocated)
        {
            total_free += block.size();
        }
    }
    return static_cast<double>(total_free);
}

std::string MemoryManager::printMemoryLayout()
{
    std::ostringstream output;

    if (memory_blocks_.empty())
    {
        output << "Memory is empty.\n";
        return output.str();
    }

    // Print global end marker
    const auto &last_block = memory_blocks_.back();
    output << "\n------end----- = " << last_block.end << "\n\n";

    // Top-down block view (only allocated blocks)
    for (auto it = memory_blocks_.rbegin(); it != memory_blocks_.rend(); ++it)
    {
        const auto &block = *it;

        if (block.allocated)
        {
            output << block.end << "\n";
            output << "Process: P" << block.process_id << "\n";
            output << block.start << "\n";
            output << "\n"; // Blank line between blocks
        }
    }

    // Print global start marker
    const auto &first_block = memory_blocks_.front();
    output << "------start----- = " << first_block.start << "\n";

    return output.str();
}

void MemoryManager::defragment()
{
    for (size_t i = 0; i < memory_blocks_.size() - 1;)
    {
        MemoryBlock &curr = memory_blocks_[i];
        MemoryBlock &next = memory_blocks_[i + 1];
        if (!curr.allocated && !next.allocated)
        {
            curr.end = next.end;
            memory_blocks_.erase(memory_blocks_.begin() + i + 1);
        }
        else
        {
            ++i;
        }
    }
}

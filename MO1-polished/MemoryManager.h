#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <string>

const int MAX_MEMORY = 65536; // 64 KB example

struct MemoryBlock
{
    size_t start;
    size_t end;
    bool allocated;
    std::string process_id;

    size_t size() const
    {
        return end - start + 1;
    }
};

class MemoryManager
{
public:
    MemoryManager(size_t max_mem = 1000);
    int allocate(size_t size, std::string process_id);
    void deallocate(const std::string &process_id);
    double getExternalFragmentation();
    std::string printMemoryLayout();

private:
    std::vector<MemoryBlock> memory_blocks_;
    void defragment();
};

#endif // MEMORY_MANAGER_H
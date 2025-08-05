#pragma once

#include <string>
#include <unordered_map>
#include <deque>
#include <vector>
#include <set>
#include <cstdint>

struct FrameInfo
{
    std::string process_name;
    int page_number;
    bool dirty;
};

class MemoryManager
{
public:
    MemoryManager(size_t total_memory, size_t page_size);

    int allocate(size_t bytes_required, const std::string &process_name);
    void deallocate(const std::string &process_name);

    bool isPageInMemory(const std::string &process_name, int page_number) const;
    int getFrameNumber(const std::string &process_name, int page_number) const;

    void loadPage(const std::string &process_name, int page_number);
    void markPageDirty(const std::string &process_name, int page_number);

    std::string printMemoryLayout() const;
    size_t getExternalFragmentation() const;

    size_t getTotalMemory() const { return (total_frames * page_size); };
    size_t getPageSize() const { return page_size; };

    void write(uint32_t physicalAddr, uint16_t value);
    uint16_t read(uint32_t physicalAddr);

    size_t getPagesPagedIn() const { return pages_paged_in; }
    size_t getPagesPagedOut() const { return pages_paged_out; }

private:
    void evictPageIfNeeded();
    void evictOldestPage();
    int findFreeFrame() const;

    std::vector<uint8_t> memory;

    size_t total_frames;
    size_t page_size;

    std::vector<bool> frame_used;
    std::vector<FrameInfo> frame_table;

    std::unordered_map<std::string, std::set<int>> process_pages;
    std::unordered_map<std::string, std::vector<int>> process_page_table;

    std::deque<std::pair<std::string, int>> fifo_queue;
    std::unordered_map<std::string, std::set<int>> backing_store;

    // Page stats
    size_t pages_paged_in = 0;
    size_t pages_paged_out = 0;
};

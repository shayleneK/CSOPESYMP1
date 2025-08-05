#include "MemoryManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>

MemoryManager::MemoryManager(size_t total_memory, size_t page_size_bytes)
    : page_size(page_size_bytes)
{
    std::cout << "pages" << page_size << "\n"
              << std::endl;

    total_frames = (total_memory) / page_size;
    frame_used.resize(total_frames, false);
    frame_table.resize(total_frames);
    memory.resize(total_memory, 0);
}

// int MemoryManager::allocate(size_t bytes_required, const std::string &process_name)
// {
//     // Ensure at least one page
//     if (bytes_required < page_size)
//     {
//         bytes_required = page_size;
//     }

//     // Align to page boundary
//     bytes_required = ((bytes_required + page_size - 1) / page_size) * page_size;

//     size_t pages_needed = bytes_required / page_size;

//     // Safety: prevent allocating more pages than available
//     if (pages_needed > total_frames)
//     {
//         throw std::invalid_argument(
//             "invalid memory allocation: requested " +
//             std::to_string(bytes_required) +
//             " bytes, but only " +
//             std::to_string(total_frames * page_size) +
//             " bytes available");
//     }

//     // Create page table
//     process_page_table[process_name] = std::vector<int>(pages_needed, -1);
//     for (int i = 0; i < static_cast<int>(pages_needed); ++i)
//     {
//         writeToBackingStore(process_name, i);
//     }

//     return 0; // base virtual address
// }

// In MemoryManager.cpp
#include "MemoryManager.h"
#include <stdexcept>
#include <iostream>

AllocationResult MemoryManager::allocate(size_t requested, const std::string &process_name)
{
    std::cout << "Allocating " << requested << " bytes for process " << process_name << std::endl;
    if (requested < page_size)
        requested = page_size;

    // Align to page size
    size_t aligned_size = ((requested + page_size - 1) / page_size) * page_size;
    size_t num_pages_needed = aligned_size / page_size;

    if (num_pages_needed > total_frames)
    {
        aligned_size = total_frames * page_size;
        num_pages_needed = total_frames;
    }

    if (num_pages_needed == 0)
    {
        throw std::invalid_argument("Requested size too small to allocate a frame.");
    }

    // Track allocated frame indices
    std::vector<int> allocated_frames;
    int start_address = -1;

    for (size_t i = 0; i < num_pages_needed; ++i)
    {
        int frame = findFreeFrame();
        if (frame == -1)
        {
            // Rollback partial allocation
            for (int f : allocated_frames)
            {
                frame_used[f] = false;
                frame_table[f] = FrameInfo{};
            }
            throw std::invalid_argument("Not enough contiguous frames for allocation.");
        }

        // Mark frame as used
        frame_used[frame] = true;
        frame_table[frame] = {process_name, static_cast<int>(i), false};
        allocated_frames.push_back(frame);
        process_pages[process_name].insert(frame);
        fifo_queue.push_back({process_name, static_cast<int>(i)});

        if (i == 0)
        {
            start_address = frame * page_size;
        }
    }

    process_page_table[process_name] = allocated_frames;

    return {start_address, aligned_size};
}

void MemoryManager::deallocate(const std::string &process_name)
{
    for (int i = 0; i < static_cast<int>(total_frames); ++i)
    {
        if (frame_used[i] && frame_table[i].process_name == process_name)
        {
            frame_used[i] = false;
            frame_table[i] = {};
            pages_paged_out++;
        }
    }

    process_pages.erase(process_name);
    process_page_table.erase(process_name);
    removeFromBackingStore(process_name);

    fifo_queue.erase(
        std::remove_if(fifo_queue.begin(), fifo_queue.end(),
                       [&](auto &pair)
                       { return pair.first == process_name; }),
        fifo_queue.end());
}

bool MemoryManager::isPageInMemory(const std::string &process_name, int page_number) const
{
    auto it = process_page_table.find(process_name);
    if (it == process_page_table.end())
        return false;

    return (page_number >= 0 &&
            page_number < static_cast<int>(it->second.size()) &&
            it->second[page_number] != -1);
}

int MemoryManager::getFrameNumber(const std::string &process_name, int page_number) const
{
    if (!isPageInMemory(process_name, page_number))
        return -1;
    return process_page_table.at(process_name)[page_number];
}

void MemoryManager::markPageDirty(const std::string &process_name, int page_number)
{
    int frame = getFrameNumber(process_name, page_number);
    if (frame >= 0 && frame < static_cast<int>(frame_table.size()))
        frame_table[frame].dirty = true;
}

void MemoryManager::loadPage(const std::string &process_name, int page_number)
{
    if (isPageInMemory(process_name, page_number))
        return;

    evictPageIfNeeded();

    int frame = findFreeFrame();
    if (frame == -1)
    {
        std::cerr << "[MM] No free frame available after eviction!\n";
        return;
    }

    frame_used[frame] = true;
    frame_table[frame] = {process_name, page_number, false};
    process_page_table[process_name][page_number] = frame;
    process_pages[process_name].insert(page_number);
    fifo_queue.push_back({process_name, page_number});

    std::cout << "[MM] Loaded page " << page_number << " of " << process_name
              << " into frame " << frame << "\n";

    pages_paged_in++;
}

void MemoryManager::evictPageIfNeeded()
{
    int used = std::count(frame_used.begin(), frame_used.end(), true);
    if (used >= static_cast<int>(total_frames))
        evictOldestPage();
}

void MemoryManager::evictOldestPage()
{
    if (fifo_queue.empty())
        return;

    auto [proc, page] = fifo_queue.front();
    fifo_queue.pop_front();

    int frame = getFrameNumber(proc, page);
    if (frame >= 0)
    {
        if (frame_table[frame].dirty)
        {
            std::cout << "[MM] Writing dirty page " << page << " of " << proc
                      << " back to backing store\n";
            writeToBackingStore(proc, page);
        }

        frame_used[frame] = false;
        frame_table[frame] = {};
        process_page_table[proc][page] = -1;
        process_pages[proc].erase(page);

        std::cout << "[MM] Evicted page " << page << " of " << proc
                  << " from frame " << frame << "\n";

        pages_paged_out++;
    }
}

int MemoryManager::findFreeFrame() const
{
    for (int i = 0; i < static_cast<int>(total_frames); ++i)
    {
        if (!frame_used[i])
            return i;
    }
    return -1;
}

std::string MemoryManager::printMemoryLayout() const
{
    std::ostringstream oss;
    for (int i = 0; i < static_cast<int>(total_frames); ++i)
    {
        if (frame_used[i])
        {
            const FrameInfo &fi = frame_table[i];
            oss << "Frame " << i << ": " << fi.process_name << " (Page "
                << fi.page_number << (fi.dirty ? ", Dirty" : "") << ")\n";
        }
        else
        {
            oss << "Frame " << i << ": [Free]\n";
        }
    }
    return oss.str();
}

size_t MemoryManager::getExternalFragmentation() const
{
    size_t free_frames = std::count(frame_used.begin(), frame_used.end(), false);
    return (free_frames * page_size);
}

void MemoryManager::write(uint32_t physicalAddr, uint16_t value)
{
    if (physicalAddr + 1 < memory.size())
    {
        memory[physicalAddr] = value & 0xFF;
        memory[physicalAddr + 1] = (value >> 8) & 0xFF;
    }
}

uint16_t MemoryManager::read(uint32_t physicalAddr)
{
    if (physicalAddr + 1 < memory.size())
    {
        return memory[physicalAddr] | (memory[physicalAddr + 1] << 8);
    }
    return 0;
}

void MemoryManager::writeToBackingStore(const std::string &process_name, int page_number)
{
    std::ofstream ofs("csopesy-backing-store.txt", std::ios::app);
    if (ofs.is_open())
    {
        ofs << process_name << " " << page_number << "\n";
    }
}

void MemoryManager::removeFromBackingStore(const std::string &process_name)
{
    std::ifstream ifs("csopesy-backing-store.txt");
    std::ofstream ofs("temp-backing-store.txt");
    std::string line;

    if (!ifs.is_open() || !ofs.is_open())
        return;

    while (std::getline(ifs, line))
    {
        std::istringstream iss(line);
        std::string pname;
        int page;
        if (iss >> pname >> page)
        {
            if (pname != process_name)
            {
                ofs << pname << " " << page << "\n";
            }
        }
    }

    ifs.close();
    ofs.close();
    std::remove("csopesy-backing-store.txt");
    std::rename("temp-backing-store.txt", "csopesy-backing-store.txt");
}

std::set<int> MemoryManager::readProcessPagesFromBackingStore(const std::string &process_name)
{
    std::ifstream ifs("csopesy-backing-store.txt");
    std::set<int> pages;
    std::string pname;
    int page;

    if (!ifs.is_open())
        return pages;

    while (ifs >> pname >> page)
    {
        if (pname == process_name)
        {
            pages.insert(page);
        }
    }

    return pages;
}

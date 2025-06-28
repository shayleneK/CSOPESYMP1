#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <unordered_map>
#include <cstdint>
#include <string>

// Symbol table: maps variable names to uint16_t values
extern std::unordered_map<std::string, uint16_t> global_symbol_table;

// Memory heap: simulates memory using address-value pairs
extern std::unordered_map<int, uint16_t> memory_heap;

#endif // GLOBAL_STATE_H
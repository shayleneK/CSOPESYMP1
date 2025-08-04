#include "Utils.h"
#include <cstdint> 
#include <iostream> // For std::cerr in parse_hex_address

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

bool is_number(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

uint16_t parse_hex_address(const std::string& str) {
    std::string s = trim(str);
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.empty()) throw std::invalid_argument("Empty hex address");
    try {
        return static_cast<uint16_t>(std::stoul(s, nullptr, 16));
    } catch (const std::out_of_range& e) {
        std::cerr << "[ERROR] Hex address out of range: " << str << "\n";
        throw;
    } catch (const std::invalid_argument& e) {
        std::cerr << "[ERROR] Invalid hex address format: " << str << "\n";
        throw;
    }
}
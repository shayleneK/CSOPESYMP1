#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstdint> 

std::string trim(const std::string &str);
bool is_number(const std::string &s);
uint16_t parse_hex_address(const std::string& str);

#endif 
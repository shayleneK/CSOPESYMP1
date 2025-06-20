#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool ConfigManager::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open config file: " << filename << "\n";
        return false;
    }

    std::string key, value;
    while (file >> key >> value) {
        config_map[key] = value;
    }

    file.close();
    return true;
}

int ConfigManager::getInt(const std::string& key, int default_val) const {
    auto it = config_map.find(key);
    if (it != config_map.end()) {
        return std::stoi(it->second);
    }
    return default_val;
}

std::string ConfigManager::getString(const std::string& key, const std::string& default_val) const {
    auto it = config_map.find(key);
    if (it != config_map.end()) {
        return it->second;
    }
    return default_val;
}

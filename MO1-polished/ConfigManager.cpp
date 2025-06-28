#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

bool ConfigManager::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open config file: " << filename << "\n";
        return false;
    }

    std::string key, value;
    while (file >> key >> value) {
        // Strip whitespace
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

        if (!key.empty()) {
            config_map[key] = value;
            //std::cout << "[DEBUG] Loaded config: " << key << " = " << value << "\n";
        }
    }

    file.close();
    return true;
}


int ConfigManager::getInt(const std::string& key, int default_val) const {
    auto it = config_map.find(key);
    if (it != config_map.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return default_val;
        }
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
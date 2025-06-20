#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <unordered_map>

class ConfigManager {
private:
    std::unordered_map<std::string, std::string> config_map;

public:
    bool load(const std::string& filename = "config.txt");

    int getInt(const std::string& key, int default_val = 0) const;
    std::string getString(const std::string& key, const std::string& default_val = "") const;
};

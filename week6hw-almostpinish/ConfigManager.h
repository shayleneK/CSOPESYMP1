#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <unordered_map>

class ConfigManager {
public:
    bool read(const std::string& filename);
    std::string get(const std::string& key) const;
    int getInt(const std::string& key) const;

private:
    std::unordered_map<std::string, std::string> configMap;
};

#endif

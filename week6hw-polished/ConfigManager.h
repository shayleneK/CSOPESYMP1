#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <map>
#include <string>
#include <unordered_map>

class ConfigManager {
private:
    std::map<std::string, std::string> config_map;

public:
    // Loads configuration from a file
    bool load(const std::string& filename);

    // Retrieves an integer value by key, with a default fallback
    int getInt(const std::string& key, int default_val) const;

    // Retrieves a string value by key, with a default fallback
    std::string getString(const std::string& key, const std::string& default_val) const;
};

#endif // CONFIG_MANAGER_H
#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>

// Enum for Log Levels
enum class LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3,
    LOG_LEVELS = 4
};

enum class LogOutput {
     LOG_TO_TERMINAL,
     LOG_TO_FILE
};

class Config {
public:
    // Constructor and Destructor
    Config() = default;
    ~Config() = default;

    // Load configuration from a file
    bool loadConfig(const std::string& filename);

    // Getter methods for various data types
    std::string getString(const std::string& key, const std::string& defaultValue = "");
    int getInt(const std::string& key, int defaultValue = 0);

    LogLevel getLogLevel(const std::string& key, LogLevel defaultValue = LogLevel::LOG_ERROR);

    LogOutput getLogTarget(const std::string& key, LogOutput defaultValue = LogOutput::LOG_TO_TERMINAL);

private:
    // Method to trim leading and trailing whitespaces from strings
    void trim(std::string& str);

    // Map to store the configuration key-value pairs
    std::map<std::string, std::string> configMap;
};

#endif // CONFIG_H

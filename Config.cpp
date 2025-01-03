#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

void Config::trim(std::string& str) {
    // Trim leading and trailing whitespace
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    if (first == std::string::npos || last == std::string::npos) {
        str = "";
    } else {
        str = str.substr(first, (last - first + 1));
    }
}

bool Config::loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Split the line by the '=' character
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            trim(key);
            trim(value);
            configMap[key] = value;
        }
    }

    file.close();
    return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    auto it = configMap.find(key);
    if (it != configMap.end()) {
        return it->second;
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    auto it = configMap.find(key);
    if (it != configMap.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::invalid_argument&) {
            return defaultValue;
        }
    }
    return defaultValue;
}

LogLevel Config::getLogLevel(const std::string& key, LogLevel defaultValue) {
    std::string logLevelString = getString(key, "");

    if (logLevelString == "ERROR") {
        return LogLevel::LOG_ERROR;
    } else if (logLevelString == "WARNING") {
        return LogLevel::LOG_WARNING;
    } else if (logLevelString == "INFO") {
        return LogLevel::LOG_INFO;
    } else if (logLevelString == "DEBUG") {
        return LogLevel::LOG_DEBUG;
    }
    return defaultValue;
}

LogOutput Config::getLogTarget(const std::string& key, LogOutput defaultValue) {
    std::string logTargetString = getString(key, "");

    if (logTargetString == "FILE") {
        return LogOutput::LOG_TO_FILE;
    } else if (logTargetString == "WARNING") {
        return LogOutput::LOG_TO_TERMINAL;
        }
    return defaultValue;
}
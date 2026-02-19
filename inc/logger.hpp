#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <mutex>
#include <ctime>

enum class LogLevel { INFO, WARN, ERR, DEBUG };

class Logger {
public:
    static void log(LogLevel level, const std::string& message) {
        static std::mutex log_mutex;
        std::lock_guard<std::mutex> lock(log_mutex);

        std::time_t now = std::time(nullptr);
        char timestamp[20];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        std::string label;
        switch (level) {
            case LogLevel::INFO:  label = "\033[32m[INFO]\033[0m "; break;  // Green
            case LogLevel::WARN:  label = "\033[33m[WARN]\033[0m "; break;  // Yellow
            case LogLevel::ERR:   label = "\033[31m[ERR ]\033[0m "; break;  // Red
            case LogLevel::DEBUG: label = "\033[36m[DEBG]\033[0m "; break;  // Cyan
        }

        std::cout << "[" << timestamp << "] " << label << message << "\n";
    }
};

#endif
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>

#include <stdio.h>
#include <stdarg.h>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    // Singleton access instance
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Initialize output file (optional)
    void initFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(logMutex);
        fp = fopen(filename.c_str(), "a");
    }
    
    // Set the minimum level required to print a log
    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex);
        minLevel = level;
    }

    // Core log method
    void log(LogLevel level, const char *s, ...) {
        std::string levelStr = getLevelString(level);
        std::string timeStr = getCurrentTime();

        std::lock_guard<std::mutex> lock(logMutex);
        
        if (level < minLevel) return;

        std::stringstream ss;
        ss << "[" << timeStr << "] [" << levelStr << "] ";
        
        va_list ap;
        va_start(ap, s);

        // Print to console
        printf("%s", ss.str().c_str());
        vfprintf(stdout, s, ap);
        printf("%s", "\n");

        // Print to file if configured
        if (fp) {
            fprintf(fp, "%s", ss.str().c_str());
            vfprintf(fp, s, ap);
            fprintf(fp, "%s", "\n");
        }
        va_end(ap);
        
    }

    Logger() : minLevel(LogLevel::INFO) {
        fp = NULL;
    } // Default log level
    
    ~Logger() {
        if (fp != NULL) {
            fclose(fp);
        }
    }


private:
    LogLevel minLevel;
    std::ofstream fileStream;
    std::mutex logMutex;
    FILE *fp;
    
    std::string getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR:   return "ERROR";
            default:                return "UNKNOWN";
        }
    }

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }
};

// Helper macros for quicker access
#define LOG_DEBUG(msg)   Logger::getInstance().log(LogLevel::DEBUG, msg)
#define LOG_INFO(msg)    Logger::getInstance().log(LogLevel::INFO, msg)
#define LOG_WARN(msg)    Logger::getInstance().log(LogLevel::WARNING, msg)
#define LOG_ERROR(msg)   Logger::getInstance().log(LogLevel::ERROR, msg)

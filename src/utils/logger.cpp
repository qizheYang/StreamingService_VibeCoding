#include "utils/logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

Logger::Level Logger::current_level_ = Logger::Level::INFO;

void Logger::set_level(Level level) {
    current_level_ = level;
}

void Logger::debug(const std::string& msg) { log(Level::DEBUG, msg); }
void Logger::info(const std::string& msg)  { log(Level::INFO,  msg); }
void Logger::warn(const std::string& msg)  { log(Level::WARN,  msg); }
void Logger::error(const std::string& msg) { log(Level::ERROR, msg); }

void Logger::log(Level level, const std::string& msg) {
    if (level < current_level_) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&time);

    const char* level_str = "";
    const char* color = "";
    switch (level) {
        case Level::DEBUG: level_str = "DEBUG"; color = "\033[36m"; break;
        case Level::INFO:  level_str = "INFO";  color = "\033[32m"; break;
        case Level::WARN:  level_str = "WARN";  color = "\033[33m"; break;
        case Level::ERROR: level_str = "ERROR"; color = "\033[31m"; break;
    }

    std::cerr << color
              << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
              << " [" << level_str << "] "
              << "\033[0m"
              << msg << std::endl;
}

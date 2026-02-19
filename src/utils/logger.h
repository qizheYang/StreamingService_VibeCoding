#pragma once

#include <string>

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERROR };

    static void set_level(Level level);
    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);

private:
    static void log(Level level, const std::string& msg);
    static Level current_level_;
};

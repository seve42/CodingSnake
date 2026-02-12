#include "../include/utils/Logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

namespace snake {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : level_(Level::INFO)
    , consoleEnabled_(true) {
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::setLevel(Level level) {
    level_ = level;
}

void Logger::setLogFile(const std::string& filename) {
    // TODO: 实现日志文件设置
}

void Logger::enableConsole(bool enable) {
    consoleEnabled_ = enable;
}

void Logger::debug(const std::string& msg) {
    log(Level::DEBUG, msg);
}

void Logger::info(const std::string& msg) {
    log(Level::INFO, msg);
}

void Logger::warning(const std::string& msg) {
    log(Level::WARNING, msg);
}

void Logger::error(const std::string& msg) {
    log(Level::ERROR, msg);
}

void Logger::log(Level level, const std::string& msg) {
    // TODO: 实现完整的日志记录逻辑
    if (consoleEnabled_ && level >= level_) {
        std::cout << "[" << levelToString(level) << "] " << msg << std::endl;
    }
}

std::string Logger::levelToString(Level level) const {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTime() const {
    // TODO: 实现时间格式化
    return "";
}

void Logger::writeLog(Level level, const std::string& msg) {
    // TODO: 实现日志写入
}

} // namespace snake

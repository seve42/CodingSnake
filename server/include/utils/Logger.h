#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace snake {

/**
 * @brief 日志系统
 */
class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static Logger& getInstance();

    void setLevel(Level level);
    void setLogFile(const std::string& filename);
    void enableConsole(bool enable);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);

    void log(Level level, const std::string& msg);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string levelToString(Level level) const;
    std::string getCurrentTime() const;
    void writeLog(Level level, const std::string& msg);

    Level level_;
    std::ofstream logFile_;
    bool consoleEnabled_;
    std::mutex mutex_;
};

// 便捷宏
#define LOG_DEBUG(msg) snake::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) snake::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) snake::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) snake::Logger::getInstance().error(msg)

} // namespace snake

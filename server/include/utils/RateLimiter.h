#pragma once

#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <deque>

namespace snake {

/**
 * @brief 速率限制器
 * 实现基于滑动窗口的速率限制
 */
class RateLimiter {
public:
    RateLimiter();
    ~RateLimiter();

    // 检查是否超过速率限制
    bool checkLimit(const std::string& key, int maxRequests, int windowSeconds);
    
    // 获取需要等待的时间（秒）
    int getRetryAfter(const std::string& key, int maxRequests, int windowSeconds) const;

    // 清理过期记录
    void cleanup();
    
    // 清理特定前缀的所有记录（用于回合重置）
    void clearByPrefix(const std::string& prefix);

private:
    struct RequestRecord {
        std::deque<std::chrono::system_clock::time_point> timestamps;
    };

    std::map<std::string, RequestRecord> records_;
    mutable std::mutex mutex_;
};

} // namespace snake

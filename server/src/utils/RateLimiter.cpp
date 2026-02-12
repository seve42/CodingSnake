#include "utils/RateLimiter.h"

namespace snake {

RateLimiter::RateLimiter() {
    // 构造函数
}

RateLimiter::~RateLimiter() {
    // 析构函数
}

bool RateLimiter::checkLimit(const std::string& key, int maxRequests, int windowSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto windowStart = now - std::chrono::seconds(windowSeconds);
    
    // 获取或创建记录
    auto& record = records_[key];
    
    // 清理过期的时间戳
    while (!record.timestamps.empty() && record.timestamps.front() < windowStart) {
        record.timestamps.pop_front();
    }
    
    // 检查是否超过限制
    if (static_cast<int>(record.timestamps.size()) >= maxRequests) {
        return false; // 超过限制
    }
    
    // 记录当前请求时间
    record.timestamps.push_back(now);
    
    return true; // 允许通过
}

int RateLimiter::getRetryAfter(const std::string& key, int maxRequests, int windowSeconds) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = records_.find(key);
    if (it == records_.end() || it->second.timestamps.empty()) {
        return 0;
    }
    
    auto now = std::chrono::system_clock::now();
    auto windowStart = now - std::chrono::seconds(windowSeconds);
    
    // 找到最早的有效时间戳
    auto& timestamps = it->second.timestamps;
    for (const auto& timestamp : timestamps) {
        if (timestamp >= windowStart) {
            auto nextAllowedTime = timestamp + std::chrono::seconds(windowSeconds);
            auto waitTime = nextAllowedTime - now;
            
            if (waitTime.count() > 0) {
                return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(waitTime).count());
            }
            break;
        }
    }
    
    return 0;
}

void RateLimiter::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::hours(1); // 清理1小时前的记录
    
    for (auto it = records_.begin(); it != records_.end();) {
        auto& record = it->second;
        
        // 清理过期的时间戳
        while (!record.timestamps.empty() && record.timestamps.front() < cutoff) {
            record.timestamps.pop_front();
        }
        
        // 如果记录为空，删除整个记录
        if (record.timestamps.empty()) {
            it = records_.erase(it);
        } else {
            ++it;
        }
    }
}

void RateLimiter::clearByPrefix(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = records_.begin(); it != records_.end();) {
        if (it->first.find(prefix) == 0) {  // 检查键是否以prefix开头
            it = records_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace snake

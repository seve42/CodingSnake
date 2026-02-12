#pragma once

#include <atomic>
#include <cstdint>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace snake {

/**
 * @brief 性能监控器
 * 采集请求指标、回合耗时、锁等待、队列长度和内存占用
 */
class PerformanceMonitor {
public:
    struct Config {
        bool enabled = false;
        double sampleRate = 0.2;          // 采样率 [0,1]
        int windowSeconds = 60;           // QPS 统计窗口
        int maxSamples = 2000;            // 延迟样本上限
        bool logEnabled = false;          // 是否落盘
        int logIntervalSeconds = 10;      // 日志间隔
        std::string logPath = "./data/metrics.log";
        std::size_t logMaxBytes = 5 * 1024 * 1024; // 单文件上限
        int logMaxFiles = 3;              // 滚动文件数量
    };

    class ScopedRequest {
    public:
        explicit ScopedRequest(const std::string& endpoint);
        ~ScopedRequest();

    private:
        std::string endpoint_;
        std::chrono::steady_clock::time_point start_;
        bool enabled_ = false;
    };

    static PerformanceMonitor& getInstance();

    void configure(const Config& config);
    void start();
    void stop();
    bool isEnabled() const;

    void recordRequest(const std::string& endpoint, double latencyMs);
    void recordLockWait(const std::string& lockName, double waitMs);
    void observeRoundDuration(double roundMs);
    void setGauge(const std::string& name, double value);

    nlohmann::json toJson() const;
    std::string toPrometheus() const;

private:
    struct LockStat {
        std::uint64_t count = 0;
        double totalWaitMs = 0.0;
        double maxWaitMs = 0.0;
        double lastWaitMs = 0.0;
    };

    PerformanceMonitor();
    ~PerformanceMonitor();
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    bool shouldSample() const;
    void pruneDeque(std::deque<std::chrono::steady_clock::time_point>& deque,
                    const std::chrono::steady_clock::time_point& now) const;

    double percentile(const std::deque<double>& samples, double p) const;
    std::uint64_t getRssBytes() const;

    void logLoop();
    void writeSnapshotToLog(const nlohmann::json& snapshot);
    void rotateLogsIfNeeded();

    Config config_;
    std::atomic<bool> enabled_;
    std::atomic<bool> running_;

    mutable std::mutex mutex_;

    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> qpsByEndpoint_;
    std::deque<std::chrono::steady_clock::time_point> qpsAll_;

    std::unordered_map<std::string, std::deque<double>> latencyByEndpoint_;
    std::deque<double> latencyAll_;

    std::unordered_map<std::string, std::uint64_t> requestCounts_;
    std::uint64_t totalRequests_ = 0;

    std::unordered_map<std::string, LockStat> lockStats_;
    std::unordered_map<std::string, double> gauges_;

    std::deque<double> roundSamples_;
    double lastRoundMs_ = 0.0;

    std::thread logThread_;
};

} // namespace snake

#include "utils/PerformanceMonitor.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

namespace snake {

namespace {
constexpr double kPercentile95 = 0.95;
constexpr double kPercentile99 = 0.99;
}

PerformanceMonitor& PerformanceMonitor::getInstance() {
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::PerformanceMonitor()
    : enabled_(false), running_(false) {
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
}

PerformanceMonitor::ScopedRequest::ScopedRequest(const std::string& endpoint)
    : endpoint_(endpoint), start_(std::chrono::steady_clock::now()) {
    enabled_ = PerformanceMonitor::getInstance().isEnabled();
}

PerformanceMonitor::ScopedRequest::~ScopedRequest() {
    if (!enabled_) {
        return;
    }
    auto end = std::chrono::steady_clock::now();
    double latencyMs = std::chrono::duration<double, std::milli>(end - start_).count();
    PerformanceMonitor::getInstance().recordRequest(endpoint_, latencyMs);
}

void PerformanceMonitor::configure(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    enabled_.store(config_.enabled, std::memory_order_release);
}

void PerformanceMonitor::start() {
    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }
    if (!config_.logEnabled || config_.logIntervalSeconds <= 0) {
        return;
    }
    if (running_.exchange(true)) {
        return;
    }
    logThread_ = std::thread(&PerformanceMonitor::logLoop, this);
}

void PerformanceMonitor::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (logThread_.joinable()) {
        logThread_.join();
    }
}

bool PerformanceMonitor::isEnabled() const {
    return enabled_.load(std::memory_order_acquire);
}

void PerformanceMonitor::recordRequest(const std::string& endpoint, double latencyMs) {
    if (!isEnabled()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(mutex_);
    pruneDeque(qpsAll_, now);
    qpsAll_.push_back(now);

    auto& endpointQps = qpsByEndpoint_[endpoint];
    pruneDeque(endpointQps, now);
    endpointQps.push_back(now);

    totalRequests_ += 1;
    requestCounts_[endpoint] += 1;

    if (!shouldSample()) {
        return;
    }

    latencyAll_.push_back(latencyMs);
    if (static_cast<int>(latencyAll_.size()) > config_.maxSamples) {
        latencyAll_.pop_front();
    }

    auto& samples = latencyByEndpoint_[endpoint];
    samples.push_back(latencyMs);
    if (static_cast<int>(samples.size()) > config_.maxSamples) {
        samples.pop_front();
    }
}

void PerformanceMonitor::recordLockWait(const std::string& lockName, double waitMs) {
    if (!isEnabled()) {
        return;
    }
    if (!shouldSample()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& stat = lockStats_[lockName];
    stat.count += 1;
    stat.totalWaitMs += waitMs;
    stat.lastWaitMs = waitMs;
    if (waitMs > stat.maxWaitMs) {
        stat.maxWaitMs = waitMs;
    }
}

void PerformanceMonitor::observeRoundDuration(double roundMs) {
    if (!isEnabled()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    lastRoundMs_ = roundMs;
    roundSamples_.push_back(roundMs);
    if (static_cast<int>(roundSamples_.size()) > config_.maxSamples) {
        roundSamples_.pop_front();
    }
}

void PerformanceMonitor::setGauge(const std::string& name, double value) {
    if (!isEnabled()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    gauges_[name] = value;
}

nlohmann::json PerformanceMonitor::toJson() const {
    nlohmann::json snapshot;
    if (!isEnabled()) {
        snapshot["enabled"] = false;
        return snapshot;
    }

    auto nowSystem = std::chrono::system_clock::now();
    long long timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        nowSystem.time_since_epoch()).count();
    auto nowSteady = std::chrono::steady_clock::now();
    auto windowStart = nowSteady - std::chrono::seconds(std::max(1, config_.windowSeconds));

    std::lock_guard<std::mutex> lock(mutex_);
    snapshot["enabled"] = true;
    snapshot["timestamp_ms"] = timestampMs;
    snapshot["config"] = {
        {"window_seconds", config_.windowSeconds},
        {"sample_rate", config_.sampleRate},
        {"max_samples", config_.maxSamples}
    };

    std::size_t overallCount = static_cast<std::size_t>(std::count_if(
        qpsAll_.begin(), qpsAll_.end(),
        [&](const auto& ts) { return ts >= windowStart; }));
    double overallQps = static_cast<double>(overallCount) /
                        static_cast<double>(std::max(1, config_.windowSeconds));
    snapshot["qps"]["overall"] = overallQps;

    nlohmann::json qpsByEndpoint = nlohmann::json::object();
    for (const auto& kv : qpsByEndpoint_) {
        std::size_t count = static_cast<std::size_t>(std::count_if(
            kv.second.begin(), kv.second.end(),
            [&](const auto& ts) { return ts >= windowStart; }));
        double qps = static_cast<double>(count) /
                     static_cast<double>(std::max(1, config_.windowSeconds));
        qpsByEndpoint[kv.first] = qps;
    }
    snapshot["qps"]["per_endpoint"] = qpsByEndpoint;

    snapshot["requests_total"] = totalRequests_;
    nlohmann::json reqByEndpoint = nlohmann::json::object();
    for (const auto& kv : requestCounts_) {
        reqByEndpoint[kv.first] = kv.second;
    }
    snapshot["requests_total_per_endpoint"] = reqByEndpoint;

    snapshot["latency_ms"]["overall"] = {
        {"p95", percentile(latencyAll_, kPercentile95)},
        {"p99", percentile(latencyAll_, kPercentile99)},
        {"sample_count", latencyAll_.size()}
    };

    nlohmann::json latencyPerEndpoint = nlohmann::json::object();
    for (const auto& kv : latencyByEndpoint_) {
        latencyPerEndpoint[kv.first] = {
            {"p95", percentile(kv.second, kPercentile95)},
            {"p99", percentile(kv.second, kPercentile99)},
            {"sample_count", kv.second.size()}
        };
    }
    snapshot["latency_ms"]["per_endpoint"] = latencyPerEndpoint;

    snapshot["round_ms"] = {
        {"last", lastRoundMs_},
        {"p95", percentile(roundSamples_, kPercentile95)},
        {"p99", percentile(roundSamples_, kPercentile99)},
        {"sample_count", roundSamples_.size()}
    };

    nlohmann::json lockStats = nlohmann::json::object();
    for (const auto& kv : lockStats_) {
        const auto& stat = kv.second;
        double avg = (stat.count == 0) ? 0.0 : (stat.totalWaitMs / stat.count);
        lockStats[kv.first] = {
            {"count", stat.count},
            {"avg_ms", avg},
            {"max_ms", stat.maxWaitMs},
            {"last_ms", stat.lastWaitMs}
        };
    }
    snapshot["locks"] = lockStats;

    snapshot["gauges"] = gauges_;

    snapshot["memory"] = {
        {"rss_bytes", getRssBytes()}
    };

    return snapshot;
}

std::string PerformanceMonitor::toPrometheus() const {
    if (!isEnabled()) {
        return "";
    }

    nlohmann::json snapshot = toJson();
    std::ostringstream out;

    out << "# HELP snake_qps Overall QPS in the configured window\n";
    out << "# TYPE snake_qps gauge\n";
    out << "snake_qps " << snapshot["qps"]["overall"].get<double>() << "\n";

    out << "# HELP snake_requests_total Total HTTP requests\n";
    out << "# TYPE snake_requests_total counter\n";
    out << "snake_requests_total " << snapshot["requests_total"].get<std::uint64_t>() << "\n";

    out << "# HELP snake_endpoint_qps Endpoint QPS\n";
    out << "# TYPE snake_endpoint_qps gauge\n";
    for (auto& kv : snapshot["qps"]["per_endpoint"].items()) {
        out << "snake_endpoint_qps{endpoint=\"" << kv.key() << "\"} "
            << kv.value().get<double>() << "\n";
    }

    out << "# HELP snake_request_latency_ms Request latency percentiles\n";
    out << "# TYPE snake_request_latency_ms gauge\n";
    auto overallLatency = snapshot["latency_ms"]["overall"];
    out << "snake_request_latency_ms{quantile=\"0.95\",endpoint=\"all\"} "
        << overallLatency["p95"].get<double>() << "\n";
    out << "snake_request_latency_ms{quantile=\"0.99\",endpoint=\"all\"} "
        << overallLatency["p99"].get<double>() << "\n";

    for (auto& kv : snapshot["latency_ms"]["per_endpoint"].items()) {
        const auto& entry = kv.value();
        out << "snake_request_latency_ms{quantile=\"0.95\",endpoint=\"" << kv.key() << "\"} "
            << entry["p95"].get<double>() << "\n";
        out << "snake_request_latency_ms{quantile=\"0.99\",endpoint=\"" << kv.key() << "\"} "
            << entry["p99"].get<double>() << "\n";
    }

    out << "# HELP snake_round_duration_ms Round duration percentiles\n";
    out << "# TYPE snake_round_duration_ms gauge\n";
    auto roundMs = snapshot["round_ms"];
    out << "snake_round_duration_ms{quantile=\"last\"} " << roundMs["last"].get<double>() << "\n";
    out << "snake_round_duration_ms{quantile=\"0.95\"} " << roundMs["p95"].get<double>() << "\n";
    out << "snake_round_duration_ms{quantile=\"0.99\"} " << roundMs["p99"].get<double>() << "\n";

    out << "# HELP snake_lock_wait_ms Lock wait statistics\n";
    out << "# TYPE snake_lock_wait_ms gauge\n";
    for (auto& kv : snapshot["locks"].items()) {
        out << "snake_lock_wait_ms{lock=\"" << kv.key() << "\",stat=\"avg\"} "
            << kv.value()["avg_ms"].get<double>() << "\n";
        out << "snake_lock_wait_ms{lock=\"" << kv.key() << "\",stat=\"max\"} "
            << kv.value()["max_ms"].get<double>() << "\n";
        out << "snake_lock_wait_ms{lock=\"" << kv.key() << "\",stat=\"last\"} "
            << kv.value()["last_ms"].get<double>() << "\n";
    }

    out << "# HELP snake_gauge Generic gauges\n";
    out << "# TYPE snake_gauge gauge\n";
    for (auto& kv : snapshot["gauges"].items()) {
        out << "snake_gauge{name=\"" << kv.key() << "\"} "
            << kv.value().get<double>() << "\n";
    }

    out << "# HELP snake_memory_rss_bytes Resident memory size\n";
    out << "# TYPE snake_memory_rss_bytes gauge\n";
    out << "snake_memory_rss_bytes " << snapshot["memory"]["rss_bytes"].get<std::uint64_t>() << "\n";

    return out.str();
}

bool PerformanceMonitor::shouldSample() const {
    if (config_.sampleRate >= 1.0) {
        return true;
    }
    if (config_.sampleRate <= 0.0) {
        return false;
    }
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng) < config_.sampleRate;
}

void PerformanceMonitor::pruneDeque(std::deque<std::chrono::steady_clock::time_point>& deque,
                                   const std::chrono::steady_clock::time_point& now) const {
    auto windowStart = now - std::chrono::seconds(std::max(1, config_.windowSeconds));
    while (!deque.empty() && deque.front() < windowStart) {
        deque.pop_front();
    }
}

double PerformanceMonitor::percentile(const std::deque<double>& samples, double p) const {
    if (samples.empty()) {
        return 0.0;
    }
    std::vector<double> values(samples.begin(), samples.end());
    std::sort(values.begin(), values.end());
    double idx = std::ceil(p * (values.size() - 1));
    std::size_t pos = static_cast<std::size_t>(std::min<double>(idx, values.size() - 1));
    return values[pos];
}

std::uint64_t PerformanceMonitor::getRssBytes() const {
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) {
        return 0;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream iss(line);
            std::string key;
            std::uint64_t value = 0;
            std::string unit;
            iss >> key >> value >> unit;
            return value * 1024;
        }
    }

    return 0;
}

void PerformanceMonitor::logLoop() {
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.logIntervalSeconds));
        if (!running_.load(std::memory_order_acquire)) {
            break;
        }
        nlohmann::json snapshot = toJson();
        writeSnapshotToLog(snapshot);
    }
}

void PerformanceMonitor::writeSnapshotToLog(const nlohmann::json& snapshot) {
    if (!config_.logEnabled || config_.logPath.empty()) {
        return;
    }

    rotateLogsIfNeeded();

    std::ofstream out(config_.logPath, std::ios::app);
    if (!out.is_open()) {
        return;
    }

    out << snapshot.dump() << "\n";
}

void PerformanceMonitor::rotateLogsIfNeeded() {
    if (config_.logMaxBytes == 0 || config_.logPath.empty()) {
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(config_.logPath, ec)) {
        return;
    }

    std::uint64_t size = std::filesystem::file_size(config_.logPath, ec);
    if (ec || size < config_.logMaxBytes) {
        return;
    }

    int maxFiles = std::max(1, config_.logMaxFiles);
    for (int i = maxFiles - 1; i >= 1; --i) {
        std::filesystem::path src = config_.logPath + "." + std::to_string(i);
        std::filesystem::path dst = config_.logPath + "." + std::to_string(i + 1);
        if (std::filesystem::exists(src, ec)) {
            std::filesystem::rename(src, dst, ec);
        }
    }

    std::filesystem::path first = config_.logPath + ".1";
    std::filesystem::rename(config_.logPath, first, ec);
}

} // namespace snake

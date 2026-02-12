#pragma once

#include <string>
#include <cstddef>
#include <nlohmann/json.hpp>

namespace snake {

/**
 * @brief 服务器配置
 */
class Config {
public:
    struct ServerConfig {
        int port = 18080;
        int threads = 4;
    };

    struct GameConfig {
        int mapWidth = 50;
        int mapHeight = 50;
        int roundTimeMs = 1000;
        int initialSnakeLength = 3;
        int invincibleRounds = 5;
        double foodDensity = 0.05;
    };

    struct DatabaseConfig {
        std::string path = "./data/snake.db";
        int snapshotInterval = 10;          // 每N回合保存一次快照
        int snapshotRetentionHours = 24;    // 保留多少小时的快照
        bool backupEnabled = true;
        int backupIntervalHours = 6;
    };

    struct RateLimitConfig {
        bool enabled = true;
        int statusPerMinute = 60;
        int statusWindowSeconds = 60;
        int loginPerHour = 10;
        int loginWindowSeconds = 3600;
        int joinPerMinute = 5;
        int joinWindowSeconds = 60;
        int movePerRound = 1;
        int mapPerSecond = 10;
        int mapWindowSeconds = 1;
    };

    struct AuthConfig {
        std::string luoguValidationText = "SnakeGameVerification2026";
    };

    struct LeaderboardConfig {
        int refreshIntervalRounds = 5;
        int maxEntries = 200;
        int cacheTtlSeconds = 5;
    };

    struct PerformanceMonitorConfig {
        bool enabled = false;
        double sampleRate = 0.2;
        int windowSeconds = 60;
        int maxSamples = 2000;
        bool logEnabled = false;
        int logIntervalSeconds = 10;
        std::string logPath = "./data/metrics.log";
        std::size_t logMaxBytes = 5 * 1024 * 1024;
        int logMaxFiles = 3;
    };

    static Config& getInstance();

    bool loadFromFile(const std::string& filename);
    bool loadFromJson(const nlohmann::json& j);
    bool validate() const;  // 验证配置合法性

    const ServerConfig& getServer() const;
    const GameConfig& getGame() const;
    GameConfig& getGameMutable();
    const DatabaseConfig& getDatabase() const;
    const RateLimitConfig& getRateLimit() const;
    const AuthConfig& getAuth() const;
    const LeaderboardConfig& getLeaderboard() const;
    const PerformanceMonitorConfig& getPerformanceMonitor() const;

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    ServerConfig server_;
    GameConfig game_;
    DatabaseConfig database_;
    RateLimitConfig rateLimit_;
    AuthConfig auth_;
    LeaderboardConfig leaderboard_;
    PerformanceMonitorConfig performanceMonitor_;
};

} // namespace snake

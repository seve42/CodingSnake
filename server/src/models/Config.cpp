#include "models/Config.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace snake {

// 单例模式实现
Config& Config::getInstance() {
    static Config instance;
    return instance;
}

/**
 * @brief 从文件加载配置
 * @param filename 配置文件路径
 * @return 成功返回 true，失败返回 false
 */
bool Config::loadFromFile(const std::string& filename) {
    try {
        // 打开配置文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[Config] 无法打开配置文件: " << filename << std::endl;
            return false;
        }

        // 解析 JSON
        nlohmann::json j;
        file >> j;
        file.close();

        // 使用 loadFromJson 加载配置
        return loadFromJson(j);
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[Config] JSON 解析错误: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[Config] 加载配置失败: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 从 JSON 对象加载配置
 * @param j JSON 对象
 * @return 成功返回 true，失败返回 false
 */
bool Config::loadFromJson(const nlohmann::json& j) {
    try {
        // 加载服务器配置
        if (j.contains("server")) {
            const auto& server = j["server"];
            if (server.contains("port")) {
                server_.port = server["port"].get<int>();
            }
            if (server.contains("threads")) {
                server_.threads = server["threads"].get<int>();
            }
        }

        // 加载游戏配置
        if (j.contains("game")) {
            const auto& game = j["game"];
            if (game.contains("map_width")) {
                game_.mapWidth = game["map_width"].get<int>();
            }
            if (game.contains("map_height")) {
                game_.mapHeight = game["map_height"].get<int>();
            }
            if (game.contains("round_time_ms")) {
                game_.roundTimeMs = game["round_time_ms"].get<int>();
            }
            if (game.contains("initial_snake_length")) {
                game_.initialSnakeLength = game["initial_snake_length"].get<int>();
            }
            if (game.contains("invincible_rounds")) {
                game_.invincibleRounds = game["invincible_rounds"].get<int>();
            }
            if (game.contains("food_density")) {
                game_.foodDensity = game["food_density"].get<double>();
            }
        }

        // 加载数据库配置
        if (j.contains("database")) {
            const auto& db = j["database"];
            if (db.contains("path")) {
                database_.path = db["path"].get<std::string>();
            }
            if (db.contains("snapshot_interval")) {
                database_.snapshotInterval = db["snapshot_interval"].get<int>();
            }
            if (db.contains("snapshot_retention_hours")) {
                database_.snapshotRetentionHours = db["snapshot_retention_hours"].get<int>();
            }
            if (db.contains("backup_enabled")) {
                database_.backupEnabled = db["backup_enabled"].get<bool>();
            }
            if (db.contains("backup_interval_hours")) {
                database_.backupIntervalHours = db["backup_interval_hours"].get<int>();
            }
        }

        // 加载速率限制配置
        if (j.contains("rate_limits")) {
            const auto& rate = j["rate_limits"];
            if (rate.contains("enabled")) {
                rateLimit_.enabled = rate["enabled"].get<bool>();
            }
            if (rate.contains("status_per_minute")) {
                rateLimit_.statusPerMinute = rate["status_per_minute"].get<int>();
            }
            if (rate.contains("status_window_seconds")) {
                rateLimit_.statusWindowSeconds = rate["status_window_seconds"].get<int>();
            }
            if (rate.contains("login_per_hour")) {
                rateLimit_.loginPerHour = rate["login_per_hour"].get<int>();
            }
            if (rate.contains("login_window_seconds")) {
                rateLimit_.loginWindowSeconds = rate["login_window_seconds"].get<int>();
            }
            if (rate.contains("join_per_minute")) {
                rateLimit_.joinPerMinute = rate["join_per_minute"].get<int>();
            }
            if (rate.contains("join_window_seconds")) {
                rateLimit_.joinWindowSeconds = rate["join_window_seconds"].get<int>();
            }
            if (rate.contains("move_per_round")) {
                rateLimit_.movePerRound = rate["move_per_round"].get<int>();
            }
            if (rate.contains("map_per_second")) {
                rateLimit_.mapPerSecond = rate["map_per_second"].get<int>();
            }
            if (rate.contains("map_window_seconds")) {
                rateLimit_.mapWindowSeconds = rate["map_window_seconds"].get<int>();
            }
        }

        // 加载认证配置
        if (j.contains("auth")) {
            const auto& auth = j["auth"];
            if (auth.contains("luogu_validation_text")) {
                auth_.luoguValidationText = auth["luogu_validation_text"].get<std::string>();
            }
        }

        // 加载排行榜配置
        if (j.contains("leaderboard")) {
            const auto& leaderboard = j["leaderboard"];
            if (leaderboard.contains("refresh_interval_rounds")) {
                leaderboard_.refreshIntervalRounds = leaderboard["refresh_interval_rounds"].get<int>();
            }
            if (leaderboard.contains("max_entries")) {
                leaderboard_.maxEntries = leaderboard["max_entries"].get<int>();
            }
            if (leaderboard.contains("cache_ttl_seconds")) {
                leaderboard_.cacheTtlSeconds = leaderboard["cache_ttl_seconds"].get<int>();
            }
        }

        // 加载性能监控配置
        if (j.contains("performance_monitor")) {
            const auto& perf = j["performance_monitor"];
            if (perf.contains("enabled")) {
                performanceMonitor_.enabled = perf["enabled"].get<bool>();
            }
            if (perf.contains("sample_rate")) {
                performanceMonitor_.sampleRate = perf["sample_rate"].get<double>();
            }
            if (perf.contains("window_seconds")) {
                performanceMonitor_.windowSeconds = perf["window_seconds"].get<int>();
            }
            if (perf.contains("max_samples")) {
                performanceMonitor_.maxSamples = perf["max_samples"].get<int>();
            }
            if (perf.contains("log_enabled")) {
                performanceMonitor_.logEnabled = perf["log_enabled"].get<bool>();
            }
            if (perf.contains("log_interval_seconds")) {
                performanceMonitor_.logIntervalSeconds = perf["log_interval_seconds"].get<int>();
            }
            if (perf.contains("log_path")) {
                performanceMonitor_.logPath = perf["log_path"].get<std::string>();
            }
            if (perf.contains("log_max_bytes")) {
                performanceMonitor_.logMaxBytes = perf["log_max_bytes"].get<std::size_t>();
            }
            if (perf.contains("log_max_files")) {
                performanceMonitor_.logMaxFiles = perf["log_max_files"].get<int>();
            }
        }

        // 配置验证
        if (!validate()) {
            std::cerr << "[Config] 配置验证失败" << std::endl;
            return false;
        }

        std::cout << "[Config] 配置加载成功" << std::endl;
        return true;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[Config] JSON 解析错误: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[Config] 加载配置失败: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 验证配置的合法性
 * @return 配置合法返回 true，否则返回 false
 */
bool Config::validate() const {
    // 验证服务器配置
    if (server_.port < 1024 || server_.port > 65535) {
        std::cerr << "[Config] 端口号无效: " << server_.port << " (应在 1024-65535 之间)" << std::endl;
        return false;
    }
    if (server_.threads < 1 || server_.threads > 128) {
        std::cerr << "[Config] 线程数无效: " << server_.threads << " (应在 1-128 之间)" << std::endl;
        return false;
    }

    // 验证游戏配置
    if (game_.mapWidth < 10 || game_.mapWidth > 200000) {
        std::cerr << "[Config] 地图宽度无效: " << game_.mapWidth << " (应在 10-200000 之间)" << std::endl;
        return false;
    }
    if (game_.mapHeight < 10 || game_.mapHeight > 200000) {
        std::cerr << "[Config] 地图高度无效: " << game_.mapHeight << " (应在 10-200000 之间)" << std::endl;
        return false;
    }
    if (game_.roundTimeMs < 100 || game_.roundTimeMs > 100000000) {
        std::cerr << "[Config] 回合时间无效: " << game_.roundTimeMs << " (应在 100-100000000 之间)" << std::endl;
        return false;
    }
    if (game_.initialSnakeLength < 1 || game_.initialSnakeLength > 10) {
        std::cerr << "[Config] 初始蛇长度无效: " << game_.initialSnakeLength << " (应在 1-10 之间)" << std::endl;
        return false;
    }
    if (game_.invincibleRounds < 0 || game_.invincibleRounds > 100) {
        std::cerr << "[Config] 无敌回合数无效: " << game_.invincibleRounds << " (应在 0-100 之间)" << std::endl;
        return false;
    }
    if (game_.foodDensity < 0.0 || game_.foodDensity > 1.0) {
        std::cerr << "[Config] 食物密度无效: " << game_.foodDensity << " (应在 0.0-1.0 之间)" << std::endl;
        return false;
    }

    // 验证数据库配置
    if (database_.path.empty()) {
        std::cerr << "[Config] 数据库路径不能为空" << std::endl;
        return false;
    }
    if (database_.snapshotInterval < 1 || database_.snapshotInterval > 1000) {
        std::cerr << "[Config] 快照间隔无效: " << database_.snapshotInterval << " (应在 1-1000 之间)" << std::endl;
        return false;
    }
    if (database_.snapshotRetentionHours < 1 || database_.snapshotRetentionHours > 720) {
        std::cerr << "[Config] 快照保留时间无效: " << database_.snapshotRetentionHours << " (应在 1-720 之间)" << std::endl;
        return false;
    }
    if (database_.backupIntervalHours < 1 || database_.backupIntervalHours > 168) {
        std::cerr << "[Config] 备份间隔无效: " << database_.backupIntervalHours << " (应在 1-168 之间)" << std::endl;
        return false;
    }

    // 验证排行榜配置
    if (leaderboard_.refreshIntervalRounds < 1 || leaderboard_.refreshIntervalRounds > 10000) {
        std::cerr << "[Config] 排行榜刷新回合间隔无效: " << leaderboard_.refreshIntervalRounds
                  << " (应在 1-10000 之间)" << std::endl;
        return false;
    }
    if (leaderboard_.maxEntries < 1 || leaderboard_.maxEntries > 10000) {
        std::cerr << "[Config] 排行榜最大条目无效: " << leaderboard_.maxEntries
                  << " (应在 1-10000 之间)" << std::endl;
        return false;
    }
    if (leaderboard_.cacheTtlSeconds < 0 || leaderboard_.cacheTtlSeconds > 3600) {
        std::cerr << "[Config] 排行榜缓存TTL无效: " << leaderboard_.cacheTtlSeconds
                  << " (应在 0-3600 之间)" << std::endl;
        return false;
    }

    // 验证性能监控配置
    if (performanceMonitor_.sampleRate < 0.0 || performanceMonitor_.sampleRate > 1.0) {
        std::cerr << "[Config] 采样率无效: " << performanceMonitor_.sampleRate
                  << " (应在 0.0-1.0 之间)" << std::endl;
        return false;
    }
    if (performanceMonitor_.windowSeconds < 1 || performanceMonitor_.windowSeconds > 3600) {
        std::cerr << "[Config] 性能监控窗口无效: " << performanceMonitor_.windowSeconds
                  << " (应在 1-3600 之间)" << std::endl;
        return false;
    }
    if (performanceMonitor_.maxSamples < 10 || performanceMonitor_.maxSamples > 200000) {
        std::cerr << "[Config] 性能监控样本上限无效: " << performanceMonitor_.maxSamples
                  << " (应在 10-200000 之间)" << std::endl;
        return false;
    }
    if (performanceMonitor_.logEnabled) {
        if (performanceMonitor_.logIntervalSeconds < 1 || performanceMonitor_.logIntervalSeconds > 3600) {
            std::cerr << "[Config] 监控日志间隔无效: " << performanceMonitor_.logIntervalSeconds
                      << " (应在 1-3600 之间)" << std::endl;
            return false;
        }
    }
    if (performanceMonitor_.logMaxBytes > 0 && performanceMonitor_.logMaxBytes < 1024) {
        std::cerr << "[Config] 监控日志最大大小无效: " << performanceMonitor_.logMaxBytes
                  << " (应至少 1024 字节)" << std::endl;
        return false;
    }
    if (performanceMonitor_.logMaxFiles < 1 || performanceMonitor_.logMaxFiles > 50) {
        std::cerr << "[Config] 监控日志滚动数量无效: " << performanceMonitor_.logMaxFiles
                  << " (应在 1-50 之间)" << std::endl;
        return false;
    }

    // 验证速率限制配置（允许通过 enabled 关闭限制）
    if (rateLimit_.enabled) {
        if (rateLimit_.statusPerMinute < 1 || rateLimit_.statusPerMinute > 10000) {
            std::cerr << "[Config] 状态查询速率限制无效: " << rateLimit_.statusPerMinute << " (应在 1-10000 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.statusWindowSeconds < 1 || rateLimit_.statusWindowSeconds > 3600) {
            std::cerr << "[Config] 状态查询窗口无效: " << rateLimit_.statusWindowSeconds << " (应在 1-3600 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.loginPerHour < 1 || rateLimit_.loginPerHour > 10000) {
            std::cerr << "[Config] 登录速率限制无效: " << rateLimit_.loginPerHour << " (应在 1-10000 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.loginWindowSeconds < 1 || rateLimit_.loginWindowSeconds > 86400) {
            std::cerr << "[Config] 登录窗口无效: " << rateLimit_.loginWindowSeconds << " (应在 1-86400 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.joinPerMinute < 1 || rateLimit_.joinPerMinute > 10000) {
            std::cerr << "[Config] 加入速率限制无效: " << rateLimit_.joinPerMinute << " (应在 1-10000 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.joinWindowSeconds < 1 || rateLimit_.joinWindowSeconds > 3600) {
            std::cerr << "[Config] 加入窗口无效: " << rateLimit_.joinWindowSeconds << " (应在 1-3600 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.movePerRound < 1 || rateLimit_.movePerRound > 100) {
            std::cerr << "[Config] 移动速率限制无效: " << rateLimit_.movePerRound << " (应在 1-100 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.mapPerSecond < 1 || rateLimit_.mapPerSecond > 10000) {
            std::cerr << "[Config] 地图查询速率限制无效: " << rateLimit_.mapPerSecond << " (应在 1-10000 之间)" << std::endl;
            return false;
        }
        if (rateLimit_.mapWindowSeconds < 1 || rateLimit_.mapWindowSeconds > 60) {
            std::cerr << "[Config] 地图查询窗口无效: " << rateLimit_.mapWindowSeconds << " (应在 1-60 之间)" << std::endl;
            return false;
        }
    }

    return true;
}

const Config::ServerConfig& Config::getServer() const {
    return server_;
}

const Config::GameConfig& Config::getGame() const {
    return game_;
}

Config::GameConfig& Config::getGameMutable() {
    return game_;
}

const Config::DatabaseConfig& Config::getDatabase() const {
    return database_;
}

const Config::RateLimitConfig& Config::getRateLimit() const {
    return rateLimit_;
}

const Config::AuthConfig& Config::getAuth() const {
    return auth_;
}

const Config::LeaderboardConfig& Config::getLeaderboard() const {
    return leaderboard_;
}

const Config::PerformanceMonitorConfig& Config::getPerformanceMonitor() const {
    return performanceMonitor_;
}

} // namespace snake

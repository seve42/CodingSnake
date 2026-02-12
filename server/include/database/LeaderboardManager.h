#pragma once

#include "DatabaseManager.h"
#include <string>
#include <vector>
#include <memory>

namespace snake {

/**
 * @brief 排行榜类型
 */
enum class LeaderboardType {
    KILLS,
    MAX_LENGTH
};

/**
 * @brief 排行榜刷新策略
 */
struct LeaderboardPolicy {
    int refreshIntervalRounds;
    std::string seasonId;
    long long seasonStart;
    long long seasonEnd;

    LeaderboardPolicy()
        : refreshIntervalRounds(1)
        , seasonId("all_time")
        , seasonStart(0)
        , seasonEnd(0) {}
};

/**
 * @brief 排行榜条目
 */
struct LeaderboardEntry {
    std::string uid;
    std::string playerName;
    std::string seasonId;
    int nowLength;
    int maxLength;
    int kills;
    int deaths;
    int gamesPlayed;
    int totalFood;
    int lastRound;
    long long timestamp;
    int rank;  // 排名（查询时填充）

    LeaderboardEntry()
        : nowLength(0)
        , maxLength(0)
        , kills(0)
        , deaths(0)
        , gamesPlayed(0)
        , totalFood(0)
        , lastRound(0)
        , timestamp(0)
        , rank(0) {}
};

/**
 * @brief 玩家统计数据
 */
struct PlayerStats {
    int maxLength;
    int kills;      // 击杀数（导致其他玩家死亡）
    int deaths;     // 死亡次数
    int totalFood;  // 总共吃的食物数

    PlayerStats()
    : maxLength(0), kills(0), deaths(0), totalFood(0) {}
};

/**
 * @brief 排行榜管理器
 * 负责玩家排行榜的更新和查询
 */
class LeaderboardManager {
public:
    explicit LeaderboardManager(std::shared_ptr<DatabaseManager> dbManager);
    ~LeaderboardManager();

    // 回合/死亡/结束时更新
    bool updateOnRound(const std::string& uid,
                       const std::string& playerName,
                       int round,
                       int currentLength,
                       int foodDelta,
                       int killsDelta);
    bool updateOnDeath(const std::string& uid,
                       const std::string& playerName,
                       int round,
                       int finalLength);
    bool updateOnGameEnd(const std::string& uid,
                         const std::string& playerName,
                         int round,
                         const PlayerStats& gameDelta);
    
    // 增量更新
    bool incrementGamesPlayed(const std::string& uid);
    bool incrementKills(const std::string& uid);
    bool incrementDeaths(const std::string& uid);
    bool addFood(const std::string& uid, int count);

    // 查询排行榜
    std::vector<LeaderboardEntry> getTopPlayersByKills(int limit = 100,
                                                        int offset = 0);
    std::vector<LeaderboardEntry> getTopPlayersByMaxLength(int limit = 100,
                                                            int offset = 0);
    std::vector<LeaderboardEntry> getTopPlayers(LeaderboardType type,
                                                int limit,
                                                int offset,
                                                long long startTimestamp,
                                                long long endTimestamp);

    // 查询个人排名
    LeaderboardEntry getPlayerRank(const std::string& uid);
    // 获取玩家统计
    PlayerStats getPlayerStats(const std::string& uid);

    // 管理操作
    bool resetLeaderboard();
    bool deletePlayerStats(const std::string& uid);

private:
    bool ensurePlayerExists(const std::string& uid,
                            const std::string& playerName);
    bool applyDelta(const std::string& uid,
                    const std::string& playerName,
                    int round,
                    const PlayerStats& delta,
                    int lengthCandidate,
                    int gamesDelta,
                    int deathsDelta);
    long long currentTimestampMs() const;
    
    std::shared_ptr<DatabaseManager> dbManager_;
    LeaderboardPolicy policy_;
};

} // namespace snake

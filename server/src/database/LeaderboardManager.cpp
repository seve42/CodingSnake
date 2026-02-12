#include "../include/database/LeaderboardManager.h"
#include "../include/utils/Logger.h"
#include <algorithm>
#include <chrono>

namespace snake {

LeaderboardManager::LeaderboardManager(std::shared_ptr<DatabaseManager> dbManager)
    : dbManager_(dbManager) {
    LOG_INFO("LeaderboardManager initialized");
}

LeaderboardManager::~LeaderboardManager() {
    LOG_INFO("LeaderboardManager destroyed");
}

bool LeaderboardManager::updateOnRound(const std::string& uid,
                                       const std::string& playerName,
                                       int round,
                                       int currentLength,
                                       int foodDelta,
                                       int killsDelta) {
    PlayerStats delta;
    delta.totalFood = std::max(0, foodDelta);
    delta.kills = std::max(0, killsDelta);
    return applyDelta(uid, playerName, round, delta, currentLength, 0, 0);
}

bool LeaderboardManager::updateOnDeath(const std::string& uid,
                                       const std::string& playerName,
                                       int round,
                                       int finalLength) {
    PlayerStats delta;
    return applyDelta(uid, playerName, round, delta, finalLength, 0, 1);
}

bool LeaderboardManager::updateOnGameEnd(const std::string& uid,
                                         const std::string& playerName,
                                         int round,
                                         const PlayerStats& gameDelta) {
    return applyDelta(uid, playerName, round, gameDelta, gameDelta.maxLength, 1, 0);
}

bool LeaderboardManager::incrementGamesPlayed(const std::string& uid) {
    if (!ensurePlayerExists(uid, "")) {
        return false;
    }

    const std::string sql =
        "UPDATE leaderboard "
        "SET games_played = games_played + 1, timestamp = ? "
        "WHERE uid = ? AND season_id = ?";

    return dbManager_->executeWithParams(sql, {
        std::to_string(currentTimestampMs()),
        uid,
        policy_.seasonId
    });
}

bool LeaderboardManager::incrementKills(const std::string& uid) {
    if (!ensurePlayerExists(uid, "")) {
        return false;
    }

    const std::string sql =
        "UPDATE leaderboard "
        "SET kills = kills + 1, timestamp = ? "
        "WHERE uid = ? AND season_id = ?";

    return dbManager_->executeWithParams(sql, {
        std::to_string(currentTimestampMs()),
        uid,
        policy_.seasonId
    });
}

bool LeaderboardManager::incrementDeaths(const std::string& uid) {
    if (!ensurePlayerExists(uid, "")) {
        return false;
    }

    const std::string sql =
        "UPDATE leaderboard "
        "SET deaths = deaths + 1, timestamp = ? "
        "WHERE uid = ? AND season_id = ?";

    return dbManager_->executeWithParams(sql, {
        std::to_string(currentTimestampMs()),
        uid,
        policy_.seasonId
    });
}

bool LeaderboardManager::addFood(const std::string& uid, int count) {
    if (count <= 0) {
        return true;
    }

    if (!ensurePlayerExists(uid, "")) {
        return false;
    }

    const std::string sql =
        "UPDATE leaderboard "
        "SET total_food = total_food + ?, timestamp = ? "
        "WHERE uid = ? AND season_id = ?";

    return dbManager_->executeWithParams(sql, {
        std::to_string(count),
        std::to_string(currentTimestampMs()),
        uid,
        policy_.seasonId
    });
}

std::vector<LeaderboardEntry> LeaderboardManager::getTopPlayersByKills(int limit, int offset) {
    return getTopPlayers(LeaderboardType::KILLS, limit, offset, 0, 0);
}

std::vector<LeaderboardEntry> LeaderboardManager::getTopPlayersByMaxLength(int limit, int offset) {
    return getTopPlayers(LeaderboardType::MAX_LENGTH, limit, offset, 0, 0);
}

std::vector<LeaderboardEntry> LeaderboardManager::getTopPlayers(LeaderboardType type,
                                                                int limit,
                                                                int offset,
                                                                long long startTimestamp,
                                                                long long endTimestamp) {
    std::vector<LeaderboardEntry> results;
    if (limit <= 0) {
        return results;
    }
    if (offset < 0) {
        offset = 0;
    }

    std::string orderColumn = "kills";
    switch (type) {
        case LeaderboardType::KILLS:
            orderColumn = "kills";
            break;
        case LeaderboardType::MAX_LENGTH:
            orderColumn = "max_length";
            break;
        default:
            orderColumn = "kills";
            break;
    }

    std::string sql =
        "SELECT uid, player_name, season_id, now_length, max_length, kills, deaths, "
        "games_played, total_food, last_round, timestamp "
        "FROM leaderboard WHERE season_id = ?";

    std::vector<std::string> params;
    params.push_back(policy_.seasonId);

    if (startTimestamp > 0) {
        sql += " AND timestamp >= ?";
        params.push_back(std::to_string(startTimestamp));
    }
    if (endTimestamp > 0) {
        sql += " AND timestamp <= ?";
        params.push_back(std::to_string(endTimestamp));
    }

    sql += " ORDER BY " + orderColumn + " DESC, timestamp ASC LIMIT ? OFFSET ?";
    params.push_back(std::to_string(limit));
    params.push_back(std::to_string(offset));

    auto rs = dbManager_->queryWithParams(sql, params);

    int rank = offset + 1;
    while (rs.next()) {
        LeaderboardEntry entry;
        entry.uid = rs.getString(0);
        entry.playerName = rs.getString(1);
        entry.seasonId = rs.getString(2);
        entry.nowLength = rs.getInt(3);
        entry.maxLength = rs.getInt(4);
        entry.kills = rs.getInt(5);
        entry.deaths = rs.getInt(6);
        entry.gamesPlayed = rs.getInt(7);
        entry.totalFood = rs.getInt(8);
        entry.lastRound = rs.getInt(9);
        entry.timestamp = rs.getInt64(10);
        entry.rank = rank++;
        results.push_back(entry);
    }

    return results;
}

LeaderboardEntry LeaderboardManager::getPlayerRank(const std::string& uid) {
    LeaderboardEntry entry;
    const std::string sql =
        "SELECT uid, player_name, season_id, now_length, max_length, kills, deaths, "
        "games_played, total_food, last_round, timestamp "
        "FROM leaderboard WHERE uid = ? AND season_id = ?";

    auto rs = dbManager_->queryWithParams(sql, {uid, policy_.seasonId});
    if (!rs.next()) {
        return entry;
    }

    entry.uid = rs.getString(0);
    entry.playerName = rs.getString(1);
    entry.seasonId = rs.getString(2);
    entry.nowLength = rs.getInt(3);
    entry.maxLength = rs.getInt(4);
    entry.kills = rs.getInt(5);
    entry.deaths = rs.getInt(6);
    entry.gamesPlayed = rs.getInt(7);
    entry.totalFood = rs.getInt(8);
    entry.lastRound = rs.getInt(9);
    entry.timestamp = rs.getInt64(10);

    entry.rank = 0;
    return entry;
}

PlayerStats LeaderboardManager::getPlayerStats(const std::string& uid) {
    PlayerStats stats;
    const std::string sql =
        "SELECT max_length, kills, deaths, total_food "
        "FROM leaderboard WHERE uid = ? AND season_id = ?";

    auto rs = dbManager_->queryWithParams(sql, {uid, policy_.seasonId});
    if (rs.next()) {
        stats.maxLength = rs.getInt(0);
        stats.kills = rs.getInt(1);
        stats.deaths = rs.getInt(2);
        stats.totalFood = rs.getInt(3);
    }

    return stats;
}

bool LeaderboardManager::resetLeaderboard() {
    const std::string sql = "DELETE FROM leaderboard WHERE season_id = ?";
    return dbManager_->executeWithParams(sql, {policy_.seasonId});
}

bool LeaderboardManager::deletePlayerStats(const std::string& uid) {
    const std::string sql = "DELETE FROM leaderboard WHERE uid = ? AND season_id = ?";
    return dbManager_->executeWithParams(sql, {uid, policy_.seasonId});
}

bool LeaderboardManager::ensurePlayerExists(const std::string& uid,
                                            const std::string& playerName) {
    const std::string checkSql =
        "SELECT 1 FROM leaderboard WHERE uid = ? AND season_id = ?";
    auto rs = dbManager_->queryWithParams(checkSql, {uid, policy_.seasonId});
    if (rs.next()) {
        if (!playerName.empty()) {
            const std::string updateSql =
                "UPDATE leaderboard SET player_name = ?, timestamp = ? "
                "WHERE uid = ? AND season_id = ?";
            dbManager_->executeWithParams(updateSql, {
                playerName,
                std::to_string(currentTimestampMs()),
                uid,
                policy_.seasonId
            });
        }
        return true;
    }

    const long long now = currentTimestampMs();
    const std::string insertSql =
        "INSERT INTO leaderboard "
        "(uid, player_name, season_id, now_length, max_length, kills, deaths, "
        "games_played, total_food, last_round, timestamp, season_start, season_end) "
        "VALUES (?, ?, ?, 0, 0, 0, 0, 0, 0, 0, ?, ?, ?)";

    return dbManager_->executeWithParams(insertSql, {
        uid,
        playerName.empty() ? uid : playerName,
        policy_.seasonId,
        std::to_string(now),
        std::to_string(policy_.seasonStart),
        std::to_string(policy_.seasonEnd)
    });
}

bool LeaderboardManager::applyDelta(const std::string& uid,
                                    const std::string& playerName,
                                    int round,
                                    const PlayerStats& delta,
                                    int lengthCandidate,
                                    int gamesDelta,
                                    int deathsDelta) {
    if (!ensurePlayerExists(uid, playerName)) {
        return false;
    }

    PlayerStats current = getPlayerStats(uid);
    current.kills += std::max(0, delta.kills);
    current.deaths += std::max(0, deathsDelta + delta.deaths);
    current.totalFood += std::max(0, delta.totalFood);
    current.maxLength = std::max(current.maxLength, lengthCandidate);

    const std::string sql =
        "UPDATE leaderboard "
        "SET player_name = ?, now_length = ?, max_length = ?, kills = ?, deaths = ?, "
        "games_played = games_played + ?, total_food = ?, last_round = ?, "
        "timestamp = ? "
        "WHERE uid = ? AND season_id = ?";

    return dbManager_->executeWithParams(sql, {
        playerName.empty() ? uid : playerName,
        std::to_string(lengthCandidate),
        std::to_string(current.maxLength),
        std::to_string(current.kills),
        std::to_string(current.deaths),
        std::to_string(std::max(0, gamesDelta)),
        std::to_string(current.totalFood),
        std::to_string(round),
        std::to_string(currentTimestampMs()),
        uid,
        policy_.seasonId
    });
}

long long LeaderboardManager::currentTimestampMs() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

} // namespace snake

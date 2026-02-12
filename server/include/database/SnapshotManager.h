#pragma once

#include "DatabaseManager.h"
#include "../models/GameState.h"
#include <string>
#include <vector>
#include <memory>

namespace snake {

/**
 * @brief 快照信息
 */
struct SnapshotInfo {
    int id;
    int round;
    long long timestamp;
    long long createdAt;
    size_t size;  // JSON 字符串大小

    SnapshotInfo()
        : id(0), round(0), timestamp(0), createdAt(0), size(0) {}
};

/**
 * @brief 游戏快照管理器
 * 负责游戏状态快照的保存、加载和管理
 */
class SnapshotManager {
public:
    explicit SnapshotManager(std::shared_ptr<DatabaseManager> dbManager);
    ~SnapshotManager();

    // 快照保存
    bool saveSnapshot(int round, const GameState& gameState);
    bool saveSnapshotJson(int round, const std::string& jsonState);

    // 快照加载
    bool loadSnapshot(int round, GameState& gameState);
    std::string loadSnapshotJson(int round);

    // 批量查询
    std::vector<SnapshotInfo> getSnapshotList(int startRound, 
                                               int endRound,
                                               int limit = 100);
    std::vector<SnapshotInfo> getRecentSnapshots(int count = 10);

    // 查询快照信息
    bool hasSnapshot(int round);
    SnapshotInfo getSnapshotInfo(int round);
    int getLatestSnapshotRound();
    int getOldestSnapshotRound();

    // 快照清理
    bool cleanOldSnapshots(int keepHours);
    bool cleanSnapshotsBefore(long long timestamp);
    bool deleteSnapshot(int round);
    bool deleteSnapshotsRange(int startRound, int endRound);

    // 统计信息
    int getSnapshotCount();
    long long getTotalSnapshotSize();

    // 回放支持
    std::vector<std::string> getReplayData(int startRound, int endRound);

private:
    std::shared_ptr<DatabaseManager> dbManager_;
};

} // namespace snake

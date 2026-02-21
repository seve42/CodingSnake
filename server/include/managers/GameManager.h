#pragma once

#include "../models/GameState.h"
#include "../models/Direction.h"
#include "../models/Snake.h"
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace snake {

class MapManager;
class PlayerManager;
class LeaderboardManager;

/**
 * @brief 游戏管理器
 * 负责游戏状态管理、回合推进、移动指令处理
 */
class GameManager {
public:
    GameManager(std::shared_ptr<MapManager> mapManager, 
                std::shared_ptr<PlayerManager> playerManager,
                std::shared_ptr<LeaderboardManager> leaderboardManager);
    ~GameManager();

    // 游戏控制
    void start();
    void stop();
    bool isRunning() const;

    // 回合推进（由定时器线程调用）
    void tick();

    // 移动指令
    bool submitMove(const std::string& playerId, Direction direction);

    // 状态查询
    GameState getGameState() const;
    int getCurrentRound() const;
    nlohmann::json getDeltaState() const;

    // 玩家管理
    bool addPlayer(std::shared_ptr<Player> player);
    void removePlayer(const std::string& playerId);
    void respawnPlayer(const std::string& playerId);

private:
    void gameLoop();
    void processMovements();
    void checkCollisions();
    void handleFoodCollection();
    void generateFood();
    void updateInvincibility();
    void addSnakeToOccupancy(const Snake& snake);
    void removeSnakeFromOccupancy(const Snake& snake);
    void createSnakeDeathDrops(std::vector<Point> pos);

    std::shared_ptr<MapManager> mapManager_;
    std::shared_ptr<PlayerManager> playerManager_;
    std::shared_ptr<LeaderboardManager> leaderboardManager_;
    
    GameState gameState_;
    mutable std::mutex stateMutex_;
    
    // 双缓冲移动指令：当前回合收到的指令存入 currentMoves_，下回合执行时从 nextMoves_ 读取
    std::map<std::string, Direction> currentMoves_;  // 本回合收到的移动指令（下回合执行）
    std::map<std::string, Direction> nextMoves_;     // 下回合要执行的移动指令（上回合收到的）
    std::mutex movesMutex_;

    // 预判自撞：在移动前计算，移动后用于判定
    std::unordered_set<std::string> pendingSelfCollisions_;

    // 空间索引：蛇身占用计数（用于 O(1) 碰撞判断）
    std::unordered_map<Point, int, PointHash> occupiedCounts_;
    
    // 游戏循环线程
    std::thread gameThread_;
    std::atomic<bool> running_;
};

} // namespace snake

#pragma once

#include "Player.h"
#include "Food.h"
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace snake {

/**
 * @brief 游戏状态
 */
class GameState {
public:
    GameState();

    // 回合管理
    int getCurrentRound() const;
    void setCurrentRound(int round);
    void incrementRound();
    void reset();

    // 玩家管理
    void addPlayer(std::shared_ptr<Player> player);
    void removePlayer(const std::string& playerId);
    std::shared_ptr<Player> getPlayer(const std::string& playerId);
    std::shared_ptr<Player> getPlayer(const std::string& playerId) const;
    const std::vector<std::shared_ptr<Player>>& getPlayers() const;

    // 食物管理
    void addFood(const Food& food);
    void removeFood(const Point& position);
    void clearFood();
    const std::vector<Food>& getFoods() const;
    bool hasFoodAt(const Point& position) const;
    const std::unordered_set<Point, PointHash>& getFoodSet() const;

    // 时间戳
    long long getTimestamp() const;
    void setTimestamp(long long timestamp);
    void updateTimestamp();

    // 下一回合时间戳
    long long getNextRoundTimestamp() const;
    void setNextRoundTimestamp(long long nextRoundTimestamp);

    // 序列化
    nlohmann::json toJson() const;
    // 高性能版本，直接填充传入的 JSON 对象，避免拷贝
    void toJsonOptimized(nlohmann::json& j) const;
    // 增量序列化，只返回变化的数据
    nlohmann::json toDeltaJson() const;

    // 增量变化追踪
    void trackPlayerJoined(const std::string& playerId);
    void trackPlayerDied(const std::string& playerId);
    void trackFoodAdded(const Point& position);
    void trackFoodRemoved(const Point& position);
    void clearDeltaTracking();

private:
    int currentRound_;
    std::vector<std::shared_ptr<Player>> players_;
    std::vector<Food> foods_;
    std::unordered_set<Point, PointHash> foodSet_;  // 快速查询食物位置
    std::unordered_map<Point, std::size_t, PointHash> foodIndex_;  // 位置 -> foods_ 下标
    long long timestamp_;
    long long nextRoundTimestamp_;  // 下一回合的时间戳
    
    // 增量变化追踪
    std::vector<std::string> joinedPlayers_;  // 本回合加入的玩家ID
    std::vector<std::string> diedPlayers_;    // 本回合死亡的玩家ID
    std::vector<Point> addedFoods_;           // 本回合新增的食物
    std::vector<Point> removedFoods_;         // 本回合移除的食物
};

} // namespace snake

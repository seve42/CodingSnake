#pragma once

#include "../models/Point.h"
#include "../models/Food.h"
#include "../models/Player.h"
#include <vector>
#include <random>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace snake {

/**
 * @brief 地图管理器
 * 负责地图、食物管理和碰撞检测
 */
class MapManager {
public:
    MapManager(int width, int height);
    ~MapManager();

    // 地图信息
    int getWidth() const;
    int getHeight() const;

    // 坐标验证
    bool isValidPosition(const Point& pos) const;
    bool isOutOfBounds(const Point& pos) const;

    // 安全位置生成
    Point getRandomSafePosition(const std::vector<std::shared_ptr<Player>>& players, int safeRadius);
    
    // 碰撞检测
    enum class CollisionType {
        NONE,
        WALL,
        SELF,
        OTHER_SNAKE
    };
    
    CollisionType checkCollision(const Player& player, const Point& newPos,
                                  const std::vector<std::shared_ptr<Player>>& allPlayers) const;

    // 食物管理
    std::vector<Food> generateFood(int count, 
                                    const std::vector<std::shared_ptr<Player>>& players);
    std::vector<Food> generateFoodFast(int count,
                                       const std::unordered_map<Point, int, PointHash>& occupiedCounts,
                                       const std::unordered_set<Point, PointHash>& existingFoods);
    std::vector<Food> generateFoodByDensity(double density,
                                            const std::vector<std::shared_ptr<Player>>& players);
    bool isFoodAt(const Point& pos, const std::vector<Food>& foods) const;

private:
    bool isPositionOccupied(const Point& pos, 
                           const std::vector<std::shared_ptr<Player>>& players) const;
    bool isSafeArea(const Point& center, int radius,
                    const std::vector<std::shared_ptr<Player>>& players) const;

    int width_;
    int height_;
    std::mt19937 rng_;
};

} // namespace snake

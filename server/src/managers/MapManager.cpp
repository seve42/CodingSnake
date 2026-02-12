#include "../include/managers/MapManager.h"
#include "../include/utils/Logger.h"

namespace snake {

MapManager::MapManager(int width, int height)
    : width_(width)
    , height_(height)
    , rng_(std::random_device{}()) {
    LOG_INFO("MapManager initialized: " + std::to_string(width) + "x" + std::to_string(height));
}

MapManager::~MapManager() {
    LOG_INFO("MapManager destroyed");
}

int MapManager::getWidth() const {
    return width_;
}

int MapManager::getHeight() const {
    return height_;
}

bool MapManager::isValidPosition(const Point& pos) const {
    return pos.x >= 0 && pos.x < width_ && pos.y >= 0 && pos.y < height_;
}

bool MapManager::isOutOfBounds(const Point& pos) const {
    return !isValidPosition(pos);
}

Point MapManager::getRandomSafePosition(const std::vector<std::shared_ptr<Player>>& players, int safeRadius) {
    const int clampedRadius = std::max(0, safeRadius);
    
    if (width_ <= 0 || height_ <= 0) {
        LOG_WARNING("Invalid map dimensions");
        return Point::Null();
    }

    // 自适应尝试次数：小地图更多尝试，大地图限制尝试
    const int totalCells = width_ * height_;
    const int maxAttempts = std::min(totalCells, std::max(100, totalCells / 10));

    // 计算有效的采样范围，避免分布范围无效
    int minX = std::max(0, clampedRadius);
    int maxX = std::max(0, width_ - 1 - clampedRadius);
    int minY = std::max(0, clampedRadius);
    int maxY = std::max(0, height_ - 1 - clampedRadius);
    
    // 如果采样范围无效，尝试使用整个地图
    if (minX > maxX || minY > maxY) {
        minX = 0;
        maxX = width_ - 1;
        minY = 0;
        maxY = height_ - 1;
    }
    
    // 如果地图太小，返回空点
    if (minX > maxX || minY > maxY) {
        LOG_WARNING("Map too small for safe position");
        return Point::Null();
    }

    std::uniform_int_distribution<int> distX(minX, maxX);
    std::uniform_int_distribution<int> distY(minY, maxY);

    // 快速随机采样
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        Point candidate{distX(rng_), distY(rng_)};
        
        if (isValidPosition(candidate) && isSafeArea(candidate, clampedRadius, players)) {
            return candidate;
        }
    }

    LOG_WARNING("Failed to find safe position after " + std::to_string(maxAttempts) + " attempts");
    return Point::Null();
}

/**
 * @brief 检测玩家在新位置是否发生碰撞
 * @param player 当前玩家
 * @param newPos 新位置（蛇头将要移动到的位置）
 * @param allPlayers 所有玩家列表
 * @return 碰撞类型枚举
 * 
 * 碰撞检测优先级：
 * 1. 墙壁碰撞（地图边界）
 * 2. 自身碰撞（撞到自己的身体）
 * 3. 其他蛇碰撞（撞到其他玩家的蛇）
 * 
 * 无敌状态处理：
 * - 有无敌回合时，不会因为碰撞而死亡
 * - 但仍然返回碰撞类型，供上层逻辑判断
 */
MapManager::CollisionType MapManager::checkCollision(
    const Player& player,
    const Point& newPos,
    const std::vector<std::shared_ptr<Player>>& allPlayers) const {
    
    const Snake& currentSnake = player.getSnake();
    
    // 1. 检测墙壁碰撞（越界）
    if (isOutOfBounds(newPos)) {
        // 如果有无敌回合，依然返回碰撞类型但不会死亡
        if (currentSnake.getInvincibleRounds() > 0) {
            LOG_DEBUG("Player " + player.getId() + " hit wall but is invincible");
        }
        return CollisionType::WALL;
    }
    
    // 2. 检测自身碰撞
    // 注意：collidesWithSelf 会跳过头部，只检测身体部分
    if (currentSnake.collidesWithSelf(newPos)) {
        if (currentSnake.getInvincibleRounds() > 0) {
            LOG_DEBUG("Player " + player.getId() + " hit self but is invincible");
        }
        return CollisionType::SELF;
    }
    
    // 3. 检测与其他蛇的碰撞
    for (const auto& otherPlayer : allPlayers) {
        // 跳过空指针
        if (!otherPlayer) {
            continue;
        }
        
        // 跳过自己
        if (otherPlayer->getId() == player.getId()) {
            continue;
        }
        
        // 跳过未在游戏中的玩家
        if (!otherPlayer->isInGame()) {
            continue;
        }
        
        const Snake& otherSnake = otherPlayer->getSnake();
        
        // 跳过死亡的蛇
        if (!otherSnake.isAlive()) {
            continue;
        }
        
        // 检测是否撞到其他蛇的身体（包括头部）
        if (otherSnake.collidesWithBody(newPos)) {
            // 如果当前玩家处于无敌状态，返回碰撞类型但不会死亡
            // 如果对方处于无敌状态，也不会相互影响
            if (currentSnake.getInvincibleRounds() > 0 || 
                otherSnake.getInvincibleRounds() > 0) {
                LOG_DEBUG("Player " + player.getId() + " hit other snake but someone is invincible");
            }
            return CollisionType::OTHER_SNAKE;
        }
    }
    
    // 无碰撞
    return CollisionType::NONE;
}

/**
 * @brief 生成指定数量的食物
 * @param count 要生成的食物数量
 * @param players 当前所有玩家列表
 * @return 生成的食物列表
 * 
 * 食物生成规则：
 * - 避免生成在蛇身上（包括头部和身体）
 * - 避免重复生成在同一位置
 * - 如果尝试多次仍无法生成足够数量，返回已生成的食物
 */
std::vector<Food> MapManager::generateFood(
    int count,
    const std::vector<std::shared_ptr<Player>>& players) {
    
    std::vector<Food> foods;
    
    if (count <= 0) {
        return foods;
    }
    
    if (width_ <= 0 || height_ <= 0) {
        LOG_WARNING("Invalid map dimensions for food generation");
        return foods;
    }
    
    // 计算已生成的食物位置集合，避免重复
    std::set<Point> generatedPositions;
    
    // 计算最大尝试次数：每个食物最多尝试100次
    const int maxAttemptsPerFood = 100;
    const int totalCells = width_ * height_;
    
    // 如果请求的食物数量接近地图大小，降低数量
    if (count > totalCells / 2) {
        LOG_WARNING("Too many foods requested, reducing count");
        count = std::max(1, totalCells / 2);
    }
    
    std::uniform_int_distribution<int> distX(0, width_ - 1);
    std::uniform_int_distribution<int> distY(0, height_ - 1);
    
    // 生成食物
    for (int i = 0; i < count; ++i) {
        bool generated = false;
        
        for (int attempt = 0; attempt < maxAttemptsPerFood; ++attempt) {
            Point candidate{distX(rng_), distY(rng_)};
            
            // 检查位置是否有效
            if (!isValidPosition(candidate)) {
                continue;
            }
            
            // 检查是否已经在该位置生成过食物
            if (generatedPositions.find(candidate) != generatedPositions.end()) {
                continue;
            }
            
            // 检查位置是否被蛇占用
            if (isPositionOccupied(candidate, players)) {
                continue;
            }
            
            // 成功生成食物
            foods.emplace_back(candidate);
            generatedPositions.insert(candidate);
            generated = true;
            break;
        }
        
        if (!generated) {
            LOG_WARNING("Failed to generate food #" + std::to_string(i + 1) + 
                       " after " + std::to_string(maxAttemptsPerFood) + " attempts");
        }
    }
    
    LOG_DEBUG("Generated " + std::to_string(foods.size()) + " foods (requested: " + 
             std::to_string(count) + ")");
    
    return foods;
}

/**
 * @brief 基于空间索引生成指定数量的食物（高性能版本）
 * @param count 要生成的食物数量
 * @param occupiedCounts 已占用的蛇身位置计数
 * @param existingFoods 当前已有的食物位置集合
 * @return 生成的食物列表
 *
 * 说明：
 * - 使用哈希集合进行 O(1) 占用判断
 * - 避免遍历所有玩家与蛇身
 * - 仍保留最大尝试次数，避免极端情况死循环
 */
std::vector<Food> MapManager::generateFoodFast(
    int count,
    const std::unordered_map<Point, int, PointHash>& occupiedCounts,
    const std::unordered_set<Point, PointHash>& existingFoods) {

    std::vector<Food> foods;

    if (count <= 0) {
        return foods;
    }

    if (width_ <= 0 || height_ <= 0) {
        LOG_WARNING("Invalid map dimensions for food generation");
        return foods;
    }

    // 计算最大尝试次数：每个食物最多尝试100次
    const int maxAttemptsPerFood = 100;
    const int totalCells = width_ * height_;

    if (count > totalCells / 2) {
        LOG_WARNING("Too many foods requested, reducing count");
        count = std::max(1, totalCells / 2);
    }

    std::uniform_int_distribution<int> distX(0, width_ - 1);
    std::uniform_int_distribution<int> distY(0, height_ - 1);

    std::unordered_set<Point, PointHash> generatedPositions;
    generatedPositions.reserve(static_cast<size_t>(count * 2));

    for (int i = 0; i < count; ++i) {
        bool generated = false;

        for (int attempt = 0; attempt < maxAttemptsPerFood; ++attempt) {
            Point candidate{distX(rng_), distY(rng_)};

            if (!isValidPosition(candidate)) {
                continue;
            }

            if (existingFoods.find(candidate) != existingFoods.end()) {
                continue;
            }

            if (generatedPositions.find(candidate) != generatedPositions.end()) {
                continue;
            }

            if (occupiedCounts.find(candidate) != occupiedCounts.end()) {
                continue;
            }

            foods.emplace_back(candidate);
            generatedPositions.insert(candidate);
            generated = true;
            break;
        }

        if (!generated) {
            LOG_WARNING("Failed to generate food #" + std::to_string(i + 1) +
                       " after " + std::to_string(maxAttemptsPerFood) + " attempts");
        }
    }

    LOG_DEBUG("Generated " + std::to_string(foods.size()) + " foods (requested: " +
             std::to_string(count) + ")");

    return foods;
}

/**
 * @brief 检查指定位置是否有食物
 * @param pos 要检查的位置
 * @param foods 当前地图上的食物列表
 * @return 如果该位置有食物返回 true，否则返回 false
 */
bool MapManager::isFoodAt(const Point& pos, const std::vector<Food>& foods) const {
    for (const auto& food : foods) {
        if (food.getPosition() == pos) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 根据食物密度生成食物
 * @param density 食物密度，范围 [0.0, 1.0]，表示地图上食物占总格子的比例
 * @param players 当前所有玩家列表
 * @return 生成的食物列表
 * 
 * 例如：密度为 0.05 表示生成 地图总格子数 * 0.05 个食物
 */
std::vector<Food> MapManager::generateFoodByDensity(
    double density,
    const std::vector<std::shared_ptr<Player>>& players) {
    
    // 限制密度范围
    if (density < 0.0) {
        density = 0.0;
    }
    if (density > 1.0) {
        density = 1.0;
    }
    
    // 计算需要生成的食物数量
    const int totalCells = width_ * height_;
    const int count = static_cast<int>(totalCells * density);
    
    LOG_DEBUG("Generating food by density: " + std::to_string(density) + 
             " (count: " + std::to_string(count) + ")");
    
    return generateFood(count, players);
}

bool MapManager::isPositionOccupied(
    const Point& pos,
    const std::vector<std::shared_ptr<Player>>& players) const {
    for (const auto& player : players) {
        if (!player) {
            continue;
        }

        const Snake& snake = player->getSnake();

        // 仅对仍在游戏中且蛇存活的玩家进行占用检测
        if (!player->isInGame() || !snake.isAlive()) {
            continue;
        }

        if (snake.collidesWithBody(pos)) {
            return true;
        }
    }
    return false;
}

bool MapManager::isSafeArea(
    const Point& center,
    int radius,
    const std::vector<std::shared_ptr<Player>>& players) const {
    
    // 提前构建占用点集合，避免重复遍历
    std::vector<Point> occupiedPoints;
    occupiedPoints.reserve(players.size() * 20); // 预估容量
    
    for (const auto& player : players) {
        if (!player || !player->isInGame()) {
            continue;
        }
        
        const Snake& snake = player->getSnake();
        if (!snake.isAlive()) {
            continue;
        }
        
        const auto& body = snake.getBlocks();
        occupiedPoints.insert(occupiedPoints.end(), body.begin(), body.end());
    }
    
    // 检查安全区域内是否有占用点
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            Point p{center.x + dx, center.y + dy};
            
            // 越界点跳过（但不算不安全）
            if (!isValidPosition(p)) {
                continue;
            }
            
            // 检查是否被占用
            for (const auto& occupied : occupiedPoints) {
                if (p == occupied) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

} // namespace snake

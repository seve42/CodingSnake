#include "../include/managers/GameManager.h"
#include "../include/managers/MapManager.h"
#include "../include/managers/PlayerManager.h"
#include "../include/models/Config.h"
#include "../include/database/LeaderboardManager.h"
#include "../include/utils/Logger.h"
#include "../include/utils/PerformanceMonitor.h"
#include <chrono>
#include <thread>

namespace snake {

namespace {
std::unique_lock<std::mutex> lockWithMetrics(std::mutex& mutex, const char* name) {
    auto start = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> lock(mutex);
    double waitMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    PerformanceMonitor::getInstance().recordLockWait(name, waitMs);
    return lock;
}
}

GameManager::GameManager(std::shared_ptr<MapManager> mapManager,
                         std::shared_ptr<PlayerManager> playerManager,
                         std::shared_ptr<LeaderboardManager> leaderboardManager)
    : mapManager_(mapManager)
    , playerManager_(playerManager)
    , leaderboardManager_(leaderboardManager)
    , running_(false) {
    LOG_INFO("GameManager initialized");
}

GameManager::~GameManager() {
    stop();
}

void GameManager::start() {
    if (running_) {
        LOG_WARNING("GameManager is already running");
        return;
    }
    
    // 初始化下一回合的时间戳
    {
        auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
        const auto& config = Config::getInstance().getGame();
        auto nextRoundStart = std::chrono::system_clock::now() + 
                              std::chrono::milliseconds(config.roundTimeMs);
        long long nextRoundTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            nextRoundStart.time_since_epoch()).count();
        gameState_.setNextRoundTimestamp(nextRoundTimestamp);
    }

    // 初始化占用索引（仅在启动时构建一次）
    {
        auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
        occupiedCounts_.clear();
        for (const auto& player : gameState_.getPlayers()) {
            if (player && player->isInGame()) {
                const auto& snake = player->getSnake();
                if (snake.isAlive()) {
                    addSnakeToOccupancy(snake);
                }
            }
        }
    }
    
    running_ = true;
    gameThread_ = std::thread(&GameManager::gameLoop, this);
    LOG_INFO("GameManager started, game loop thread launched");
}

void GameManager::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping GameManager...");
    running_ = false;
    
    if (gameThread_.joinable()) {
        gameThread_.join();
    }
    
    LOG_INFO("GameManager stopped");
}

bool GameManager::isRunning() const {
    return running_;
}

void GameManager::tick() {
    LOG_DEBUG("Tick - Round: " + std::to_string(gameState_.getCurrentRound()));
    
    // 0. 交换移动指令缓冲区：将上回合收到的指令准备执行
    {
        auto lock = lockWithMetrics(movesMutex_, "GameManager.moves");
        double pendingSize = static_cast<double>(currentMoves_.size());
        nextMoves_ = std::move(currentMoves_);
        currentMoves_.clear();
        PerformanceMonitor::getInstance().setGauge("moves_current_size", 0.0);
        PerformanceMonitor::getInstance().setGauge("moves_pending_size", pendingSize);
    }
    
    // 0.5. 清空上一回合的增量追踪数据（为本回合的变化记录做准备）
    {
        auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
        gameState_.clearDeltaTracking();
    }
    
    // 1. 处理所有玩家的移动（应用上回合提交的方向指令）
    processMovements();
    
    // 2. 检测碰撞（无敌玩家不会死亡）
    checkCollisions();
    
    // 3. 处理食物收集
    handleFoodCollection();
    
    // 4. 生成新食物
    generateFood();
    
    // 5. 更新无敌状态（在回合结束时递减，这样无敌1回合的玩家在整个回合内都保持无敌）
    updateInvincibility();
    
    // 6. 增加回合数和时间戳
    {
        auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
        gameState_.incrementRound();
        gameState_.updateTimestamp();
        // 注意：增量追踪数据在这个回合内保持有效，
        // 将在下一个回合开始时清空（在步骤0之后）
    }
    
    LOG_DEBUG("Tick completed - Round: " + std::to_string(gameState_.getCurrentRound()));
}

bool GameManager::submitMove(const std::string& playerId, Direction direction) {
    auto lock = lockWithMetrics(movesMutex_, "GameManager.moves");
    
    // 检查玩家是否已经提交过移动指令
    if (currentMoves_.find(playerId) != currentMoves_.end()) {
        LOG_WARNING("Player " + playerId + " already submitted a move this round");
        return false;
    }
    
    // 记录移动指令到当前缓冲区（下回合执行）
    currentMoves_[playerId] = direction;
    PerformanceMonitor::getInstance().setGauge("moves_current_size", static_cast<double>(currentMoves_.size()));
    LOG_DEBUG("Player " + playerId + " submitted move: " + DirectionUtils::toString(direction) + " (will execute next round)");
    return true;
}

GameState GameManager::getGameState() const {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    return gameState_;
}

int GameManager::getCurrentRound() const {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    return gameState_.getCurrentRound();
}

nlohmann::json GameManager::getDeltaState() const {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    return gameState_.toDeltaJson();
}

bool GameManager::addPlayer(std::shared_ptr<Player> player) {
    if (!player) {
        LOG_ERROR("Cannot add null player");
        return false;
    }
    
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    
    // 检查玩家是否已存在
    if (gameState_.getPlayer(player->getId()) != nullptr) {
        LOG_WARNING("Player " + player->getId() + " already in game");
        return false;
    }
    
    gameState_.addPlayer(player);
    // 追踪玩家加入
    gameState_.trackPlayerJoined(player->getId());
    // 初始化占用索引
    if (player->isInGame() && player->getSnake().isAlive()) {
        addSnakeToOccupancy(player->getSnake());
    }
    if (leaderboardManager_) {
        leaderboardManager_->updateOnRound(
            player->getUid(),
            player->getName(),
            gameState_.getCurrentRound(),
            player->getSnake().getLength(),
            0,
            0
        );
    }
    LOG_INFO("Player " + player->getId() + " (" + player->getName() + ") joined the game");
    return true;
}

void GameManager::removePlayer(const std::string& playerId) {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    
    auto player = gameState_.getPlayer(playerId);
    if (player != nullptr) {
        if (player->isInGame() && player->getSnake().isAlive()) {
            removeSnakeFromOccupancy(player->getSnake());
        }
        gameState_.removePlayer(playerId);
        LOG_INFO("Player " + playerId + " removed from game");
    }
}

void GameManager::respawnPlayer(const std::string& playerId) {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    
    auto player = gameState_.getPlayer(playerId);
    if (!player) {
        LOG_WARNING("Cannot respawn non-existent player: " + playerId);
        return;
    }
    
    // 获取安全位置
    const int safeRadius = 5;
    Point spawnPos = mapManager_->getRandomSafePosition(gameState_.getPlayers(), safeRadius);
    
    // 重新初始化蛇
    const auto& config = Config::getInstance().getGame();
    player->initSnake(spawnPos, config.initialSnakeLength);
    player->setInGame(true);
    addSnakeToOccupancy(player->getSnake());
    
    LOG_INFO("Player " + playerId + " respawned at (" + 
             std::to_string(spawnPos.x) + ", " + std::to_string(spawnPos.y) + ")");
}

void GameManager::gameLoop() {
    const auto& config = Config::getInstance().getGame();
    const auto roundTime = std::chrono::milliseconds(config.roundTimeMs);
    
    LOG_INFO("Game loop started with round time: " + std::to_string(config.roundTimeMs) + "ms");
    
    while (running_) {
        auto startTime = std::chrono::steady_clock::now();
        
        // 执行一个回合
        tick();
        
        // 计算下一回合的开始时间（系统时钟）
        auto nextRoundStart = std::chrono::system_clock::now() + roundTime;
        long long nextRoundTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            nextRoundStart.time_since_epoch()).count();
        
        // 更新游戏状态中的下一回合时间戳
        {
            auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
            gameState_.setNextRoundTimestamp(nextRoundTimestamp);
        }
        
        // 计算已用时间
        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        PerformanceMonitor::getInstance().observeRoundDuration(static_cast<double>(elapsed.count()));
        
        // 等待剩余时间
        if (elapsed < roundTime) {
            std::this_thread::sleep_for(roundTime - elapsed);
        } else {
            LOG_WARNING("Tick took longer than round time: " + 
                       std::to_string(elapsed.count()) + "ms");
        }
    }
    
    LOG_INFO("Game loop ended");
}

void GameManager::processMovements() {
    auto moveLock = lockWithMetrics(movesMutex_, "GameManager.moves");
    auto stateLock = lockWithMetrics(stateMutex_, "GameManager.state");

    // 清空上一回合的自撞预判
    pendingSelfCollisions_.clear();
    
    // 第一阶段：应用上回合提交的方向指令
    for (auto& [playerId, direction] : nextMoves_) {
        auto player = gameState_.getPlayer(playerId);
        if (!player || !player->isInGame()) {
            continue;
        }
        
        auto& snake = player->getSnake();
        Direction currentDir = snake.getCurrentDirection();
        
        // 验证方向：不能反向移动
        if (currentDir != Direction::NONE && 
            DirectionUtils::isOpposite(currentDir, direction)) {
            LOG_WARNING("Player " + playerId + " tried to move in opposite direction");
            continue;
        }
        
        // 只设置方向，稍后统一移动
        snake.setDirection(direction);
        LOG_DEBUG("Player " + playerId + " direction set to " + DirectionUtils::toString(direction));
    }
    
    // 第二阶段：预判自撞（使用移动前的身体位置）
    for (auto& player : gameState_.getPlayers()) {
        if (!player->isInGame()) {
            continue;
        }

        auto& snake = player->getSnake();
        Direction dir = snake.getCurrentDirection();
        if (dir == Direction::NONE) {
            continue;
        }

        Point nextHead = snake.getHead();
        switch (dir) {
            case Direction::UP:
                nextHead.y -= 1;
                break;
            case Direction::DOWN:
                nextHead.y += 1;
                break;
            case Direction::LEFT:
                nextHead.x -= 1;
                break;
            case Direction::RIGHT:
                nextHead.x += 1;
                break;
            case Direction::NONE:
                break;
        }

        if (snake.collidesWithSelf(nextHead)) {
            pendingSelfCollisions_.insert(player->getId());
        }
    }

    // 第三阶段：所有蛇统一移动（包括没有提交新方向的蛇，它们会沿用当前方向）
    for (auto& player : gameState_.getPlayers()) {
        if (!player->isInGame()) {
            continue;
        }
        
        auto& snake = player->getSnake();
        if (snake.getCurrentDirection() != Direction::NONE) {
            auto result = snake.moveWithDelta();
            if (result.moved) {
                // 新头加入占用索引
                occupiedCounts_[result.newHead] += 1;

                // 旧尾移除占用索引
                if (result.tailRemoved) {
                    auto it = occupiedCounts_.find(result.removedTail);
                    if (it != occupiedCounts_.end()) {
                        it->second -= 1;
                        if (it->second <= 0) {
                            occupiedCounts_.erase(it);
                        }
                    }
                }
            }
            LOG_DEBUG("Player " + player->getId() + " moved");
        }
    }
    
    // 清空下回合的移动指令缓冲区
    nextMoves_.clear();
    PerformanceMonitor::getInstance().setGauge("moves_pending_size", 0.0);
}

void GameManager::checkCollisions() {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    
    // 先收集所有碰撞信息，避免顺序依赖
    std::vector<std::pair<std::string, MapManager::CollisionType>> collisions;

    // 构建位置 -> 玩家ID列表，用于击杀归因（只包含非无敌玩家）
    std::unordered_map<Point, std::vector<std::string>, PointHash> occupancyByPlayer;
    for (const auto& player : gameState_.getPlayers()) {
        if (!player || !player->isInGame()) {
            continue;
        }
        // 无敌玩家的身体不作为障碍物
        if (player->getSnake().getInvincibleRounds() > 0) {
            continue;
        }
        for (const auto& block : player->getSnake().getBlocks()) {
            occupancyByPlayer[block].push_back(player->getId());
        }
    }
    
    // 重新构建仅包含非无敌玩家的占用索引
    std::unordered_map<Point, int, PointHash> validOccupiedCounts;
    for (const auto& player : gameState_.getPlayers()) {
        if (!player || !player->isInGame()) {
            continue;
        }
        // 无敌玩家的身体不参与碰撞检测
        if (player->getSnake().getInvincibleRounds() > 0) {
            continue;
        }
        for (const auto& block : player->getSnake().getBlocks()) {
            validOccupiedCounts[block] += 1;
        }
    }
    
    // 检查每个玩家的碰撞
    for (auto& player : gameState_.getPlayers()) {
        if (!player->isInGame()) {
            continue;
        }
        
        auto& snake = player->getSnake();
        const Point& head = snake.getHead();
        
        // 跳过无敌玩家
        if (snake.getInvincibleRounds() > 0) {
            continue;
        }

        const bool pendingSelfCollision =
            (pendingSelfCollisions_.find(player->getId()) != pendingSelfCollisions_.end());
        
        // 检查碰撞（使用仅包含非无敌玩家的空间索引）
        MapManager::CollisionType collision = MapManager::CollisionType::NONE;

        if (mapManager_->isOutOfBounds(head)) {
            collision = MapManager::CollisionType::WALL;
        } else if (pendingSelfCollision) {
            collision = MapManager::CollisionType::SELF;
        } else {
            auto it = validOccupiedCounts.find(head);
            if (it != validOccupiedCounts.end() && it->second > 1) {
                collision = MapManager::CollisionType::OTHER_SNAKE;
            }
        }
        
        if (collision != MapManager::CollisionType::NONE) {
            collisions.push_back({player->getId(), collision});
        }
    }
    
    // 统一处理所有碰撞
    for (const auto& [playerId, collisionType] : collisions) {
        auto player = gameState_.getPlayer(playerId);
        if (player && player->isInGame()) {
            const int finalLength = player->getSnake().getLength();
            if (collisionType == MapManager::CollisionType::OTHER_SNAKE && leaderboardManager_) {
                const Point& head = player->getSnake().getHead();
                auto it = occupancyByPlayer.find(head);
                if (it != occupancyByPlayer.end()) {
                    for (const auto& occupantId : it->second) {
                        if (occupantId == playerId) {
                            continue;
                        }
                        auto killerPlayer = gameState_.getPlayer(occupantId);
                        if (!killerPlayer || !killerPlayer->isInGame()) {
                            continue;
                        }
                        leaderboardManager_->updateOnRound(
                            killerPlayer->getUid(),
                            killerPlayer->getName(),
                            gameState_.getCurrentRound(),
                            killerPlayer->getSnake().getLength(),
                            0,
                            1
                        );
                        break;
                    }
                }
            }
            if (leaderboardManager_) {
                leaderboardManager_->updateOnDeath(
                    player->getUid(),
                    player->getName(),
                    gameState_.getCurrentRound(),
                    finalLength
                );
            }

            player->setInGame(false);
            // 追踪玩家死亡
            gameState_.trackPlayerDied(playerId);
            // 从占用索引中移除该蛇
            removeSnakeFromOccupancy(player->getSnake());
            std::string reason;
            switch(collisionType) {
                case MapManager::CollisionType::WALL: reason = "hit wall"; break;
                case MapManager::CollisionType::SELF: reason = "hit self"; break;
                case MapManager::CollisionType::OTHER_SNAKE: reason = "hit other snake"; break;
                default: reason = "unknown"; break;
            }
            LOG_INFO("Player " + player->getId() + " (" + player->getName() + ") died: " + reason);
        }
    }

    // 清空本回合自撞预判
    pendingSelfCollisions_.clear();
}

void GameManager::handleFoodCollection() {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    
    // 检查每个玩家是否吃到食物
    for (auto& player : gameState_.getPlayers()) {
        if (!player->isInGame()) {
            continue;
        }
        
        const Point& head = player->getSnake().getHead();
        
        // 检查是否有食物在蛇头位置（O(1)）
        if (gameState_.hasFoodAt(head)) {
            // 蛇成长
            player->getSnake().grow();

            // 追踪食物移除
            gameState_.trackFoodRemoved(head);
            // 移除食物
            gameState_.removeFood(head);

            LOG_INFO("Player " + player->getId() + " ate food at (" +
                    std::to_string(head.x) + ", " + std::to_string(head.y) + ")");

            if (leaderboardManager_) {
                leaderboardManager_->updateOnRound(
                    player->getUid(),
                    player->getName(),
                    gameState_.getCurrentRound(),
                    player->getSnake().getLength(),
                    1,
                    0
                );
            }
        }
    }
}

void GameManager::generateFood() {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    
    const auto& config = Config::getInstance().getGame();
    int mapSize = config.mapWidth * config.mapHeight;
    int targetFoodCount = static_cast<int>(mapSize * config.foodDensity);
    int currentFoodCount = gameState_.getFoods().size();
    
    // 如果食物不足，生成新食物
    if (currentFoodCount < targetFoodCount) {
        int toGenerate = targetFoodCount - currentFoodCount;

        std::unordered_map<Point, int, PointHash> authoritativeOccupied;
        authoritativeOccupied.reserve(occupiedCounts_.size() + 64);

        for (const auto& player : gameState_.getPlayers()) {
            if (!player || !player->isInGame()) {
                continue;
            }

            const auto& blocks = player->getSnake().getBlocks();
            for (const auto& block : blocks) {
                if (!mapManager_->isValidPosition(block)) {
                    continue;
                }
                authoritativeOccupied[block] += 1;
            }
        }

        auto newFoods = mapManager_->generateFoodFast(toGenerate, authoritativeOccupied, gameState_.getFoodSet());
        
        for (const auto& food : newFoods) {
            // 追踪食物添加
            gameState_.trackFoodAdded(food.getPosition());
            gameState_.addFood(food);
        }
        
        if (!newFoods.empty()) {
            LOG_DEBUG("Generated " + std::to_string(newFoods.size()) + " new food(s)");
        } else {
            LOG_WARNING("Food generation produced 0 items | target=" + std::to_string(targetFoodCount) +
                        ", current=" + std::to_string(currentFoodCount) +
                        ", occupied=" + std::to_string(authoritativeOccupied.size()) +
                        ", existing_foods=" + std::to_string(gameState_.getFoodSet().size()));
        }
    }
}

void GameManager::addSnakeToOccupancy(const Snake& snake) {
    if (!snake.isAlive()) {
        return;
    }

    for (const auto& block : snake.getBlocks()) {
        occupiedCounts_[block] += 1;
    }
}

void GameManager::removeSnakeFromOccupancy(const Snake& snake) {
    if (!snake.isAlive()) {
        return;
    }

    createSnakeDeathDrops(snake.getBlocks()); // 添加蛇被移除时掉落的食物

    for (const auto& block : snake.getBlocks()) {
        auto it = occupiedCounts_.find(block);
        if (it != occupiedCounts_.end()) {
            it->second -= 1;
            if (it->second <= 0) {
                occupiedCounts_.erase(it);
            }
        }
    }
}

void GameManager::updateInvincibility() {
    auto lock = lockWithMetrics(stateMutex_, "GameManager.state");
    
    // 递减每个玩家的无敌回合数
    for (auto& player : gameState_.getPlayers()) {
        if (player->isInGame()) {
            auto& snake = player->getSnake();
            if (snake.getInvincibleRounds() > 0) {
                snake.setInvincibleRounds(snake.getInvincibleRounds() - 1);
                
                if (snake.getInvincibleRounds() == 0) {
                    LOG_INFO("Player " + player->getId() + " invincibility expired");
                }
            }
        }
    }
}

void GameManager::createSnakeDeathDrops(std::vector<Point> pos)
{
    for (auto p : pos)
    {
        if (!gameState_.hasFoodAt(p))
        {
            gameState_.trackFoodAdded(p);
            gameState_.addFood(Food(p));
        }
    }
}

} // namespace snake

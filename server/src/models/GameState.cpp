#include "models/GameState.h"
#include <chrono>
#include <algorithm>

namespace snake {

/**
 * @brief 默认构造函数
 * 
 * 初始化游戏状态：
 * - 当前回合数为 0
 * - 玩家列表为空
 * - 食物列表为空
 * - 时间戳为 0
 */
GameState::GameState() 
    : currentRound_(0), timestamp_(0), nextRoundTimestamp_(0) {
}

/**
 * @brief 获取当前回合数
 * @return 当前回合数
 */
int GameState::getCurrentRound() const {
    return currentRound_;
}

/**
 * @brief 回合数递增
 * 
 * 每次游戏推进一个回合时调用
 */
void GameState::incrementRound() {
    currentRound_++;
    // 注意：增量追踪的清理工作在 GameManager::tick() 的开始处统一处理
}

/**
 * @brief 重置游戏状态
 * 
 * 清空所有数据，回到初始状态：
 * - 回合数归零
 * - 清空所有玩家
 * - 清空所有食物
 * - 时间戳归零
 */
void GameState::reset() {
    currentRound_ = 0;
    players_.clear();
    foods_.clear();
    timestamp_ = 0;
    nextRoundTimestamp_ = 0;
    clearDeltaTracking();
}

/**
 * @brief 添加玩家到游戏状态
 * @param player 要添加的玩家（智能指针）
 * 
 * 说明：
 * - 如果玩家已存在（通过 ID 判断），则不重复添加
 * - 使用智能指针管理玩家对象，避免内存泄漏
 */
void GameState::addPlayer(std::shared_ptr<Player> player) {
    if (!player) {
        return; // 空指针检查
    }

    // 检查玩家是否已存在
    const std::string& playerId = player->getId();
    for (const auto& p : players_) {
        if (p && p->getId() == playerId) {
            return; // 玩家已存在，不重复添加
        }
    }

    // 添加新玩家
    players_.push_back(player);
}

/**
 * @brief 从游戏状态中移除玩家
 * @param playerId 要移除的玩家 ID
 * 
 * 说明：
 * - 通过 ID 查找并移除玩家
 * - 如果玩家不存在，不做任何操作
 */
void GameState::removePlayer(const std::string& playerId) {
    // 使用 remove_if + erase 习惯用法（erase-remove idiom）
    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
            [&playerId](const std::shared_ptr<Player>& p) {
                return p && p->getId() == playerId;
            }),
        players_.end()
    );
}

/**
 * @brief 根据 ID 获取玩家
 * @param playerId 玩家 ID
 * @return 玩家智能指针，如果未找到则返回 nullptr
 */
std::shared_ptr<Player> GameState::getPlayer(const std::string& playerId) {
    for (auto& player : players_) {
        if (player && player->getId() == playerId) {
            return player;
        }
    }
    return nullptr; // 未找到
}

/**
 * @brief 根据 ID 获取玩家 (const 版本)
 * @param playerId 玩家 ID
 * @return 玩家智能指针，如果未找到则返回 nullptr
 */
std::shared_ptr<Player> GameState::getPlayer(const std::string& playerId) const {
    for (const auto& player : players_) {
        if (player && player->getId() == playerId) {
            return player;
        }
    }
    return nullptr; // 未找到
}

/**
 * @brief 获取所有玩家列表（常量引用）
 * @return 玩家列表的常量引用
 */
const std::vector<std::shared_ptr<Player>>& GameState::getPlayers() const {
    return players_;
}

/**
 * @brief 添加食物
 * @param food 要添加的食物对象
 * 
 * 说明：
 * - 如果食物位置已存在，则不重复添加
 * - 避免同一位置出现多个食物
 */
void GameState::addFood(const Food& food) {
    const Point& pos = food.getPosition();
    
    // 检查该位置是否已有食物
    if (foodSet_.find(pos) != foodSet_.end()) {
        return; // 位置已有食物，不重复添加
    }

    // 添加新食物
    foods_.push_back(food);
    foodSet_.insert(pos);
    foodIndex_[pos] = foods_.size() - 1;
}

/**
 * @brief 移除指定位置的食物
 * @param position 食物位置
 * 
 * 说明：
 * - 通过坐标查找并移除食物
 * - 如果该位置没有食物，不做任何操作
 */
void GameState::removeFood(const Point& position) {
    auto it = foodIndex_.find(position);
    if (it == foodIndex_.end()) {
        return;
    }

    const std::size_t index = it->second;
    const std::size_t lastIndex = foods_.size() - 1;

    if (index != lastIndex) {
        // 交换到末尾，保持 O(1) 删除
        std::swap(foods_[index], foods_[lastIndex]);
        const Point& swappedPos = foods_[index].getPosition();
        foodIndex_[swappedPos] = index;
    }

    foods_.pop_back();
    foodSet_.erase(position);
    foodIndex_.erase(it);
}

/**
 * @brief 清空所有食物
 */
void GameState::clearFood() {
    foods_.clear();
    foodSet_.clear();
    foodIndex_.clear();
}

/**
 * @brief 获取所有食物列表（常量引用）
 * @return 食物列表的常量引用
 */
const std::vector<Food>& GameState::getFoods() const {
    return foods_;
}

/**
 * @brief 快速检查指定位置是否有食物
 * @param position 要检查的位置
 * @return 如果该位置有食物返回 true，否则返回 false
 */
bool GameState::hasFoodAt(const Point& position) const {
    return foodSet_.find(position) != foodSet_.end();
}

/**
 * @brief 获取食物位置集合（只读）
 */
const std::unordered_set<Point, PointHash>& GameState::getFoodSet() const {
    return foodSet_;
}

/**
 * @brief 获取时间戳
 * @return 时间戳（毫秒）
 */
long long GameState::getTimestamp() const {
    return timestamp_;
}

/**
 * @brief 更新时间戳为当前时间
 * 
 * 使用系统时钟获取当前毫秒级时间戳
 */
void GameState::updateTimestamp() {
    auto now = std::chrono::system_clock::now();
    timestamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}

/**
 * @brief 获取下一回合时间戳
 * @return 下一回合开始的毫秒级时间戳
 */
long long GameState::getNextRoundTimestamp() const {
    return nextRoundTimestamp_;
}

/**
 * @brief 设置下一回合时间戳
 * @param nextRoundTimestamp 下一回合的时间戳（毫秒）
 */
void GameState::setNextRoundTimestamp(long long nextRoundTimestamp) {
    nextRoundTimestamp_ = nextRoundTimestamp;
}

/**
 * @brief 序列化为 JSON 对象
 * @return JSON 对象，包含完整游戏状态
 * 
 * 返回格式示例：
 * {
 *   "round": 42,
 *   "timestamp": 1706342400000,
 *   "players": [
 *     {
 *       "id": "玩家ID",
 *       "name": "玩家名称",
 *       "color": "#FF0000",
 *       "head": {"x": 10, "y": 15},
 *       "blocks": [{"x": 10, "y": 15}, {"x": 10, "y": 16}],
 *       "length": 3,
 *       "invincible_rounds": 2
 *     }
 *   ],
 *   "foods": [
 *     {"x": 5, "y": 5},
 *     {"x": 20, "y": 25}
 *   ]
 * }
 * 
 * 说明：
 * - 使用 toPublicJson() 序列化玩家，不包含敏感令牌
 * - 适合广播给所有客户端
 */
nlohmann::json GameState::toJson() const {
    nlohmann::json j;
    
    j["round"] = currentRound_;
    j["timestamp"] = timestamp_;
    j["next_round_timestamp"] = nextRoundTimestamp_;
    
    // 序列化玩家列表（使用公开版本，不包含敏感信息）
    nlohmann::json playersJson = nlohmann::json::array();
    for (const auto& player : players_) {
        if (player && player->isInGame()) {
            playersJson.push_back(player->toPublicJson());
        }
    }
    j["players"] = playersJson;
    
    // 序列化食物列表
    nlohmann::json foodsJson = nlohmann::json::array();
    for (const auto& food : foods_) {
        foodsJson.push_back(food.toJson());
    }
    j["foods"] = foodsJson;
    
    return j;
}

/**
 * @brief 高性能序列化为 JSON 对象（避免拷贝）
 * @param j 输出的 JSON 对象引用
 * 
 * 说明：
 * - 直接填充传入的 JSON 对象，避免创建临时对象
 * - 适合高频调用场景（如每回合广播地图状态）
 * - 减少内存分配和拷贝操作，提升性能
 * - 使用 toPublicJsonOptimized() 序列化玩家，进一步优化性能
 * 
 * 使用示例：
 * ```cpp
 * nlohmann::json response;
 * gameState.toJsonOptimized(response);
 * ```
 */
void GameState::toJsonOptimized(nlohmann::json& j) const {
    j["round"] = currentRound_;
    j["timestamp"] = timestamp_;
    j["next_round_timestamp"] = nextRoundTimestamp_;
    
    // 序列化玩家列表（使用高性能版本）
    auto& playersJson = j["players"] = nlohmann::json::array();
    for (const auto& player : players_) {
        if (player && player->isInGame()) {
            nlohmann::json playerJson;
            player->toPublicJsonOptimized(playerJson);
            playersJson.push_back(std::move(playerJson));
        }
    }
    
    // 序列化食物列表
    auto& foodsJson = j["foods"] = nlohmann::json::array();
    for (const auto& food : foods_) {
        foodsJson.push_back(food.toJson());
    }
}

/**
 * @brief 增量序列化为 JSON 对象
 * @return 包含变化数据的 JSON 对象
 * 
 * 说明：
 * - 只返回上一回合到当前回合之间的变化
 * - 包含：所有玩家的简化信息（方向、长度、头部、无敌时间）
 * - 包含：新加入的玩家完整信息
 * - 包含：死亡的玩家ID
 * - 包含：新增和移除的食物
 * - 极大减少数据传输量，节省80%+流量
 * 
 * 返回格式：
 * {
 *   "round": 42,
 *   "timestamp": 1706342400000,
 *   "players": [  // 所有玩家的简化信息
 *     {
 *       "id": "player1",
 *       "direction": "up",
 *       "length": 5,
 *       "head": {"x": 10, "y": 15},
 *       "invincible_rounds": 0
 *     }
 *   ],
 *   "joined_players": [  // 新加入的玩家完整信息
 *     {
 *       "id": "player2",
 *       "name": "新玩家",
 *       "color": "#FF0000",
 *       "head": {"x": 20, "y": 20},
 *       "blocks": [...],
 *       "length": 3,
 *       "invincible_rounds": 5
 *     }
 *   ],
 *   "died_players": ["player3"],  // 死亡的玩家ID
 *   "added_foods": [{"x": 5, "y": 5}],  // 新增的食物
 *   "removed_foods": [{"x": 10, "y": 10}]  // 移除的食物
 * }
 */
nlohmann::json GameState::toDeltaJson() const {
    nlohmann::json j;
    
    j["round"] = currentRound_;
    j["timestamp"] = timestamp_;
    j["next_round_timestamp"] = nextRoundTimestamp_;
    
    // 所有玩家的简化信息（不包含完整blocks数组）
    auto& playersJson = j["players"] = nlohmann::json::array();
    for (const auto& player : players_) {
        if (player && player->isInGame()) {
            nlohmann::json playerJson;
            playerJson["id"] = player->getId();
            
            const auto& snake = player->getSnake();
            const auto& blocks = snake.getBlocks();
            
            // 只发送头部位置
            if (!blocks.empty()) {
                playerJson["head"] = {
                    {"x", blocks[0].x},
                    {"y", blocks[0].y}
                };
            }
            
            playerJson["direction"] = DirectionUtils::toString(snake.getCurrentDirection());
            playerJson["length"] = snake.getLength();
            playerJson["invincible_rounds"] = snake.getInvincibleRounds();
            
            playersJson.push_back(std::move(playerJson));
        }
    }
    
    // 新加入的玩家（完整信息）
    auto& joinedJson = j["joined_players"] = nlohmann::json::array();
    for (const auto& playerId : joinedPlayers_) {
        auto player = getPlayer(playerId);
        if (player && player->isInGame()) {
            nlohmann::json playerJson;
            player->toPublicJsonOptimized(playerJson);
            joinedJson.push_back(std::move(playerJson));
        }
    }
    
    // 死亡的玩家ID
    j["died_players"] = diedPlayers_;
    
    // 新增的食物
    auto& addedFoodsJson = j["added_foods"] = nlohmann::json::array();
    for (const auto& food : addedFoods_) {
        addedFoodsJson.push_back({{"x", food.x}, {"y", food.y}});
    }
    
    // 移除的食物
    auto& removedFoodsJson = j["removed_foods"] = nlohmann::json::array();
    for (const auto& food : removedFoods_) {
        removedFoodsJson.push_back({{"x", food.x}, {"y", food.y}});
    }
    
    return j;
}

/**
 * @brief 追踪玩家加入
 * @param playerId 加入的玩家ID
 */
void GameState::trackPlayerJoined(const std::string& playerId) {
    joinedPlayers_.push_back(playerId);
}

/**
 * @brief 追踪玩家死亡
 * @param playerId 死亡的玩家ID
 */
void GameState::trackPlayerDied(const std::string& playerId) {
    diedPlayers_.push_back(playerId);
}

/**
 * @brief 追踪食物添加
 * @param position 食物位置
 */
void GameState::trackFoodAdded(const Point& position) {
    addedFoods_.push_back(position);
}

/**
 * @brief 追踪食物移除
 * @param position 食物位置
 */
void GameState::trackFoodRemoved(const Point& position) {
    removedFoods_.push_back(position);
}

/**
 * @brief 清空增量变化追踪
 * 
 * 每回合结束时调用，为下一回合的追踪做准备
 */
void GameState::clearDeltaTracking() {
    joinedPlayers_.clear();
    diedPlayers_.clear();
    addedFoods_.clear();
    removedFoods_.clear();
}

} // namespace snake

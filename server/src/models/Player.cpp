#include "models/Player.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>

namespace snake {

/**
 * @brief 生成简单的随机 ID（线程安全版本）
 * @return 随机 ID 字符串（16个十六进制字符）
 * 
 * 说明：
 * - 使用 thread_local 确保每个线程都有自己的随机数生成器
 * - 避免了多线程环境下对静态 mt19937 的竞争条件
 * - 性能优于加锁，因为每个线程独立操作自己的生成器
 */
static std::string generateId() {
    // thread_local 确保每个线程有独立的随机数生成器
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd() + std::time(nullptr));
    thread_local std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 16; ++i) {
        ss << dis(gen);
    }
    
    return ss.str();
}

/**
 * @brief 默认构造函数
 * 
 * 初始化玩家对象，所有字段为空或默认值
 * 游戏状态设置为不在游戏中
 * 
 * 说明：
 * - std::string 默认构造为空字符串，无需显式初始化为 ""
 * - Snake 默认构造函数会初始化为合理的默认状态
 */
Player::Player()
    : inGame_(false) {
}

/**
 * @brief 构造函数，初始化玩家的基本信息
 * @param uid 洛谷用户 ID
 * @param name 玩家显示名称
 * @param color 蛇的颜色（十六进制格式，如 "#FF0000"）
 * 
 * 说明：
 * - uid 是玩家在洛谷的唯一标识，在整个游戏过程中保持不变
 * - id 是玩家在本局游戏中的唯一标识，自动生成，用于区分同一玩家的多次加入
 * - key 和 token 需要通过 setKey() 和 setToken() 单独设置
 * - 蛇对象初始为空，需要通过 initSnake() 初始化
 * - 未显式初始化的 std::string 成员会自动初始化为空字符串
 */
Player::Player(std::string uid, std::string name, std::string color)
    : uid_(std::move(uid))
    , id_(generateId())
    , name_(std::move(name))
    , color_(std::move(color))
    , inGame_(false) {
}

/**
 * @brief 获取洛谷 UID
 * @return UID 的常量引用
 */
const std::string& Player::getUid() const {
    return uid_;
}

/**
 * @brief 获取玩家游戏 ID（UUID）
 * @return ID 的常量引用
 */
const std::string& Player::getId() const {
    return id_;
}

/**
 * @brief 获取玩家显示名称
 * @return 名称的常量引用
 */
const std::string& Player::getName() const {
    return name_;
}

/**
 * @brief 获取蛇的颜色
 * @return 颜色的常量引用（十六进制格式）
 */
const std::string& Player::getColor() const {
    return color_;
}

/**
 * @brief 获取账号级别令牌（key）
 * @return key 的常量引用
 * 
 * 说明：key 用于验证玩家身份，由登录接口分配
 */
const std::string& Player::getKey() const {
    return key_;
}

/**
 * @brief 获取游戏会话令牌（token）
 * @return token 的常量引用
 * 
 * 说明：token 用于标识玩家在当前游戏中的会话，由加入游戏接口分配
 */
const std::string& Player::getToken() const {
    return token_;
}

/**
 * @brief 设置账号级别令牌（key）
 * @param key 要设置的 key
 */
void Player::setKey(const std::string& key) {
    key_ = key;
}

/**
 * @brief 设置游戏会话令牌（token）
 * @param token 要设置的 token
 */
void Player::setToken(const std::string& token) {
    token_ = token;
}

/**
 * @brief 设置玩家游戏 ID
 * @param id 要设置的 ID
 * 
 * 说明：通常由构造函数自动生成，但可通过此方法覆盖
 */
void Player::setId(const std::string& id) {
    id_ = id;
}

/**
 * @brief 获取玩家的蛇对象（非常量引用）
 * @return 蛇对象的非常量引用，可用于修改
 */
Snake& Player::getSnake() {
    return snake_;
}

/**
 * @brief 获取玩家的蛇对象（常量引用）
 * @return 蛇对象的常量引用
 */
const Snake& Player::getSnake() const {
    return snake_;
}

/**
 * @brief 初始化玩家的蛇
 * @param position 蛇头的初始位置
 * @param initialLength 蛇的初始长度（默认为 3）
 * 
 * 说明：
 * - 创建一个新的 Snake 对象，替换原有的蛇
 * - 通常在玩家加入游戏或重生时调用
 */
void Player::initSnake(const Point& position, int initialLength) {
    snake_ = Snake(position, initialLength);
}

/**
 * @brief 检查玩家是否在游戏中
 * @return 如果玩家在游戏中返回 true
 */
bool Player::isInGame() const {
    return inGame_;
}

/**
 * @brief 设置玩家的游戏状态
 * @param inGame 是否在游戏中
 * 
 * 说明：
 * - true 表示玩家已加入游戏
 * - false 表示玩家已离开或被淘汰
 * - 当设置为 false 时，同步更新蛇的状态以保证数据一致性
 */
void Player::setInGame(bool inGame) {
    inGame_ = inGame;
    // 当玩家离开或被淘汰游戏时，同步杀死蛇，防止逻辑残留
    if (!inGame && snake_.isAlive()) {
        snake_.kill();
    }
}

/**
 * @brief 序列化为 JSON 对象（完整版本）
 * @return JSON 对象，包含玩家的所有信息
 * 
 * 说明：
 * - 包含敏感信息（key、token），仅用于以下场景：
 *   1. 玩家数据持久化到数据库
 *   2. 发送给玩家本人（以便玩家获取自己的令牌）
 * - 不应该广播给其他玩家或公网
 * 
 * 返回格式示例：
 * {
 *   "id": "玩家ID",
 *   "uid": "洛谷UID",
 *   "name": "玩家名称",
 *   "color": "#FF0000",
 *   "key": "账号令牌",
 *   "token": "游戏令牌",
 *   "snake": {...},
 *   "in_game": true
 * }
 */
nlohmann::json Player::toJson() const {
    nlohmann::json j;
    
    j["id"] = id_;
    j["uid"] = uid_;
    j["name"] = name_;
    j["color"] = color_;
    j["key"] = key_;
    j["token"] = token_;
    j["snake"] = snake_.toJson();
    j["in_game"] = inGame_;
    
    return j;
}

/**
 * @brief 序列化为 JSON 对象（公开版本）
 * @return JSON 对象，包含玩家的非敏感信息
 * 
 * 说明：
 * - 不包含敏感信息（key、token），可安全地：
 *   1. 广播给其他玩家（用于地图状态同步）
 *   2. 发送到客户端（用于游戏展示）
 *   3. 用于公网传输
 * - 去掉 key、token 字段以防止令牌泄露导致的越权问题
 * - 将蛇的属性扁平化到玩家对象中，符合 API 规范
 * 
 * 返回格式示例：
 * {
 *   "id": "玩家ID",
 *   "name": "玩家名称",
 *   "color": "#FF0000",
 *   "head": {"x": 10, "y": 15},
 *   "blocks": [{"x": 10, "y": 15}, ...],
 *   "length": 3,
 *   "invincible_rounds": 2
 * }
 */
nlohmann::json Player::toPublicJson() const {
    nlohmann::json j;
    
    // 基本信息
    j["id"] = id_;
    j["name"] = name_;
    j["color"] = color_;
    
    // 蛇的属性扁平化（符合 API 规范）
    const auto& blocks = snake_.getBlocks();
    if (!blocks.empty()) {
        // head 是蛇头位置（blocks[0]）
        j["head"] = {
            {"x", blocks[0].x},
            {"y", blocks[0].y}
        };
        
        // blocks 数组包含所有身体块
        nlohmann::json blocksArray = nlohmann::json::array();
        for (const auto& block : blocks) {
            blocksArray.push_back({
                {"x", block.x},
                {"y", block.y}
            });
        }
        j["blocks"] = blocksArray;
    } else {
        // 如果蛇还没有身体块（理论上不应该发生）
        j["head"] = {{"x", 0}, {"y", 0}};
        j["blocks"] = nlohmann::json::array();
    }
    
    j["length"] = snake_.getLength();
    j["invincible_rounds"] = snake_.getInvincibleRounds();
    
    return j;
}

/**
 * @brief 高性能序列化为 JSON 对象（公开版本，避免拷贝）
 * @param j 输出的 JSON 对象引用
 * 
 * 说明：
 * - 直接填充传入的 JSON 对象，避免创建临时对象
 * - 适合高频调用场景（如每回合广播地图状态）
 * - 减少内存分配和拷贝操作，提升性能
 * - 当蛇的身体很长时，性能优势更明显
 * - 将蛇的属性扁平化到玩家对象中，符合 API 规范
 * 
 * 使用示例：
 * ```cpp
 * nlohmann::json j;
 * player.toPublicJsonOptimized(j);
 * ```
 */
void Player::toPublicJsonOptimized(nlohmann::json& j) const {
    // 基本信息
    j["id"] = id_;
    j["name"] = name_;
    j["color"] = color_;
    
    // 蛇的属性扁平化（符合 API 规范）
    const auto& blocks = snake_.getBlocks();
    if (!blocks.empty()) {
        // head 是蛇头位置（blocks[0]）
        j["head"] = {
            {"x", blocks[0].x},
            {"y", blocks[0].y}
        };
        
        // blocks 数组包含所有身体块
        nlohmann::json blocksArray = nlohmann::json::array();
        for (const auto& block : blocks) {
            blocksArray.push_back({
                {"x", block.x},
                {"y", block.y}
            });
        }
        j["blocks"] = blocksArray;
    } else {
        // 如果蛇还没有身体块（理论上不应该发生）
        j["head"] = {{"x", 0}, {"y", 0}};
        j["blocks"] = nlohmann::json::array();
    }
    
    j["length"] = snake_.getLength();
    j["invincible_rounds"] = snake_.getInvincibleRounds();
}

} // namespace snake

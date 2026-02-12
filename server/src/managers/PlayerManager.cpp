#include "../include/managers/PlayerManager.h"
#include "../include/utils/Logger.h"
#include "../include/utils/Validator.h"
#include "../include/models/Config.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <openssl/sha.h>

namespace snake {

PlayerManager::PlayerManager() 
    : db_(std::make_shared<DatabaseManager>()) {
    // 初始化数据库
    const std::string& dbPath = Config::getInstance().getDatabase().path;
    if (!db_->initialize(dbPath)) {
        LOG_ERROR("Failed to initialize database for PlayerManager");
    }
    LOG_INFO("PlayerManager initialized");
}

PlayerManager::~PlayerManager() {
    db_->close();
    LOG_INFO("PlayerManager destroyed");
}



std::string PlayerManager::login(const std::string& uid, const std::string& paste) {
    // 1. 验证洛谷剪贴板
    if (!checkLogin(uid, paste)) {
        LOG_WARNING("Login validation failed for UID: " + uid);
        return "";
    }

    std::unique_lock<std::shared_mutex> lock(mutex_);

    // 2. 检查用户是否已存在
    std::string sql = "SELECT key, paste FROM players WHERE uid = ?";
    auto rs = db_->queryWithParams(sql, {uid});
    
    if (rs.next()) {
        // 用户已存在，检查paste是否匹配
        std::string existingKey = rs.getString(0);
        std::string existingPaste = rs.getString(1);
        
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        
        if (existingPaste == paste) {
            // paste匹配，返回现有的key并更新最后登录时间
            std::string updateSql = "UPDATE players SET last_login = ? WHERE uid = ?";
            db_->executeWithParams(updateSql, {std::to_string(now), uid});
            
            LOG_INFO("Existing user login with matching paste: UID=" + uid);
            return existingKey;
        } else {
            // paste不匹配，生成新的key并使旧key失效
            std::string newKey = generateKey(uid);
            std::string updateSql = "UPDATE players SET paste = ?, key = ?, last_login = ? WHERE uid = ?";
            bool success = db_->executeWithParams(updateSql, {
                paste, 
                newKey, 
                std::to_string(now), 
                uid
            });
            
            if (!success) {
                LOG_ERROR("Failed to update player key for UID=" + uid);
                return "";
            }
            
            // 更新内存缓存
            keyToUid_.erase(existingKey);  // 移除旧key
            uidToKey_[uid] = newKey;
            keyToUid_[newKey] = uid;
            
            LOG_INFO("User login with new paste, key updated: UID=" + uid + ", old_key=" + existingKey + ", new_key=" + newKey);
            return newKey;
        }
    }

    // 3. 新用户，生成key并存储到数据库
    std::string key = generateKey(uid);
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    
    std::string insertSql = "INSERT INTO players (uid, paste, key, created_at, last_login) VALUES (?, ?, ?, ?, ?)";
    bool success = db_->executeWithParams(insertSql, {
        uid, 
        paste, 
        key, 
        std::to_string(now), 
        std::to_string(now)
    });
    
    if (!success) {
        LOG_ERROR("Failed to insert new player into database: UID=" + uid);
        return "";
    }

    // 4. 缓存到内存映射
    uidToKey_[uid] = key;
    keyToUid_[key] = uid;

    LOG_INFO("New user registered: UID=" + uid + ", key=" + key);
    return key;
}

bool PlayerManager::checkLogin(const std::string& uid, const std::string& paste) {
    // 获取配置中的验证文本
    const std::string& expectedText = Config::getInstance().getAuth().luoguValidationText;
    
    // 调用 Validator 进行验证
    bool isValid = Validator::validateLuoguPaste(uid, paste);
    
    if (isValid) {
        LOG_INFO("Login validation successful for UID: " + uid);
    } else {
        LOG_WARNING("Login validation failed for UID: " + uid);
    }
    
    return isValid;
}

PlayerManager::JoinResult PlayerManager::join(const std::string& key, 
                                               const std::string& name, 
                                               const std::string& color) {
    JoinResult result;
    result.success = false;

    // 1. 验证key
    std::string uid;
    if (!validateKey(key, uid)) {
        result.errorMsg = "Invalid key";
        LOG_WARNING("Join failed: invalid key");
        return result;
    }

    // 2. 验证玩家名称
    if (!Validator::isValidPlayerName(name)) {
        result.errorMsg = "Invalid player name";
        LOG_WARNING("Join failed: invalid player name");
        return result;
    }

    // 3. 验证或生成颜色
    std::string playerColor = color;
    if (playerColor.empty()) {
        playerColor = generateRandomColor();
    } else if (!Validator::isValidColor(playerColor)) {
        result.errorMsg = "Invalid color format";
        LOG_WARNING("Join failed: invalid color");
        return result;
    }

    std::unique_lock<std::shared_mutex> lock(mutex_);

    // 4. 检查玩家是否已在游戏中
    for (const auto& [pid, player] : players_) {
        if (player->getUid() == uid && player->isInGame()) {
            result.errorMsg = "Player already in game";
            LOG_WARNING("Join failed: player already in game");
            return result;
        }
    }

    // 5. 生成playerId和token
    /**
     * 会话管理核心逻辑
     * 
     * PlayerId 生成：
     * - 为本次游戏会话分配唯一的玩家ID
     * - 格式：p_{uid}_{随机数}
     * 
     * Token 生成：
     * - 生成游戏会话凭证
     * - 基于 playerId + 时间戳 + 随机数的 SHA256 哈希
     * - 用于后续所有游戏操作的身份认证
     * 
     * 存储策略：
     * - tokenToPlayerId_: 快速通过 token 查找 playerId
     * - players_: 存储完整的 Player 对象
     * 
     * 关联关系：
     * token -> playerId -> Player
     */
    std::string playerId = generatePlayerId(uid);
    std::string token = generateToken(playerId);

    // 6. 创建玩家对象
    auto player = std::make_shared<Player>(uid, name, playerColor);
    player->setId(playerId);
    player->setToken(token);
    player->setKey(key);
    player->setInGame(true);

    // 7. 存储到内存映射
    players_[playerId] = player;
    tokenToPlayerId_[token] = playerId;

    result.success = true;
    result.token = token;
    result.playerId = playerId;

    LOG_INFO("Player joined: UID=" + uid + ", Name=" + name + ", PlayerId=" + playerId);
    return result;
}

bool PlayerManager::validateKey(const std::string& key, std::string& uid) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    // 1. 先检查内存缓存
    auto it = keyToUid_.find(key);
    if (it != keyToUid_.end()) {
        uid = it->second;
        return true;
    }

    // 2. 从数据库查询
    lock.unlock();
    std::string sql = "SELECT uid FROM players WHERE key = ?";
    auto rs = db_->queryWithParams(sql, {key});
    
    if (rs.next()) {
        uid = rs.getString(0);
        
        // 注意：不能在const函数中修改成员变量
        // 这里我们选择不缓存，或者需要使用mutable关键字
        
        return true;
    }

    return false;
}

bool PlayerManager::validateToken(const std::string& token, std::string& playerId) const {
    /**
     * Token 验证
     * 
     * 功能：验证 token 是否有效，并返回对应的 playerId
     * 
     * 验证流程：
     * 1. 使用读锁保护共享数据（支持多线程并发读取）
     * 2. 在内存映射表中查找 token
     * 3. 如果找到，返回 true 并设置 playerId
     * 4. 如果未找到，返回 false
     * 
     * 线程安全性：
     * - 使用 shared_lock 允许多个线程同时验证
     * - 不会阻塞其他读操作
     * 
     * 性能优化：
     * - 仅查询内存，不访问数据库（token 是临时会话凭证）
     * - O(log n) 时间复杂度（使用 std::map）
     */
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = tokenToPlayerId_.find(token);
    if (it != tokenToPlayerId_.end()) {
        playerId = it->second;
        LOG_DEBUG("Token validated successfully: playerId=" + playerId);
        return true;
    }

    LOG_DEBUG("Token validation failed: token not found");
    return false;
}

std::shared_ptr<Player> PlayerManager::getPlayerById(const std::string& playerId) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = players_.find(playerId);
    if (it != players_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Player> PlayerManager::getPlayerByToken(const std::string& token) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = tokenToPlayerId_.find(token);
    if (it != tokenToPlayerId_.end()) {
        return getPlayerById(it->second);
    }
    return nullptr;
}

std::shared_ptr<Player> PlayerManager::getPlayerByKey(const std::string& key) {
    std::string uid;
    if (!validateKey(key, uid)) {
        return nullptr;
    }
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    for (const auto& [playerId, player] : players_) {
        if (player->getUid() == uid) {
            return player;
        }
    }
    return nullptr;
}

void PlayerManager::removePlayer(const std::string& playerId) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = players_.find(playerId);
    if (it != players_.end()) {
        // 移除token映射
        const std::string& token = it->second->getToken();
        tokenToPlayerId_.erase(token);
        
        // 移除玩家
        players_.erase(it);
        
        LOG_INFO("Player removed: " + playerId);
    }
}

bool PlayerManager::isPlayerInGame(const std::string& playerId) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = players_.find(playerId);
    return it != players_.end() && it->second->isInGame();
}

int PlayerManager::getPlayerCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return players_.size();
}

std::vector<std::shared_ptr<Player>> PlayerManager::getAllPlayers() const {
    /**
     * 获取所有在线玩家列表
     * 
     * 功能：返回当前所有在游戏中的玩家对象
     * 
     * 返回值：
     * - 包含所有玩家智能指针的向量
     * - 按照插入顺序（实际上 map 会按 key 排序）
     * 
     * 线程安全性：
     * - 使用 shared_lock 保护读操作
     * - 可与其他读操作并发执行
     * 
     * 使用场景：
     * - GameManager 需要遍历所有玩家进行回合推进
     * - MapManager 检测碰撞时需要获取所有玩家位置
     * - RouteHandler 返回游戏状态时需要玩家列表
     * 
     * 性能考虑：
     * - 返回 vector 会进行拷贝，但拷贝的是智能指针（开销小）
     * - 时间复杂度：O(n)，n 为玩家数量
     */
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Player>> result;
    result.reserve(players_.size()); // 预分配空间，避免多次扩容
    
    for (const auto& [playerId, player] : players_) {
        if (player && player->isInGame()) {
            result.push_back(player);
        }
    }
    
    LOG_DEBUG("getAllPlayers() returned " + std::to_string(result.size()) + " players");
    return result;
}

void PlayerManager::removeAllPlayers() {
    /**
     * 批量移除所有玩家
     * 
     * 功能：清空所有玩家数据，用于游戏重置或服务器关闭
     * 
     * 操作步骤：
     * 1. 获取写锁（独占访问）
     * 2. 清空玩家映射表
     * 3. 清空 token 映射表
     * 
     * 线程安全性：
     * - 使用 unique_lock 保护写操作
     * - 会阻塞所有其他读写操作
     * 
     * 注意事项：
     * - 不清空 uidToKey_ 和 keyToUid_（账号级别数据保留）
     * - 这样用户可以重新加入游戏而无需重新登录
     * - 数据库中的玩家记录不受影响
     */
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    int playerCount = players_.size();
    
    // 清空玩家相关的映射表
    players_.clear();
    tokenToPlayerId_.clear();
    
    LOG_INFO("Removed all players, count: " + std::to_string(playerCount));
}

std::vector<std::shared_ptr<Player>> PlayerManager::getPlayersByUid(const std::string& uid) const {
    /**
     * 通过 UID 查询玩家
     * 
     * 功能：返回指定 UID 的所有游戏会话
     * 
     * 场景：
     * - 用于调试和管理功能
     * - 检测是否有重复登录
     * 
     * 实现：
     * - 遍历所有玩家，筛选出匹配的 UID
     * - 时间复杂度：O(n)
     * 
     * 优化空间：
     * - 如果频繁使用，可以添加 uidToPlayerIds_ 映射
     * - 当前实现简单直接，适合当前需求
     */
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Player>> result;
    
    for (const auto& [playerId, player] : players_) {
        if (player && player->getUid() == uid) {
            result.push_back(player);
        }
    }
    
    LOG_DEBUG("getPlayersByUid(" + uid + ") found " + std::to_string(result.size()) + " players");
    return result;
}

std::string PlayerManager::generateKey(const std::string& uid) {
    // 使用 SHA256 生成key
    // 输入: uid + 当前时间戳 + 随机数
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);
    
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::string input = uid + std::to_string(now) + std::to_string(dis(gen));
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    
    // 转换为十六进制字符串
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

std::string PlayerManager::generateToken(const std::string& playerId) {
    /**
     * Token 生成算法
     * 
     * Token 是游戏会话级别的凭证，用于玩家在游戏过程中的身份认证。
     * 特点：
     * 1. 每次玩家加入游戏时重新生成
     * 2. 与具体的游戏会话绑定
     * 3. 玩家退出游戏后失效
     * 
     * 生成策略：
     * - 输入源：playerId + 当前纳秒级时间戳 + 6位随机数
     * - 算法：SHA256 哈希
     * - 输出格式：64位十六进制字符串
     * 
     * 安全性保证：
     * - 时间戳确保不同时间生成的token不同
     * - 随机数防止同一时间并发请求产生相同token
     * - SHA256 保证token不可逆推
     */
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);
    
    // 组合输入：playerId + 纳秒时间戳 + 随机数
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::string input = playerId + std::to_string(now) + std::to_string(dis(gen));
    
    // 计算 SHA256 哈希
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    
    // 转换为十六进制字符串（64个字符）
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    std::string token = oss.str();
    LOG_DEBUG("Generated token for playerId=" + playerId + ", token=" + token.substr(0, 8) + "...");
    
    return token;
}

std::string PlayerManager::generatePlayerId(const std::string& uid) {
    /**
     * PlayerId 生成算法
     * 
     * PlayerId 是玩家在单次游戏会话中的唯一标识。
     * 特点：
     * 1. 每次加入游戏时重新生成
     * 2. 可读性强，便于日志和调试
     * 3. 包含用户ID信息，方便追溯
     * 
     * 生成策略：
     * - 格式：p_{uid}_{6位随机数}
     * - 示例：p_123456_789012
     * 
     * 设计考虑：
     * - 前缀 "p_" 标识这是一个 playerId
     * - uid 部分便于快速识别是哪个用户
     * - 随机数部分确保同一用户多次加入时 ID 不重复
     */
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    
    std::string playerId = "p_" + uid + "_" + std::to_string(dis(gen));
    LOG_DEBUG("Generated playerId=" + playerId + " for uid=" + uid);
    
    return playerId;
}

std::string PlayerManager::generateRandomColor() {
    // 生成随机颜色
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::ostringstream oss;
    oss << "#" 
        << std::hex << std::setw(2) << std::setfill('0') << dis(gen)
        << std::hex << std::setw(2) << std::setfill('0') << dis(gen)
        << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    
    return oss.str();
}

} // namespace snake

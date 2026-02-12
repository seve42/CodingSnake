#pragma once

#include "../models/Player.h"
#include "../database/DatabaseManager.h"
#include <memory>
#include <string>
#include <map>
#include <shared_mutex>

namespace snake {

/**
 * @brief 玩家管理器
 * 负责玩家认证、会话管理
 */
class PlayerManager {
public:
    PlayerManager();
    ~PlayerManager();

    // 登录与认证
    std::string login(const std::string& uid, const std::string& paste);
    bool checkLogin(const std::string& uid, const std::string& paste);
    
    // 加入游戏
    struct JoinResult {
        bool success;
        std::string token;
        std::string playerId;
        std::string errorMsg;
    };
    JoinResult join(const std::string& key, const std::string& name, 
                    const std::string& color);

    // 验证
    bool validateKey(const std::string& key, std::string& uid) const;
    bool validateToken(const std::string& token, std::string& playerId) const;

    // 玩家查询
    std::shared_ptr<Player> getPlayerById(const std::string& playerId);
    std::shared_ptr<Player> getPlayerByToken(const std::string& token);
    std::shared_ptr<Player> getPlayerByKey(const std::string& key);

    // 玩家管理
    void removePlayer(const std::string& playerId);
    bool isPlayerInGame(const std::string& playerId) const;
    int getPlayerCount() const;
    
    // 获取所有在线玩家列表（用于游戏逻辑和统计）
    std::vector<std::shared_ptr<Player>> getAllPlayers() const;
    
    // 批量移除玩家（用于游戏结束或重置）
    void removeAllPlayers();
    
    // 通过 UID 查询玩家（一个用户可能有多个游戏会话）
    std::vector<std::shared_ptr<Player>> getPlayersByUid(const std::string& uid) const;

private:
    std::string generateKey(const std::string& uid);
    std::string generateToken(const std::string& playerId);
    std::string generatePlayerId(const std::string& uid);
    std::string generateRandomColor();

    // 数据库管理器
    std::shared_ptr<DatabaseManager> db_;

    // uid -> key 映射
    std::map<std::string, std::string> uidToKey_;
    
    // key -> uid 映射
    std::map<std::string, std::string> keyToUid_;
    
    // token -> playerId 映射
    std::map<std::string, std::string> tokenToPlayerId_;
    
    // playerId -> Player 对象
    std::map<std::string, std::shared_ptr<Player>> players_;
    
    mutable std::shared_mutex mutex_;
};

} // namespace snake

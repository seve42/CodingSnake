#pragma once

#include "Snake.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace snake {

/**
 * @brief 玩家信息
 */
class Player {
public:
    Player();
    // 使用值传递配合 std::move 提高效率，避免多次拷贝
    Player(std::string uid, std::string name, std::string color);

    // 基本信息
    const std::string& getUid() const;
    const std::string& getId() const;
    const std::string& getName() const;
    const std::string& getColor() const;
    const std::string& getKey() const;
    const std::string& getToken() const;

    void setKey(const std::string& key);
    void setToken(const std::string& token);
    void setId(const std::string& id);

    // 蛇相关
    Snake& getSnake();
    const Snake& getSnake() const;
    void initSnake(const Point& position, int initialLength);

    // 状态
    bool isInGame() const;
    void setInGame(bool inGame);

    // 序列化
    // toJson() 返回完整信息（包含敏感令牌），仅用于存档或发送给玩家本人
    nlohmann::json toJson() const;
    // toPublicJson() 返回公开信息（不含 key, token），用于广播给其他玩家或客户端
    nlohmann::json toPublicJson() const;
    // toPublicJsonOptimized() 高性能版本，直接填充传入的 JSON 对象，避免拷贝
    void toPublicJsonOptimized(nlohmann::json& j) const;

private:
    std::string uid_;       // 洛谷 UID（用户账号标识，不变）
    std::string id_;        // 游戏内 ID（本局游戏的玩家唯一标识，随机生成）
    std::string name_;      // 显示名称
    std::string color_;     // 颜色
    std::string key_;       // 账号级别令牌
    std::string token_;     // 游戏会话令牌
    Snake snake_;           // 蛇对象
    bool inGame_;           // 是否在游戏中
};

} // namespace snake

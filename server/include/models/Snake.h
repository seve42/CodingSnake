#pragma once

#include "Point.h"
#include "Direction.h"
#include <deque>
#include <set>
#include <nlohmann/json.hpp>

namespace snake {

/**
 * @brief 蛇的数据结构
 */
class Snake {
public:
    /**
     * @brief 移动结果（用于增量更新占用索引）
     */
    struct MoveResult {
        bool moved = false;        // 是否发生移动
        Point newHead;             // 新头部位置
        bool tailRemoved = false;  // 是否移除了尾部
        Point removedTail;         // 被移除的尾部位置
    };

    Snake();
    Snake(const Point& initialHead, int initialLength);

    // 移动相关
    void move(); 
    MoveResult moveWithDelta();
    void grow();
    
    // 状态查询
    const Point& getHead() const;
    const std::deque<Point>& getBlocks() const;
    int getLength() const;
    Direction getCurrentDirection() const;
    int getInvincibleRounds() const;
    bool isAlive() const;

    // 状态修改
    void setDirection(Direction dir);
    void setInvincibleRounds(int rounds);
    void kill();
    void decreaseInvincibleRounds();

    // 碰撞检测
    bool collidesWithSelf(const Point& point) const;
    bool collidesWithBody(const Point& point) const;

    // 序列化
    nlohmann::json toJson() const;

private:
    std::deque<Point> blocks_;  // blocks_[0] 是头部，
    std::set<Point> blockSet_; // 快速查找身体块
    Direction currentDirection_;
    int invincibleRounds_;
    bool alive_;
    int growthPending_;  // 待成长次数，用于初始化和吃食物后的成长
};

} // namespace snake

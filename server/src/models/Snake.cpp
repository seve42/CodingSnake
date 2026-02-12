#include "models/Snake.h"
#include <algorithm>
#include <stdexcept>

namespace snake {

/**
 * @brief 默认构造函数
 */
Snake::Snake() 
    : currentDirection_(Direction::NONE)
    , invincibleRounds_(0)
    , alive_(false)
    , growthPending_(0) {
}

/**
 * @brief 构造函数，初始化蛇的位置和长度
 * @param initialHead 蛇头的初始位置
 * @param initialLength 蛇的目标长度（必须>=1）
 * @throws std::invalid_argument 如果initialLength < 1
 * 
 * 实现说明：
 * - 初始时蛇只占一格（头部）
 * - 前 (initialLength-1) 次移动不会移除尾部，实现自然成长
 * - 这样避免了特殊判断，优雅地实现了初始化长度
 */
Snake::Snake(const Point& initialHead, int initialLength)
    : currentDirection_(Direction::NONE)
    , invincibleRounds_(0)
    , alive_(true)
    , growthPending_(initialLength - 1) {
    
    if (initialLength < 1) {
        throw std::invalid_argument("Snake initial length must be at least 1");
    }
    
    // 初始时只占一格（头部）
    blocks_.push_back(initialHead);
    blockSet_.insert(initialHead);
}

/**
 * @brief 移动蛇（使用当前设定的方向）
 * 
 * 移动逻辑：
 * 1. 检查存活状态和方向有效性
 * 2. 根据当前方向计算新的头部位置
 * 3. 将新头部添加到队列头部
 * 4. 根据待成长次数决定是否移除尾部
 * 
 * 注意：使用 setDirection() 设置移动方向，move() 不接收参数
 */
void Snake::move() {
    (void)moveWithDelta();
}

/**
 * @brief 移动蛇并返回增量信息（用于占用索引更新）
 */
Snake::MoveResult Snake::moveWithDelta() {
    MoveResult result;

    if (!alive_) {
        return result;
    }

    // 如果方向未设置，不进行移动
    if (currentDirection_ == Direction::NONE) {
        return result;
    }

    // 计算新的头部位置
    Point newHead = blocks_[0];
    switch (currentDirection_) {
        case Direction::UP:
            newHead.y -= 1;
            break;
        case Direction::DOWN:
            newHead.y += 1;
            break;
        case Direction::LEFT:
            newHead.x -= 1;
            break;
        case Direction::RIGHT:
            newHead.x += 1;
            break;
        case Direction::NONE:
            return result;  // 双重保险
    }

    result.moved = true;
    result.newHead = newHead;

    // 如果有待成长次数，下次移动不移除尾部：先插入新头
    if (growthPending_ > 0) {
        blocks_.push_front(newHead);
        blockSet_.insert(newHead);
        growthPending_--;
        result.tailRemoved = false;
    } else {
        // 非生长情况：先移除尾部再插入新头
        const Point tail = blocks_.back();
        blocks_.pop_back();
        blockSet_.erase(tail);

        blocks_.push_front(newHead);
        blockSet_.insert(newHead);

        result.tailRemoved = true;
        result.removedTail = tail;
    }

    return result;
}

/**
 * @brief 蛇成长（吃食物后调用）
 * 
 * 成长逻辑：增加待成长次数，下次移动时不移除尾部
 */
void Snake::grow() {
    growthPending_++;
}

/**
 * @brief 获取蛇头位置
 * @return 蛇头位置的常量引用
 * @note 调用前必须确保蛇是存活的（isAlive() == true）
 */
const Point& Snake::getHead() const {
    // 断言：蛇必须存活才能获取头部位置
    // 如果蛇已死亡（blocks_为空），这是程序逻辑错误
    if (blocks_.empty()) {
        throw std::logic_error("Cannot get head of a dead snake (blocks_ is empty)");
    }
    return blocks_[0];
}

/**
 * @brief 获取蛇的所有身体块
 */
const std::deque<Point>& Snake::getBlocks() const {
    return blocks_;
}

/**
 * @brief 获取蛇的长度
 */
int Snake::getLength() const {
    return static_cast<int>(blocks_.size());
}

/**
 * @brief 获取当前移动方向
 */
Direction Snake::getCurrentDirection() const {
    return currentDirection_;
}

/**
 * @brief 获取剩余无敌回合数
 */
int Snake::getInvincibleRounds() const {
    return invincibleRounds_;
}

/**
 * @brief 查询蛇是否存活
 */
bool Snake::isAlive() const {
    return alive_;
}

/**
 * @brief 设置移动方向
 * @param dir 新的移动方向
 */
void Snake::setDirection(Direction dir) {
    // 防止设置为相反方向
    if (currentDirection_ != Direction::NONE && 
        DirectionUtils::isOpposite(currentDirection_, dir)) {
        return;
    }
    currentDirection_ = dir;
}

/**
 * @brief 设置无敌回合数
 * @param rounds 无敌回合数
 */
void Snake::setInvincibleRounds(int rounds) {
    invincibleRounds_ = rounds;
}

/**
 * @brief 标记蛇为死亡状态（销毁）
 */
void Snake::kill() {
    alive_ = false;
    blocks_.clear();
    blockSet_.clear();
}

/**
 * @brief 减少无敌回合数
 */
void Snake::decreaseInvincibleRounds() {
    if (invincibleRounds_ > 0) {
        invincibleRounds_--;
    }
}

/**
 * @brief 检测指定点是否与蛇自身碰撞
 * @param point 要检测的点
 * @return 如果碰撞返回 true
 * 
 * 注意：这个方法用于检测蛇头是否撞到自己的身体
 * 从索引1开始检测（跳过头部blocks_[0]，检测身体部分）
 */
bool Snake::collidesWithSelf(const Point& point) const {
    if (blocks_.size() <= 1) {
        return false;
    }

    // 跳过头部，避免自判
    if (!blocks_.empty() && point == blocks_.front()) {
        return false;
    }

    // 使用哈希集合进行 O(1) 查询
    return blockSet_.find(point) != blockSet_.end();
}

/**
 * @brief 检测指定点是否与蛇身体碰撞（包括头部）
 * @param point 要检测的点
 * @return 如果碰撞返回 true
 * 
 * 注意：这个方法用于检测其他蛇头是否撞到本蛇的任何部分（包括头部）
 * 遍历所有块进行检测，确保与 blocks_ 保持一致
 */
bool Snake::collidesWithBody(const Point& point) const {
    // 使用哈希集合进行 O(1) 查询（包含头部）
    return blockSet_.find(point) != blockSet_.end();
}

/**
 * @brief 序列化为 JSON
 * @return JSON 对象
 */
nlohmann::json Snake::toJson() const {
    nlohmann::json j;
    
    j["blocks"] = nlohmann::json::array();
    for (const auto& block : blocks_) {
        j["blocks"].push_back(block.toJson());
    }
    
    j["direction"] = DirectionUtils::toString(currentDirection_);
    j["length"] = getLength();
    j["invincible_rounds"] = invincibleRounds_;
    j["alive"] = alive_;
    
    return j;
}

} // namespace snake

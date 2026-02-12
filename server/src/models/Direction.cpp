#include "models/Direction.h"
#include <stdexcept>
#include <algorithm>

namespace snake {

/**
 * @brief 将字符串转换为Direction枚举
 * @param str 方向字符串 ("UP", "DOWN", "LEFT", "RIGHT", "NONE")
 * @return Direction 对应的枚举值
 * @throws std::invalid_argument 如果字符串不是有效的方向
 */
Direction DirectionUtils::fromString(const std::string& str) {
    std::string upper_str = str;
    // 转换为大写
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    
    if (upper_str == "UP") {
        return Direction::UP;
    } else if (upper_str == "DOWN") {
        return Direction::DOWN;
    } else if (upper_str == "LEFT") {
        return Direction::LEFT;
    } else if (upper_str == "RIGHT") {
        return Direction::RIGHT;
    } else if (upper_str == "NONE") {
        return Direction::NONE;
    } else {
        throw std::invalid_argument("Invalid direction string: " + str);
    }
}

/**
 * @brief 将Direction枚举转换为字符串
 * @param dir Direction枚举值
 * @return std::string 对应的方向字符串
 */
std::string DirectionUtils::toString(Direction dir) {
    switch (dir) {
        case Direction::UP:
            return "UP";
        case Direction::DOWN:
            return "DOWN";
        case Direction::LEFT:
            return "LEFT";
        case Direction::RIGHT:
            return "RIGHT";
        case Direction::NONE:
            return "NONE";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief 判断两个方向是否相反
 * @param dir1 第一个方向
 * @param dir2 第二个方向
 * @return bool 如果两个方向相反返回true，否则返回false
 */
bool DirectionUtils::isOpposite(Direction dir1, Direction dir2) {
    return (dir1 == Direction::UP && dir2 == Direction::DOWN) ||
           (dir1 == Direction::DOWN && dir2 == Direction::UP) ||
           (dir1 == Direction::LEFT && dir2 == Direction::RIGHT) ||
           (dir1 == Direction::RIGHT && dir2 == Direction::LEFT);
}

/**
 * @brief 获取给定方向的相反方向
 * @param dir 输入方向
 * @return Direction 相反的方向
 */
Direction DirectionUtils::getOpposite(Direction dir) {
    switch (dir) {
        case Direction::UP:
            return Direction::DOWN;
        case Direction::DOWN:
            return Direction::UP;
        case Direction::LEFT:
            return Direction::RIGHT;
        case Direction::RIGHT:
            return Direction::LEFT;
        case Direction::NONE:
            return Direction::NONE;
        default:
            return Direction::NONE;
    }
}

} // namespace snake

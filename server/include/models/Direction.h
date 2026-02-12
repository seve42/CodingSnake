#pragma once

#include <string>

namespace snake {

/**
 * @brief 移动方向枚举
 */
enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE  // 未指定方向
};

/**
 * @brief 方向工具函数
 */
class DirectionUtils {
public:
    static Direction fromString(const std::string& str);
    static std::string toString(Direction dir);
    static bool isOpposite(Direction dir1, Direction dir2);
    static Direction getOpposite(Direction dir);
};

} // namespace snake

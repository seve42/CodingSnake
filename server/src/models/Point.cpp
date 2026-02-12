#include "models/Point.h"

namespace snake {

/**
 * @brief 比较两个点是否相等
 * @param other 另一个点
 * @return 如果x和y坐标都相等则返回true
 */
bool Point::operator==(const Point& other) const {
    return x == other.x && y == other.y;
}

/**
 * @brief 比较两个点是否不相等
 * @param other 另一个点
 * @return 如果x或y坐标不相等则返回true
 */
bool Point::operator!=(const Point& other) const {
    return !(*this == other);
}

bool Point::operator<(const Point& other) const {
    if (x == other.x) {
        return y < other.y;
    }
    return x < other.x;
}

/**
 * @brief 判断点是否为空点
 * @return 如果是空点则返回true
 */
bool Point::isNull() const {
    return x == -1 && y == -1;
}

/**
 * @brief 创建一个空点
 * @return 空点对象，使用(-1, -1)表示
 */
Point Point::Null() {
    return Point(-1, -1);
}

/**
 * @brief 将点序列化为JSON对象
 * @return JSON对象，格式为 {"x": x坐标, "y": y坐标}
 */
nlohmann::json Point::toJson() const {
    return nlohmann::json{
        {"x", x},
        {"y", y}
    };
}

/**
 * @brief 从JSON对象反序列化点
 * @param j JSON对象，应包含x和y字段
 * @return 反序列化的Point对象
 */
Point Point::fromJson(const nlohmann::json& j) {
    return Point(
        j.at("x").get<int>(),
        j.at("y").get<int>()
    );
}

} // namespace snake

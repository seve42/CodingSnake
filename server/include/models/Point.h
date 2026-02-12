#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>

namespace snake {

/**
 * @brief 二维坐标点
 */
struct Point {
    int x;
    int y;

    Point() : x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}

    bool operator==(const Point& other) const;
    bool operator!=(const Point& other) const;
    bool operator<(const Point& other) const;

    // 空点表示
    bool isNull() const;
    static Point Null();

    nlohmann::json toJson() const;
    static Point fromJson(const nlohmann::json& j);
};

/**
 * @brief Point 的哈希器（用于 unordered_set/unordered_map）
 */
struct PointHash {
    std::size_t operator()(const Point& p) const noexcept {
        // 使用 32 位拆分组合，兼容负数坐标
        const std::uint64_t ux = static_cast<std::uint64_t>(static_cast<std::uint32_t>(p.x));
        const std::uint64_t uy = static_cast<std::uint64_t>(static_cast<std::uint32_t>(p.y));
        return static_cast<std::size_t>((ux << 32) ^ uy);
    }
};

} // namespace snake

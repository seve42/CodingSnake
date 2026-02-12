#pragma once

#include "Point.h"
#include <nlohmann/json.hpp>

namespace snake {

/**
 * @brief 食物
 */
class Food {
public:
    Food();
    explicit Food(const Point& position);
    Food(int x, int y);

    const Point& getPosition() const;
    void setPosition(const Point& position);

    nlohmann::json toJson() const;

private:
    Point position_;
};

} // namespace snake

#include "models/Food.h"

namespace snake {

Food::Food() : position_(0, 0) {
}

Food::Food(const Point& position) : position_(position) {
}

Food::Food(int x, int y) : position_(x, y) {
}

const Point& Food::getPosition() const {
    return position_;
}

void Food::setPosition(const Point& position) {
    position_ = position;
}

nlohmann::json Food::toJson() const {
    return position_.toJson();
}

} // namespace snake

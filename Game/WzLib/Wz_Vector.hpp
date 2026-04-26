#pragma once
#include <cstdint>

namespace WzLibCpp {

class Wz_Vector {
public:
    Wz_Vector(int32_t x, int32_t y) : x(x), y(y) {}

    int32_t getX() const { return x; }
    void setX(int32_t val) { x = val; }

    int32_t getY() const { return y; }
    void setY(int32_t val) { y = val; }

private:
    int32_t x;
    int32_t y;
};

} // namespace WzLibCpp

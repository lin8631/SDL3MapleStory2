#pragma once
#include <vector>
#include <memory>

namespace WzLibCpp {

class Wz_Vector;

class Wz_Convex {
public:
    Wz_Convex(const std::vector<std::shared_ptr<Wz_Vector>>& points);

    const std::vector<std::shared_ptr<Wz_Vector>>& getPoints() const { return points; }
    void setPoints(const std::vector<std::shared_ptr<Wz_Vector>>& pts) { points = pts; }

private:
    std::vector<std::shared_ptr<Wz_Vector>> points;
};

} // namespace WzLibCpp

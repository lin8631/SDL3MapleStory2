#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace WzLibCpp {

class Wz_Image;

class Wz_Video {
public:
    Wz_Video(int64_t length, std::shared_ptr<Wz_Image> wz_i);

    int64_t getLength() const { return length; }
    void setLength(int64_t len) { length = len; }

    std::shared_ptr<Wz_Image> getWzImage() const { return wz_i.lock(); }
    void setWzImage(std::shared_ptr<Wz_Image> img) { wz_i = img; }

    int64_t copyTo(std::vector<uint8_t>& buffer, int64_t offset);

private:
    int64_t length;
    std::weak_ptr<Wz_Image> wz_i;
};

} // namespace WzLibCpp

#pragma once
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

namespace WzLibCpp {

class Wz_Image;
class IMapleStoryFile;

class Wz_Sound {
public:
    Wz_Sound(uint32_t offset, int32_t dataLength, int32_t ms, void* mediaType, std::shared_ptr<Wz_Image> wz_i);

    uint32_t getOffset() const { return offset; }
    void setOffset(uint32_t o) { offset = o; }

    int32_t getDataLength() const { return dataLength; }
    void setDataLength(int32_t len) { dataLength = len; }

    int32_t getMs() const { return ms; }
    void setMs(int32_t m) { ms = m; }

    int32_t getChannels() const;
    int32_t getFrequency() const;

    std::shared_ptr<Wz_Image> getWzImage() const { return wz_i.lock(); }
    void setWzImage(std::shared_ptr<Wz_Image> img) { wz_i = img; }

    std::shared_ptr<IMapleStoryFile> getWzFile() const;
    std::vector<uint8_t> extractSound();
    int64_t copyTo(std::vector<uint8_t>& buffer, int64_t offset);

private:
    uint32_t offset;
    int32_t dataLength;
    int32_t ms;
    void* mediaType;
    std::weak_ptr<Wz_Image> wz_i;
};

} // namespace WzLibCpp

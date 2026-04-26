#pragma once
#include <string>
#include <memory>
#include <cstdint>

namespace WzLibCpp {

class IMapleStoryFile;

class Wz_Directory {
public:
    Wz_Directory(const std::string& name, int32_t size, int32_t cs32,
                 uint32_t hashOff, uint32_t hashPos,
                 std::shared_ptr<IMapleStoryFile> wzFile);
    virtual ~Wz_Directory() = default;

    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }

    int32_t getSize() const { return size; }
    void setSize(int32_t s) { size = s; }

    int32_t getChecksum() const { return checksum; }
    void setChecksum(int32_t cs) { checksum = cs; }

    uint32_t getHashedOffset() const { return hashedOffset; }
    void setHashedOffset(uint32_t off) { hashedOffset = off; }

    uint32_t getHashedOffsetPosition() const { return hashedOffsetPosition; }
    void setHashedOffsetPosition(uint32_t pos) { hashedOffsetPosition = pos; }

    std::shared_ptr<IMapleStoryFile> getWzFile() const { return wzFile.lock(); }

private:
    std::string name;
    int32_t size;
    int32_t checksum;
    uint32_t hashedOffset;
    uint32_t hashedOffsetPosition;
    std::weak_ptr<IMapleStoryFile> wzFile;
};

} // namespace WzLibCpp

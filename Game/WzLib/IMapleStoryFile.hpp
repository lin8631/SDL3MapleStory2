#pragma once
#include <memory>
#include <cstdint>

namespace WzLibCpp {

class Wz_Structure;

class IMapleStoryFile {
public:
    virtual ~IMapleStoryFile() = default;

    virtual std::shared_ptr<Wz_Structure> getWzStructure() const = 0;
    virtual void* getFileStream() const = 0;
    virtual void* getReadLock() const = 0;
    virtual void close() = 0;
};

} // namespace WzLibCpp
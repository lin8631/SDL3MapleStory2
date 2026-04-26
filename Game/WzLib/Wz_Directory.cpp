#include "Wz_Directory.hpp"
#include "IMapleStoryFile.hpp"

namespace WzLibCpp {

Wz_Directory::Wz_Directory(const std::string& n, int32_t s, int32_t cs,
                          uint32_t hashOff, uint32_t hashPos,
                          std::shared_ptr<IMapleStoryFile> wz_f)
    : name(n), size(s), checksum(cs), hashedOffset(hashOff), 
      hashedOffsetPosition(hashPos), wzFile(wz_f) {
}

} // namespace WzLibCpp

#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "Wz_Crypto.hpp"

// Forward declarations
namespace WzLibCpp {
    class IMapleStoryFile;
    class Wz_Node;
}

namespace WzLibCpp {

class Wz_Image : public std::enable_shared_from_this<Wz_Image> {
public:
    Wz_Image(const std::string& name, int32_t size, int32_t cs32, 
             uint32_t hashOff, uint32_t hashPos, 
             std::shared_ptr<class Wz_File> wz_f);
    virtual ~Wz_Image() = default;

    // Properties
    const std::string& getName() const;
    void setName(const std::string& name);

    std::shared_ptr<Wz_File> getWzFile() const;
    void setWzFile(std::shared_ptr<Wz_File> wz_f);

    int32_t getSize() const;
    void setSize(int32_t size);

    int32_t getChecksum() const;
    void setChecksum(int32_t cs32);

    uint32_t getHashedOffset() const;
    void setHashedOffset(uint32_t hashOff);

    uint32_t getHashedOffsetPosition() const;
    void setHashedOffsetPosition(uint32_t hashPos);

    int64_t getOffset() const;
    void setOffset(int64_t offset);

    std::shared_ptr<Wz_Node> getNode() const;
    void setNode(std::shared_ptr<Wz_Node> node);

    std::shared_ptr<Wz_Node> getOwnerNode() const;
    void setOwnerNode(std::shared_ptr<Wz_Node> ownerNode);

    bool getIsChecksumChecked() const;
    void setIsChecksumChecked(bool isChecked);

    bool getIsExtracted() const;
    void setIsExtracted(bool isExtracted);

    bool getIsChecksumEncrypted() const;
    void setIsChecksumEncrypted(bool isEncrypted);

    Wz_CryptoKeyType getEncType() const { return encType; }
    void setEncType(Wz_CryptoKeyType type) { encType = type; }

    // Methods
    virtual bool tryExtract();
    virtual std::vector<uint8_t> extractPng();
    virtual std::vector<uint8_t> extractSound();
    virtual void copyTo(std::vector<uint8_t>& data, int64_t offset);
    virtual int64_t getDataLength() const;

protected:
    std::string Name;
    std::shared_ptr<Wz_File> WzFile;
    int32_t Size;
    int32_t Checksum;
    uint32_t HashedOffset;
    uint32_t HashedOffsetPosition;
    int64_t Offset;

    std::weak_ptr<Wz_Node> Node;
    std::shared_ptr<Wz_Node> ExtractedNode;
    std::weak_ptr<Wz_Node> OwnerNode;

    bool isExtracted;
    bool isChecksumChecked;
    bool isChecksumEncrypted;
    Wz_CryptoKeyType encType;
};

} // namespace WzLibCpp
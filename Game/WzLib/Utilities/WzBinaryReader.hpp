#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

namespace WzLibCpp::Utilities {

class IWzStringPool;

class WzBinaryReader {
public:
    WzBinaryReader(std::shared_ptr<std::istream> stream, bool useStringPool);
    WzBinaryReader(std::shared_ptr<std::istream> stream, std::shared_ptr<IWzStringPool> stringPool);
    ~WzBinaryReader();

    std::shared_ptr<std::istream> getBaseStream() const { return stream; }
    void setStringReferenceOffsetBytes(int bytes) { StringReferenceOffsetBytes = bytes; }
    int StringReferenceOffsetBytes = 0;

    uint8_t readByte();
    int8_t readSByte();
    int16_t readInt16();
    uint16_t readUInt16();
    int32_t readCompressedInt32();
    int32_t readInt32();
    uint32_t readUInt32();
    int64_t readInt64();
    uint64_t readUInt64();
    float readSingle();
    double readDouble();
    std::string readString();
    std::string readString(int32_t length);
    std::string readStringBlock();
    std::vector<uint8_t> readBytes(int32_t count);
    std::vector<uint8_t> readAvailableBytes();

    void readFully(uint8_t* buffer, size_t count);

    int64_t getPosition() const;
    void setPosition(int64_t pos);
    int64_t getLength() const;

private:
    std::shared_ptr<std::istream> stream;
    std::shared_ptr<IWzStringPool> stringPool;
};

} // namespace WzLibCpp::Utilities

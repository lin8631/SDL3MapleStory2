#pragma once
#include <fstream>
#include <memory>
#include <vector>
#include <cstring>

namespace WzLibCpp {

/**
 * PartialStream - 偏移量转换流
 * 对应 C# 代码中的 PartialStream
 */
class PartialStream {
public:
    PartialStream(std::shared_ptr<std::fstream> baseStream, int64_t offset, int64_t size)
        : baseStream(baseStream), offset(offset), size(size), position(0) {}

    int64_t getPosition() const { return position; }
    void setPosition(int64_t pos) { 
        position = pos; 
        baseStream->seekg(offset + position);
    }

    int64_t getLength() const { return size; }

    int readByte() {
        if (position >= size) return -1;
        baseStream->seekg(offset + position);
        int result = baseStream->get();
        if (result != -1) position++;
        return result;
    }

    bool readBytes(char* buffer, int64_t count) {
        if (position + count > size) return false;
        baseStream->seekg(offset + position);
        baseStream->read(buffer, count);
        position += count;
        return baseStream->good();
    }

    std::vector<uint8_t> readBytes(int64_t count) {
        std::vector<uint8_t> result(count);
        if (!readBytes(reinterpret_cast<char*>(result.data()), count)) {
            return {};
        }
        return result;
    }

    int64_t readInt32() {
        uint8_t bytes[4];
        if (!readBytes(reinterpret_cast<char*>(bytes), 4)) return 0;
        return static_cast<int32_t>(bytes[0]) |
               (static_cast<int32_t>(bytes[1]) << 8) |
               (static_cast<int32_t>(bytes[2]) << 16) |
               (static_cast<int32_t>(bytes[3]) << 24);
    }

    int8_t readSByte() {
        int result = readByte();
        return static_cast<int8_t>(result);
    }

private:
    std::shared_ptr<std::fstream> baseStream;
    int64_t offset;
    int64_t size;
    int64_t position;
};

} // namespace WzLibCpp

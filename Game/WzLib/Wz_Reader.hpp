#pragma once
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include "Wz_Value.hpp"
#include "Wz_Crypto.hpp"

namespace WzLibCpp {

/**
 * PartialStream - 部分流，支持偏移量转换
 * 对应 C# 代码中的 PartialStream
 */
class PartialStream : public std::streambuf {
public:
    PartialStream(std::shared_ptr<std::fstream> baseStream, int64_t offset, int64_t length)
        : baseStream(baseStream), offset(offset), length(length), position(0) {
    }

    int64_t getLength() const { return length; }
    int64_t getPosition() const { return position; }

    bool seek(int64_t pos) {
        if (pos < 0 || pos > length) return false;
        position = pos;
        baseStream->seekg(offset + position);
        return baseStream->good();
    }

    bool read(char* buffer, int64_t count) {
        if (position + count > length) return false;
        baseStream->read(buffer, count);
        position += count;
        return baseStream->good();
    }

    int readByte() {
        char c;
        if (!read(&c, 1)) return -1;
        return static_cast<unsigned char>(c);
    }

    std::vector<uint8_t> readBytes(int64_t count) {
        std::vector<uint8_t> result(count);
        if (!read(reinterpret_cast<char*>(result.data()), count)) {
            result.clear();
        }
        return result;
    }

private:
    std::shared_ptr<std::fstream> baseStream;
    int64_t offset;
    int64_t length;
    int64_t position;
};

/**
 * WzBinaryReader - 二进制读取器
 * 对应 C# 代码中的 WzBinaryReader
 */
class WzBinaryReader {
public:
    WzBinaryReader(std::shared_ptr<PartialStream> stream, std::shared_ptr<IWzDecrypter> decrypter)
        : stream(stream), decrypter(decrypter) {}

    int64_t getPosition() const { return stream->getPosition(); }
    void setPosition(int64_t pos) { stream->seek(pos); }

    // 读取基本类型
    uint8_t readByte() {
        return static_cast<uint8_t>(stream->readByte());
    }

    int8_t readSByte() {
        return static_cast<int8_t>(stream->readByte());
    }

    int16_t readInt16() {
        uint8_t bytes[2];
        stream->read(reinterpret_cast<char*>(bytes), 2);
        return static_cast<int16_t>(bytes[0] | (bytes[1] << 8));
    }

    uint16_t readUInt16() {
        uint8_t bytes[2];
        stream->read(reinterpret_cast<char*>(bytes), 2);
        return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
    }

    int32_t readInt32() {
        uint8_t bytes[4];
        stream->read(reinterpret_cast<char*>(bytes), 4);
        return static_cast<int32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
    }

    uint32_t readUInt32() {
        uint8_t bytes[4];
        stream->read(reinterpret_cast<char*>(bytes), 4);
        return static_cast<uint32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
    }

    int64_t readInt64() {
        uint8_t bytes[8];
        stream->read(reinterpret_cast<char*>(bytes), 8);
        return static_cast<int64_t>(bytes[0]) | (static_cast<int64_t>(bytes[1]) << 8) |
               (static_cast<int64_t>(bytes[2]) << 16) | (static_cast<int64_t>(bytes[3]) << 24) |
               (static_cast<int64_t>(bytes[4]) << 32) | (static_cast<int64_t>(bytes[5]) << 40) |
               (static_cast<int64_t>(bytes[6]) << 48) | (static_cast<int64_t>(bytes[7]) << 56);
    }

    float readFloat() {
        uint32_t bits = readUInt32();
        float result;
        std::memcpy(&result, &bits, 4);
        return result;
    }

    double readDouble() {
        uint64_t bits = static_cast<uint64_t>(readInt64());
        double result;
        std::memcpy(&result, &bits, 8);
        return result;
    }

    // 压缩整数格式
    int32_t readCompressedInt32() {
        int8_t s = readSByte();
        return (s == -128) ? readInt32() : s;
    }

    int64_t readCompressedInt64() {
        int8_t s = readSByte();
        return (s == -128) ? readInt64() : s;
    }

    // 跳过字节
    void skipBytes(int64_t count) {
        setPosition(getPosition() + count);
    }

    // 读取字符串（ASCII）
    std::string readString(int32_t length) {
        if (length <= 0) return "";
        
        std::vector<uint8_t> data = stream->readBytes(length);
        if (decrypter) {
            decrypter->decrypt(data, 0);
        }
        
        std::string result;
        result.reserve(length);
        uint8_t mask = 0xAA;
        for (int32_t i = 0; i < length; i++) {
            result += static_cast<char>(data[i] ^ mask);
            mask++;
        }
        return result;
    }

    // 读取字符串（UTF-16LE）
    std::string readStringUtf16(int32_t length) {
        if (length <= 0) return "";
        
        int32_t byteLength = length * 2;
        std::vector<uint8_t> data = stream->readBytes(byteLength);
        if (decrypter) {
            decrypter->decrypt(data, 0);
        }
        
        std::string result;
        result.reserve(length);
        uint16_t mask = 0xAAAA;
        for (int32_t i = 0; i < length; i++) {
            uint16_t wchar = static_cast<uint16_t>(data[i * 2]) | (static_cast<uint16_t>(data[i * 2 + 1]) << 8);
            wchar ^= mask;
            // 简单 ASCII 转换
            if (wchar < 128) {
                result += static_cast<char>(wchar);
            } else {
                result += '?';
            }
            mask++;
        }
        return result;
    }

    // 读取字符串（自动检测编码）
    std::string readString() {
        int8_t firstByte = readSByte();
        
        if (firstByte == -128) {
            // 长字符串（4字节长度）
            int32_t length = readInt32();
            return readString(length);
        } else if (firstByte < 0) {
            // ASCII 字符串
            int32_t length = -firstByte;
            return readString(length);
        } else if (firstByte > 0) {
            // UTF-16LE 字符串
            int32_t length = firstByte;
            if (length == 127) {
                length = readInt32();
            }
            return readStringUtf16(length);
        }
        return "";
    }

    // 读取图像对象类型名称
    std::string readImageObjectTypeName() {
        int8_t flag = readSByte();
        switch (flag) {
            case 0x73:
                return readString();
            case 0x1B:
                return readStringAt(readInt32() + stringReferenceOffsetBytes);
            default:
                return "";
        }
    }

    // 读取图像字符串（带标志）
    std::string readImageString() {
        int8_t flag = readSByte();
        switch (flag) {
            case 0x00:
                return readString();
            case 0x01:
                return readStringAt(readInt32() + stringReferenceOffsetBytes);
            case 0x04:
                skipBytes(8);
                return "";
            default:
                return "";
        }
    }

    // 读取指定位置的字符串
    std::string readStringAt(int64_t offset) {
        int64_t oldPos = getPosition();
        setPosition(offset);
        std::string result = readString();
        setPosition(oldPos);
        return result;
    }

    // 读取字节数组
    std::vector<uint8_t> readBytes(int64_t count) {
        return stream->readBytes(count);
    }

    // 设置字符串引用偏移
    void setStringReferenceOffsetBytes(int32_t offset) {
        stringReferenceOffsetBytes = offset;
    }

private:
    std::shared_ptr<PartialStream> stream;
    std::shared_ptr<IWzDecrypter> decrypter;
    int32_t stringReferenceOffsetBytes = 2;
};

} // namespace WzLibCpp

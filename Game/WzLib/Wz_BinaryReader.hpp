#pragma once
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "Wz_Crypto.hpp"
#include "Wz_Value.hpp"

namespace WzLibCpp {

class PartialStream {
public:
    PartialStream(std::shared_ptr<std::fstream> baseStream, int64_t offset, int64_t length)
        : baseStream(baseStream), offset(offset), length(length), position(0) {}

    int64_t getPosition() const { return position; }
    void setPosition(int64_t pos) {
        position = pos;
        baseStream->seekg(offset + position, std::ios::beg);
    }

    int64_t getLength() const { return length; }
    int64_t getOffset() const { return offset; }
    bool eof() const { return position >= length; }

    bool read(char* buffer, size_t count) {
        if (position + static_cast<int64_t>(count) > length) {
            size_t available = length - position;
            if (available == 0) return false;
            baseStream->seekg(offset + position, std::ios::beg);
            baseStream->read(buffer, available);
            position += available;
            return baseStream->good();
        }
        baseStream->seekg(offset + position, std::ios::beg);
        baseStream->read(buffer, count);
        position += count;
        return baseStream->good();
    }

    int readByte() {
        if (position >= length) return -1;
        baseStream->seekg(offset + position, std::ios::beg);
        int result = baseStream->get();
        if (result != std::char_traits<char>::eof()) position++;
        return result;
    }

    std::vector<uint8_t> readBytes(size_t count) {
        std::vector<uint8_t> result(count);
        size_t actual = 0;
        if (!read(reinterpret_cast<char*>(result.data()), count)) {
            result.resize(0);
        }
        return result;
    }

    void skip(size_t count) {
        position += count;
        if (position > length) position = length;
    }

    std::shared_ptr<std::fstream> getBaseStream() const { return baseStream; }

private:
    std::shared_ptr<std::fstream> baseStream;
    int64_t offset;
    int64_t length;
    int64_t position;
};

class WzBinaryReader {
public:
    WzBinaryReader(std::shared_ptr<PartialStream> stream, std::shared_ptr<IWzDecrypter> decrypter = nullptr)
        : stream_(stream), decrypter_(decrypter) {}

    int64_t getPosition() const { return stream_->getPosition(); }
    void setPosition(int64_t pos) { stream_->setPosition(pos); }
    bool eof() const { return stream_->eof(); }

    std::shared_ptr<IWzDecrypter> getDecrypter() const { return decrypter_; }
    void setDecrypter(std::shared_ptr<IWzDecrypter> dec) { decrypter_ = dec; }

    std::shared_ptr<PartialStream> getPartialStream() const { return stream_; }

    uint8_t readByte() {
        int result = stream_->readByte();
        return static_cast<uint8_t>(result >= 0 ? result : 0);
    }

    int8_t readSByte() {
        int result = stream_->readByte();
        return static_cast<int8_t>(result);
    }

    int16_t readInt16() {
        uint8_t bytes[2];
        if (!stream_->read(reinterpret_cast<char*>(bytes), 2)) return 0;
        return static_cast<int16_t>(bytes[0] | (bytes[1] << 8));
    }

    uint16_t readUInt16() {
        uint8_t bytes[2];
        if (!stream_->read(reinterpret_cast<char*>(bytes), 2)) return 0;
        return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
    }

    int32_t readInt32() {
        uint8_t bytes[4];
        if (!stream_->read(reinterpret_cast<char*>(bytes), 4)) return 0;
        return static_cast<int32_t>(bytes[0] | (bytes[1] << 8) |
                                    (bytes[2] << 16) | (bytes[3] << 24));
    }

    uint32_t readUInt32() {
        uint8_t bytes[4];
        if (!stream_->read(reinterpret_cast<char*>(bytes), 4)) return 0;
        return static_cast<uint32_t>(bytes[0] | (bytes[1] << 8) |
                                     (bytes[2] << 16) | (bytes[3] << 24));
    }

    int32_t readCompressedInt32() {
        int8_t s = readSByte();
        return (s == -128) ? readInt32() : s;
    }

    int64_t readCompressedInt64() {
        int8_t s = readSByte();
        if (s == -128) {
            int64_t low = static_cast<int64_t>(static_cast<uint32_t>(readInt32()));
            int64_t high = static_cast<int64_t>(static_cast<uint32_t>(readInt32()));
            return low | (high << 32);
        }
        return s;
    }

    float readCompressedSingle() {
        int32_t value = readCompressedInt32();
        float result;
        std::memcpy(&result, &value, sizeof(float));
        return result;
    }

    double readDouble() {
        uint8_t bytes[8];
        if (!stream_->read(reinterpret_cast<char*>(bytes), 8)) return 0.0;
        uint64_t value = static_cast<uint64_t>(bytes[0]) |
                        (static_cast<uint64_t>(bytes[1]) << 8) |
                        (static_cast<uint64_t>(bytes[2]) << 16) |
                        (static_cast<uint64_t>(bytes[3]) << 24) |
                        (static_cast<uint64_t>(bytes[4]) << 32) |
                        (static_cast<uint64_t>(bytes[5]) << 40) |
                        (static_cast<uint64_t>(bytes[6]) << 48) |
                        (static_cast<uint64_t>(bytes[7]) << 56);
        double result;
        std::memcpy(&result, &value, sizeof(double));
        return result;
    }

    std::string readImageObjectTypeName() {
        uint8_t flag = readByte();
        switch (flag) {
            case 0x73:
                return readString();
            case 0x1B: {
                int64_t currentPos = stream_->getPosition();
                int32_t refOffset = readInt32();
                // refOffset 是字符串在字符串池中的绝对位置
                return readStringAt(refOffset);
            }
            default:
                throw std::runtime_error("Unknown flag in readImageObjectTypeName: " + std::to_string(flag));
        }
    }

    std::string readImageString() {
        uint8_t flag = readByte();
        switch (flag) {
            case 0x00:
                return readString();
            case 0x01: {
                int64_t currentPos = stream_->getPosition();
                int32_t refOffset = readInt32();
                return readStringAt(refOffset);
            }
            case 0x04:
                skipBytes(8);
                return "";
            default:
                return "";
        }
    }

    std::string readString() {
        int8_t size = readSByte();
        if (size < 0) {
            int32_t length = (size == -128) ? readInt32() : static_cast<int32_t>(-size);
            return readAsciiString(length);
        } else if (size > 0) {
            int32_t length = (size == 127) ? readInt32() : size;
            return readUtf16String(length);
        }
        return "";
    }

    std::string readStringAt(int64_t relOffset) {
        int64_t oldPos = stream_->getPosition();
        stream_->setPosition(relOffset);
        std::string result = readString();
        stream_->setPosition(oldPos);
        return result;
    }

    void skipBytes(size_t count) {
        stream_->skip(count);
    }

    std::vector<uint8_t> readBytes(size_t count) {
        return stream_->readBytes(count);
    }

private:
    std::shared_ptr<PartialStream> stream_;
    std::shared_ptr<IWzDecrypter> decrypter_;

    std::string readAsciiString(int32_t length) {
        if (length <= 0) return "";
        std::vector<uint8_t> buffer = stream_->readBytes(length);
        if (buffer.empty()) return "";

        if (decrypter_) {
            decrypter_->decrypt(buffer, 0);
        }

        std::string result;
        result.reserve(length);
        uint8_t mask = 0xAA;
        for (size_t i = 0; i < buffer.size(); i++) {
            result += static_cast<char>(buffer[i] ^ mask);
            mask++;
        }
        return result;
    }

    std::string readUtf16String(int32_t length) {
        if (length <= 0) return "";
        std::vector<uint8_t> buffer = stream_->readBytes(length * 2);
        if (buffer.empty()) return "";

        if (decrypter_) {
            decrypter_->decrypt(buffer, 0);
        }

        std::string result;
        result.reserve(length);
        uint16_t mask = 0xAAAA;
        for (int32_t i = 0; i < length; i++) {
            uint16_t wchar = static_cast<uint16_t>(buffer[i * 2]) |
                            (static_cast<uint16_t>(buffer[i * 2 + 1]) << 8);
            wchar ^= mask;
            if (wchar < 128) {
                result += static_cast<char>(wchar);
            } else {
                result += '?';
            }
            mask++;
        }
        return result;
    }
};

std::shared_ptr<IWzDecrypter> TryDetectEncryption(std::shared_ptr<PartialStream> stream);

} // namespace WzLibCpp

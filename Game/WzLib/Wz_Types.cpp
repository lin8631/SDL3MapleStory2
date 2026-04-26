#include "Wz_Types.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <fstream>

namespace WzLibCpp {

Wz_Sound::Wz_Sound(uint32_t offset, int dataLength, int ms, std::weak_ptr<Wz_Image> wzImage)
    : Offset(offset)
    , DataLength(dataLength)
    , Ms(ms)
    , WzImage(wzImage) {
}

std::shared_ptr<Wz_File> Wz_Sound::getWzFile() const {
    auto img = WzImage.lock();
    if (img) {
        return img->getWzFile();
    }
    return nullptr;
}

std::vector<uint8_t> Wz_Sound::extractSound() {
    auto img = WzImage.lock();
    if (!img) {
        return {};
    }
    
    auto wzFile = img->getWzFile();
    if (!wzFile) {
        return {};
    }
    
    auto fStream = wzFile->getFStream();
    if (!fStream || !fStream->is_open()) {
        return {};
    }
    
    fStream->seekg(0, std::ios::end);
    int64_t fileSize = fStream->tellg();
    
    int64_t readOffset = img->getOffset() + Offset;
    if (readOffset < 0 || readOffset + DataLength > fileSize) {
        return {};
    }
    
    std::vector<uint8_t> data(DataLength);
    fStream->seekg(readOffset, std::ios::beg);
    fStream->read(reinterpret_cast<char*>(data.data()), DataLength);
    
    if (fStream->gcount() != DataLength) {
        return {};
    }
    
    return data;
}

Wz_Vector::Wz_Vector(int x, int y)
    : X(x)
    , Y(y) {
}

Wz_Uol::Wz_Uol(const std::string& uol)
    : Uol(uol) {
}

std::shared_ptr<Wz_Node> Wz_Uol::resolve(std::shared_ptr<Wz_Node> currentNode) {
    return nullptr;
}

Wz_RawData::Wz_RawData(uint32_t offset, int length, std::weak_ptr<Wz_Image> wzImage)
    : Offset(offset)
    , Length(length)
    , WzImage(wzImage) {
}

std::shared_ptr<Wz_File> Wz_RawData::getWzFile() const {
    auto img = WzImage.lock();
    if (img) {
        return img->getWzFile();
    }
    return nullptr;
}

void Wz_RawData::copyTo(std::vector<uint8_t>& buffer, int offset) {
    auto img = WzImage.lock();
    if (!img) {
        throw std::runtime_error("Wz_Image expired");
    }
    
    auto wzFile = img->getWzFile();
    if (!wzFile) {
        throw std::runtime_error("Wz_File expired");
    }
    
    auto fStream = wzFile->getFStream();
    if (!fStream || !fStream->is_open()) {
        throw std::runtime_error("File stream not open");
    }
    
    int64_t readOffset = img->getOffset() + Offset;
    if (offset + Length > static_cast<int>(buffer.size())) {
        throw std::runtime_error("Insufficient buffer size");
    }
    
    fStream->seekg(readOffset, std::ios::beg);
    fStream->read(reinterpret_cast<char*>(buffer.data() + offset), Length);
    
    if (fStream->gcount() != Length) {
        throw std::runtime_error("Failed to read all data");
    }
}

void Wz_RawData::copyTo(std::vector<uint8_t>& buffer) {
    copyTo(buffer, 0);
}

Wz_Video::Wz_Video(uint32_t offset, int length, std::weak_ptr<Wz_Image> wzImage)
    : Offset(offset)
    , Length(length)
    , WzImage(wzImage) {
}

std::shared_ptr<Wz_File> Wz_Video::getWzFile() const {
    auto img = WzImage.lock();
    if (img) {
        return img->getWzFile();
    }
    return nullptr;
}

void Wz_Video::copyTo(std::vector<uint8_t>& buffer, int offset) {
    auto img = WzImage.lock();
    if (!img) {
        throw std::runtime_error("Wz_Image expired");
    }
    
    auto wzFile = img->getWzFile();
    if (!wzFile) {
        throw std::runtime_error("Wz_File expired");
    }
    
    auto fStream = wzFile->getFStream();
    if (!fStream || !fStream->is_open()) {
        throw std::runtime_error("File stream not open");
    }
    
    int64_t readOffset = img->getOffset() + Offset;
    if (offset + Length > static_cast<int>(buffer.size())) {
        throw std::runtime_error("Insufficient buffer size");
    }
    
    fStream->seekg(readOffset, std::ios::beg);
    fStream->read(reinterpret_cast<char*>(buffer.data() + offset), Length);
    
    if (fStream->gcount() != Length) {
        throw std::runtime_error("Failed to read all data");
    }
}

void Wz_Video::copyTo(std::vector<uint8_t>& buffer) {
    copyTo(buffer, 0);
}

Wz_Convex::Wz_Convex() {
}

void Wz_Convex::addPoint(std::shared_ptr<Wz_Vector> point) {
    Points.push_back(point);
}

} // namespace WzLibCpp

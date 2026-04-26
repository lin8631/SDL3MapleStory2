#include "Wz_Png.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"
#include "Wz_Structure.hpp"
#include "Wz_Types.hpp"
#include "Wz_Header.hpp"
#include "Wz_Crypto.hpp"

#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include <memory>
#include <fstream>
#include <cstring>
#include <zlib.h>
#include <stdexcept>

namespace WzLibCpp {

static bool isZlibHeader(uint16_t header) {
    return header == 0x9C78 || header == 0x789C ||
           header == 0x785E || header == 0x787C ||
           header == 0x78DA || header == 0x7801 ||
           header == 0x7800;
}

std::vector<uint8_t> Wz_Png::getCompressedData(int32_t skipBytes) {
    auto fStream = BaseStream.lock();
    if (!fStream || !fStream->is_open()) {
        auto wzFile = std::dynamic_pointer_cast<Wz_File>(getWzFile());
        if (!wzFile) return {};
        fStream = wzFile->getFStream();
        if (!fStream || !fStream->is_open()) return {};
    }
    
    auto wzImg = WzImage.lock();
    if (!wzImg) return {};
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(getWzFile());
    if (!wzFile) return {};
    
    auto wzStruct = wzFile->getWzStructure();
    if (!wzStruct) return {};
    
    auto crypto = wzStruct->getEncryption();
    if (!crypto) return {};
    
    // auto kmsDecrypter = crypto->getKeyByType(Wz_CryptoKeyType::KMS);
    // auto gmsDecrypter = crypto->getKeyByType(Wz_CryptoKeyType::GMS);
    
    int64_t wzImgOffset = wzImg->getOffset();
    size_t dataStart = wzImgOffset + Offset;
    size_t dataEnd = dataStart + DataLength;
    
    // SDL_Log("Wz_Png::getCompressedData: wzImgOffset=%lld, pngOffset=%u, dataStart=%zu, dataLen=%d",
    //         (long long)wzImgOffset, Offset, dataStart, DataLength);
    
    fStream->seekg(dataStart, std::ios::beg);
    uint8_t firstBytes[4];
    if (!fStream->read(reinterpret_cast<char*>(firstBytes), 4)) {
        return {};
    }
    
    uint16_t headerAtStart = (firstBytes[0] << 8) | firstBytes[1];
    uint16_t headerAfterFirst = (firstBytes[1] << 8) | firstBytes[2];
    
    if (isZlibHeader(headerAtStart)) {
        std::vector<uint8_t> zlibData(DataLength);
        fStream->seekg(dataStart, std::ios::beg);
        fStream->read(reinterpret_cast<char*>(zlibData.data()), DataLength);
        return zlibData;
    }
    
    if (isZlibHeader(headerAfterFirst)) {
        // SDL_Log("PNG: zlib header at offset 1, skipping first byte");
        std::vector<uint8_t> zlibData(DataLength - 1);
        fStream->seekg(dataStart + 1, std::ios::beg);
        fStream->read(reinterpret_cast<char*>(zlibData.data()), DataLength - 1);
        return zlibData;
    }
    
    if (firstBytes[1] == 0x78) {
        // SDL_Log("PNG: second byte is 0x78, skipping first byte");
        std::vector<uint8_t> zlibData(DataLength - 1);
        fStream->seekg(dataStart + 1, std::ios::beg);
        fStream->read(reinterpret_cast<char*>(zlibData.data()), DataLength - 1);
        return zlibData;
    }
    
    // SDL_Log("PNG: no zlib header found, returning raw data");
    fStream->seekg(dataStart, std::ios::beg);
    std::vector<uint8_t> raw(DataLength);
    fStream->read(reinterpret_cast<char*>(raw.data()), DataLength);
    return raw;
}

Wz_Png::Wz_Png(int32_t w, int32_t h, int32_t dataLength,
               Wz_TextureFormat format, int32_t scale, int32_t pages,
               uint32_t offs, std::shared_ptr<Wz_Image> wz_i)
    : Width(w), Height(h), DataLength(dataLength), Format(format),
      Scale(scale), Pages(pages), Offset(offs), IsEncrypted(false), WzImage(wz_i) {
}

Wz_Png::Wz_Png(int32_t w, int32_t h, int32_t dataLength,
               Wz_TextureFormat format, int32_t scale, int32_t pages,
               uint32_t offs, std::shared_ptr<Wz_Image> wz_i,
               std::shared_ptr<std::fstream> baseStream)
    : Width(w), Height(h), DataLength(dataLength), Format(format),
      Scale(scale), Pages(pages), Offset(offs), IsEncrypted(false), WzImage(wz_i), BaseStream(baseStream) {
}

std::shared_ptr<IMapleStoryFile> Wz_Png::getWzFile() const {
    auto wzImg = WzImage.lock();
    if (wzImg) {
        return wzImg->getWzFile();
    }
    return nullptr;
}

int32_t Wz_Png::getRawDataSize() const {
    return getUncompressedDataSize(Format, Width, Height);
}

int32_t Wz_Png::getRawDataSizePerPage() const {
    return getUncompressedDataSize(Format, Scale, Width, Height);
}

int32_t Wz_Png::getUncompressedDataSize(Wz_TextureFormat format, int32_t width, int32_t height) {
    return getUncompressedDataSize(format, 0, width, height);
}

int32_t Wz_Png::getUncompressedDataSize(Wz_TextureFormat format, int32_t scale, int32_t width, int32_t height) {
    int32_t s = scale > 0 ? (1 << scale) : 1;
    int32_t w = (width + s - 1) / s;
    int32_t h = (height + s - 1) / s;
    int32_t bpp = 4;
    if (format == Wz_TextureFormat::DXT3) {
        bpp = 8;
    }
    return w * h * bpp;
}

std::vector<uint8_t> Wz_Png::getRawData() {
    std::vector<uint8_t> buffer;
    getRawData(buffer);
    return buffer;
}

int32_t Wz_Png::getRawData(std::vector<uint8_t>& buffer) {
    return getRawData(0, buffer);
}

int32_t Wz_Png::getRawData(int32_t skipBytes, std::vector<uint8_t>& buffer) {
    auto compressed = getCompressedData(skipBytes);
    if (compressed.empty()) {
        return -1;
    }

    if (compressed.size() >= 8 && compressed[0] == 0x89 && compressed[1] == 0x50) {
        // SDL_Log("PNG: data is PNG format");
        buffer = compressed;
        return static_cast<int32_t>(buffer.size());
    }

    int32_t rawSize = getRawDataSize();
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressed.size();
    strm.next_in = const_cast<uint8_t*>(compressed.data());
    
    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        return -1;
    }
    
    buffer.resize(rawSize);
    strm.avail_out = rawSize;
    strm.next_out = buffer.data();
    
    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret != Z_STREAM_END && ret != Z_OK) {
        inflateEnd(&strm);
        buffer.clear();
        return -1;
    }
    
    uLongf destLen = rawSize - strm.avail_out;
    inflateEnd(&strm);
    buffer.resize(destLen);
    return destLen;
}

std::vector<uint8_t> Wz_Png::extractPng() {
    return getCompressedData(0);
}

} // namespace WzLibCpp

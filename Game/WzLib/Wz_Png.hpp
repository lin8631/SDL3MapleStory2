#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include "Wz_Types.hpp"

namespace WzLibCpp {

class IMapleStoryFile;

class Wz_Png {
public:
    Wz_Png(int32_t w, int32_t h, int32_t dataLength, 
           Wz_TextureFormat format, int32_t scale, int32_t pages, 
           uint32_t offs, std::shared_ptr<Wz_Image> wz_i);

    Wz_Png(int32_t w, int32_t h, int32_t dataLength, 
           Wz_TextureFormat format, int32_t scale, int32_t pages, 
           uint32_t offs, std::shared_ptr<Wz_Image> wz_i,
           std::shared_ptr<std::fstream> baseStream);

    int32_t getWidth() const { return Width; }
    void setWidth(int32_t w) { Width = w; }

    int32_t getHeight() const { return Height; }
    void setHeight(int32_t h) { Height = h; }

    int32_t getDataLength() const { return DataLength; }
    void setDataLength(int32_t len) { DataLength = len; }

    uint32_t getOffset() const { return Offset; }
    void setOffset(uint32_t offs) { Offset = offs; }

    Wz_TextureFormat getFormat() const { return Format; }
    void setFormat(Wz_TextureFormat format) { Format = format; }

    int32_t getScale() const { return Scale; }
    void setScale(int32_t scale) { Scale = scale; }

    int32_t getActualScale() const { return Scale > 0 ? (1 << Scale) : 1; }

    int32_t getPages() const { return Pages; }
    void setPages(int32_t pages) { Pages = pages; }

    int32_t getActualPages() const { return Pages > 0 ? Pages : 1; }

    std::shared_ptr<IMapleStoryFile> getWzFile() const;
    std::shared_ptr<Wz_Image> getWzImage() const { return WzImage.lock(); }
    void setWzImage(std::shared_ptr<Wz_Image> img) { WzImage = img; }

    void setBaseStream(std::shared_ptr<std::fstream> stream) { BaseStream = stream; }

    void setIsEncrypted(bool encrypted) { IsEncrypted = encrypted; }
    bool getIsEncrypted() const { return IsEncrypted; }

    int32_t getRawDataSize() const;
    int32_t getRawDataSizePerPage() const;

    std::vector<uint8_t> getRawData();
    int32_t getRawData(std::vector<uint8_t>& buffer);
    int32_t getRawData(int32_t skipBytes, std::vector<uint8_t>& buffer);

    std::vector<uint8_t> getCompressedData(int32_t skipBytes = 0);

    std::vector<uint8_t> extractPng();

    static int32_t getUncompressedDataSize(Wz_TextureFormat format, int32_t width, int32_t height);
    static int32_t getUncompressedDataSize(Wz_TextureFormat format, int32_t scale, int32_t width, int32_t height);

private:
    int32_t Width;
    int32_t Height;
    int32_t DataLength;
    uint32_t Offset;
    Wz_TextureFormat Format;
    int32_t Scale;
    int32_t Pages;
    bool IsEncrypted;
    std::weak_ptr<Wz_Image> WzImage;
    std::weak_ptr<std::fstream> BaseStream;
};

} // namespace WzLibCpp

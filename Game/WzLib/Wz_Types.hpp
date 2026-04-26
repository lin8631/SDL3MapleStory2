#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <vector>

namespace WzLibCpp {

class Wz_Node;
class Wz_Image;
class Wz_File;
class Wz_Image;
class Wz_Node;
class Wz_NodeCollection;

enum class Wz_ValueType {
    Null,
    Int16,
    Int32,
    Int64,
    Float,
    Double,
    String,
    WzImage,
    WzPng,
    WzSound,
    WzVector,
    WzUol,
    WzRawData,
    WzVideo,
    WzConvex,
};

enum class Wz_TextureFormat : uint32_t {
    Unknown = 0,
    ARGB4444 = 1,
    ARGB8888 = 2,
    ARGB1555 = 257,
    RGB565 = 513,
    DXT3 = 1026,
    DXT5 = 2050,
    A8 = 2304,
    RGBA1010102 = 2562,
    DXT1 = 4097,
    BC7 = 4098,
    RGBA32Float = 4100,
};

enum class Wz_SoundType {
    Unknown = 0,
    Mp3,
    Pcm,
    Binary,
};

class Wz_Png;

class Wz_Sound {
public:
    Wz_Sound(uint32_t offset, int dataLength, int ms, std::weak_ptr<Wz_Image> wzImage);
    
    uint32_t getOffset() const { return Offset; }
    int getDataLength() const { return DataLength; }
    int getMs() const { return Ms; }
    
    std::weak_ptr<Wz_Image> getWzImage() const { return WzImage; }
    std::shared_ptr<Wz_File> getWzFile() const;
    
    std::vector<uint8_t> extractSound();

private:
    uint32_t Offset;
    int DataLength;
    int Ms;
    std::weak_ptr<Wz_Image> WzImage;
};

class Wz_Vector {
public:
    Wz_Vector(int x, int y);
    
    int getX() const { return X; }
    int getY() const { return Y; }
    
    void setX(int x) { X = x; }
    void setY(int y) { Y = y; }

private:
    int X;
    int Y;
};

class Wz_Uol {
public:
    explicit Wz_Uol(const std::string& uol);
    
    std::string getUol() const { return Uol; }
    
    std::shared_ptr<Wz_Node> resolve(std::shared_ptr<Wz_Node> currentNode);

private:
    std::string Uol;
};

class Wz_RawData {
public:
    Wz_RawData(uint32_t offset, int length, std::weak_ptr<Wz_Image> wzImage);
    
    uint32_t getOffset() const { return Offset; }
    int getLength() const { return Length; }
    
    std::weak_ptr<Wz_Image> getWzImage() const { return WzImage; }
    std::shared_ptr<Wz_File> getWzFile() const;
    
    void copyTo(std::vector<uint8_t>& buffer, int offset);
    void copyTo(std::vector<uint8_t>& buffer);

private:
    uint32_t Offset;
    int Length;
    std::weak_ptr<Wz_Image> WzImage;
};

class Wz_Video {
public:
    Wz_Video(uint32_t offset, int length, std::weak_ptr<Wz_Image> wzImage);
    
    uint32_t getOffset() const { return Offset; }
    int getLength() const { return Length; }
    
    std::weak_ptr<Wz_Image> getWzImage() const { return WzImage; }
    std::shared_ptr<Wz_File> getWzFile() const;
    
    void copyTo(std::vector<uint8_t>& buffer, int offset);
    void copyTo(std::vector<uint8_t>& buffer);

private:
    uint32_t Offset;
    int Length;
    std::weak_ptr<Wz_Image> WzImage;
};

class Wz_Convex {
public:
    Wz_Convex();
    
    std::vector<std::shared_ptr<Wz_Vector>>& getPoints() { return Points; }
    const std::vector<std::shared_ptr<Wz_Vector>>& getPoints() const { return Points; }
    
    void addPoint(std::shared_ptr<Wz_Vector> point);

private:
    std::vector<std::shared_ptr<Wz_Vector>> Points;
};

} // namespace WzLibCpp

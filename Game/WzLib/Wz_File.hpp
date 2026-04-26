#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include "Wz_Type.hpp"
#include "IMapleStoryFile.hpp"

namespace WzLibCpp {

class Wz_Header;
class Wz_Node;
class Wz_Structure;
class Wz_Directory;
class Wz_Image;
class IWzDecrypter;

enum class WzVersionVerifyMode {
    Detect,
    ThrowOnMismatch,
    AllowMismatch
};

class Wz_File : public IMapleStoryFile, public std::enable_shared_from_this<Wz_File> {
public:
    Wz_File(const std::string& fileName, std::shared_ptr<Wz_Structure> wz);
    virtual ~Wz_File();

    std::shared_ptr<Wz_Structure> getWzStructure() const override;
    void setWzStructure(std::shared_ptr<Wz_Structure> wz);
    void* getFileStream() const override { return nullptr; }
    void* getReadLock() const override { return nullptr; }
    std::shared_ptr<std::fstream> getFStream() const { return fileStream; }
    void close() override;

    std::shared_ptr<Wz_Header> getHeader() const { return header; }
    std::shared_ptr<Wz_Node> getNode() const { return node; }
    void setNode(std::shared_ptr<Wz_Node> n) { node = n; }

    int32_t getImageCount() const { return imageCount; }
    bool isLoaded() const { return loaded; }
    bool getIsSubDir() const { return isSubDir; }

    Wz_Type getType() const { return type; }
    void setType(Wz_Type t) { type = t; }

    const std::vector<std::shared_ptr<Wz_File>>& getMergedWzFiles() const;
    void mergeWzFile(std::shared_ptr<Wz_File> other);

    std::shared_ptr<Wz_File> getOwnerWzFile() const { return ownerWzFile; }
    void setOwnerWzFile(std::shared_ptr<Wz_File> f) { ownerWzFile = f; }
    
    int64_t getFileStreamPosition() const;

    uint32_t calcOffset(uint32_t filePos, uint32_t hashedOffset);
    uint32_t calcOffsetPkg2(uint32_t filePos, uint32_t hashedOffset);
    int32_t decryptPkg2EntryCount(int32_t encryptedEntryCount);
    uint32_t calcHashVersionFromEntryCount(int32_t encryptedEntryCount, int32_t entryCount);

    void getDirTree(std::shared_ptr<Wz_Node> parent, bool useBaseWz = false, bool loadWzAsFolder = false, int depth = 0);
    void detectWzType();
    void detectWzVersion(WzVersionVerifyMode mode = WzVersionVerifyMode::Detect);
    bool verifyWzVersion(); // 在 getDirTree 之后验证版本
    void calcAllOffset(const std::vector<std::shared_ptr<Wz_Image>>& images); // 为所有 Wz_Image 计算偏移

private:
    bool getHeaderFromFile(const std::string& fileName);
    void readDirTree(void* reader, std::shared_ptr<Wz_Node> parent, std::vector<std::string>& dirs, std::vector<int64_t>& dirOffsets);
    void readDirTreePkg2(void* reader, std::shared_ptr<Wz_Node> parent, std::vector<std::string>& dirs);
    
    // 版本验证相关
    bool fastCheckFirstByte(std::shared_ptr<Wz_Image> image, uint8_t firstByte);
    bool detectWithWzImage(std::shared_ptr<Wz_Image> testImg);
    bool detectWithAllWzDir();
    std::vector<std::shared_ptr<Wz_Image>> getAllWzImages(std::shared_ptr<Wz_Node> parent);

    std::string readStringAt(int32_t offset, std::shared_ptr<IWzDecrypter> key);
    std::string readStringWithLength(uint8_t len, std::shared_ptr<IWzDecrypter> key);
    std::string readStringUtf16(int32_t len, std::shared_ptr<IWzDecrypter> key);
    std::string readStringAtPosition(uint8_t len, std::shared_ptr<IWzDecrypter> key);
    std::string readString(std::shared_ptr<IWzDecrypter> key);

    int32_t readCompressedInt32();
    int32_t readInt32();
    uint32_t readUInt32();

    std::shared_ptr<Wz_Structure> wzStructure;
    std::shared_ptr<Wz_Header> header;
    std::shared_ptr<Wz_Node> node;
    int32_t imageCount = 0;
    bool loaded = false;
    bool isSubDir = false;
    Wz_Type type = Wz_Type::Unknown;
    std::vector<std::shared_ptr<Wz_File>> mergedWzFiles;
    std::shared_ptr<Wz_File> ownerWzFile;
    std::vector<std::shared_ptr<Wz_Directory>> directories;
    std::shared_ptr<std::fstream> fileStream;
    int32_t encryptedVersion = 0;
};

} // namespace WzLibCpp

#include "Wz_File.hpp"
#include "Wz_Header.hpp"
#include "Wz_Node.hpp"
#include "Wz_Structure.hpp"
#include "Wz_Image.hpp"
#include "Wz_Directory.hpp"
#include "Wz_Crypto.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <filesystem>
#include <regex>
#include <iomanip>
#include <iostream>

namespace WzLibCpp {

Wz_File::Wz_File(const std::string& fileName, std::shared_ptr<Wz_Structure> wz)
    : wzStructure(wz), header(nullptr), node(nullptr), 
      imageCount(0), loaded(false), isSubDir(false), 
      type(Wz_Type::Unknown) {
    
    fileStream = std::make_shared<std::fstream>(fileName, std::ios::binary | std::ios::in);
    if (!fileStream->is_open()) {
        fileStream.reset();
        loaded = false;
        return;
    }
    
    loaded = getHeaderFromFile(fileName);
    if (!loaded) {
        fileStream->close();
        fileStream.reset();
    }
}

Wz_File::~Wz_File() {
    close();
}

std::shared_ptr<Wz_Structure> Wz_File::getWzStructure() const {
    return wzStructure;
}

void Wz_File::setWzStructure(std::shared_ptr<Wz_Structure> wz) {
    wzStructure = wz;
}

void Wz_File::close() {
    wzStructure.reset();
    header.reset();
    node.reset();
    mergedWzFiles.clear();
    directories.clear();
    if (fileStream) {
        if (fileStream->is_open()) {
            fileStream->close();
        }
        fileStream.reset();
    }
}

const std::vector<std::shared_ptr<Wz_File>>& Wz_File::getMergedWzFiles() const {
    return mergedWzFiles;
}

void Wz_File::mergeWzFile(std::shared_ptr<Wz_File> other) {
    if (other) {
        other->setOwnerWzFile(shared_from_this());
        mergedWzFiles.push_back(other);
    }
}

int64_t Wz_File::getFileStreamPosition() const {
    if (!fileStream) return 0;
    return fileStream->tellg();
}

uint32_t Wz_File::calcOffset(uint32_t filePos, uint32_t hashedOffset) {
    uint32_t offset = filePos - 0x3C;
    offset = ~offset;
    
    auto headerSP = header;
    if (!headerSP) return 0;
    
    uint32_t hashVersion = headerSP->getHashVersion();
    if (hashVersion == 0) return 0; // 版本未验证时返回0
    
    offset *= hashVersion;
    offset -= 0x581C3F6D;
    
    int distance = offset & 0x1F;
    offset = (offset << distance) | (offset >> (32 - distance));
    offset ^= hashedOffset;
    offset += 0x78;
    
    return offset;
}

uint32_t Wz_File::calcOffsetPkg2(uint32_t filePos, uint32_t hashedOffset) {
    auto headerSP = header;
    if (!headerSP) return 0;
    
    uint32_t headerLen = headerSP->getHeaderSize();
    uint32_t hashVersion = headerSP->getHashVersion();
    uint32_t hash1 = headerSP->getPkg2Hash1();
    
    uint32_t offset = 0xFFFFFFFC + filePos - headerLen;
    int distance;
    
    offset = ~offset;
    offset *= hashVersion;
    offset -= 0x581C3F6D;
    offset ^= hash1 * 0x01010101;
    distance = (hashVersion ^ hash1) & 0x1F;
    offset = (offset << distance) | (offset >> (32 - distance));
    offset ^= hashedOffset;
    offset += headerLen;
    
    return offset;
}

int32_t Wz_File::decryptPkg2EntryCount(int32_t encryptedEntryCount) {
    if (!header) return 0;
    uint32_t hash1 = header->getPkg2Hash1();
    uint32_t hashVersion = header->getHashVersion();
    return encryptedEntryCount ^ ((hash1 << 24) + (0x7F4A7C15 * hashVersion));
}

uint32_t Wz_File::calcHashVersionFromEntryCount(int32_t encryptedEntryCount, int32_t entryCount) {
    if (!header) return 0;
    uint32_t hash1 = header->getPkg2Hash1();
    return ((entryCount ^ encryptedEntryCount) - (hash1 << 24)) * 0x9937733D;
}

int32_t Wz_File::readCompressedInt32() {
    if (!fileStream) return 0;
    
    int8_t s = 0;
    if (!fileStream->read(reinterpret_cast<char*>(&s), 1)) return 0;
    
    if (s == -128) {
        int32_t result = 0;
        uint8_t bytes[4];
        if (fileStream->read(reinterpret_cast<char*>(bytes), 4)) {
            result = static_cast<int32_t>(bytes[0]) |
                   (static_cast<int32_t>(bytes[1]) << 8) |
                   (static_cast<int32_t>(bytes[2]) << 16) |
                   (static_cast<int32_t>(bytes[3]) << 24);
        }
        return result;
    }
    
    return s;
}

uint32_t Wz_File::readUInt32() {
    if (!fileStream) return 0;
    
    uint32_t value = 0;
    uint8_t bytes[4];
    if (fileStream->read(reinterpret_cast<char*>(bytes), 4)) {
        value = static_cast<uint32_t>(bytes[0]) |
               (static_cast<uint32_t>(bytes[1]) << 8) |
               (static_cast<uint32_t>(bytes[2]) << 16) |
               (static_cast<uint32_t>(bytes[3]) << 24);
    }
    return value;
}

int32_t Wz_File::readInt32() {
    if (!fileStream) return 0;
    
    uint8_t bytes[4];
    if (fileStream->read(reinterpret_cast<char*>(bytes), 4)) {
        return static_cast<int32_t>(bytes[0]) |
               (static_cast<int32_t>(bytes[1]) << 8) |
               (static_cast<int32_t>(bytes[2]) << 16) |
               (static_cast<int32_t>(bytes[3]) << 24);
    }
    return 0;
}

std::string Wz_File::readString(std::shared_ptr<IWzDecrypter> key) {
    if (!fileStream) return "";
    
    int8_t firstByte = 0;
    if (!fileStream->read(reinterpret_cast<char*>(&firstByte), 1)) return "";
    
    // 调试：打印第一个字节
    // std::cerr << "  readString: firstByte=0x" << std::hex << (int)(uint8_t)firstByte << std::dec;
    // if (firstByte < 0) {
    //     std::cerr << " (ASCII, len=" << (int)-(int)firstByte << ")";
    // } else if (firstByte == 127) {
    //     std::cerr << " (UTF16, 4-byte len)";
    // } else {
    //     std::cerr << " (UTF16, len=" << (int)firstByte << ")";
    // }
    
    if (firstByte == -128) {
        // firstByte == -128: 使用 readInt32()（直接读取 4 字节）
        int32_t strLen = readInt32();
        if (strLen < 0) return "";
        return readStringWithLength(static_cast<uint8_t>(strLen), key);
    } else if (firstByte < 0) {
        // 负数：ASCII 字符串，长度为 -firstByte
        int32_t strLen = -firstByte;
        return readStringWithLength(static_cast<uint8_t>(strLen), key);
    } else {
        // 正数：UTF-16LE 字符串，长度为 firstByte
        int32_t strLen = firstByte;
        if (strLen == 127) {
            strLen = readInt32();
        }
        return readStringUtf16(static_cast<int32_t>(strLen), key);
    }
}

std::string Wz_File::readStringWithLength(uint8_t len, std::shared_ptr<IWzDecrypter> key) {
    if (!fileStream || len == 0) return "";
    
    std::vector<uint8_t> data(len);
    if (!fileStream->read(reinterpret_cast<char*>(data.data()), len)) return "";
    
    // 调试：打印原始加密字节
    // std::cerr << "  encrypted: ";
    // for (int i = 0; i < std::min((int)len, 10); i++) {
    //     std::cerr << std::hex << (int)data[i] << " ";
    // }
    // if (len > 10) std::cerr << "... ";
    // std::cerr << std::dec;
    
    if (key) {
        key->decrypt(data, 0);
        // 调试：打印解密后（XOR前）的字节
        // std::cerr << "  after_aes: ";
        // for (int i = 0; i < std::min((int)len, 10); i++) {
        //     std::cerr << std::hex << (int)data[i] << " ";
        // }
        // if (len > 10) std::cerr << "... ";
        // std::cerr << std::dec;
    }
    
    std::string result;
    result.reserve(len);
    uint8_t mask = 0xAA;
    for (size_t i = 0; i < len; i++) {
        uint8_t decrypted = data[i] ^ mask;
        result += static_cast<char>(decrypted);
        mask++;
    }
    
    // 调试：打印解密后的结果
    // std::cerr << "  result=\"" << result << "\"" << std::endl;
    
    return result;
}

std::string Wz_File::readStringUtf16(int32_t len, std::shared_ptr<IWzDecrypter> key) {
    if (!fileStream || len <= 0) return "";
    
    int32_t byteLen = len * 2;
    std::vector<uint8_t> data(byteLen);
    if (!fileStream->read(reinterpret_cast<char*>(data.data()), byteLen)) return "";
    
    // 调试：打印原始加密字节
    // std::cerr << "  utf16 encrypted: ";
    // for (int i = 0; i < std::min(byteLen, 20); i++) {
    //     std::cerr << std::hex << (int)data[i] << " ";
    // }
    // if (byteLen > 20) std::cerr << "... ";
    // std::cerr << std::dec;
    
    if (key) {
        key->decrypt(data, 0);
    }
    
    // UTF-16LE 解码，mask 从 0xAAAA 开始（每两个字节一个 mask）
    std::string result;
    result.reserve(len);
    uint16_t mask = 0xAAAA;
    for (int32_t i = 0; i < len; i++) {
        uint16_t wchar = static_cast<uint16_t>(data[i * 2]) | (static_cast<uint16_t>(data[i * 2 + 1]) << 8);
        wchar ^= mask;
        if (wchar < 128) {
            result += static_cast<char>(wchar);
        } else {
            result += '?';
        }
        mask++;
    }
    
    // std::cerr << "  utf16 result=\"" << result << "\"" << std::endl;
    
    return result;
}

std::string Wz_File::readStringAtPosition(uint8_t len, std::shared_ptr<IWzDecrypter> key) {
    if (!fileStream || len == 0) return "";
    
    int32_t offset = readCompressedInt32();
    
    if (offset < 0) return "";
    
    auto pos = fileStream->tellg();
    
    if (offset >= pos) {
        fileStream->seekg(offset - pos, std::ios::cur);
    } else {
        fileStream->seekg(offset);
    }
    
    if (!fileStream->good()) {
        fileStream->seekg(pos);
        return "";
    }
    
    std::vector<uint8_t> data(len);
    if (!fileStream->read(reinterpret_cast<char*>(data.data()), len)) {
        fileStream->seekg(pos);
        return "";
    }
    
    if (key) {
        key->decrypt(data, 0);
    }
    
    std::string result;
    result.reserve(len);
    uint8_t mask = 0xAA;
    for (size_t i = 0; i < len; i++) {
        result += static_cast<char>(data[i] ^ mask);
        mask++;
    }
    
    fileStream->seekg(pos);
    
    return result;
}

std::string Wz_File::readStringAt(int32_t offset, std::shared_ptr<IWzDecrypter> key) {
    if (!fileStream) return "";
    
    if (offset < 0) return "";
    
    auto pos = fileStream->tellg();
    fileStream->seekg(offset);
    
    if (!fileStream->good()) {
        fileStream->seekg(pos);
        return "";
    }
    
    std::string result = readString(key);
    
    fileStream->seekg(pos);
    
    return result;
}

void Wz_File::getDirTree(std::shared_ptr<Wz_Node> parent, bool useBaseWz, bool loadWzAsFolder, int depth) {
    if (!loaded || !parent || !fileStream) return;
    
    if (depth > 10) {
        return;
    }
    
    directories.clear();
    
    // 只有在首次调用时（depth=0）才跳转到 DataStartPosition
    if (depth == 0) {
        fileStream->seekg(header->getDataStartPosition());
    }
    
    std::vector<std::string> dirs;
    std::vector<int64_t> dirOffsets;
    
    if (header->getSignature() == "PKG1") {
        readDirTree(fileStream.get(), parent, dirs, dirOffsets);
    } else if (header->getSignature() == "PKG2") {
        readDirTreePkg2(fileStream.get(), parent, dirs);
    }
    
    int dirCount = static_cast<int>(dirs.size());
    
    std::string parentText = parent->getText();
    std::transform(parentText.begin(), parentText.end(), parentText.begin(), ::tolower);
    bool willLoadBaseWz = useBaseWz && (parentText == "base.wz");
    
    if (willLoadBaseWz && wzStructure && wzStructure->getAutoDetectExtFiles()) {
        std::string baseFolder = std::filesystem::path(header->getFileName()).parent_path().string();
        
        for (int i = 0; i < dirCount; i++) {
            std::string dirName = dirs[i];
            std::regex alphaPattern("^([A-Za-z]+)$");
            std::smatch match;
            
            if (std::regex_match(dirName, match, alphaPattern)) {
                std::string wzTypeName = match[1].str();
                
                for (int fileID = 2; ; fileID++) {
                    std::string extDirName = wzTypeName + std::to_string(fileID);
                    std::filesystem::path extWzFile = std::filesystem::path(baseFolder) / (extDirName + ".wz");
                    
                    if (std::filesystem::exists(extWzFile)) {
                        bool exists = false;
                        for (int j = 0; j < dirCount; j++) {
                            std::string existing = dirs[j];
                            std::transform(existing.begin(), existing.end(), existing.begin(), ::tolower);
                            if (existing == extDirName) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            dirs.push_back(extDirName);
                        }
                    } else {
                        break;
                    }
                }
                
                for (int fileID = 1; ; fileID++) {
                    std::ostringstream oss;
                    oss << wzTypeName << std::setw(3) << std::setfill('0') << fileID;
                    std::string extDirName = oss.str();
                    std::filesystem::path extWzFile = std::filesystem::path(baseFolder) / (extDirName + ".wz");
                    
                    if (std::filesystem::exists(extWzFile)) {
                        bool exists = false;
                        for (int j = 0; j < dirCount; j++) {
                            std::string existing = dirs[j];
                            std::transform(existing.begin(), existing.end(), existing.begin(), ::tolower);
                            if (existing == extDirName) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            dirs.push_back(extDirName);
                        }
                    } else {
                        break;
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < static_cast<int>(dirs.size()); i++) {
        const std::string& dir = dirs[i];
        auto childNode = parent->getNodes()->add(dir);
        
        if (i < dirCount && depth < 2) {
            getDirTree(childNode, false, loadWzAsFolder, depth + 1);
        }
        
        if (willLoadBaseWz && wzStructure) {
            wzStructure->setHasBasewz(true);
            
            std::string baseFolder = std::filesystem::path(header->getFileName()).parent_path().string();
            std::filesystem::path wzFile = std::filesystem::path(baseFolder) / (dir + ".wz");
            
            if (std::filesystem::exists(wzFile)) {
                try {
                    wzStructure->loadFile(wzFile.string(), childNode, false, false);
                } catch (const std::exception& e) {
                    // std::cerr << "Wz_File::getDirTree: Failed to load file " << wzFile << ": " << e.what() << std::endl;
                }
            }
        }
    }
    
    parent->getNodes()->trim();
}

void Wz_File::readDirTree(void* reader, std::shared_ptr<Wz_Node> parent, std::vector<std::string>& dirs, std::vector<int64_t>& dirOffsets) {
    if (!fileStream) return;
    
    std::shared_ptr<IWzDecrypter> key;
    if (wzStructure) {
        auto crypto = wzStructure->getEncryption();
        if (crypto) {
            key = crypto->getCurrentKey();
        }
    }
    
    int count = readCompressedInt32();
    
    if (count < 0 || count > 1000000) {
        return;
    }
    
    bool encverMissing = header && header->hasCapabilities(Wz_Capabilities::EncverMissing);
    int stringOffAdd = encverMissing ? 2 : -1;
    
    for (int i = 0; i < count; i++) {
        uint8_t nodeType = 0;
        if (!fileStream->read(reinterpret_cast<char*>(&nodeType), 1)) break;
        
        std::string name;
        switch (nodeType) {
            case 0x02: {
                int32_t strOffset = readInt32();
                // strOffset 是相对于 DataStartPosition 的偏移
                name = readStringAt(strOffset + static_cast<int32_t>(header->getDataStartPosition()) + stringOffAdd, key);
                break;
            }
            case 0x03:
        case 0x04: {
            // 记录读取前的位置
            int64_t posBefore = fileStream->tellg();
            name = readString(key);
            int64_t posAfter = fileStream->tellg();
            // std::cerr << "readDirTree: nodeType=0x04 at pos=" << posBefore 
            //           << ", strLen estimate=" << (posAfter - posBefore - 1) 
            //           << ", encType=" << (key ? "KMS/GMS" : "None")
            //           << std::endl;
            break;
        }
            default:
                continue;
        }
        
        int32_t size = readCompressedInt32();
        int32_t cs32 = readCompressedInt32();
        int64_t pos = fileStream->tellg();  // 先记录 pos（hashOffset 字段的绝对偏移）
        uint32_t hashOffset = readUInt32();
        
        switch (nodeType) {
            case 0x02:
            case 0x04: {
                auto img = std::make_shared<Wz_Image>(name, size, cs32, hashOffset, static_cast<uint32_t>(pos), shared_from_this());
                auto childNode = parent->getNodes()->add(name);
                childNode->setValue(img);
                img->setOwnerNode(childNode);
                imageCount++;
                break;
            }
            case 0x03: {
                int64_t dataOffset = calcOffset(pos, hashOffset);
                auto dir = std::make_shared<Wz_Directory>(name, size, cs32, hashOffset, static_cast<uint32_t>(pos), shared_from_this());
                directories.push_back(dir);
                dirs.push_back(name);
                dirOffsets.push_back(dataOffset);
                break;
            }
        }
    }
}

void Wz_File::readDirTreePkg2(void* reader, std::shared_ptr<Wz_Node> parent, std::vector<std::string>& dirs) {
    if (!fileStream) return;
    
    std::shared_ptr<IWzDecrypter> key;
    if (wzStructure) {
        auto crypto = wzStructure->getEncryption();
        if (crypto) {
            key = crypto->getCurrentKey();
        }
    }
    
    int32_t encryptedEntryCount = readCompressedInt32();
    
    struct Pkg2Entry {
        uint8_t nodeType;
        std::string name;
        int32_t dataLength;
        int32_t checksum;
    };
    
    std::vector<Pkg2Entry> entries;
    
    while (true) {
        uint8_t nodeType = 0;
        if (!fileStream->read(reinterpret_cast<char*>(&nodeType), 1)) break;
        
        if (nodeType == 0x03 || nodeType == 0x04) {
            std::string name = readString(key);
            int32_t size = readCompressedInt32();
            int32_t cs32 = readCompressedInt32();
            
            Pkg2Entry entry;
            entry.nodeType = nodeType;
            entry.name = name;
            entry.dataLength = size;
            entry.checksum = cs32;
            entries.push_back(entry);
        } else if (nodeType == 0x80 || (encryptedEntryCount >= -127 && encryptedEntryCount <= 127 && nodeType == static_cast<uint8_t>(encryptedEntryCount))) {
            fileStream->seekg(-1, std::ios::cur);
            break;
        } else {
            continue;
        }
    }
    
    int32_t encryptedOffsetCount = readCompressedInt32();
    
    if (encryptedOffsetCount == encryptedEntryCount && !entries.empty()) {
        for (size_t i = 0; i < entries.size(); i++) {
            uint32_t hashOffset = readUInt32();
            uint32_t pos = static_cast<uint32_t>(fileStream->tellg());
            
            const auto& entry = entries[i];
            
            if (entry.nodeType == 0x04) {
                auto img = std::make_shared<Wz_Image>(entry.name, entry.dataLength, entry.checksum, hashOffset, pos, shared_from_this());
                auto childNode = parent->getNodes()->add(entry.name);
                childNode->setValue(img);
                img->setOwnerNode(childNode);
                imageCount++;
            } else if (entry.nodeType == 0x03) {
                auto dir = std::make_shared<Wz_Directory>(entry.name, entry.dataLength, entry.checksum, hashOffset, pos, shared_from_this());
                directories.push_back(dir);
                dirs.push_back(entry.name);
            }
        }
    }
}

void Wz_File::detectWzType() {
    if (!header) return;
    
    std::string filePath = header->getFileName();
    
    // 只提取文件名部分，避免路径中的关键词干扰
    std::string fileName = std::filesystem::path(filePath).filename().string();
    
    std::string lowerName = fileName;
    for (auto& c : lowerName) {
        c = std::tolower(c);
    }
    
    if (lowerName.find("character") != std::string::npos) {
        type = Wz_Type::Character;
    } else if (lowerName.find("effect") != std::string::npos) {
        type = Wz_Type::Effect;
    } else if (lowerName.find("etc") != std::string::npos) {
        type = Wz_Type::Etc;
    } else if (lowerName.find("item") != std::string::npos) {
        type = Wz_Type::Item;
    } else if (lowerName.find("map") != std::string::npos) {
        type = Wz_Type::Map;
    } else if (lowerName.find("tamingmob") != std::string::npos) {
        type = Wz_Type::TamingMob;
    } else if (lowerName.find("mob") != std::string::npos) {
        type = Wz_Type::Mob;
    } else if (lowerName.find("morph") != std::string::npos) {
        type = Wz_Type::Morph;
    } else if (lowerName.find("npc") != std::string::npos) {
        type = Wz_Type::Npc;
    } else if (lowerName.find("quest") != std::string::npos) {
        type = Wz_Type::Quest;
    } else if (lowerName.find("reactor") != std::string::npos) {
        type = Wz_Type::Reactor;
    } else if (lowerName.find("skill") != std::string::npos) {
        type = Wz_Type::Skill;
    } else if (lowerName.find("sound") != std::string::npos) {
        type = Wz_Type::Sound;
    } else if (lowerName.find("string") != std::string::npos) {
        type = Wz_Type::String;
    } else if (lowerName.find("ui") != std::string::npos) {
        type = Wz_Type::UI;
    } else if (lowerName.find("base") != std::string::npos) {
        type = Wz_Type::Base;
    }
}

bool Wz_File::fastCheckFirstByte(std::shared_ptr<Wz_Image> image, uint8_t firstByte) {
    // 验证首字节是否符合预期
    // 对于普通图像，首字节应为 0x73 或 0x1b
    // 对于 Lua 图像，首字节为 0x01
    if (firstByte == 0x01) {
        return true; // Lua image
    }
    return firstByte == 0x73 || firstByte == 0x1b;
}

bool Wz_File::detectWithWzImage(std::shared_ptr<Wz_Image> testImg) {
    if (!testImg || !header || !fileStream) return false;
    
    // 获取当前的 DirEndPosition
    int64_t dirEndPosition = header->getDirEndPosition();
    int64_t fileSize = header->getFileSize();
    
    // 计算真实偏移
    uint32_t offs = calcOffset(testImg->getHashedOffsetPosition(), testImg->getHashedOffset());
    
    // 检查偏移是否在有效范围内
    if (offs < dirEndPosition || offs + testImg->getSize() > fileSize) {
        return false; // 偏移无效
    }
    
    // 跳转到该偏移位置
    fileStream->seekg(offs);
    if (!fileStream->good()) {
        return false;
    }
    
    // 读取首字节
    uint8_t firstByte = 0;
    if (!fileStream->read(reinterpret_cast<char*>(&firstByte), 1)) {
        return false;
    }
    
    // 验证首字节
    if (!fastCheckFirstByte(testImg, firstByte)) {
        return false; // 首字节不符合预期
    }
    
    // 尝试提取图像
    testImg->setOffset(offs);
    if (testImg->tryExtract()) {
        testImg->setIsExtracted(false); // 清除提取状态
        header->setVersionChecked(true);
        return true;
    }
    
    return false;
}

bool Wz_File::detectWithAllWzDir() {
    if (!header || !fileStream || directories.empty()) return false;
    
    // 获取当前的 DirEndPosition
    int64_t dirEndPosition = header->getDirEndPosition();
    int64_t dataStartPosition = header->getDataStartPosition();
    
    // 遍历所有候选版本
    while (header->tryGetNextVersion()) {
        bool allValid = true;
        
        // 验证所有目录的偏移
        for (const auto& dir : directories) {
            // 计算真实偏移
            uint32_t offs = calcOffset(dir->getHashedOffsetPosition(), dir->getHashedOffset());
            
            // 检查偏移是否在有效范围内（在 DataStartPosition 和 DirEndPosition 之间）
            if (offs < dataStartPosition || offs > dirEndPosition) {
                allValid = false;
                break; // 偏移无效，尝试下一个版本
            }
            
            // 跳转到该偏移位置
            fileStream->seekg(offs);
            if (!fileStream->good()) {
                allValid = false;
                break;
            }
            
            // 读取首字节
            uint8_t firstByte = 0;
            if (!fileStream->read(reinterpret_cast<char*>(&firstByte), 1)) {
                allValid = false;
                break;
            }
            
            // 对于目录，首字节应为 0x00
            if (firstByte != 0x00) {
                allValid = false;
                break; // 首字节不符合预期，尝试下一个版本
            }
        }
        
        if (allValid) {
            header->setVersionChecked(true);
            return true;
        }
    }
    
    return false;
}

std::vector<std::shared_ptr<Wz_Image>> Wz_File::getAllWzImages(std::shared_ptr<Wz_Node> parent) {
    std::vector<std::shared_ptr<Wz_Image>> images;
    
    if (!parent) return images;
    
    auto nodes = parent->getNodes();
    if (!nodes) return images;
    
    for (const auto& node : *nodes) {
        // 检查当前节点是否有 Wz_Image 值
        auto img = node->getValue<Wz_Image>();
        if (img) {
            images.push_back(img);
        }
        
        // 递归检查子节点
        auto childImages = getAllWzImages(node);
        images.insert(images.end(), childImages.begin(), childImages.end());
    }
    
    return images;
}

bool Wz_File::verifyWzVersion() {
    // 在 getDirTree 之后验证版本
    if (!header || header->getVersionChecked()) {
        return true;
    }
    
    auto wzStruct = getWzStructure();
    if (!wzStruct) {
        return false;
    }
    
    auto crypto = wzStruct->getEncryption();
    if (!crypto || !crypto->encryption_detected) {
        return false;
    }
    
    // 获取所有 Wz_Image 对象用于验证
    auto images = getAllWzImages(node);
    
    if (!images.empty()) {
        // 使用 Wz_Image 验证版本
        // 对每个 Wz_Image，尝试当前已设置的版本
        for (const auto& img : images) {
            // 获取当前的 DirEndPosition
            int64_t dirEndPosition = header->getDirEndPosition();
            int64_t fileSize = header->getFileSize();
            
            // 计算真实偏移
            uint32_t offs = calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset());
            
            // 检查偏移是否在有效范围内
            if (offs < dirEndPosition || offs + img->getSize() > fileSize) {
                continue; // 偏移无效
            }
            
            // 跳转到该偏移位置
            fileStream->seekg(offs);
            if (!fileStream->good()) {
                continue;
            }
            
            // 读取首字节
            uint8_t firstByte = 0;
            if (!fileStream->read(reinterpret_cast<char*>(&firstByte), 1)) {
                continue;
            }
            
            // 验证首字节
            if (!fastCheckFirstByte(img, firstByte)) {
                continue; // 首字节不符合预期
            }
            
            // 尝试提取图像
            img->setOffset(offs);
            if (img->tryExtract()) {
                img->setIsExtracted(false); // 清除提取状态
                header->setVersionChecked(true);
                // 验证成功后，为所有 Wz_Image 计算偏移
                calcAllOffset(images);
                return true; // 找到有效版本
            }
        }
        
        // 如果第一个版本不匹配，尝试下一个版本
        while (header->tryGetNextVersion()) {
            for (const auto& img : images) {
                int64_t dirEndPosition = header->getDirEndPosition();
                int64_t fileSize = header->getFileSize();
                
                uint32_t offs = calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset());
                
                if (offs < dirEndPosition || offs + img->getSize() > fileSize) {
                    continue;
                }
                
                fileStream->seekg(offs);
                if (!fileStream->good()) {
                    continue;
                }
                
                uint8_t firstByte = 0;
                if (!fileStream->read(reinterpret_cast<char*>(&firstByte), 1)) {
                    continue;
                }
                
                if (!fastCheckFirstByte(img, firstByte)) {
                    continue;
                }
                
                img->setOffset(offs);
                if (img->tryExtract()) {
                    img->setIsExtracted(false);
                    header->setVersionChecked(true);
                    calcAllOffset(images);
                    return true;
                }
            }
        }
    }
    
    // 如果 Wz_Image 验证失败，尝试使用 Wz_Directory 验证
    if (!directories.empty()) {
        if (detectWithAllWzDir()) {
            // 验证成功后，为所有 Wz_Image 计算偏移
            calcAllOffset(images);
            return true; // 找到有效版本
        }
    }
    
    return false;
}

void Wz_File::calcAllOffset(const std::vector<std::shared_ptr<Wz_Image>>& images) {
    // 对应 C# 源码: protected void CalcOffset(Wz_File wzFile, IEnumerable<Wz_Image> imgList)
    // foreach (var img in imgList)
    // {
    //     img.Offset = wzFile.CalcOffset(img.HashedOffsetPosition, img.HashedOffset);
    // }
    for (const auto& img : images) {
        if (img) {
            uint32_t offset = calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset());
            img->setOffset(offset);
        }
    }
}

void Wz_File::detectWzVersion(WzVersionVerifyMode mode) {
    if (!header) return;
    
    int32_t currentVersion = header->getWzVersion();
    
    // 如果已经设置过版本，跳过
    if (currentVersion != 0) {
        return;
    }
    
    auto wzStruct = getWzStructure();
    if (!wzStruct) return;
    
    auto crypto = wzStruct->getEncryption();
    if (!crypto || !crypto->encryption_detected) {
        return;
    }
    
    // 首先设置 OrdinalVersionDetector
    header->setOrdinalVersionDetector(this->encryptedVersion);
    // std::cerr << "detectWzVersion: encryptedVersion=" << this->encryptedVersion << ", file=" << header->getFileName() << std::endl;
    
    // 获取第一个版本（用于后续验证）
    if (!header->tryGetNextVersion()) {
        return;
    }
    
    // 调用 verifyWzVersion 验证版本并计算偏移
    bool success = verifyWzVersion();
    if (!success) {
        // 版本验证失败，尝试使用 PKG2 版本检测器
        // std::cerr << "detectWzVersion: OrdinalVersion 失败，尝试 PKG2, file=" << header->getFileName() << std::endl;
    }
}

bool Wz_File::getHeaderFromFile(const std::string& fileName) {
    if (!fileStream || !fileStream->is_open()) {
        return false;
    }
    
    fileStream->seekg(0, std::ios::end);
    int64_t fileSize = fileStream->tellg();
    fileStream->seekg(0, std::ios::beg);
    
    if (fileSize < 4) {
        return false;
    }
    
    char signature[5] = {0};
    fileStream->read(signature, 4);
    std::string sig(signature);
    
    if (sig != "PKG1" && sig != "PKG2") {
        return false;
    }
    
    int64_t dataSize = 0;
    fileStream->read(reinterpret_cast<char*>(&dataSize), 8);
    
    int32_t headerSize = 0;
    fileStream->read(reinterpret_cast<char*>(&headerSize), 4);
    
    std::string copyright;
    int64_t remainingHeader = headerSize - 4 - 8 - 4;
    if (remainingHeader > 0 && remainingHeader < 4096) {
        std::vector<char> copyrightBuf(remainingHeader);
        fileStream->read(copyrightBuf.data(), remainingHeader);
        copyright = std::string(copyrightBuf.begin(), copyrightBuf.end());
    }
    
    int64_t dataStartPos = headerSize;
    
    if (sig == "PKG1") {
        bool encverMissing = false;
        int32_t encver = -1;
        
        if (fileSize >= headerSize + 2) {
            fileStream->seekg(headerSize);
            fileStream->read(reinterpret_cast<char*>(&encver), 2);
            encver &= 0xFFFF;
            
            if (encver > 0xFF) {
                encverMissing = true;
            } else if (encver == 0x80 && fileSize >= headerSize + 5) {
                fileStream->seekg(headerSize);
                int32_t propCount = readCompressedInt32();
                if (propCount > 0 && (propCount & 0xff) == 0 && propCount <= 0xffff) {
                    encverMissing = true;
                }
            }
        } else {
            encverMissing = true;
        }
        
        if (!encverMissing) {
            dataStartPos += 2;
        }
        
        header = std::make_shared<Wz_Header>(sig, copyright, fileName, headerSize, dataSize, fileSize, dataStartPos);
        
        if (encverMissing) {
            header->setWzVersion(777);
            header->setVersionChecked(true);
            header->setCapabilities(std::make_shared<Wz_Capabilities>(Wz_Capabilities::EncverMissing));
        } else {
            this->encryptedVersion = encver;
            header->setOrdinalVersionDetector(encver);
        }
    } else if (sig == "PKG2") {
        uint32_t hash1 = 0, hash2 = 0;
        fileStream->read(reinterpret_cast<char*>(&hash1), 4);
        fileStream->read(reinterpret_cast<char*>(&hash2), 4);
        
        dataStartPos = headerSize + 8;
        header = std::make_shared<Wz_Header>(sig, copyright, fileName, headerSize, dataSize, fileSize, dataStartPos);
        header->setWzVersionPkg2(hash1, hash2);
    } else {
        return false;
    }
    
    return true;
}

} // namespace WzLibCpp

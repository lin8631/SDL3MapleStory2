#include "Wz_Image.hpp"
#include "Wz_Node.hpp"
#include "Wz_Value.hpp"
#include "IMapleStoryFile.hpp"
#include "Wz_File.hpp"
#include "Wz_Crypto.hpp"
#include "Wz_BinaryReader.hpp"
#include "Wz_Png.hpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <cctype>

namespace WzLibCpp {

static std::shared_ptr<IWzDecrypter> TryDetectEnc(std::shared_ptr<PartialStream> stream, Wz_CryptoKeyType& detectedType);
static bool isLegalTag(const std::string& tag);

void ExtractImg(WzBinaryReader& reader, std::shared_ptr<Wz_Node> parent, std::shared_ptr<Wz_Image> wzImg);
void ExtractValue(WzBinaryReader& reader, std::shared_ptr<Wz_Node> parent, std::shared_ptr<IWzDecrypter> encKey, std::shared_ptr<Wz_Image> wzImg);

Wz_Image::Wz_Image(const std::string& name, int32_t size, int32_t cs32,
                   uint32_t hashOff, uint32_t hashPos,
                   std::shared_ptr<Wz_File> wz_f)
    : Name(name), Size(size), Checksum(cs32),
      HashedOffset(hashOff), HashedOffsetPosition(hashPos),
      Offset(0), WzFile(wz_f),
      isExtracted(false), isChecksumChecked(false), isChecksumEncrypted(false),
      encType(Wz_CryptoKeyType::None) {
}

const std::string& Wz_Image::getName() const { return Name; }
void Wz_Image::setName(const std::string& name) { Name = name; }

std::shared_ptr<Wz_File> Wz_Image::getWzFile() const { return WzFile; }
void Wz_Image::setWzFile(std::shared_ptr<Wz_File> wz_f) { WzFile = wz_f; }

int32_t Wz_Image::getSize() const { return Size; }
void Wz_Image::setSize(int32_t size) { Size = size; }

int32_t Wz_Image::getChecksum() const { return Checksum; }
void Wz_Image::setChecksum(int32_t cs32) { Checksum = cs32; }

uint32_t Wz_Image::getHashedOffset() const { return HashedOffset; }
void Wz_Image::setHashedOffset(uint32_t hashOff) { HashedOffset = hashOff; }

uint32_t Wz_Image::getHashedOffsetPosition() const { return HashedOffsetPosition; }
void Wz_Image::setHashedOffsetPosition(uint32_t hashPos) { HashedOffsetPosition = hashPos; }

int64_t Wz_Image::getOffset() const { return Offset; }
void Wz_Image::setOffset(int64_t offset) { Offset = offset; }

std::shared_ptr<Wz_Node> Wz_Image::getNode() const { 
    if (ExtractedNode) return ExtractedNode;
    return Node.lock(); 
}
void Wz_Image::setNode(std::shared_ptr<Wz_Node> node) { 
    Node = node; 
    if (!ExtractedNode) ExtractedNode = node;
}

std::shared_ptr<Wz_Node> Wz_Image::getOwnerNode() const { return OwnerNode.lock(); }
void Wz_Image::setOwnerNode(std::shared_ptr<Wz_Node> ownerNode) { OwnerNode = ownerNode; }

bool Wz_Image::getIsChecksumChecked() const { return isChecksumChecked; }
void Wz_Image::setIsChecksumChecked(bool isChecked) { isChecksumChecked = isChecked; }

bool Wz_Image::getIsExtracted() const { return isExtracted; }
void Wz_Image::setIsExtracted(bool isExtracted) { isExtracted = isExtracted; }

bool Wz_Image::getIsChecksumEncrypted() const { return isChecksumEncrypted; }
void Wz_Image::setIsChecksumEncrypted(bool isEncrypted) { isChecksumEncrypted = isEncrypted; }

static int32_t CalcCheckSum(std::shared_ptr<PartialStream> stream, int32_t size) {
    stream->setPosition(0);
    int32_t cs = 0;
    int32_t remaining = size;

    std::vector<uint8_t> buffer(4096);
    while (remaining > 0) {
        int32_t toRead = static_cast<int32_t>(std::min(static_cast<size_t>(remaining), buffer.size()));
        auto data = stream->readBytes(toRead);
        if (data.empty()) break;
        
        // C# version reads 4 bytes at a time as int, then sums each byte individually
        int32_t i = 0;
        int32_t intCount = data.size() / 4;
        for (i = 0; i < intCount; i++) {
            int32_t val = (data[i*4] & 0xFF) | 
                         ((data[i*4+1] & 0xFF) << 8) | 
                         ((data[i*4+2] & 0xFF) << 16) | 
                         ((data[i*4+3] & 0xFF) << 24);
            cs += (val & 0xFF) + ((val >> 8) & 0xFF) + ((val >> 16) & 0xFF) + ((val >> 24) & 0xFF);
        }
        for (int32_t j = i * 4; j < static_cast<int32_t>(data.size()); j++) {
            cs += data[j];
        }
        remaining -= toRead;
    }
    return cs;
}

static bool IsTextFormatV1(std::shared_ptr<PartialStream> stream) {
    stream->setPosition(0);
    auto data = stream->readBytes(9);
    if (data.size() < 9) return false;
    return std::string(data.begin(), data.end()) == "#Property";
}

static bool IsTextFormatV2(std::shared_ptr<PartialStream> stream) {
    stream->setPosition(0);
    auto data = stream->readBytes(4);
    if (data.size() < 4) return false;
    return std::string(data.begin(), data.end()) == "Root";
}

static std::shared_ptr<IWzDecrypter> TryDetectEnc(std::shared_ptr<PartialStream> stream, Wz_CryptoKeyType& detectedType) {
    detectedType = Wz_CryptoKeyType::None;

    // std::cerr << "TryDetectEnc: Trying None..." << std::endl;
    stream->setPosition(0);
    try {
        auto reader = std::make_shared<WzBinaryReader>(stream, nullptr);
        std::string tag = reader->readImageObjectTypeName();
        // std::cerr << "TryDetectEnc: None got tag: " << tag << std::endl;
        if (isLegalTag(tag)) {
            detectedType = Wz_CryptoKeyType::None;
            return nullptr;
        }
    } catch (...) {
        // std::cerr << "TryDetectEnc: None exception" << std::endl;
    }

    // std::cerr << "TryDetectEnc: Trying KMS..." << std::endl;
    stream->setPosition(0);
    try {
        static const uint8_t iv_kms[4] = { 0xb9, 0x7d, 0x63, 0xe9 };
        auto kmsKey = std::make_shared<Wz_CryptoKey>(iv_kms);
        auto reader = std::make_shared<WzBinaryReader>(stream, kmsKey);
        std::string tag = reader->readImageObjectTypeName();
        // std::cerr << "TryDetectEnc: KMS got tag: " << tag << std::endl;
        if (isLegalTag(tag)) {
            detectedType = Wz_CryptoKeyType::KMS;
            return kmsKey;
        }
    } catch (...) {
        // std::cerr << "TryDetectEnc: KMS exception" << std::endl;
    }

    // std::cerr << "TryDetectEnc: Trying GMS..." << std::endl;
    stream->setPosition(0);
    try {
        static const uint8_t iv_gms[4] = { 0x4d, 0x23, 0xc7, 0x2b };
        auto gmsKey = std::make_shared<Wz_CryptoKey>(iv_gms);
        auto reader = std::make_shared<WzBinaryReader>(stream, gmsKey);
        std::string tag = reader->readImageObjectTypeName();
        // std::cerr << "TryDetectEnc: GMS got tag: " << tag << std::endl;
        if (isLegalTag(tag)) {
            detectedType = Wz_CryptoKeyType::GMS;
            return gmsKey;
        }
    } catch (...) {
        // std::cerr << "TryDetectEnc: GMS exception" << std::endl;
    }

    // std::cerr << "TryDetectEnc: returning nullptr" << std::endl;
    return nullptr;
}

static bool isLegalTag(const std::string& tag) {
    return tag == "Property" || tag == "Shape2D#Vector2D" || tag == "Canvas" ||
           tag == "Shape2D#Convex2D" || tag == "Sound_DX8" || tag == "UOL" ||
           tag == "RawData" || tag == "Canvas#Video";
}

static std::string trimWhitespace(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

static std::string readLine(std::shared_ptr<PartialStream> stream) {
    std::string line;
    char c;
    while (stream->getPosition() < stream->getLength()) {
        int ch = stream->readByte();
        if (ch < 0 || ch == '\n') break;
        if (ch != '\r') line += static_cast<char>(ch);
    }
    return line;
}

static void skipWhitespace(std::shared_ptr<PartialStream> stream) {
    while (stream->getPosition() < stream->getLength()) {
        int ch = stream->readByte();
        if (ch < 0) break;
        if (ch == '\n' || ch == '\r') {
            stream->setPosition(stream->getPosition() - 1);
            break;
        }
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            stream->setPosition(stream->getPosition() - 1);
            break;
        }
    }
}

void ReadPropertyV1(std::shared_ptr<PartialStream> stream, std::shared_ptr<Wz_Node> parent, std::shared_ptr<IWzDecrypter> encKey, bool isTopLevel = false);

void ReadPropertyValueV1(std::shared_ptr<PartialStream> stream, std::shared_ptr<Wz_Node> parent, std::shared_ptr<IWzDecrypter> encKey) {
    skipWhitespace(stream);
    std::string line = readLine(stream);
    line = trimWhitespace(line);

    if (line.empty()) {
        return;
    }
    else if (line == "{") {
        ReadPropertyV1(stream, parent, encKey, false);
    }
    else if (line == "}") {
        throw std::runtime_error("Unexpected closing brace");
    }
    else {
        if (line == "}") return;
        parent->setWzValue(MakeString(line));
    }
}

void ReadPropertyV1(std::shared_ptr<PartialStream> stream, std::shared_ptr<Wz_Node> parent, std::shared_ptr<IWzDecrypter> encKey, bool isTopLevel) {
    while (stream->getPosition() < stream->getLength()) {
        skipWhitespace(stream);
        if (stream->getPosition() >= stream->getLength()) break;

        std::string line = readLine(stream);
        if (line.empty()) continue;

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trimWhitespace(line.substr(0, eqPos));
        std::string value = trimWhitespace(line.substr(eqPos + 1));

        if (key.empty()) continue;

        if (key == "}" && !isTopLevel) {
            return;
        }

        auto child = parent->getNodes()->add(key);

        if (value.empty() || value == "{}") {
        }
        else if (value == "{") {
            ReadPropertyV1(stream, child, encKey, false);
        }
        else if (isdigit(value[0]) || value[0] == '-' || value[0] == '+') {
            if (value.find('.') != std::string::npos) {
                try {
                    child->setWzValue(MakeFloat(std::stof(value)));
                } catch (...) {
                    child->setWzValue(MakeString(value));
                }
            } else {
                try {
                    if (value.find('x') != std::string::npos || value.find('X') != std::string::npos) {
                        child->setWzValue(MakeInt32(static_cast<int32_t>(std::stoll(value, nullptr, 0))));
                    } else {
                        child->setWzValue(MakeInt32(std::stoi(value)));
                    }
                } catch (...) {
                    child->setWzValue(MakeString(value));
                }
            }
        }
        else {
            child->setWzValue(MakeString(value));
        }
    }
}

bool ExtractTextImgV1(std::shared_ptr<PartialStream> stream, std::shared_ptr<Wz_Node> parent) {
    try {
        ReadPropertyV1(stream, parent, nullptr, true);
        return true;
    } catch (...) {
        return false;
    }
}

enum class NodeTypeV2 {
    Unknown = 0,
    Empty = 1,
    I4 = 2,
    I8 = 3,
    R8 = 4,
    String = 5,
    Vector = 6,
    Property = 7,
};

static NodeTypeV2 parseNodeType(const std::string& typeName) {
    if (typeName == "<Empty>") return NodeTypeV2::Empty;
    if (typeName == "<I4>") return NodeTypeV2::I4;
    if (typeName == "<I8>") return NodeTypeV2::I8;
    if (typeName == "<R8>") return NodeTypeV2::R8;
    if (typeName == "<String>") return NodeTypeV2::String;
    if (typeName == "<Vector>") return NodeTypeV2::Vector;
    if (typeName == "<Property>") return NodeTypeV2::Property;
    return NodeTypeV2::Unknown;
}

static int countRepeatChars(std::shared_ptr<PartialStream> stream, char ch) {
    int count = 0;
    while (stream->getPosition() < stream->getLength()) {
        int ch2 = stream->readByte();
        if (ch2 < 0 || ch2 != ch) {
            stream->setPosition(stream->getPosition() - 1);
            break;
        }
        count++;
    }
    return count;
}

static std::string readUntilWhitespace(std::shared_ptr<PartialStream> stream) {
    std::string result;
    while (stream->getPosition() < stream->getLength()) {
        int ch = stream->readByte();
        if (ch < 0 || std::isspace(static_cast<unsigned char>(ch))) {
            if (!result.empty()) {
                stream->setPosition(stream->getPosition() - 1);
            }
            break;
        }
        result += static_cast<char>(ch);
    }
    return result;
}

bool ExtractTextImgV2(std::shared_ptr<PartialStream> stream, std::shared_ptr<Wz_Node> parent) {
    try {
        int indent = countRepeatChars(stream, '\t');
        std::string name = readUntilWhitespace(stream);
        countRepeatChars(stream, ' ');

        if (stream->getPosition() >= stream->getLength()) return false;
        std::string typeStr = readUntilWhitespace(stream);

        if (indent != 0 || name != "Root") {
            return false;
        }

        auto nodeType = parseNodeType(typeStr);
        if (nodeType != NodeTypeV2::Property) {
            return false;
        }

        countRepeatChars(stream, ' ');
        if (stream->getPosition() < stream->getLength()) {
            stream->readByte();
        }

        std::vector<std::shared_ptr<Wz_Node>> nodeStack;
        nodeStack.push_back(parent);

        while (stream->getPosition() < stream->getLength()) {
            int nodeIndent = countRepeatChars(stream, '\t');
            std::string nodeName = readUntilWhitespace(stream);
            countRepeatChars(stream, ' ');

            if (stream->getPosition() >= stream->getLength()) break;
            std::string nodeTypeStr = readUntilWhitespace(stream);

            while (nodeIndent < static_cast<int>(nodeStack.size())) {
                nodeStack.pop_back();
            }

            if (nodeIndent != static_cast<int>(nodeStack.size())) {
                break;
            }

            auto nodeType = parseNodeType(nodeTypeStr);
            auto currentNode = nodeStack.back();
            auto child = currentNode->getNodes()->add(nodeName);

            if (stream->getPosition() < stream->getLength()) {
                int ch = stream->readByte();
                if (ch == '\t') {
                    switch (nodeType) {
                        case NodeTypeV2::Empty:
                            readLine(stream);
                            break;
                        case NodeTypeV2::I4: {
                            std::string valStr = readLine(stream);
                            try { child->setWzValue(MakeInt32(std::stoi(valStr))); } catch (...) {}
                            break;
                        }
                        case NodeTypeV2::I8: {
                            std::string valStr = readLine(stream);
                            try { child->setWzValue(MakeInt64(std::stoll(valStr))); } catch (...) {}
                            break;
                        }
                        case NodeTypeV2::R8: {
                            std::string valStr = readLine(stream);
                            try { child->setWzValue(MakeDouble(std::stod(valStr))); } catch (...) {}
                            break;
                        }
                        case NodeTypeV2::String: {
                            std::string valStr = readLine(stream);
                            child->setWzValue(MakeString(valStr));
                            break;
                        }
                        case NodeTypeV2::Vector: {
                            std::string valStr = readLine(stream);
                            size_t comma = valStr.find(',');
                            if (comma != std::string::npos) {
                                try {
                                    int x = std::stoi(valStr.substr(0, comma));
                                    int y = std::stoi(valStr.substr(comma + 1));
                                    auto vec = std::make_shared<Wz_Value>();
                                    vec->setString(std::to_string(x) + "," + std::to_string(y));
                                    child->setWzValue(vec);
                                } catch (...) {}
                            }
                            break;
                        }
                        case NodeTypeV2::Property: {
                            std::string valStr = readLine(stream);
                            if (valStr == "[no_binary]") {
                            }
                            nodeStack.push_back(child);
                            break;
                        }
                        default:
                            readLine(stream);
                            break;
                    }
                } else if (ch == '\n' || ch == '\r') {
                    if (nodeType == NodeTypeV2::Property) {
                        nodeStack.push_back(child);
                    }
                } else {
                    stream->setPosition(stream->getPosition() - 1);
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

void ExtractImg(WzBinaryReader& reader, std::shared_ptr<Wz_Node> parent, std::shared_ptr<Wz_Image> wzImg) {
    std::string tag = reader.readImageObjectTypeName();

    if (tag == "Property") {
        reader.skipBytes(2);
        int32_t entries = reader.readCompressedInt32();
        for (int32_t i = 0; i < entries; i++) {
            ExtractValue(reader, parent, reader.getDecrypter(), wzImg);
        }
        // Property 标签的子节点直接添加到 parent，不要创建额外的 Property 节点
    }
    else if (tag == "Shape2D#Vector2D") {
        int32_t x = reader.readCompressedInt32();
        int32_t y = reader.readCompressedInt32();
        auto value = std::make_shared<Wz_Value>();
        value->setString(std::to_string(x) + "," + std::to_string(y));
        parent->setWzValue(value);
    }
    else if (tag == "Canvas") {
        reader.skipBytes(1);
        if (reader.readByte() == 0x01) {
            reader.skipBytes(2);
            int32_t entries = reader.readCompressedInt32();
            for (int32_t i = 0; i < entries; i++) {
                ExtractValue(reader, parent, reader.getDecrypter(), wzImg);
            }
        }
        int32_t w = reader.readCompressedInt32();
        int32_t h = reader.readCompressedInt32();
        int32_t format = reader.readCompressedInt32();
        int32_t scale = reader.readByte();
        int32_t pages = reader.readInt32();
        int32_t dataLen = reader.readInt32();

        uint32_t pngOffset = static_cast<uint32_t>(reader.getPosition());
        
        auto png = std::make_shared<Wz_Png>(w, h, dataLen,
            static_cast<Wz_TextureFormat>(format), scale, pages,
            pngOffset, wzImg,
            reader.getPartialStream()->getBaseStream());
        parent->setValue(png);

        reader.skipBytes(dataLen);
    }
    else if (tag == "Shape2D#Convex2D") {
        int32_t entries = reader.readCompressedInt32();
        for (int32_t i = 0; i < entries; i++) {
            auto virtualNode = std::make_shared<Wz_Node>();
            ExtractImg(reader, virtualNode, wzImg);
        }
    }
    else if (tag == "Sound_DX8") {
        int32_t soundVer = reader.readByte();
        if (soundVer == 1) {
            if (reader.readByte() == 0x01) {
                reader.skipBytes(2);
                int32_t entries = reader.readCompressedInt32();
                for (int32_t i = 0; i < entries; i++) {
                    ExtractValue(reader, parent, reader.getDecrypter(), wzImg);
                }
            }
        }
        int32_t dataLen = reader.readCompressedInt32();
        int32_t duration = reader.readCompressedInt32();
        reader.skipBytes(1);

        std::vector<uint8_t> mediaTypeData(48);
        for (size_t i = 0; i < 48; i++) {
            mediaTypeData[i] = reader.readByte();
        }

        int32_t fmtExLen = reader.readCompressedInt32();
        reader.skipBytes(fmtExLen);

        reader.skipBytes(dataLen);
    }
    else if (tag == "UOL") {
        reader.skipBytes(1);
        std::string link = reader.readImageString();
        auto value = std::make_shared<Wz_Value>();
        value->setString(link);
        parent->setWzValue(value);
    }
    else if (tag == "RawData") {
        int32_t ver = reader.readByte();
        if (ver == 1) {
            if (reader.readByte() == 0x01) {
                reader.skipBytes(2);
                int32_t entries = reader.readCompressedInt32();
                for (int32_t i = 0; i < entries; i++) {
                    ExtractValue(reader, parent, reader.getDecrypter(), wzImg);
                }
            }
        }
        int32_t rawDataLen = reader.readCompressedInt32();
        reader.skipBytes(rawDataLen);
    }
    else if (tag == "Canvas#Video") {
        reader.skipBytes(1);
        if (reader.readByte() == 0x01) {
            reader.skipBytes(2);
            int32_t entries = reader.readCompressedInt32();
            for (int32_t i = 0; i < entries; i++) {
                ExtractValue(reader, parent, reader.getDecrypter(), wzImg);
            }
        }
        reader.skipBytes(1);
        int32_t videoLen = reader.readCompressedInt32();
        reader.skipBytes(videoLen);
    }
}

void ExtractValue(WzBinaryReader& reader, std::shared_ptr<Wz_Node> parent, std::shared_ptr<IWzDecrypter> encKey, std::shared_ptr<Wz_Image> wzImg) {
    std::string key = reader.readImageString();
    auto child = parent->getNodes()->add(key);

    uint8_t flag = reader.readByte();
    
    switch (flag) {
        case 0x00:
            break;

        case 0x02:
        case 0x0B:
            child->setWzValue(MakeInt16(reader.readInt16()));
            break;

        case 0x03:
        case 0x13:
            child->setWzValue(MakeInt32(reader.readCompressedInt32()));
            break;

        case 0x14:
            child->setWzValue(MakeInt64(reader.readCompressedInt64()));
            break;

        case 0x04:
            child->setWzValue(MakeFloat(reader.readCompressedSingle()));
            break;

        case 0x05:
            child->setWzValue(MakeDouble(reader.readDouble()));
            break;

        case 0x08:
            child->setWzValue(MakeString(reader.readImageString()));
            break;

        case 0x09: {
            int32_t objDataLen = reader.readInt32();
            int64_t eob = reader.getPosition() + objDataLen;
            ExtractImg(reader, child, wzImg);
            if (reader.getPosition() != eob) {
                reader.setPosition(eob);
            }
            break;
        }

        default:
            break;
    }
}

bool Wz_Image::tryExtract() {
    if (isExtracted && ExtractedNode) {
        return true;
    }

    auto wzFile = getWzFile();
    if (!wzFile) {
        if (!Node.lock()) {
            auto node = std::make_shared<Wz_Node>(Name);
            Node = node;
            ExtractedNode = node;
        }
        isExtracted = true;
        return true;
    }

    if (Offset == 0) {
        return false;
    }

    auto node = Node.lock();
    if (!node) {
        node = std::make_shared<Wz_Node>(Name);
        Node = node;
        ExtractedNode = node;
    } else if (!ExtractedNode) {
        ExtractedNode = node;
    }

    try {
        auto fileStream = wzFile->getFStream();
        if (!fileStream || !fileStream->is_open()) {
            return false;
        }

        auto stream = std::make_shared<PartialStream>(fileStream, Offset, Size);

        // Checksum verification (like C# TryExtract)
        if (!isChecksumChecked) {
            int32_t calculatedCs = CalcCheckSum(stream, Size);
            isChecksumChecked = true;
            // Compare with expected checksum - if mismatch, return false
            if (calculatedCs != Checksum) {
                return false;
            }
        }

        if (IsTextFormatV1(stream)) {
            if (ExtractTextImgV1(stream, node)) {
                isExtracted = true;
                return true;
            }
        }

        if (IsTextFormatV2(stream)) {
            if (ExtractTextImgV2(stream, node)) {
                isExtracted = true;
                return true;
            }
        }

        // Try to detect encryption
        std::shared_ptr<IWzDecrypter> encKey = nullptr;
        if (!isChecksumEncrypted) {
            encKey = TryDetectEnc(stream, encType);
            isChecksumEncrypted = true;
            // std::cerr << "tryExtract: TryDetectEnc 返回 " << (encKey ? "成功" : "失败") << std::endl;
        }

    if (!encKey) {
        // Encryption detection failed
        // std::cerr << "tryExtract: encKey 为空，返回 false" << std::endl;
        return false;
    }

        stream->setPosition(0);
        auto reader = std::make_shared<WzBinaryReader>(stream, encKey);

        try {
            ExtractImg(*reader, node, shared_from_this());
            // std::cerr << "tryExtract: ExtractImg 成功" << std::endl;
        } catch (const std::exception& e) {
            // std::cerr << "tryExtract: ExtractImg 异常: " << e.what() << std::endl;
            return false;
        } catch (...) {
            // std::cerr << "tryExtract: ExtractImg 未知异常" << std::endl;
            return false;
        }

        auto nodes = node->getNodes();
        if (nodes && nodes->getCount() == 1) {
            auto propNode = (*nodes)[0];
            if (propNode && propNode->getText() == "Property") {
                auto propNodes = propNode->getNodes();
                if (propNodes && propNodes->getCount() > 0) {
                    for (size_t i = 0; i < propNodes->getCount(); i++) {
                        auto child = (*propNodes)[i];
                        if (child) {
                            child->setParentNode({});
                            node->getNodes()->add(child);
                        }
                    }
                    nodes->clear();
                }
            }
        }

        auto ownerNode = OwnerNode.lock();
        if (ownerNode) {
            auto existingNode = ownerNode->getNodes()->operator[](Name);
            if (existingNode) {
                ownerNode->getNodes()->remove(existingNode);
            }
            node->setParentNode({});
            ownerNode->getNodes()->add(node);
        }

        isExtracted = true;
        return true;
    } catch (const std::exception& e) {
        isExtracted = false;
        auto n = Node.lock();
        if (n && n->getNodes()) {
            n->getNodes()->clear();
        }
        return false;
    }
}

std::vector<uint8_t> Wz_Image::extractPng() {
    auto node = Node.lock();
    if (!node) return {};

    auto png = node->getValue<Wz_Png>();
    if (png) {
        return png->extractPng();
    }
    return {};
}

std::vector<uint8_t> Wz_Image::extractSound() {
    return {};
}

void Wz_Image::copyTo(std::vector<uint8_t>& data, int64_t offset) {
}

int64_t Wz_Image::getDataLength() const {
    return Size;
}

} // namespace WzLibCpp

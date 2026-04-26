#include "Wz_BinaryReader.hpp"
#include <stdexcept>
#include <cctype>

namespace WzLibCpp {

static bool isValidTag(const std::string& tag) {
    return tag == "Property" || tag == "Shape2D#Vector2D" || tag == "Canvas" ||
           tag == "Shape2D#Convex2D" || tag == "Sound_DX8" || tag == "UOL" ||
           tag == "RawData" || tag == "Canvas#Video";
}

std::shared_ptr<IWzDecrypter> TryDetectEncryption(std::shared_ptr<PartialStream> stream) {
    if (!stream) return nullptr;
    
    int64_t oldPos = stream->getPosition();
    
    // 尝试 BMS（无加密）
    stream->setPosition(0);
    try {
        auto reader = std::make_shared<WzBinaryReader>(stream, nullptr);
        std::string tag = reader->readImageObjectTypeName();
        if (isValidTag(tag)) {
            stream->setPosition(oldPos);
            return nullptr; // BMS 不需要密钥
        }
    } catch (...) {
        // 继续尝试其他加密类型
    }
    
    // 尝试 KMS
    stream->setPosition(0);
    try {
        static const uint8_t iv_kms[4] = { 0xb9, 0x7d, 0x63, 0xe9 };
        auto kmsKey = std::make_shared<Wz_CryptoKey>(iv_kms);
        auto reader = std::make_shared<WzBinaryReader>(stream, kmsKey);
        std::string tag = reader->readImageObjectTypeName();
        if (isValidTag(tag)) {
            stream->setPosition(oldPos);
            return kmsKey;
        }
    } catch (...) {
        // 继续尝试
    }
    
    // 尝试 GMS
    stream->setPosition(0);
    try {
        static const uint8_t iv_gms[4] = { 0x4d, 0x23, 0xc7, 0x2b };
        auto gmsKey = std::make_shared<Wz_CryptoKey>(iv_gms);
        auto reader = std::make_shared<WzBinaryReader>(stream, gmsKey);
        std::string tag = reader->readImageObjectTypeName();
        if (isValidTag(tag)) {
            stream->setPosition(oldPos);
            return gmsKey;
        }
    } catch (...) {
        // 继续尝试
    }
    
    stream->setPosition(oldPos);
    return nullptr;
}

std::shared_ptr<IWzDecrypter> TryDetectEncryptionWithType(std::shared_ptr<PartialStream> stream, Wz_CryptoKeyType& outType) {
    outType = Wz_CryptoKeyType::None;
    
    if (!stream) return nullptr;
    
    int64_t oldPos = stream->getPosition();
    
    // 尝试 BMS（无加密）
    stream->setPosition(0);
    try {
        auto reader = std::make_shared<WzBinaryReader>(stream, nullptr);
        std::string tag = reader->readImageObjectTypeName();
        if (isValidTag(tag)) {
            stream->setPosition(oldPos);
            outType = Wz_CryptoKeyType::None;
            return nullptr;
        }
    } catch (...) {}
    
    // 尝试 KMS
    stream->setPosition(0);
    try {
        static const uint8_t iv_kms[4] = { 0xb9, 0x7d, 0x63, 0xe9 };
        auto kmsKey = std::make_shared<Wz_CryptoKey>(iv_kms);
        auto reader = std::make_shared<WzBinaryReader>(stream, kmsKey);
        std::string tag = reader->readImageObjectTypeName();
        if (isValidTag(tag)) {
            stream->setPosition(oldPos);
            outType = Wz_CryptoKeyType::KMS;
            return kmsKey;
        }
    } catch (...) {}
    
    // 尝试 GMS
    stream->setPosition(0);
    try {
        static const uint8_t iv_gms[4] = { 0x4d, 0x23, 0xc7, 0x2b };
        auto gmsKey = std::make_shared<Wz_CryptoKey>(iv_gms);
        auto reader = std::make_shared<WzBinaryReader>(stream, gmsKey);
        std::string tag = reader->readImageObjectTypeName();
        if (isValidTag(tag)) {
            stream->setPosition(oldPos);
            outType = Wz_CryptoKeyType::GMS;
            return gmsKey;
        }
    } catch (...) {}
    
    stream->setPosition(oldPos);
    return nullptr;
}

} // namespace WzLibCpp

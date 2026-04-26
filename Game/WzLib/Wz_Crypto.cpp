// Include core crypto interface header (stable absolute path to ensure portability in patch-land)
#include "Wz_Crypto.hpp"
// Fallback: ensure fixed-width integer types are available
#include <cstdint>
// Use standard local headers from include path
// These headers exist under: include/WzLibCpp/Wz/
#include "Wz_File.hpp"
#include "Wz_Header.hpp"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <regex>
#include "AES/AES.h"

namespace WzLibCpp {

const uint8_t Wz_Crypto::iv_gms[4] = { 0x4d, 0x23, 0xc7, 0x2b };
const uint8_t Wz_Crypto::iv_kms[4] = { 0xb9, 0x7d, 0x63, 0xe9 };

std::shared_ptr<Wz_NonOpCryptoKey> Wz_NonOpCryptoKey::Instance = std::make_shared<Wz_NonOpCryptoKey>();

Wz_Crypto::Wz_Crypto()
    : keys_bms(Wz_NonOpCryptoKey::Instance),
      keys_gms(std::make_shared<Wz_CryptoKey>(iv_gms)),
      keys_kms(std::make_shared<Wz_CryptoKey>(iv_kms)),
      currentKey(nullptr),
      enc_type(Wz_CryptoKeyType::Unknown) {
}

void Wz_Crypto::reset() {
    encryption_detected = false;
    listwz = false;
    enc_type = Wz_CryptoKeyType::Unknown;
    currentKey = nullptr;
    list.clear();
}

bool Wz_Crypto::listContains(const std::string& name) {
    auto it = std::find(list.begin(), list.end(), name);
    if (it != list.end()) {
        list.erase(it);
        return true;
    }
    return false;
}

void Wz_Crypto::removeFromList(const std::string& name) {
    auto it = std::find(list.begin(), list.end(), name);
    if (it != list.end()) {
        list.erase(it);
    }
}

void Wz_Crypto::setEncType(Wz_CryptoKeyType type) {
    currentKey = getKeyByType(type);
    enc_type = type;
}

std::shared_ptr<IWzDecrypter> Wz_Crypto::getKeyByType(Wz_CryptoKeyType keyType) const {
    switch (keyType) {
        case Wz_CryptoKeyType::None: return Wz_NonOpCryptoKey::Instance;
        case Wz_CryptoKeyType::BMS: return keys_bms;
        case Wz_CryptoKeyType::KMS: return keys_kms;
        case Wz_CryptoKeyType::GMS: return keys_gms;
        case Wz_CryptoKeyType::Unknown:
        default: return nullptr;
    }
}

static inline bool isLegalNodeNameImpl(const std::string& nodeName) {
    if (nodeName.empty() || nodeName.size() < 4) {
        return false;
    }
    
    // 检查是否以 .img 或 .lua 结尾
    if (nodeName.size() >= 4) {
        std::string suffix = nodeName.substr(nodeName.size() - 4);
        if (suffix == ".img" || suffix == ".lua") {
            return true;
        }
    }
    
    // 检查是否为纯 ASCII 可打印字符 (0x20 - 0x7f)
    for (char c : nodeName) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 0x20 || uc > 0x7f) {
            return false;
        }
    }
    
    return true;
}

void Wz_Crypto::detectEncryption(Wz_File* f) {
    if (!f || !f->getHeader()) return;
    
    auto header = f->getHeader();
    auto wzFile = f->getHeader()->getFileName();
    
    std::ifstream file(wzFile, std::ios::binary);
    if (!file.is_open()) return;
    
    int64_t dataStartPos = header->getDataStartPosition();
    file.seekg(dataStartPos);
    
    // 读取节点数量
    int32_t nodeCount = 0;
    int8_t firstByte;
    if (!file.read(reinterpret_cast<char*>(&firstByte), 1)) {
        file.close();
        return;
    }
    
    // 解压缩整数格式
    if (firstByte == -128) {
        int32_t val;
        if (!file.read(reinterpret_cast<char*>(&val), 4)) {
            file.close();
            return;
        }
        nodeCount = val;
    } else if (firstByte >= -127 && firstByte <= 127) {
        nodeCount = firstByte;
    } else {
        file.close();
        return;
    }
    
    // PKG2 格式特殊处理
    if (header->getSignature() == "PKG2") {
        file.seekg(dataStartPos);
        uint8_t nextByte;
        if (!file.read(reinterpret_cast<char*>(&nextByte), 1)) {
            file.close();
            return;
        }
        
        if (nextByte != 0x04 && nextByte != 0x03) {
            file.seekg(dataStartPos);
            int32_t offsetCount = 0;
            int8_t sb;
            if (file.read(reinterpret_cast<char*>(&sb), 1)) {
                if (sb == -128) {
                    int32_t val;
                    if (file.read(reinterpret_cast<char*>(&val), 4)) {
                        offsetCount = val;
                    }
                } else {
                    offsetCount = sb;
                }
            }
            
            if (offsetCount == nodeCount) {
                file.close();
                return; // 没有目录项
            }
        }
        nodeCount = 1; // 至少一个节点
    }
    
    if (nodeCount <= 0) {
        file.close();
        return;
    }
    
    // 跳过 1 字节
    file.seekg(1, std::ios::cur);
    
    // 读取字符串长度
    int8_t sbLen;
    if (!file.read(reinterpret_cast<char*>(&sbLen), 1)) {
        file.close();
        return;
    }
    
    // 长度编码: 负数表示实际长度，正数表示索引
    if (sbLen >= 0) {
        file.close();
        return;
    }
    
    // 计算实际字符串长度
    int32_t strLen;
    if (sbLen == -128) {
        // 4 字节长度
        int32_t lenVal;
        if (!file.read(reinterpret_cast<char*>(&lenVal), 4)) {
            file.close();
            return;
        }
        strLen = lenVal;
    } else {
        strLen = -sbLen;
    }
    
    if (strLen <= 0 || strLen > 4096) {
        file.close();
        return;
    }
    
    // 读取加密的字符串数据
    std::vector<uint8_t> encryptedData(strLen);
    if (!file.read(reinterpret_cast<char*>(encryptedData.data()), strLen)) {
        file.close();
        return;
    }
    
    // 尝试不同的加密方式解密
    // WZ 字符串加密算法: data[i] ^= (0xAA + i)
    
    std::string decryptedStr;
    bool detected = false;
    Wz_CryptoKeyType detectedType = Wz_CryptoKeyType::Unknown;
    
    // 尝试 BMS (只进行 XOR 解密)
    {
        std::string testStr;
        testStr.reserve(strLen);
        for (int32_t i = 0; i < strLen; i++) {
            testStr += static_cast<char>(encryptedData[i] ^ (0xAA + i));
        }
        if (isLegalNodeNameImpl(testStr)) {
            detected = true;
            detectedType = Wz_CryptoKeyType::BMS;
            decryptedStr = testStr;
        }
    }
    
    // 尝试 KMS (先 AES 解密，再 XOR)
    if (!detected) {
        auto kmsKey = std::dynamic_pointer_cast<Wz_CryptoKey>(keys_kms);
        if (kmsKey) {
            // 创建一个临时副本进行解密测试
            std::vector<uint8_t> tempData = encryptedData;
            kmsKey->decrypt(tempData, 0);
            
            std::string testStr;
            testStr.reserve(strLen);
            for (int32_t i = 0; i < strLen; i++) {
                testStr += static_cast<char>(tempData[i] ^ (0xAA + i));
            }
            if (isLegalNodeNameImpl(testStr)) {
                detected = true;
                detectedType = Wz_CryptoKeyType::KMS;
                decryptedStr = testStr;
            }
        }
    }
    
    // 尝试 GMS (先 AES 解密，再 XOR)
    if (!detected) {
        auto gmsKey = std::dynamic_pointer_cast<Wz_CryptoKey>(keys_gms);
        if (gmsKey) {
            std::vector<uint8_t> tempData = encryptedData;
            gmsKey->decrypt(tempData, 0);
            
            std::string testStr;
            testStr.reserve(strLen);
            for (int32_t i = 0; i < strLen; i++) {
                testStr += static_cast<char>(tempData[i] ^ (0xAA + i));
            }
            if (isLegalNodeNameImpl(testStr)) {
                detected = true;
                detectedType = Wz_CryptoKeyType::GMS;
                decryptedStr = testStr;
            }
        }
    }
    
    file.close();
    
    if (detected) {
        enc_type = detectedType;
        currentKey = getKeyByType(detectedType);
        encryption_detected = true;
    }
}

bool Wz_Crypto::isLegalNodeName(const std::string& nodeName) const {
    return isLegalNodeNameImpl(nodeName);
}

void Wz_Crypto::loadListWz(const std::string& path) {
    std::string listPath = path;
    if (!listPath.empty() && listPath.back() != '/' && listPath.back() != '\\') {
        listPath += "/";
    }
    listPath += "List.wz";
    
    std::ifstream file(listPath, std::ios::binary);
    if (!file.is_open()) return;
    
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = static_cast<std::streamsize>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    if (fileSize < 5) {
        file.close();
        return;
    }
    
    // 跳过前 4 字节
    file.seekg(4);
    
    // 读取第一个字节检测加密类型
    uint8_t checkByte;
    if (!file.read(reinterpret_cast<char*>(&checkByte), 1)) {
        file.close();
        return;
    }
    
    // 检测加密类型 (通过第一个字节与不同 IV 解密后是否为 'd')
    bool isGms = (checkByte ^ iv_gms[0]) == 'd';
    bool isKms = (checkByte ^ iv_kms[0]) == 'd';
    
    if (!isGms && !isKms) {
        file.close();
        return;
    }
    
    enc_type = isGms ? Wz_CryptoKeyType::GMS : Wz_CryptoKeyType::KMS;
    currentKey = isGms ? keys_gms : keys_kms;
    listwz = true;
    
    // 返回文件开头重新读取
    file.seekg(0, std::ios::beg);
    
    // 获取解密函数
    auto decryptByte = [this](size_t i) -> uint8_t {
        return enc_type == Wz_CryptoKeyType::GMS ? iv_gms[i % 4] : iv_kms[i % 4];
    };
    
    // 循环读取所有条目
    while (true) {
        auto curPos = file.tellg();
        if (curPos < 0 || curPos >= fileSize) break;
        
        // 读取长度
        uint32_t len;
        if (!file.read(reinterpret_cast<char*>(&len), 4)) break;
        
        // 实际长度为 len * 2 (UTF-16)
        uint32_t actualLen = len * 2;
        
        if (actualLen > 4096) {
            break;
        }
        
        // 检查是否有足够的字节
        auto nextPos = file.tellg();
        if (nextPos < 0 || nextPos + static_cast<std::streamoff>(actualLen) > fileSize) {
            break;
        }
        
        // 读取并解密
        std::string entry;
        for (uint32_t i = 0; i < actualLen; i++) {
            uint8_t b;
            if (!file.read(reinterpret_cast<char*>(&b), 1)) break;
            b ^= decryptByte(i);
            
            // UTF-16LE 解码 (取低字节)
            if (i % 2 == 0) {
                entry += static_cast<char>(b);
            }
        }
        
        // 跳过 2 字节
        file.seekg(2, std::ios::cur);
        
        // 验证并添加
        if (!entry.empty() && entry != "dummy") {
            // 修正 .im/ -> .img
            size_t dotPos = entry.rfind(".im/");
            if (dotPos != std::string::npos) {
                entry.replace(dotPos, 4, ".img");
            }
            list.push_back(entry);
        }
    }
    
    // 移除 dummy 条目
    auto it = std::find(list.begin(), list.end(), "dummy");
    if (it != list.end()) {
        list.erase(it);
    }
    
    file.close();
}

// ============= Wz_CryptoKey 实现 =============

Wz_CryptoKey::Wz_CryptoKey(const uint8_t* iv)
    : iv(iv) {
}

uint8_t Wz_CryptoKey::operator[](int index) const {
    if (index < 0) return 0;
    size_t idx = static_cast<size_t>(index);
    if (keys.empty() || idx >= keys.size()) {
        const_cast<Wz_CryptoKey*>(this)->ensureKeySize(idx + 1);
    }
    return keys[idx];
}

void Wz_CryptoKey::ensureKeySize(size_t size) {
    if (!keys.empty() && keys.size() >= size) {
        return;
    }
    
    // 对齐到 batch_size (65536) 字节
    size_t batchSize = 0x10000;
    size_t newSize = ((size + batchSize - 1) / batchSize) * batchSize;
    size_t startIndex = keys.size();
    
    if (keys.empty()) {
        keys.resize(newSize);
    } else {
        keys.resize(newSize);
    }
    
    // 检查 IV 是否为 0
    int32_t ivInt = 0;
    for (int i = 0; i < 4; i++) {
        ivInt |= static_cast<int32_t>(iv[i]) << (i * 8);
    }
    if (ivInt == 0) {
        return; // IV 为 0 时不需要加密
    }
    
    // 使用内置 AES 生成密钥流（块级）
    AES aes(256, 16);
    
    // 如果是从头开始生成，先处理第一个块
    if (startIndex < 16) {
        uint8_t block[16];
        for (int n = 0; n < 16; n++) block[n] = iv[n % 4];
        unsigned int outLen = 0;
        uint8_t* enc = aes.EncryptECB(block, 16, const_cast<uint8_t*>(Wz_CryptoKey::getAesKey()), outLen);
        if (enc && outLen >= 16) {
            for (int n = 0; n < 16 && startIndex + n < newSize; n++) {
                keys[startIndex + n] = enc[n];
            }
        }
        delete[] enc;
        startIndex += 16;
    }
    
    // 后续块：使用前一个密文块作为输入 (CBC-like 方式生成密钥流)
    while (startIndex + 16 <= newSize) {
        uint8_t block[16];
        for (int n = 0; n < 16; n++) block[n] = keys[startIndex - 16 + n];
        unsigned int outLen = 0;
        uint8_t* enc = aes.EncryptECB(block, 16, const_cast<uint8_t*>(Wz_CryptoKey::getAesKey()), outLen);
        if (enc && outLen >= 16) {
            for (int n = 0; n < 16; n++) keys[startIndex + n] = enc[n];
        }
        delete[] enc;
        startIndex += 16;
    }
}

void Wz_CryptoKey::decrypt(uint8_t* buffer, size_t startIndex, size_t length) {
    if (length == 0 || !buffer) return;
    decryptInternal(buffer + startIndex, length, startIndex);
}

void Wz_CryptoKey::decrypt(std::vector<uint8_t>& data, size_t keyOffset) {
    if (data.empty()) return;
    decryptInternal(data.data(), data.size(), keyOffset);
}

void Wz_CryptoKey::decryptInternal(uint8_t* data, size_t length, size_t keyOffset) {
    if (length == 0 || !data) return;
    
    size_t requiredSize = keyOffset + length;
    if (keys.empty() || requiredSize > keys.size()) {
        ensureKeySize(requiredSize);
    }
    
    // XOR 解密
    // 对于对齐的数据，按 4 字节或 8 字节处理以提高性能
    size_t i = 0;
    
    // SSE2 优化 (如果可用)
    // 这里使用简单的 4 字节处理作为基准实现
    
    // 处理 4 字节对齐的部分
    while (i + 4 <= length) {
        uint32_t* data32 = reinterpret_cast<uint32_t*>(data + i);
        const uint32_t* key32 = reinterpret_cast<const uint32_t*>(keys.data() + keyOffset + i);
        *data32 ^= *key32;
        i += 4;
    }
    
    // 处理剩余字节
    while (i < length) {
        data[i] ^= keys[keyOffset + i];
        i++;
    }
}

// ============= Wz_NonOpCryptoKey 实现 =============

// ============= Wz_CryptoKey 静态方法 =============

const uint8_t* Wz_CryptoKey::getAesKey() {
    static const uint8_t aesKey[32] = {
        0x13, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00,
        0xB4, 0x00, 0x00, 0x00,
        0x1B, 0x00, 0x00, 0x00,
        0x0F, 0x00, 0x00, 0x00,
        0x33, 0x00, 0x00, 0x00,
        0x52, 0x00, 0x00, 0x00
    };
    return aesKey;
}

} // namespace WzLibCpp

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace WzLibCpp {

class Wz_File;

enum class Wz_CryptoKeyType {
    None = 0,
    BMS = 1,
    KMS = 2,
    GMS = 3,
    Unknown = 4
};

class IWzDecrypter {
public:
    virtual ~IWzDecrypter() = default;
    virtual uint8_t operator[](int index) const = 0;
    virtual void decrypt(uint8_t* buffer, size_t startIndex, size_t length) = 0;
    virtual void decrypt(std::vector<uint8_t>& data, size_t keyOffset = 0) = 0;
};

class Wz_Crypto {
public:
    Wz_Crypto();
    void reset();
    bool listContains(const std::string& name);
    void loadListWz(const std::string& path);
    void detectEncryption(Wz_File* f);

    bool encryption_detected = false;
    bool listwz = false;

    std::shared_ptr<IWzDecrypter> getCurrentKey() const { return currentKey; }
    const std::vector<std::string>& getList() const { return list; }
    void removeFromList(const std::string& name);
    Wz_CryptoKeyType getEncType() const { return enc_type; }
    void setEncType(Wz_CryptoKeyType type);

    std::shared_ptr<IWzDecrypter> getKeyByType(Wz_CryptoKeyType keyType) const;

private:
    bool isLegalNodeName(const std::string& nodeName) const;

    static const uint8_t iv_gms[4];
    static const uint8_t iv_kms[4];
    static const uint8_t aesKey[32];

    std::shared_ptr<IWzDecrypter> keys_bms;
    std::shared_ptr<IWzDecrypter> keys_gms;
    std::shared_ptr<IWzDecrypter> keys_kms;
    std::shared_ptr<IWzDecrypter> currentKey;
    Wz_CryptoKeyType enc_type;
    std::vector<std::string> list;
};

class Wz_CryptoKey : public IWzDecrypter {
public:
    explicit Wz_CryptoKey(const uint8_t* iv);
    virtual ~Wz_CryptoKey() = default;

    uint8_t operator[](int index) const override;
    void decrypt(uint8_t* buffer, size_t startIndex, size_t length) override;
    void decrypt(std::vector<uint8_t>& data, size_t keyOffset = 0) override;

    static const uint8_t* getAesKey();

private:
    void ensureKeySize(size_t size);
    void decryptInternal(uint8_t* data, size_t length, size_t keyOffset);

    mutable std::vector<uint8_t> keys;
    const uint8_t* iv;
};

class Wz_NonOpCryptoKey : public IWzDecrypter {
public:
    static std::shared_ptr<Wz_NonOpCryptoKey> Instance;

    Wz_NonOpCryptoKey() = default;
    virtual ~Wz_NonOpCryptoKey() = default;

    uint8_t operator[](int index) const override { return 0; }
    void decrypt(uint8_t* buffer, size_t startIndex, size_t length) override {}
    void decrypt(std::vector<uint8_t>& data, size_t keyOffset = 0) override {}
};

} // namespace WzLibCpp
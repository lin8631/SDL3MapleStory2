#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <array>
#include <cstddef>

namespace WzLibCpp::Cryptography {

class Snow2CryptoTransform {
public:
    Snow2CryptoTransform(const uint8_t* key, size_t keyLen, const uint8_t* iv, size_t ivLen, bool encrypting);
    ~Snow2CryptoTransform();

    size_t getInputBlockSize() const { return 4; }
    size_t getOutputBlockSize() const { return 4; }
    bool canTransformMultipleBlocks() const { return true; }
    bool canReuseTransform() const { return false; }

    size_t transformBlock(const uint8_t* inputBuffer, size_t inputOffset, size_t inputCount,
                         uint8_t* outputBuffer, size_t outputOffset);
    void transformFinalBlock(const uint8_t* inputBuffer, size_t inputOffset, size_t inputCount,
                             uint8_t* outputBuffer, size_t outputOffset);

private:
    void loadKey(const uint8_t* key, size_t keyLen, const uint8_t* iv, size_t ivLen);
    void refreshKeyStream();
    uint32_t generateKeyStream();
    uint32_t lfsr();
    uint32_t fsm();

    bool encrypting;
    uint32_t s15, s14, s13, s12, s11, s10, s9, s8, s7, s6, s5, s4, s3, s2, s1, s0;
    uint32_t r1, r2;
    std::vector<uint32_t> keyStream;
    size_t curIndex;
    bool disposed = false;
};

} // namespace WzLibCpp::Cryptography

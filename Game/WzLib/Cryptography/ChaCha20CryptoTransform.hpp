#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <array>

namespace WzLibCpp::Cryptography {

class ChaCha20CryptoTransform {
public:
    static constexpr size_t AllowedKeyLength = 32;
    static constexpr size_t AllowedNonceLength = 12;
    static constexpr size_t ProcessBytesAtTime = 64;
    static constexpr size_t StateLength = 16;

    ChaCha20CryptoTransform(const uint8_t* key, const uint8_t* nonce, uint32_t counter);
    ~ChaCha20CryptoTransform();

    void transformBlock(const uint8_t* input, uint8_t* output, size_t length);
    void transformFinalBlock(const uint8_t* input, uint8_t* output, size_t length);

    size_t getInputBlockSize() const { return ProcessBytesAtTime; }
    size_t getOutputBlockSize() const { return ProcessBytesAtTime; }

private:
    void quarterRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d);
    void chacha20Block(uint8_t* output);
    void chacha20BlockImpl(std::array<uint32_t, 16>& state);

    std::array<uint32_t, 16> state;
    uint32_t counter;
    bool disposed = false;
};

} // namespace WzLibCpp::Cryptography

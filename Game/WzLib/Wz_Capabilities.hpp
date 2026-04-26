#pragma once
#include <cstdint>

namespace WzLibCpp {

/// <summary>
/// Wz文件的能力标志。
/// </summary>
enum class Wz_Capabilities : uint32_t {
    Default = 0,
    EncverMissing = 1,
};

// Bitwise operators for Wz_Capabilities
inline Wz_Capabilities operator|(Wz_Capabilities a, Wz_Capabilities b) {
    return static_cast<Wz_Capabilities>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline Wz_Capabilities operator&(Wz_Capabilities a, Wz_Capabilities b) {
    return static_cast<Wz_Capabilities>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline Wz_Capabilities operator^(Wz_Capabilities a, Wz_Capabilities b) {
    return static_cast<Wz_Capabilities>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}

inline Wz_Capabilities operator~(Wz_Capabilities a) {
    return static_cast<Wz_Capabilities>(~static_cast<uint32_t>(a));
}

inline Wz_Capabilities& operator|=(Wz_Capabilities& a, Wz_Capabilities b) {
    a = a | b;
    return a;
}

inline Wz_Capabilities& operator&=(Wz_Capabilities& a, Wz_Capabilities b) {
    a = a & b;
    return a;
}

inline Wz_Capabilities& operator^=(Wz_Capabilities& a, Wz_Capabilities b) {
    a = a ^ b;
    return a;
}

} // namespace WzLibCpp
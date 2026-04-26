#pragma once
#include <string>
#include <memory>
#include <cstdint>

namespace WzLibCpp::Utilities {

class IWzStringPool {
public:
    virtual ~IWzStringPool() = default;

    virtual bool tryGet(int64_t offset, std::string& s) = 0;
    virtual std::string getOrAdd(int64_t offset, const std::string& chars) = 0;
    virtual void reset() = 0;
};

} // namespace WzLibCpp::Utilities

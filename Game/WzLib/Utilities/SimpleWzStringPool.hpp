#pragma once
#include "IWzStringPool.hpp"
#include <string>
#include <unordered_map>
#include <cstdint>

namespace WzLibCpp::Utilities {

class SimpleWzStringPool : public IWzStringPool {
public:
    SimpleWzStringPool();
    virtual ~SimpleWzStringPool() = default;

    bool tryGet(int64_t offset, std::string& s) override;
    std::string getOrAdd(int64_t offset, const std::string& chars) override;
    void reset() override;

private:
    std::unordered_map<int64_t, std::string> cache;
};

} // namespace WzLibCpp::Utilities

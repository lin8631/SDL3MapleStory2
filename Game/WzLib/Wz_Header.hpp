#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <numeric>
#include <cctype>
#include <bit>
#include <span>
#include <ranges>
#include <iterator>
#include <utility>
#include <iostream>
#include "Wz_Capabilities.hpp"

namespace WzLibCpp {

// Forward declarations
class IWzVersionDetector;

class Wz_Header {
public:
    static constexpr const char* PKG1 = "PKG1";
    static constexpr const char* PKG2 = "PKG2";

    Wz_Header(const std::string& signature, const std::string& copyright, 
              const std::string& file_name, int32_t head_size, 
              int64_t data_size, int64_t file_size, int64_t dataStartPosition)
        : Signature(signature), Copyright(copyright), FileName(file_name),
          HeaderSize(head_size), DataSize(data_size), FileSize(file_size),
          DataStartPosition(dataStartPosition), VersionChecked(false) {}

    const std::string& getSignature() const { return Signature; }
    const std::string& getCopyright() const { return Copyright; }
    const std::string& getFileName() const { return FileName; }

    int32_t getHeaderSize() const { return HeaderSize; }
    int64_t getDataSize() const { return DataSize; }
    int64_t getFileSize() const { return FileSize; }
    int64_t getDataStartPosition() const { return DataStartPosition; }
    int64_t getDirEndPosition() const { return DirEndPosition; }
    void setDirEndPosition(int64_t pos) { DirEndPosition = pos; }

    bool getVersionChecked() const { return VersionChecked; }
    void setVersionChecked(bool checked) { VersionChecked = checked; }

    std::shared_ptr<Wz_Capabilities> getCapabilities() const { return Capabilities; }
    void setCapabilities(std::shared_ptr<Wz_Capabilities> caps) { Capabilities = caps; }

    int32_t getWzVersion() const {
        return versionDetector ? versionDetector->getWzVersion() : 0;
    }
    
    uint32_t getHashVersion() const {
        return versionDetector ? versionDetector->getHashVersion() : 0;
    }
    
    bool tryGetNextVersion() {
        return versionDetector ? versionDetector->tryGetNextVersion() : false;
    }

    uint32_t getPkg2Hash1() const {
        auto pkg2 = std::dynamic_pointer_cast<Pkg2WzVersionDetector>(versionDetector);
        if (pkg2) {
            return pkg2->getHash1();
        }
        throw std::logic_error("Not supported");
    }

    bool hasCapabilities(Wz_Capabilities cap) const {
        return Capabilities && ((*Capabilities & cap) == cap);
    }

    static uint32_t calcHashVersion(int32_t wzVersion) {
        uint32_t sum = 0;
        std::string versionStr = std::to_string(wzVersion);
        for (char c : versionStr) {
            sum = (sum << 5) + (static_cast<uint32_t>(c) + 1);
        }
        return sum;
    }

    // For pkg2 wz files, the version is a string that stored in MapleStory.exe, we can't find it without disassembling.
    static uint32_t calcHashVersionPkg2(const std::string& wzVersion) {
        // Convert string to bytes
        std::string bytes;
        bytes.reserve(wzVersion.size() * sizeof(char));
        for (char c : wzVersion) {
            bytes.push_back(static_cast<char>(c & 0xFF));
        }
        
        uint32_t hash = 0x811C9DC5;
        for (char c : bytes) {
            hash = (hash ^ static_cast<uint32_t>(static_cast<unsigned char>(c))) * 0x1000193;
        }
        hash = 0x85EBCA6B * (hash ^ (hash >> 13));
        return hash ^ (hash >> 16);
    }

    void setWzVersion(int32_t wzVersion) {
        versionDetector = std::make_shared<FixedVersion>(wzVersion);
    }

    void setOrdinalVersionDetector(int32_t encryptedVersion) {
        versionDetector = std::make_shared<OrdinalVersionDetector>(encryptedVersion);
    }

    void setWzVersionPkg2(uint32_t hash1, uint32_t hash2) {
        versionDetector = std::make_shared<Pkg2WzVersionDetector>(hash1, hash2);
    }

    // Interface for version detection
    class IWzVersionDetector {
    public:
        virtual ~IWzVersionDetector() = default;
        virtual bool tryGetNextVersion() = 0;
        virtual int32_t getWzVersion() const = 0;
        virtual uint32_t getHashVersion() const = 0;
    };

    class FixedVersion : public IWzVersionDetector {
    public:
        FixedVersion(int32_t wzVersion) 
            : WzVersion(wzVersion), HashVersion(calcHashVersion(wzVersion)), hasReturned(false) {}

        bool tryGetNextVersion() override {
            if (!hasReturned) {
                hasReturned = true;
                return true;
            }
            return false;
        }

        int32_t getWzVersion() const override { return WzVersion; }
        uint32_t getHashVersion() const override { return HashVersion; }

    private:
        int32_t WzVersion;
        uint32_t HashVersion;
        bool hasReturned;
    };

    class OrdinalVersionDetector : public IWzVersionDetector {
    public:
        OrdinalVersionDetector(int32_t encryptVersion)
            : EncryptedVersion(encryptVersion), startVersion(-1), searchStage(0) {}

        int32_t getEncryptedVersion() const { return EncryptedVersion; }

        int32_t getWzVersion() const override {
            if (versionTest.empty()) return 0;
            return versionTest.back();
        }

        uint32_t getHashVersion() const override {
            if (hasVersionTest.empty()) return 0;
            return hasVersionTest.back();
        }

        bool tryGetNextVersion() override {
            // 第一阶段：尝试已知版本映射（72 和 3）
            if (searchStage == 0) {
                static const std::vector<std::pair<int32_t, uint32_t>> knownVersions = {
                    {72, calcHashVersion(72)},   // hashVersion=1843
                    {3, calcHashVersion(3)}      // hashVersion=52
                };
                
                for (size_t i = 0; i < knownVersions.size(); i++) {
                    const auto& [wzVer, hashVer] = knownVersions[i];
                    uint32_t enc = 0xFF ^ ((hashVer >> 24) & 0xFF) ^ ((hashVer >> 16) & 0xFF) ^ ((hashVer >> 8) & 0xFF) ^ (hashVer & 0xFF);
                    
                    // std::cerr << "tryGetNextVersion: 尝试 wzVer=" << wzVer << ", hashVer=" << hashVer << ", enc=" << enc << " (expected " << EncryptedVersion << ")" << std::endl;
                    
                    if (enc == static_cast<uint32_t>(EncryptedVersion)) {
                        versionTest.push_back(wzVer);
                        hasVersionTest.push_back(hashVer);
                        startVersion = wzVer;
                        searchStage = 1; // 下一阶段
                        return true;
                    }
                }
                // 已知版本都不匹配，切换到第二阶段
                searchStage = 1;
            }
            
            // 第二阶段：遍历其他可能的版本（从 100 开始）
            if (searchStage == 1) {
                for (int32_t i = std::max(startVersion + 1, 100); i < INT16_MAX; i++) {
                    if (i == 3 || i == 72) continue; // 已检查
                    
                    uint32_t sum = calcHashVersion(i);
                    uint32_t enc = 0xFF ^ ((sum >> 24) & 0xFF) ^ ((sum >> 16) & 0xFF) ^ ((sum >> 8) & 0xFF) ^ (sum & 0xFF);

                    if (enc == static_cast<uint32_t>(EncryptedVersion)) {
                        versionTest.push_back(i);
                        hasVersionTest.push_back(sum);
                        startVersion = i;
                        return true;
                    }
                }
            }
            
            return false;
        }

    private:
        int32_t EncryptedVersion;
        int32_t startVersion;
        std::vector<int32_t> versionTest;
        std::vector<uint32_t> hasVersionTest;
        int searchStage; // 0 = known versions, 1 = other versions
    };

    class Pkg2WzVersionDetector : public IWzVersionDetector {
    public:
        static constexpr uint32_t magic = 0x1A2B3C4D;

        Pkg2WzVersionDetector(uint32_t hash1, uint32_t hash2)
            : Hash1(hash1), Hash2(hash2), WzVersion(0), HashVersion(0) {}

        uint32_t getHash1() const { return Hash1; }
        uint32_t getHash2() const { return Hash2; }
        int32_t getWzVersion() const override { return WzVersion; }
        uint32_t getHashVersion() const override { return HashVersion; }

        bool tryGetNextVersion() override {
            if (!resultEnumerator) {
                auto versions = getAllVersions();
                resultEnumerator = std::make_unique<VersionEnumerator>(std::move(versions));
            }
            
            bool hasNext = resultEnumerator->moveNext();
            if (hasNext) {
                HashVersion = resultEnumerator->current();
            }
            return hasNext;
        }

    private:
        uint32_t Hash1;
        uint32_t Hash2;
        int32_t WzVersion;
        uint32_t HashVersion;
        
        // Enumerator for version generation
        class VersionEnumerator {
        public:
            using Iterator = std::vector<uint32_t>::const_iterator;
            
            VersionEnumerator(std::vector<uint32_t> versions)
                : versions_(std::move(versions)), current_(versions_.cbegin()) {}
                
            bool moveNext() {
                if (current_ != versions_.cend()) {
                    ++current_;
                    return true;
                }
                return false;
            }
            
            uint32_t current() const {
                if (current_ == versions_.cend()) {
                    return 0; // or throw
                }
                return *current_;
            }
            
        private:
            std::vector<uint32_t> versions_;
            Iterator current_;
        };
        
        std::unique_ptr<VersionEnumerator> resultEnumerator;

        bool getAllVersionsInitialized() const {
            return resultEnumerator != nullptr;
        }

        std::vector<uint32_t> getAllVersions() const {
            std::vector<uint32_t> results;
            auto v1 = calcHashVersionV1();
            results.push_back(v1);
            auto v2List = calcHashVersionV2();
            results.insert(results.end(), v2List.begin(), v2List.end());
            return results;
        }

        bool verifyHashVersionV1(uint32_t hashVersion) const {
            uint32_t lt = rol(Hash1, 7) ^ hashVersion;
            return (lt ^ hashVersion) == Hash2;
        }

        bool verifyHashVersionV2(uint32_t hashVersion) const {
            uint32_t lt = rol(Hash1 ^ (hashVersion + magic), static_cast<int>(hashVersion & 0x1F));
            return (lt ^ hashVersion) == Hash2;
        }

        uint32_t calcHashVersionV1() const {
            return rol(Hash1, 7) ^ Hash2;
        }

        std::vector<uint32_t> calcHashVersionV2() const {
            std::vector<uint32_t> results;
            std::vector<uint32_t> carries(33, 0);
            std::vector<uint32_t> lhsBits(32, 0);
            
            for (int sCandidate = 0; sCandidate < 32; sCandidate++) {
                std::fill(carries.begin(), carries.end(), 0);
                std::fill(lhsBits.begin(), lhsBits.end(), 0);
                backtrack(0, 0, sCandidate, carries, lhsBits, results);
            }
            return results;
        }

        void backtrack(int bitIdx, uint32_t vHash, int s, 
                      std::vector<uint32_t>& carries, std::vector<uint32_t>& lhsBits,
                      std::vector<uint32_t>& results) const {
            if (bitIdx == 32) {
                if ((vHash & 0x1f) == s && verifyHashVersionV2(vHash)) {
                    results.push_back(vHash);
                }
                return;
            }

            uint32_t start, end;
            if (bitIdx < 5) {
                start = end = (s >> bitIdx) & 1;
            } else {
                start = 0;
                end = 1;
            }

            for (uint32_t vBit = start; vBit <= end; vBit++) {
                // backward Check
                int prevLhsIdx = (bitIdx - s + 32) & 0x1f;
                if (prevLhsIdx < bitIdx) {
                    uint32_t v_xor_h2 = vBit ^ ((Hash2 >> bitIdx) & 1);
                    if (v_xor_h2 != lhsBits[prevLhsIdx]) continue;
                }

                uint32_t sum = vBit + ((magic >> bitIdx) & 1) + carries[bitIdx];
                uint32_t currentLhsBit = (sum ^ (Hash1 >> bitIdx)) & 1;

                // forward Check
                int futureVIdx = (bitIdx + s) & 0x1f;
                if (futureVIdx <= bitIdx) {
                    uint32_t knownVBit = (vHash >> futureVIdx) & 1;
                    uint32_t targetV_xor_H2 = knownVBit ^ ((Hash2 >> futureVIdx) & 1);
                    if (currentLhsBit != targetV_xor_H2) continue;
                } else if (futureVIdx < 5) {
                    uint32_t knownVBit = (s >> futureVIdx) & 1;
                    uint32_t targetV_xor_H2 = knownVBit ^ ((Hash2 >> futureVIdx) & 1);
                    if (currentLhsBit != targetV_xor_H2) continue;
                }

                lhsBits[bitIdx] = currentLhsBit;
                if (bitIdx + 1 < static_cast<int>(carries.size())) {
                    carries[bitIdx + 1] = sum >> 1;
                }
                backtrack(bitIdx + 1, vHash | (vBit << bitIdx), s, carries, lhsBits, results);
            }
        }

        static uint32_t rol(uint32_t v, int n) {
            return (v << n) | (v >> (32 - n));
        }
    };

private:
    std::string Signature;
    std::string Copyright;
    std::string FileName;

    int32_t HeaderSize;
    int64_t DataSize;
    int64_t FileSize;
    int64_t DataStartPosition;
    int64_t DirEndPosition = 0;

    bool VersionChecked;
    std::shared_ptr<Wz_Capabilities> Capabilities;
    std::shared_ptr<IWzVersionDetector> versionDetector;
};

} // namespace WzLibCpp
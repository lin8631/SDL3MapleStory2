/**
 * =============================================================================
 * WzResourceLoader.hpp - WZ资源加载器
 * =============================================================================
 * 
 * 【功能说明】
 * 提供统一的接口来加载冒险岛WZ资源文件目录。
 * 自动扫描目录下的所有.wz文件并加载，同时收集加载统计信息。
 * 
 * 【使用方式】
 * auto result = WzResourceLoader::loadFromDirectory("/path/to/wz");
 * if (result.structure) {
 *     PluginBase::PluginManager::RegisterStructures({result.structure});
 * }
 * 
 * =============================================================================
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "Wz_Node.hpp"
#include "Wz_Structure.hpp"
#include "Wz_File.hpp"
#include "Wz_Header.hpp"

namespace WzLibCpp {

class WzResourceLoader {
public:
    struct FileInfo {
        std::string fileName;
        std::string filePath;
        std::string type;
        int imgCount;
        int64_t fileSize;
        bool loaded;
    };

    struct LoadResult {
        std::shared_ptr<Wz_Structure> structure;
        int successCount = 0;
        int failCount = 0;
        int totalImgCount = 0;
        int64_t totalSize = 0;
        int64_t loadTimeMs = 0;
        std::vector<FileInfo> files;
        bool success = false;
        std::string errorMessage;
    };

    static LoadResult loadFromDirectory(const std::string& wzPath, bool verbose = true);
    static LoadResult loadFromDirectoryMinimal(const std::string& wzPath);
    static std::string getFileListSummary(const LoadResult& result);
};

} // namespace WzLibCpp
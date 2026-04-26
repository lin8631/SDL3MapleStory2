/**
 * =============================================================================
 * WzResourceLoader.cpp - WZ资源加载器实现
 * =============================================================================
 * 
 * 【与C#源码对照】
 * 参考 WzComparerR2.WzLib/Wz_Structure.cs 中的 Load() 方法实现
 * 
 * 主要改进：
 * 1. 使用 structure->Load() 而非手动创建 Wz_Node
 * 2. LoadFile 失败会抛出异常，需要在 try-catch 中处理
 * 3. 正确处理扩展名（大小写不敏感）
 * 4. 使用文件名字段获取类型，而非手动解析
 * 
 * =============================================================================
 */

#include "WzResourceLoader.hpp"

#include <chrono>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace WzLibCpp {

/**
 * WZ类型转字符串
 */
static std::string wzTypeToString(Wz_Type type) {
    switch (type) {
        case Wz_Type::Base: return "Base";
        case Wz_Type::Character: return "Character";
        case Wz_Type::Effect: return "Effect";
        case Wz_Type::Etc: return "Etc";
        case Wz_Type::Item: return "Item";
        case Wz_Type::Map: return "Map";
        case Wz_Type::Mob: return "Mob";
        case Wz_Type::Morph: return "Morph";
        case Wz_Type::Npc: return "Npc";
        case Wz_Type::Quest: return "Quest";
        case Wz_Type::Reactor: return "Reactor";
        case Wz_Type::Skill: return "Skill";
        case Wz_Type::Sound: return "Sound";
        case Wz_Type::String: return "String";
        case Wz_Type::TamingMob: return "TamingMob";
        case Wz_Type::UI: return "UI";
        default: return "Unknown";
    }
}

/**
 * 获取 WZ 文件类型
 * 
 * 对应 C# 代码: WzComparerR2.WzLib/Wz_File.cs -> DetectWzType()
 * 通过查找特征节点来判断文件类型
 */
static std::string detectWzType(std::shared_ptr<Wz_File> wzFile) {
    if (!wzFile || !wzFile->getNode()) {
        return "Unknown";
    }
    
    auto node = wzFile->getNode();
    auto nodes = node->getNodes();
    if (!nodes) {
        return "Unknown";
    }
    
    // 使用 operator[] 查找子节点（对应 C#: node.Nodes["xxx"]）
    auto checkNode = [&nodes](const std::string& name) -> bool {
        auto child = (*nodes)[name];
        return child != nullptr;
    };
    
    // 根据特征节点判断类型（与 C# 源码完全一致）
    if (checkNode("smap.img") || checkNode("zmap.img")) {
        return "Base";
    }
    if (checkNode("00002000.img") || checkNode("Accessory") || checkNode("Weapon")) {
        return "Character";
    }
    if (checkNode("BasicEff.img") || checkNode("SetItemInfoEff.img")) {
        return "Effect";
    }
    if (checkNode("Commodity.img") || checkNode("Curse.img")) {
        return "Etc";
    }
    if (checkNode("Cash") || checkNode("Consume")) {
        return "Item";
    }
    if (checkNode("Back") || checkNode("Obj") || checkNode("Physics.img")) {
        return "Map";
    }
    if (checkNode("PQuest.img") || checkNode("QuestData")) {
        return "Quest";
    }
    if (checkNode("Attacktype.img") || checkNode("Recipe_9200.img")) {
        return "Skill";
    }
    if (checkNode("Bgm00.img") || checkNode("BgmUI.img")) {
        return "Sound";
    }
    if (checkNode("MonsterBook.img") || checkNode("EULA.img")) {
        return "String";
    }
    if (checkNode("CashShop.img") || checkNode("UIWindow.img")) {
        return "UI";
    }
    
    // 回退到使用文件名字段（与C#源码一致）
    // 对应 C# 正则: ^([A-Za-z]+)_?(\d+)?(?:\.wz)?$
    std::string wzName = node->getText();
    
    // C++ 实现：先尝试正则匹配提取 $1（字母部分）
    std::string extracted;
    bool matchSuccess = false;
    
    // 正则匹配: ^([A-Za-z]+)_?(\d+)?(?:\.wz)?$
    size_t i = 0;
    // 匹配 [A-Za-z]+
    while (i < wzName.size()) {
        char c = wzName[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            extracted += c;
            i++;
        } else {
            break;
        }
    }
    
    if (!extracted.empty()) {
        // 匹配 _?
        if (i < wzName.size() && wzName[i] == '_') {
            i++;
        }
        // 匹配 (\d+)? - 数字部分（可选，匹配但不使用）
        while (i < wzName.size() && wzName[i] >= '0' && wzName[i] <= '9') {
            i++;
        }
        // 匹配 (?:\.wz)?$ - 可选的 .wz 后缀
        if (i + 3 < wzName.size() || 
            (i + 3 == wzName.size() && 
             wzName[i] == '.' && 
             (wzName[i+1] == 'w' || wzName[i+1] == 'W') && 
             (wzName[i+2] == 'z' || wzName[i+2] == 'Z'))) {
            if (i + 3 <= wzName.size()) {
                std::string ext = wzName.substr(i);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".wz") {
                    i += 3;
                }
            }
        }
        
        // 检查是否完全匹配到字符串末尾
        if (i == wzName.size()) {
            matchSuccess = true;
        }
    }
    
    // 如果正则匹配成功，extracted 已经是纯字母部分（对应 $1）
    // 如果匹配失败，使用原始 wzName（C# Enum.TryParse(wzName, true, ...)）
    std::string parseName = matchSuccess ? extracted : wzName;
    
    // 验证是否为合法的 Wz_Type（对应 C# Enum.TryParse 大小写不敏感）
    // 转换为小写进行匹配
    std::string lowerName = parseName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    // 检查是否为合法的 Wz_Type（使用简单字符串比较避免静态初始化问题）
    if (lowerName == "base" || lowerName == "character" || lowerName == "effect" ||
        lowerName == "etc" || lowerName == "item" || lowerName == "map" ||
        lowerName == "mob" || lowerName == "morph" || lowerName == "npc" ||
        lowerName == "quest" || lowerName == "reactor" || lowerName == "skill" ||
        lowerName == "sound" || lowerName == "string" || lowerName == "tamingmob" ||
        lowerName == "ui") {
        // 首字母大写，与 C# Enum.ToString() 格式一致
        std::string result = parseName;
        if (!result.empty()) {
            result[0] = static_cast<char>(std::toupper(result[0]));
        }
        return result;
    }
    
    return "Unknown";
}

/**
 * 加载目录中的所有 WZ 文件
 * 
 * 对应 C# 代码: WzComparerR2.WzLib/Wz_Structure.cs -> Load()
 * 
 * 流程：
 * 1. 检查目录和 Base.wz 是否存在
 * 2. 创建 Wz_Structure 并设置配置
 * 3. 调用 structure->Load() 加载 Base.wz（自动递归加载所有子WZ文件）
 * 4. 遍历 wz_files 收集信息
 */
WzResourceLoader::LoadResult WzResourceLoader::loadFromDirectory(const std::string& wzPath, bool verbose) {
    LoadResult result;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::filesystem::path dirPath(wzPath);
    
    // 步骤1: 检查目录是否存在
    if (!std::filesystem::exists(dirPath)) {
        result.errorMessage = "目录不存在: " + wzPath;
        if (verbose) {
            std::cerr << "错误: " << result.errorMessage << std::endl;
        }
        return result;
    }
    
    // 步骤2: 检查 Base.wz 是否存在
    std::filesystem::path baseWzPath = dirPath / "Base.wz";
    if (!std::filesystem::exists(baseWzPath)) {
        result.errorMessage = "Base.wz 不存在于: " + baseWzPath.string();
        if (verbose) {
            std::cerr << "错误: " << result.errorMessage << std::endl;
        }
        return result;
    }
    
    if (verbose) {
        std::cout << "[MapViewer_EnTT(WzResourceLoader)]: 正在加载 WZ 文件: " << wzPath << std::endl;
    }
    
    // 步骤3: 创建 Wz_Structure 并配置
    // 对应 C# 代码:
    //   this.AutoDetectExtFiles = true;
    //   this.Load(fileName, true);
    result.structure = std::make_shared<Wz_Structure>();
    result.structure->setAutoDetectExtFiles(true);
    
    try {
        // 调用 Load 方法（对应 C#: structure.Load(baseWzPath, true)）
        // 这会自动：
        // 1. 创建 WzNode
        // 2. 调用 LoadFile 加载 Base.wz
        // 3. 递归加载子 WZ 文件（Map, Character, Item 等）
        result.structure->load(baseWzPath.string(), true);
        
        if (verbose) {
            std::cout << "[MapViewer_EnTT(WzResourceLoader)]: 正在收集 WZ 文件信息..." << std::endl;
        }
        
        // 步骤4: 遍历所有加载的 WZ 文件收集信息
        // 对应 C# 代码:
        //   foreach (var wzFile in this.wz_files) { ... }
        const auto& wzFiles = result.structure->getWzFiles();
        for (size_t i = 0; i < wzFiles.size(); i++) {
            const auto& wzFile = wzFiles[i];
            FileInfo info;
            
            if (wzFile) {
                info.loaded = wzFile->isLoaded();
                info.imgCount = wzFile->getImageCount();
                result.successCount++;
                result.totalImgCount += info.imgCount;
                
                // 获取文件信息
                auto header = wzFile->getHeader();
                auto wzNode = wzFile->getNode();
                Wz_Type autoType = wzFile->getType();
                std::string nodeText = wzNode ? wzNode->getText() : "null";
                std::string headerFileName = header ? header->getFileName() : "null";
                
                // 使用文件名字段（对应 C#: wzFile.Header.FileName）
                if (header && !headerFileName.empty()) {
                    info.filePath = headerFileName;
                    info.fileName = std::filesystem::path(headerFileName).filename().string();
                }
                
                // 如果 header 没有文件名，从 node 获取
                if (info.fileName.empty() && wzNode) {
                    info.fileName = nodeText;
                }
                
                // 确保至少有一个文件名
                if (info.fileName.empty()) {
                    info.fileName = "unknown_" + std::to_string(i);
                }
                
                // 使用 C++ 库自动检测的类型
                if (autoType != Wz_Type::Unknown) {
                    info.type = wzTypeToString(autoType);
                } else {
                    info.type = detectWzType(wzFile);
                }
                
                try {
                    if (!info.filePath.empty()) {
                        info.fileSize = std::filesystem::file_size(info.filePath);
                        result.totalSize += info.fileSize;
                    }
                } catch (const std::exception& e) {
                    // 记录警告但不影响流程
                    if (verbose) {
                        std::cerr << "警告: 无法获取文件大小 " << info.filePath << ": " << e.what() << std::endl;
                    }
                    info.fileSize = 0;
                }
                
                result.files.emplace_back(std::move(info));
            } else {
                info.loaded = false;
                info.fileName = "unknown_" + std::to_string(i);
                info.type = "Unknown";
                result.failCount++;
                result.files.emplace_back(std::move(info));
            }
            
            // 输出信息
            if (verbose) {
                const auto& lastInfo = result.files.back();
                std::cout << "  " << result.files.size() << ". " << lastInfo.fileName
                          << " (类型: " << lastInfo.type << ", IMG数: " << lastInfo.imgCount << ")";
                if (lastInfo.fileSize > 0) {
                    std::cout << " (" << (lastInfo.fileSize / 1024 / 1024) << " MB)";
                }
                std::cout << std::endl;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.loadTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        
        result.success = true;
        
        if (verbose) {
            std::cout << "--------------------------------------" << std::endl;
            std::cout << "加载完成! 耗时: " << result.loadTimeMs << " ms" << std::endl;
            std::cout << "成功: " << result.successCount << " 个, ";
            std::cout << "失败: " << result.failCount << " 个, ";
            std::cout << "总图像: " << result.totalImgCount << std::endl;
            std::cout << "总大小: " << (result.totalSize / 1024 / 1024) << " MB" << std::endl;
            std::cout << "======================================" << std::endl;
        }
        
    } catch (const std::exception& e) {
        result.errorMessage = std::string("加载异常: ") + e.what();
        // 清理已加载的资源（对应 C#: Wz_Structure.Clear()）
        if (result.structure) {
            result.structure->clear();
        }
        // 重置状态（与 C# Wz_Structure.Clear() 完全一致）
        result.successCount = 0;
        result.failCount = 0;
        result.totalImgCount = 0;
        result.totalSize = 0;
        result.files.clear();
        
        if (verbose) {
            std::cerr << result.errorMessage << std::endl;
        }
    }
    
    return result;
}

std::string WzResourceLoader::getFileListSummary(const LoadResult& result) {
    std::ostringstream oss;
    for (size_t i = 0; i < result.files.size(); i++) {
        const auto& info = result.files[i];
        oss << (i + 1) << ". " << info.fileName 
            << " (IMG: " << info.imgCount << ")\n";
    }
    return oss.str();
}

} // namespace WzLibCpp

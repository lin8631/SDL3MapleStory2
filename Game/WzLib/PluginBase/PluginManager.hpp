#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include "FindWzEventArgs.hpp"
#include "Wz_Structure.hpp"
#include "Wz_File.hpp"
#include "Wz_Node.hpp"
#include "Wz_Image.hpp"

namespace WzLibCpp::PluginBase {

/**
 * PluginManager - 插件管理器
 * 提供查找 Wz 节点的通用 API
 * 对应 C# 代码中的 PluginManager 类
 */
class PluginManager {
public:
    /**
     * 当执行 FindWz 函数时发生，用来寻找对应的 Wz_File
     */
    static FindWzEventHandler WzFileFinding;

    /**
     * 通过 wz 完整路径查找对应的 Wz_Node，若没有找到则返回 nullptr
     * @param fullPath 要查找节点的完整路径，可用 '/' 或者 '\' 作分隔符
     *                 如 "Mob/8144006.img/die1/6"
     * @return 找到的节点，未找到返回 nullptr
     */
    static std::shared_ptr<Wz_Node> FindWz(const std::string& fullPath) {
        return FindWz(fullPath, nullptr);
    }

    /**
     * 通过 wz 完整路径查找对应的 Wz_Node
     * @param fullPath 要查找节点的完整路径
     * @param sourceWzFile 源 wz 文件，可选
     * @return 找到的节点，未找到返回 nullptr
     */
    static std::shared_ptr<Wz_Node> FindWz(const std::string& fullPath, 
                                            std::shared_ptr<Wz_File> sourceWzFile) {
        FindWzEventArgs e;
        e.setFullPath(fullPath);
        e.setWzFile(sourceWzFile);
        
        if (WzFileFinding) {
            WzFileFinding(nullptr, e);
            if (e.getWzNode()) {
                return e.getWzNode();
            }
            if (e.getWzFile()) {
                return e.getWzFile()->getNode();
            }
        }
        return nullptr;
    }

    /**
     * 通过 Wz_Type 查找对应的 Wz_File
     * @param type Wz 类型
     * @param structures 已打开的 Wz_Structure 列表
     * @return 找到的 Wz_File，未找到返回 nullptr
     */
    static std::shared_ptr<Wz_File> FindWzFileByType(Wz_Type type,
                                                      const std::vector<std::shared_ptr<Wz_Structure>>& structures) {
        for (const auto& wzs : structures) {
            if (!wzs) continue;
            for (const auto& wzFile : wzs->getWzFiles()) {
                if (wzFile && wzFile->getType() == type) {
                    return wzFile;
                }
            }
        }
        return nullptr;
    }

    /**
     * 注册已打开的 Wz_Structure，用于自动处理 WzFileFinding 事件
     * @param structures 已打开的 Wz_Structure 列表
     */
    static void RegisterStructures(const std::vector<std::shared_ptr<Wz_Structure>>& structures) {
        // 复制一份 structures，避免引用失效
        auto structuresCopy = std::make_shared<std::vector<std::shared_ptr<Wz_Structure>>>(structures);
        WzFileFinding = [structuresCopy](std::shared_ptr<void> sender, FindWzEventArgs& e) {
            std::string fullPath = e.getFullPath();
            
            // 解析路径
            std::vector<std::string> pathParts;
            std::string part;
            for (char c : fullPath) {
                if (c == '\\' || c == '/') {
                    if (!part.empty()) {
                        pathParts.push_back(part);
                        part.clear();
                    }
                } else {
                    part += c;
                }
            }
            if (!part.empty()) {
                pathParts.push_back(part);
            }
            
            if (!pathParts.empty()) {
                // 解析 Wz_Type
                Wz_Type wzType = Wz_Type::Unknown;
                if (pathParts[0] == "Map" || pathParts[0] == "map") {
                    wzType = Wz_Type::Map;
                } else if (pathParts[0] == "Character" || pathParts[0] == "character") {
                    wzType = Wz_Type::Character;
                } else if (pathParts[0] == "String" || pathParts[0] == "string") {
                    wzType = Wz_Type::String;
                } else if (pathParts[0] == "Item" || pathParts[0] == "item") {
                    wzType = Wz_Type::Item;
                } else if (pathParts[0] == "UI" || pathParts[0] == "ui") {
                    wzType = Wz_Type::UI;
                } else if (pathParts[0] == "Mob" || pathParts[0] == "mob") {
                    wzType = Wz_Type::Mob;
                } else if (pathParts[0] == "Npc" || pathParts[0] == "npc") {
                    wzType = Wz_Type::Npc;
                } else if (pathParts[0] == "Skill" || pathParts[0] == "skill") {
                    wzType = Wz_Type::Skill;
                } else if (pathParts[0] == "Sound" || pathParts[0] == "sound") {
                    wzType = Wz_Type::Sound;
                } else if (pathParts[0] == "Reactor" || pathParts[0] == "reactor") {
                    wzType = Wz_Type::Reactor;
                } else if (pathParts[0] == "Effect" || pathParts[0] == "effect") {
                    wzType = Wz_Type::Effect;
                } else if (pathParts[0] == "Etc" || pathParts[0] == "etc") {
                    wzType = Wz_Type::Etc;
                } else if (pathParts[0] == "Quest" || pathParts[0] == "quest") {
                    wzType = Wz_Type::Quest;
                } else if (pathParts[0] == "Morph" || pathParts[0] == "morph") {
                    wzType = Wz_Type::Morph;
                } else if (pathParts[0] == "TamingMob" || pathParts[0] == "tamingmob") {
                    wzType = Wz_Type::TamingMob;
                } else if (pathParts[0] == "Base" || pathParts[0] == "base") {
                    wzType = Wz_Type::Base;
                }
                e.setWzType(wzType);
                
                // 收集 preSearch 列表
                std::vector<std::shared_ptr<Wz_Node>> preSearch;
                
                for (const auto& wzs : *structuresCopy) {
                    if (!wzs) continue;
                    
                    for (const auto& wzFile : wzs->getWzFiles()) {
                        if (!wzFile) continue;
                        
                        if (wzFile->getType() == wzType) {
                            auto node = wzFile->getNode();
                            if (node) {
                                preSearch.push_back(node);
                            }
                        }
                    }
                }
                
                // 从 preSearch 中的节点开始查找
                for (const auto& searchNode : preSearch) {
                    auto current = searchNode;
                    bool found = true;
                    
                    // 从第二段开始查找（跳过第一段 Wz_Type）
                    for (size_t i = 1; i < pathParts.size() && current; i++) {
                        auto nodes = current->getNodes();
                        if (!nodes) {
                            found = false;
                            break;
                        }
                        
                        auto nextNode = nodes->operator[](pathParts[i]);
                        if (nextNode) {
                            current = nextNode;
                        } else {
                            found = false;
                            break;
                        }
                    }
                    
                    if (found && current) {
                        e.setWzNode(current);
                        return;
                    }
                }
            }
        };
    }

private:
    /**
     * 默认的 WzFileFinding 事件处理函数
     * 对应 C# 代码中的 CharaSimLoader_WzFileFinding 方法
     */
    static void DefaultWzFileFinding(const std::vector<std::shared_ptr<Wz_Structure>>& structures,
                                      std::shared_ptr<void> sender,
                                      FindWzEventArgs& e) {
        std::vector<std::string> fullPath;
        
        // 解析路径
        if (!e.getFullPath().empty()) {
            fullPath = SplitPath(e.getFullPath());
            if (!fullPath.empty()) {
                // 第一段是 Wz_Type
                e.setWzType(ParseWzType(fullPath[0]));
            }
        }

        // 收集预搜索的节点列表
        std::vector<std::shared_ptr<Wz_Node>> preSearch;
        
        if (e.getWzType() != Wz_Type::Unknown) {
            // 确定要搜索的 Wz_Structure 列表
            auto preSearchWz = structures;
            if (e.getWzFile() && e.getWzFile()->getWzStructure()) {
                preSearchWz = {e.getWzFile()->getWzStructure()};
            }

            for (const auto& wzs : preSearchWz) {
                if (!wzs) continue;
                
                std::shared_ptr<Wz_File> baseWz = nullptr;
                bool find = false;
                
                for (const auto& wzFile : wzs->getWzFiles()) {
                    if (!wzFile) continue;
                    
                    if (wzFile->getType() == e.getWzType()) {
                        auto node = wzFile->getNode();
                        if (node) {
                            preSearch.push_back(node);
                            find = true;
                        }
                    }
                    if (wzFile->getType() == Wz_Type::Base) {
                        baseWz = wzFile;
                    }
                }

                // 兼容 data.wz 格式
                if (baseWz && !find) {
                    std::string key = WzTypeToString(e.getWzType());
                    auto baseNode = baseWz->getNode();
                    if (baseNode && baseNode->getNodes()) {
                        for (const auto& node : *baseNode->getNodes()) {
                            if (node && node->getText() == key && 
                                node->getNodes() && node->getNodes()->getCount() > 0) {
                                preSearch.push_back(node);
                            }
                        }
                    }
                }
            }
        }

        // 如果路径为空或只有 1 段，返回 Wz_File 节点
        if (fullPath.empty() || fullPath.size() <= 1) {
            if (e.getWzType() != Wz_Type::Unknown && !preSearch.empty()) {
                e.setWzNode(preSearch[0]);
                e.setWzFile(std::dynamic_pointer_cast<Wz_File>(
                    preSearch[0]->getValue<Wz_File>()));
            }
            return;
        }

        if (preSearch.empty()) {
            return;
        }

        // 逐级遍历子节点
        for (const auto& wzFileNode : preSearch) {
            auto searchNode = wzFileNode;
            
            // 跳过第一段（Wz_Type），从第二段开始查找
            for (size_t i = 1; i < fullPath.size() && searchNode; i++) {
                // 查找子节点
                if (!searchNode) {
                    break;
                }
                
                auto nodes = searchNode->getNodes();
                if (!nodes) {
                    searchNode = nullptr;
                    break;
                }
                
                // 使用 operator[] 查找节点，而不是 find 方法
                auto foundNode = nodes->operator[](fullPath[i]);
                if (foundNode) {
                    searchNode = foundNode;
                } else {
                    searchNode = nullptr;
                    break;
                }

                // 检查是否是 Wz_Image，如果是则解压
                if (searchNode) {
                    auto img = searchNode->getValue<Wz_Image>();
                    if (img) {
                        // 计算偏移
                        if (img->getOffset() == 0) {
                            auto wzFileBase = img->getWzFile();
                            if (wzFileBase) {
                                int64_t calculatedOffset = wzFileBase->calcOffset(
                                    img->getHashedOffsetPosition(), 
                                    img->getHashedOffset());
                                img->setOffset(calculatedOffset);
                            }
                        }
                        
                        if (img->tryExtract()) {
                            searchNode = img->getNode();
                        } else {
                            searchNode = nullptr;
                        }
                    }
                }
            }

            if (searchNode) {
                e.setWzNode(searchNode);
                // 直接获取 Wz_File，不需要类型转换
                auto wzFile = wzFileNode->getValue<Wz_File>();
                e.setWzFile(wzFile);
                return;
            }
        }
        
        // 未找到
        e.setWzNode(nullptr);
    }

    /**
     * 分割路径，支持 '/' 和 '\' 两种分隔符
     */
    static std::vector<std::string> SplitPath(const std::string& path) {
        std::vector<std::string> components;
        std::string item;
        
        for (char c : path) {
            if (c == '/' || c == '\\') {
                if (!item.empty()) {
                    components.push_back(item);
                    item.clear();
                }
            } else {
                item += c;
            }
        }
        if (!item.empty()) {
            components.push_back(item);
        }
        
        return components;
    }

    /**
     * 解析 Wz_Type
     */
    static Wz_Type ParseWzType(const std::string& name) {
        if (name == "Base" || name == "base") return Wz_Type::Base;
        if (name == "Character" || name == "character") return Wz_Type::Character;
        if (name == "String" || name == "string") return Wz_Type::String;
        if (name == "Item" || name == "item") return Wz_Type::Item;
        if (name == "UI" || name == "ui") return Wz_Type::UI;
        if (name == "Morph" || name == "morph") return Wz_Type::Morph;
        if (name == "Mob" || name == "mob") return Wz_Type::Mob;
        if (name == "Reactor" || name == "reactor") return Wz_Type::Reactor;
        if (name == "Sound" || name == "sound") return Wz_Type::Sound;
        if (name == "Npc" || name == "npc") return Wz_Type::Npc;
        if (name == "Map" || name == "map") return Wz_Type::Map;
        if (name == "Skill" || name == "skill") return Wz_Type::Skill;
        if (name == "Quest" || name == "quest") return Wz_Type::Quest;
        if (name == "Effect" || name == "effect") return Wz_Type::Effect;
        if (name == "TamingMob" || name == "tamingmob") return Wz_Type::TamingMob;
        if (name == "Etc" || name == "etc") return Wz_Type::Etc;
        return Wz_Type::Unknown;
    }

    /**
     * Wz_Type 转字符串
     */
    static std::string WzTypeToString(Wz_Type type) {
        switch (type) {
            case Wz_Type::Base: return "Base";
            case Wz_Type::Character: return "Character";
            case Wz_Type::String: return "String";
            case Wz_Type::Item: return "Item";
            case Wz_Type::UI: return "UI";
            case Wz_Type::Morph: return "Morph";
            case Wz_Type::Mob: return "Mob";
            case Wz_Type::Reactor: return "Reactor";
            case Wz_Type::Sound: return "Sound";
            case Wz_Type::Npc: return "Npc";
            case Wz_Type::Map: return "Map";
            case Wz_Type::Skill: return "Skill";
            case Wz_Type::Quest: return "Quest";
            case Wz_Type::Effect: return "Effect";
            case Wz_Type::TamingMob: return "TamingMob";
            case Wz_Type::Etc: return "Etc";
            default: return "Unknown";
        }
    }
};

// 静态成员初始化
inline FindWzEventHandler PluginManager::WzFileFinding = nullptr;

} // namespace WzLibCpp::PluginBase

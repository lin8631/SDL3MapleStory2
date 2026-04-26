#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdio>
#include "PluginBase/PluginManager.hpp"
#include "Wz_Node.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"

namespace WzLibCpp {

/**
 * MapData - 地图数据类
 * 对应 C# 代码中的 MapData 类
 */
class MapData {
public:
    /**
     * 根据地图 ID 查找地图节点
     * 路径格式：Map\Map\Map{X}\{mapID:D9}.img
     * 其中 X = mapID / 100000000（地图 ID 的首位数字）
     * 
     * @param mapID 地图 ID，如 100000000（射手村）
     * @param mapImgNode 输出参数，找到的节点
     * @return 是否找到
     */
    static bool FindMapByID(int mapID, std::shared_ptr<Wz_Node>& mapImgNode) {
        // 计算子文件夹编号
        int folderNum = mapID / 100000000;
        
        // 构造路径：Map\Map\Map{X}\{mapID:D9}.img
        // 第一个 Map 是 Wz_Type，用于定位 Map.wz
        // 第二个 Map 是 Map.wz 根节点下的 "Map" 子节点
        // Map{X} 是 Map 子节点下的目录节点
        char fullPath[128];
        snprintf(fullPath, sizeof(fullPath), 
                 "Map\\Map\\Map%d\\%09d.img", folderNum, mapID);
        
        // 使用 PluginManager 查找节点
        mapImgNode = PluginBase::PluginManager::FindWz(fullPath);
        
        // 获取 Wz_Image 并解压
        if (mapImgNode) {
            auto img = mapImgNode->getValue<Wz_Image>();
            if (img) {
                // 确保偏移已计算
                if (img->getOffset() == 0) {
                    auto wzFile = std::dynamic_pointer_cast<Wz_File>(img->getWzFile());
                    if (wzFile) {
                        int64_t calculatedOffset = wzFile->calcOffset(
                            img->getHashedOffsetPosition(), 
                            img->getHashedOffset());
                        img->setOffset(calculatedOffset);
                    }
                }
                
                // 尝试解压
                if (img->tryExtract()) {
                    mapImgNode = img->getNode();
                    return true;
                }
            }
        }
        
        mapImgNode = nullptr;
        return false;
    }

    /**
     * 根据地图 ID 查找地图节点（带输出 Wz_Image）
     * 
     * @param mapID 地图 ID
     * @param mapImgNode 输出参数，找到的节点
     * @param outImg 输出参数，找到的 Wz_Image
     * @return 是否找到
     */
    static bool FindMapByID(int mapID, std::shared_ptr<Wz_Node>& mapImgNode, 
                            std::shared_ptr<Wz_Image>& outImg) {
        outImg = nullptr;
        
        // 计算子文件夹编号
        int folderNum = mapID / 100000000;
        
        // 构造路径：Map\Map\Map{X}\{mapID:D9}.img
        char fullPath[128];
        snprintf(fullPath, sizeof(fullPath), 
                 "Map\\Map\\Map%d\\%09d.img", folderNum, mapID);
        
        // 使用 PluginManager 查找节点
        auto foundNode = PluginBase::PluginManager::FindWz(fullPath);
        
        // 获取 Wz_Image 并解压
        if (foundNode) {
            mapImgNode = foundNode;
            outImg = foundNode->getValue<Wz_Image>();
            if (outImg) {
                // 确保偏移已计算
                if (outImg->getOffset() == 0) {
                    auto wzFile = std::dynamic_pointer_cast<Wz_File>(outImg->getWzFile());
                    if (wzFile) {
                        int64_t calculatedOffset = wzFile->calcOffset(
                            outImg->getHashedOffsetPosition(), 
                            outImg->getHashedOffset());
                        outImg->setOffset(calculatedOffset);
                    }
                }
                
                // 尝试解压
                outImg->tryExtract();
                return true;
            }
        }
        
        mapImgNode = nullptr;
        return false;
    }

    /**
     * 加载地图数据
     * 
     * @param mapID 地图 ID
     * @param structures 已打开的 Wz_Structure 列表
     * @return 找到的地图节点，未找到返回 nullptr
     */
    static std::shared_ptr<Wz_Node> Load(int mapID, 
                                          const std::vector<std::shared_ptr<Wz_Structure>>& structures) {
        // 注册结构到 PluginManager
        PluginBase::PluginManager::RegisterStructures(structures);
        
        // 查找地图节点
        std::shared_ptr<Wz_Node> mapImgNode;
        if (FindMapByID(mapID, mapImgNode)) {
            return mapImgNode;
        }
        
        return nullptr;
    }

    /**
     * 获取地图的 Link 目标
     * 
     * @param mapNode 地图节点
     * @return Link 目标的地图 ID，如果没有 Link 返回 -1
     */
    static int GetMapLink(std::shared_ptr<Wz_Node> mapNode) {
        if (!mapNode || !mapNode->getNodes()) {
            return -1;
        }
        
        // 查找 info 节点
        auto infoNode = mapNode->getNodes()->operator[]("info");
        if (!infoNode || !infoNode->getNodes()) {
            return -1;
        }
        
        // 查找 link 节点
        auto linkNode = infoNode->getNodes()->operator[]("link");
        if (!linkNode) {
            return -1;
        }
        
        // 获取 link 值（需要实现属性读取）
        // 这里暂时返回 -1，需要完善属性系统
        return -1;
    }
};

} // namespace WzLibCpp

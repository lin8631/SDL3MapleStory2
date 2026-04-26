/**
 * =============================================================================
 * MapLoader.cpp - 地图加载器实现
 * =============================================================================
 * 
 * 参考 C# 源码: WzComparerR2.MapRender/MapData.cs -> FindMapByID()
 * 
 * C# 原实现:
 *   public static bool FindMapByID(int mapID, out Wz_Node mapImgNode)
 *   {
 *       string fullPath = string.Format(@"Map\Map\Map{0}\{1:D9}.img", (mapID / 100000000), mapID);
 *       mapImgNode = PluginManager.FindWz(fullPath);
 *       Wz_Image mapImg;
 *       if (mapImgNode != null
 *           && (mapImg = mapImgNode.GetValueEx<Wz_Image>(null)) != null
 *           && mapImg.TryExtract())
 *       {
 *           mapImgNode = mapImg.Node;
 *           return true;
 *       }
 *       ...
 *   }
 * 
 * 主要改进:
 * 1. 不手动计算偏移 - Offset 已在 Wz_File 加载阶段由框架计算完毕
 * 2. 直接调用 TryExtract() - TryExtract 内部会使用正确的偏移
 * 3. 保持详细日志输出以便调试
 * 
 * =============================================================================
 */

#include "MapLoader.hpp"

#include <iostream>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace WzLibCpp {

MapLoader::MapLoader(std::shared_ptr<Wz_Structure> structure)
    : m_structure(structure)
    , m_mapNode(nullptr)
    , m_lastMapID(-1)
{
}

/**
 * FindMapByID - 根据地图ID查找并提取地图节点
 * 
 * 对应 C# 源码: WzComparerR2.MapRender/MapData.cs -> FindMapByID()
 * 
 * C# 原实现只做三件事：
 * 1. 构造路径: string.Format(@"Map\Map\Map{0}\{1:D9}.img", mapID / 100000000, mapID)
 * 2. FindWz(fullPath) - 查找节点
 * 3. TryExtract() - 提取数据（框架已计算好 Offset）
 * 
 * 注意：Offset 由 Wz_File.detectWzVersion() 阶段的 DefaultVersionVerifier.CalcOffset() 批量计算
 *       无需在业务层手动计算
 */
bool MapLoader::FindMapByID(int mapID, std::shared_ptr<Wz_Node>& outMapImgNode) {
    // 构造路径，与 C# 一致: string.Format(@"Map\Map\Map{0}\{1:D9}.img", mapID / 100000000, mapID)
    std::ostringstream path;
    path << "Map\\Map\\Map" << (mapID / 100000000) << "\\" 
         << std::setw(9) << std::setfill('0') << mapID << ".img";
    
    // 通过 PluginManager 查找节点
    std::shared_ptr<Wz_Node> mapImgNode = PluginBase::PluginManager::FindWz(path.str());
    if (!mapImgNode) {
        return false;
    }
    
    // 获取 Wz_Image 并提取
    std::shared_ptr<Wz_Image> mapImg = mapImgNode->getValue<Wz_Image>();
    if (!mapImg) {
        return false;
    }
    
    // 调用 TryExtract - Offset 已由框架计算完毕，无需手动处理
    if (!mapImg->tryExtract()) {
        return false;
    }
    
    // 返回提取后的节点
    outMapImgNode = mapImg->getNode();
    return outMapImgNode != nullptr;
}

std::shared_ptr<Wz_Node> MapLoader::loadMap(int mapID) {
    m_lastError.clear();
    m_mapNode = nullptr;
    m_lastMapID = mapID;

   
    std::cout << "[MapLoader(loadMap)]: 查找地图: " << mapID << std::endl;
   

    std::string path = calculateMapPath(mapID);
    std::cout << "步骤 1: 计算路径" << std::endl;
    std::cout << "  - 路径 = " << path << std::endl;

    std::cout << "步骤 2: 查找节点" << std::endl;
    auto foundNode = findMapNode(path);
    if (!foundNode) {
        m_lastError = "未找到地图节点: " + path;
        std::cerr << m_lastError << std::endl;
        return nullptr;
    }
    std::cout << "  ✓ 找到节点: " << foundNode->getText() << std::endl;

    std::cout << "步骤 3: 获取 Wz_Image" << std::endl;
    auto img = foundNode->getValue<Wz_Image>();
    if (!img) {
        m_lastError = "节点不是 Wz_Image 类型";
        std::cerr << m_lastError << std::endl;
        return nullptr;
    }
    std::cout << "  ✓ Wz_Image 名称: " << img->getName() << std::endl;
    std::cout << "  ✓ Wz_Image 大小: " << img->getSize() << " bytes" << std::endl;
    std::cout << "  ✓ 偏移(框架已计算): " << img->getOffset() << std::endl;

    std::cout << "步骤 4: 提取地图数据" << std::endl;
    m_mapNode = extractMapData(img);
    if (!m_mapNode) {
        m_lastError = "提取地图数据失败";
        return nullptr;
    }
    std::cout << "  ✓ 节点获取成功" << std::endl;
    std::cout << "  ✓ 子节点数量: " << m_mapNode->getNodes()->getCount() << std::endl;

    std::cout << "地图加载完成!" << std::endl;
    std::cout << "========================================" << std::endl;

    return m_mapNode;
}

std::string MapLoader::calculateMapPath(int mapID) const {
    int folderNum = mapID / 100000000;
    char path[128];
    snprintf(path, sizeof(path), "Map\\Map\\Map%d\\%09d.img", folderNum, mapID);
    return path;
}

std::shared_ptr<Wz_Node> MapLoader::findMapNode(const std::string& path) const {
    return PluginBase::PluginManager::FindWz(path);
}

std::shared_ptr<Wz_Node> MapLoader::extractMapData(std::shared_ptr<Wz_Image> img) const {
    std::cout << "  - 调用 tryExtract..." << std::endl;
    if (!img->tryExtract()) {
        std::cerr << "tryExtract 失败" << std::endl;
        return nullptr;
    }
    std::cout << "  ✓ tryExtract 成功" << std::endl;
    return img->getNode();
}

} // namespace WzLibCpp

#pragma once

#include <string>
#include <memory>
#include <stdexcept>

#include "Wz_Node.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"
#include "Wz_Structure.hpp"
#include "PluginBase/PluginManager.hpp"

namespace WzLibCpp {

class MapLoader {
public:
    explicit MapLoader(std::shared_ptr<Wz_Structure> structure);

    /**
     * FindMapByID - 根据地图ID查找并提取地图节点
     * 
     * 对应 C# 源码: WzComparerR2.MapRender/MapData.cs -> FindMapByID()
     * 
     * @param mapID 地图ID
     * @param outMapImgNode 输出参数，返回提取后的地图节点
     * @return 是否成功
     */
    static bool FindMapByID(int mapID, std::shared_ptr<Wz_Node>& outMapImgNode);

    std::shared_ptr<Wz_Node> loadMap(int mapID);

    const std::string& getLastError() const { return m_lastError; }
    int getLastMapID() const { return m_lastMapID; }
    bool isLoaded() const { return m_mapNode != nullptr; }
    std::shared_ptr<Wz_Node> getMapNode() const { return m_mapNode; }

private:
    std::string calculateMapPath(int mapID) const;
    std::shared_ptr<Wz_Node> findMapNode(const std::string& path) const;
    std::shared_ptr<Wz_Node> extractMapData(std::shared_ptr<Wz_Image> img) const;

    std::shared_ptr<Wz_Structure> m_structure;
    std::shared_ptr<Wz_Node> m_mapNode;
    std::string m_lastError;
    int m_lastMapID = -1;
};

} // namespace WzLibCpp

#pragma once
#include <string>
#include <memory>
#include <vector>
#include <regex>
#include "PluginBase/PluginManager.hpp"
#include "Wz_Node.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"

namespace WzLibCpp {

/**
 * 地图数据类 - 完整实现
 * 对应 C# 代码中的 MapData 类
 */
class MapData {
public:
    MapData() = default;
    virtual ~MapData() = default;

    /**
     * 加载地图数据
     * @param mapImgNode 地图图片节点（已解压）
     * @return 是否加载成功
     */
    bool Load(std::shared_ptr<Wz_Node> mapImgNode) {
        if (!mapImgNode) {
            return false;
        }

        // 加载基本信息
        auto infoNode = mapImgNode->getNodes()->operator[]("info");
        if (!infoNode) {
            return false;
        }

        // 读取 ID 和名称
        LoadIDOrName(mapImgNode);

        // 加载基本信息
        LoadInfo(infoNode);

        // 加载小地图
        auto miniMapNode = mapImgNode->getNodes()->operator[]("miniMap");
        if (miniMapNode) {
            LoadMinimap(miniMapNode);
        }

        // 加载背景
        auto backNode = mapImgNode->getNodes()->operator[]("back");
        if (backNode) {
            LoadBack(backNode);
        }

        // 加载图层（0~7）
        for (int i = 0; i <= 7; i++) {
            auto layerNode = mapImgNode->getNodes()->operator[](std::to_string(i));
            if (layerNode) {
                LoadLayer(layerNode, i);
            }
        }

        // 加载脚踏板
        auto footholdNode = mapImgNode->getNodes()->operator[]("foothold");
        if (footholdNode) {
            for (int i = 0; i <= 7; i++) {
                auto fhLevel = footholdNode->getNodes()->operator[](std::to_string(i));
                if (fhLevel) {
                    LoadFoothold(fhLevel, i);
                }
            }
        }

        // 加载生命（mob/npc）
        auto lifeNode = mapImgNode->getNodes()->operator[]("life");
        if (lifeNode) {
            LoadLife(lifeNode);
        }

        // 加载反应器
        auto reactorNode = mapImgNode->getNodes()->operator[]("reactor");
        if (reactorNode) {
            LoadReactor(reactorNode);
        }

        // 加载传送门
        auto portalNode = mapImgNode->getNodes()->operator[]("portal");
        if (portalNode) {
            LoadPortal(portalNode);
        }

        // 加载绳梯
        auto ladderRopeNode = mapImgNode->getNodes()->operator[]("ladderRope");
        if (ladderRopeNode) {
            LoadLadderRope(ladderRopeNode);
        }

        // 加载工具提示
        auto tooltipNode = mapImgNode->getNodes()->operator[]("ToolTip");
        if (tooltipNode) {
            LoadTooltip(tooltipNode);
        }

        // 加载粒子
        auto particleNode = mapImgNode->getNodes()->operator[]("particle");
        if (particleNode) {
            LoadParticle(particleNode);
        }

        // 加载光照
        auto lightNode = mapImgNode->getNodes()->operator[]("light");
        if (lightNode) {
            LoadLight(lightNode);
        }

        // 计算地图大小
        CalcMapSize();

        return true;
    }

    // Getters
    int getID() const { return ID; }
    const std::string& getName() const { return Name; }
    const std::string& getBgm() const { return Bgm; }
    int getLink() const { return Link; }
    const std::string& getMapMark() const { return MapMark; }
    bool getIsTown() const { return IsTown; }
    bool getCanFly() const { return CanFly; }
    bool getCanSwim() const { return CanSwim; }
    int getReturnMap() const { return ReturnMap; }
    bool getHideMinimap() const { return HideMinimap; }
    int getFieldLimit() const { return FieldLimit; }
    
    // 地图区域
    int getVRLeft() const { return VRLeft; }
    int getVRRight() const { return VRRight; }
    int getVRTop() const { return VRTop; }
    int getVRBottom() const { return VRBottom; }

private:
    /**
     * 从节点名中提取 ID
     */
    void LoadIDOrName(std::shared_ptr<Wz_Node> mapImgNode) {
        std::string text = mapImgNode->getText();
        std::regex pattern(R"((\d{9})\.img)");
        std::smatch match;
        if (std::regex_search(text, match, pattern)) {
            ID = std::stoi(match[1]);
        }
        Name = text;
    }

    /**
     * 加载基本信息
     */
    void LoadInfo(std::shared_ptr<Wz_Node> infoNode) {
        auto getValueInt = [](std::shared_ptr<Wz_Node> node, int defaultVal) -> int {
            if (!node) return defaultVal;
            // TODO: 实现属性值读取
            return defaultVal;
        };

        auto getValueStr = [](std::shared_ptr<Wz_Node> node, const std::string& defaultVal) -> std::string {
            if (!node) return defaultVal;
            // TODO: 实现属性值读取
            return defaultVal;
        };

        auto getValueBool = [](std::shared_ptr<Wz_Node> node, bool defaultVal) -> bool {
            if (!node) return defaultVal;
            // TODO: 实现属性值读取
            return defaultVal;
        };

        // 视野矩形
        VRLeft = getValueInt(infoNode->getNodes()->operator[]("VRLeft"), 0);
        VRTop = getValueInt(infoNode->getNodes()->operator[]("VRTop"), 0);
        VRRight = getValueInt(infoNode->getNodes()->operator[]("VRRight"), 0);
        VRBottom = getValueInt(infoNode->getNodes()->operator[]("VRBottom"), 0);

        // 其他属性
        Bgm = getValueStr(infoNode->getNodes()->operator[]("bgm"), "");
        Link = getValueInt(infoNode->getNodes()->operator[]("link"), -1);
        MapMark = getValueStr(infoNode->getNodes()->operator[]("mapMark"), "");

        IsTown = getValueBool(infoNode->getNodes()->operator[]("town"), false);
        CanFly = getValueBool(infoNode->getNodes()->operator[]("fly"), false);
        CanSwim = getValueBool(infoNode->getNodes()->operator[]("swim"), false);
        ReturnMap = getValueInt(infoNode->getNodes()->operator[]("returnMap"), 999999999);
        HideMinimap = getValueBool(infoNode->getNodes()->operator[]("hideMinimap"), false);
        FieldLimit = getValueInt(infoNode->getNodes()->operator[]("fieldLimit"), 0);
    }

    /**
     * 加载小地图
     */
    void LoadMinimap(std::shared_ptr<Wz_Node> miniMapNode) {
        // TODO: 实现小地图加载
    }

    /**
     * 加载背景
     */
    void LoadBack(std::shared_ptr<Wz_Node> backNode) {
        // TODO: 实现背景加载
    }

    /**
     * 加载图层
     */
    void LoadLayer(std::shared_ptr<Wz_Node> layerNode, int layerIndex) {
        // TODO: 实现图层加载（obj 和 tile）
    }

    /**
     * 加载脚踏板
     */
    void LoadFoothold(std::shared_ptr<Wz_Node> fhLevelNode, int level) {
        // TODO: 实现脚踏板加载
    }

    /**
     * 加载生命（mob/npc）
     */
    void LoadLife(std::shared_ptr<Wz_Node> lifeNode) {
        // TODO: 实现生命加载
    }

    /**
     * 加载反应器
     */
    void LoadReactor(std::shared_ptr<Wz_Node> reactorNode) {
        // TODO: 实现反应器加载
    }

    /**
     * 加载传送门
     */
    void LoadPortal(std::shared_ptr<Wz_Node> portalNode) {
        // TODO: 实现传送门加载
    }

    /**
     * 加载绳梯
     */
    void LoadLadderRope(std::shared_ptr<Wz_Node> ladderRopeNode) {
        // TODO: 实现绳梯加载
    }

    /**
     * 加载工具提示
     */
    void LoadTooltip(std::shared_ptr<Wz_Node> tooltipNode) {
        // TODO: 实现工具提示加载
    }

    /**
     * 加载粒子
     */
    void LoadParticle(std::shared_ptr<Wz_Node> particleNode) {
        // TODO: 实现粒子加载
    }

    /**
     * 加载光照
     */
    void LoadLight(std::shared_ptr<Wz_Node> lightNode) {
        // TODO: 实现光照加载
    }

    /**
     * 计算地图大小
     */
    void CalcMapSize() {
        // TODO: 实现地图大小计算
    }

private:
    int ID = -1;
    std::string Name;
    std::string Bgm;
    int Link = -1;
    std::string MapMark;
    bool IsTown = false;
    bool CanFly = false;
    bool CanSwim = false;
    int ReturnMap = 999999999;
    bool HideMinimap = false;
    int FieldLimit = 0;

    // 视野矩形
    int VRLeft = 0;
    int VRRight = 0;
    int VRTop = 0;
    int VRBottom = 0;
};

} // namespace WzLibCpp

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <regex>
#include <functional>
#include <variant>
#include <iostream>

#if defined(USE_SDL_LOG)
#include <SDL3/SDL.h>
#endif

#include "Wz_Node.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"
#include "PluginBase/PluginManager.hpp"

namespace WzLibCpp {

#if defined(USE_SDL_LOG)
    #define WZ_LOG_DEBUG(...) SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
    #define WZ_LOG_INFO(...) SDL_Log(__VA_ARGS__)
    #define WZ_LOG_WARN(...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
    #define WZ_LOG_ERROR(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#else
    class WzLogger {
    public:
        template<typename... Args>
        static void debug(Args&&... args) {
            print("DEBUG", std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        static void info(Args&&... args) {
            print("INFO", std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        static void warn(Args&&... args) {
            print("WARN", std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        static void error(Args&&... args) {
            print("ERROR", std::forward<Args>(args)...);
        }
        
    private:
        template<typename T>
        static void printImpl(T&& value) {
            std::cout << value;
        }
        
        template<typename T, typename... Args>
        static void printImpl(T&& first, Args&&... rest) {
            std::cout << first;
            printImpl(std::forward<Args>(rest)...);
        }
        
        template<typename... Args>
        static void print(const char* level, Args&&... args) {
            std::cout << "[WzLib] [" << level << "] ";
            printImpl(std::forward<Args>(args)...);
            std::cout << std::endl;
        }
    };
    
    // #define WZ_LOG_DEBUG(...) WzLogger::debug(__VA_ARGS__)
    #define WZ_LOG_DEBUG(...) ((void)0)
    #define WZ_LOG_INFO(...) WzLogger::info(__VA_ARGS__)
    #define WZ_LOG_WARN(...) WzLogger::warn(__VA_ARGS__)
    #define WZ_LOG_ERROR(...) WzLogger::error(__VA_ARGS__)
#endif

/**
 * 从 Wz_Node 获取整数值
 */
inline int GetInt(std::shared_ptr<Wz_Node> node, int defaultValue = 0) {
    if (!node) return defaultValue;
    return node->getInt(defaultValue);
}

/**
 * 从 Wz_Node 获取字符串值
 */
inline std::string GetString(std::shared_ptr<Wz_Node> node, const std::string& defaultValue = "") {
    if (!node) return defaultValue;
    return node->getString(defaultValue);
}

/**
 * 从 Wz_Node 获取布尔值
 */
inline bool GetBool(std::shared_ptr<Wz_Node> node, bool defaultValue = false) {
    if (!node) return defaultValue;
    return node->getBool(defaultValue);
}

/**
 * 从 Wz_Node 获取浮点数值
 */
inline float GetFloat(std::shared_ptr<Wz_Node> node, float defaultValue = 0.0f) {
    if (!node) return defaultValue;
    return node->getFloat(defaultValue);
}

/**
 * 矩形结构
 */
struct Rectangle {
    int X = 0, Y = 0, Width = 0, Height = 0;
    Rectangle() = default;
    Rectangle(int x, int y, int w, int h) : X(x), Y(y), Width(w), Height(h) {}
};

/**
 * 二维向量结构
 */
struct Vector2 {
    int X = 0, Y = 0;
    Vector2() = default;
    Vector2(int x, int y) : X(x), Y(y) {}
};

/**
 * 小地图数据
 */
struct MiniMapData {
    int Width = 0;
    int Height = 0;
    int CenterX = 0;
    int CenterY = 0;
    int Mag = 0;
    std::string Mark;
};

/**
 * 传送门数据
 */
struct PortalData {
    int Index = 0;
    std::string Name;
    int Type = 0;
    int X = 0, Y = 0;
    int ToMap = -1;
    std::string ToName;
    std::string Script;
    bool IsHideTooltip = false;
    int Delay = 0;
    int VerticalImpact = 0;
    int HorizontalImpact = 0;
    int HRange = 0;
    int VRange = 0;
};

/**
 * 脚踏板数据
 */
struct FootholdData {
    int ID = 0;
    int X1 = 0, Y1 = 0;
    int X2 = 0, Y2 = 0;
    int Next = 0;
    int Prev = 0;
    int Force = 0;
    int Type = 0;
    bool CantThrough = false;
};

/**
 * 生命数据（Mob/NPC）
 */
struct LifeData {
    int ID = 0;
    int X = 0, Y = 0;
    int Foothold = 0;
    int RespawnTime = 0;
    int MobTime = 0;
    bool Flip = false;
    bool Hide = false;
    int Cy = 0;
    int Rx0 = 0, Rx1 = 0;
    std::string Type;
};

/**
 * 背景数据
 */
struct BackData {
    int Type = 0;
    int X = 0, Y = 0;
    int CX = 0, CY = 0;
    int RX = 0, RY = 0;
    int A = 0;
    int Front = 0;
    std::string BSMove;
    int Ani = 0;
    std::string No;
};

/**
 * 反应器数据
 */
struct ReactorData {
    int ID = 0;
    int X = 0, Y = 0;
    int Foothold = 0;
    bool Flip = false;
    int ReactorTime = 0;
    std::string Name;
};

/**
 * 绳梯数据
 */
struct LadderRopeData {
    int X = 0;
    int Y1 = 0, Y2 = 0;
    int Page = 0;
    bool Ladder = false;
};

/**
 * 地图数据类
 */
class MapData {
public:
    MapData();
    virtual ~MapData() = default;

    /**
     * 加载地图数据
     * @param mapImgNode 地图图片节点（已解压）
     * @return 是否加载成功
     */
    bool Load(std::shared_ptr<Wz_Node> mapImgNode);

    /**
     * 根据地图 ID 查找并加载地图
     * @param mapID 地图 ID
     * @return 是否加载成功
     */
    bool LoadByID(int mapID);

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
    int getFieldType() const { return FieldType; }
    int getFirstUserEnter() const { return FirstUserEnter; }
    int getForcedReturn() const { return ForcedReturn; }
    int getDecHP() const { return DecHP; }
    int getProtectItem() const { return ProtectItem; }
    
    const Rectangle& getVRect() const { return VRect; }
    const MiniMapData& getMiniMap() const { return MiniMap; }
    const std::vector<PortalData>& getPortals() const { return Portals; }
    const std::vector<LifeData>& getLifes() const { return Lifes; }
    const std::vector<FootholdData>& getFootholds() const { return Footholds; }
    const std::vector<ReactorData>& getReactors() const { return Reactors; }
    const std::vector<BackData>& getBacks() const { return Backs; }
    const std::vector<LadderRopeData>& getLadderRopes() const { return LadderRopes; }

private:
    void LoadIDOrName(std::shared_ptr<Wz_Node> mapImgNode);
    void LoadInfo(std::shared_ptr<Wz_Node> infoNode);
    void LoadMinimap(std::shared_ptr<Wz_Node> miniMapNode);
    void LoadBack(std::shared_ptr<Wz_Node> backNode);
    void LoadLayer(std::shared_ptr<Wz_Node> layerNode, int layerIndex);
    void LoadFoothold(std::shared_ptr<Wz_Node> fhLevelNode, int level);
    void LoadLife(std::shared_ptr<Wz_Node> lifeNode);
    void LoadReactor(std::shared_ptr<Wz_Node> reactorNode);
    void LoadPortal(std::shared_ptr<Wz_Node> portalNode);
    void LoadLadderRope(std::shared_ptr<Wz_Node> ladderRopeNode);
    void LoadTooltip(std::shared_ptr<Wz_Node> tooltipNode);
    void LoadParticle(std::shared_ptr<Wz_Node> particleNode);
    void LoadLight(std::shared_ptr<Wz_Node> lightNode);
    void CalcMapSize();

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
    int FieldType = 0;
    int FirstUserEnter = -1;
    int ForcedReturn = 999999999;
    int DecHP = 0;
    int ProtectItem = 0;
    bool Swim = false;
    bool Fly = false;
    bool Town = false;
    
    Rectangle VRect;
    MiniMapData MiniMap;
    
    std::vector<PortalData> Portals;
    std::vector<LifeData> Lifes;
    std::vector<FootholdData> Footholds;
    std::vector<ReactorData> Reactors;
    std::vector<BackData> Backs;
    std::vector<LadderRopeData> LadderRopes;
    
    int MapLeft = 0;
    int MapRight = 0;
    int MapTop = 0;
    int MapBottom = 0;
};

inline MapData::MapData() {
}

inline static bool FindMapByID(int mapID, std::shared_ptr<Wz_Node>& mapImgNode) {
    int folderNum = mapID / 100000000;
    
    char fullPath[128];
    snprintf(fullPath, sizeof(fullPath), 
             "Map\\Map\\Map%d\\%09d.img", folderNum, mapID);
    
    mapImgNode = PluginBase::PluginManager::FindWz(fullPath);
    
    if (mapImgNode) {
        auto img = mapImgNode->getValue<Wz_Image>();
        if (img) {
            if (img->getOffset() == 0) {
                auto wzFile = std::dynamic_pointer_cast<Wz_File>(img->getWzFile());
                if (wzFile) {
                    int64_t calculatedOffset = wzFile->calcOffset(
                        img->getHashedOffsetPosition(), 
                        img->getHashedOffset());
                    img->setOffset(calculatedOffset);
                }
            }
            
            img->tryExtract();
            return true;
        }
    }
    
    mapImgNode = nullptr;
    return false;
}

inline bool MapData::LoadByID(int mapID) {
    std::shared_ptr<Wz_Node> mapImgNode;
    
    if (!FindMapByID(mapID, mapImgNode)) {
        WZ_LOG_ERROR("Failed to find map by ID: ", mapID);
        return false;
    }
    
    return Load(mapImgNode);
}

inline bool MapData::Load(std::shared_ptr<Wz_Node> mapImgNode) {
    if (!mapImgNode) {
        WZ_LOG_ERROR("MapData::Load: mapImgNode is null");
        return false;
    }

    LoadIDOrName(mapImgNode);
    WZ_LOG_DEBUG("MapData::Load: ID=%d, Name='%s'", ID, Name.c_str());

    auto nodes = mapImgNode->getNodes();
    
    if (!nodes) {
        WZ_LOG_WARN("MapData::Load: nodes is null");
        return true;
    }

    WZ_LOG_DEBUG("MapData::Load: nodes count=%zu", nodes->getCount());

    auto infoNode = nodes->operator[]("info");
    if (infoNode) {
        LoadInfo(infoNode);
    } else {
        WZ_LOG_WARN("MapData::Load: infoNode is null");
    }

    auto miniMapNode = nodes->operator[]("miniMap");
    if (miniMapNode) {
        LoadMinimap(miniMapNode);
    }

    auto backNode = nodes->operator[]("back");
    if (backNode) {
        LoadBack(backNode);
    }

    for (int i = 0; i <= 7; i++) {
        auto layerNode = nodes->operator[](std::to_string(i));
        if (layerNode) {
            LoadLayer(layerNode, i);
        }
    }

    auto footholdNode = nodes->operator[]("foothold");
    if (footholdNode) {
        for (int i = 0; i <= 7; i++) {
            auto fhLevel = footholdNode->getNodes()->operator[](std::to_string(i));
            if (fhLevel) {
                LoadFoothold(fhLevel, i);
            }
        }
    }

    auto lifeNode = nodes->operator[]("life");
    if (lifeNode) {
        LoadLife(lifeNode);
    }

    auto reactorNode = nodes->operator[]("reactor");
    if (reactorNode) {
        LoadReactor(reactorNode);
    }

    auto portalNode = nodes->operator[]("portal");
    if (portalNode) {
        LoadPortal(portalNode);
    }

    auto ladderRopeNode = nodes->operator[]("ladderRope");
    if (ladderRopeNode) {
        LoadLadderRope(ladderRopeNode);
    }

    auto tooltipNode = nodes->operator[]("ToolTip");
    if (tooltipNode) {
        LoadTooltip(tooltipNode);
    }

    auto particleNode = nodes->operator[]("particle");
    if (particleNode) {
        LoadParticle(particleNode);
    }

    auto lightNode = nodes->operator[]("light");
    if (lightNode) {
        LoadLight(lightNode);
    }

    CalcMapSize();

    return true;
}

inline void MapData::LoadIDOrName(std::shared_ptr<Wz_Node> mapImgNode) {
    std::string text = mapImgNode->getText();
    std::regex pattern(R"((\d{9})\.img)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        ID = std::stoi(match[1]);
    }
    Name = text;
}

inline void MapData::LoadInfo(std::shared_ptr<Wz_Node> infoNode) {
    if (!infoNode) {
        WZ_LOG_WARN("MapData::LoadInfo: infoNode is null");
        return;
    }
    if (!infoNode->getNodes()) {
        WZ_LOG_WARN("MapData::LoadInfo: infoNode->getNodes() is null");
        return;
    }
    
    auto infoNodes = infoNode->getNodes();
    
    WZ_LOG_DEBUG("MapData::LoadInfo: infoNode has %zu children", infoNodes->getCount());
    
    int vrLeft = GetInt(infoNodes->operator[]("VRLeft"), 0);
    int vrTop = GetInt(infoNodes->operator[]("VRTop"), 0);
    int vrRight = GetInt(infoNodes->operator[]("VRRight"), 0);
    int vrBottom = GetInt(infoNodes->operator[]("VRBottom"), 0);
    if (vrRight > vrLeft && vrBottom > vrTop) {
        VRect = Rectangle(vrLeft, vrTop, vrRight - vrLeft, vrBottom - vrTop);
        MapLeft = vrLeft;
        MapTop = vrTop;
        MapRight = vrRight;
        MapBottom = vrBottom;
    }
    
    Bgm = GetString(infoNodes->operator[]("bgm"), "");
    Link = GetInt(infoNodes->operator[]("link"), -1);
    MapMark = GetString(infoNodes->operator[]("mapMark"), "");
    
    IsTown = GetBool(infoNodes->operator[]("town"), false);
    CanFly = GetBool(infoNodes->operator[]("fly"), false);
    CanSwim = GetBool(infoNodes->operator[]("swim"), false);
    ReturnMap = GetInt(infoNodes->operator[]("returnMap"), 999999999);
    HideMinimap = GetBool(infoNodes->operator[]("hideMinimap"), false);
    FieldLimit = GetInt(infoNodes->operator[]("fieldLimit"), 0);
    
    WZ_LOG_DEBUG("MapData::LoadInfo: VRect=(%d,%d,%d,%d), town=%d, swim=%d", 
                 VRect.X, VRect.Y, VRect.Width, VRect.Height, IsTown, CanSwim);
}

inline void MapData::LoadMinimap(std::shared_ptr<Wz_Node> miniMapNode) {
    if (!miniMapNode || !miniMapNode->getNodes()) return;
    
    auto nodes = miniMapNode->getNodes();
    MiniMap.Width = GetInt(nodes->operator[]("width"), 0);
    MiniMap.Height = GetInt(nodes->operator[]("height"), 0);
    MiniMap.CenterX = GetInt(nodes->operator[]("centerX"), 0);
    MiniMap.CenterY = GetInt(nodes->operator[]("centerY"), 0);
    MiniMap.Mag = GetInt(nodes->operator[]("mag"), 0);
}

inline void MapData::LoadBack(std::shared_ptr<Wz_Node> backNode) {
    if (!backNode || !backNode->getNodes()) return;
    
    for (const auto& child : *backNode->getNodes()) {
        if (!child || !child->getNodes()) continue;
        
        BackData back;
        auto nodes = child->getNodes();
        
        back.Type = GetInt(nodes->operator[]("type"), 0);
        back.X = GetInt(nodes->operator[]("x"), 0);
        back.Y = GetInt(nodes->operator[]("y"), 0);
        back.CX = GetInt(nodes->operator[]("cx"), 0);
        back.CY = GetInt(nodes->operator[]("cy"), 0);
        back.RX = GetInt(nodes->operator[]("rx"), 0);
        back.RY = GetInt(nodes->operator[]("ry"), 0);
        back.A = GetInt(nodes->operator[]("a"), 0);
        back.Front = GetInt(nodes->operator[]("front"), 0);
        back.BSMove = GetString(nodes->operator[]("bs"), "");
        back.Ani = GetInt(nodes->operator[]("ani"), 0);
        back.No = std::to_string(GetInt(nodes->operator[]("no"), 0));
        
        Backs.push_back(back);
    }
}

inline void MapData::LoadLayer(std::shared_ptr<Wz_Node> layerNode, int layerIndex) {
    if (!layerNode || !layerNode->getNodes()) return;
    
    WZ_LOG_DEBUG("MapData::LoadLayer: layer=%d, children=%zu", layerIndex, layerNode->getNodes()->getCount());
    
    for (size_t i = 0; i < layerNode->getNodes()->getCount(); i++) {
        auto child = (*layerNode->getNodes())[i];
        if (child) {
            WZ_LOG_DEBUG("MapData::LoadLayer: layer child: '%s'", child->getText().c_str());
        }
    }
    
    auto objNode = layerNode->getNodes()->operator[]("obj");
    if (objNode && objNode->getNodes()) {
        WZ_LOG_DEBUG("MapData::LoadLayer: obj count=%zu", objNode->getNodes()->getCount());
        for (const auto& child : *objNode->getNodes()) {
            if (!child || !child->getNodes()) continue;
        }
    }
    
    auto tileNode = layerNode->getNodes()->operator[]("tile");
    if (tileNode && tileNode->getNodes()) {
        WZ_LOG_DEBUG("MapData::LoadLayer: tile count=%zu", tileNode->getNodes()->getCount());
        for (const auto& child : *tileNode->getNodes()) {
            if (!child || !child->getNodes()) continue;
        }
    }
}

inline void MapData::LoadFoothold(std::shared_ptr<Wz_Node> fhLevelNode, int level) {
    if (!fhLevelNode || !fhLevelNode->getNodes()) return;
    
    WZ_LOG_DEBUG("MapData::LoadFoothold: level=%d, count=%zu", level, fhLevelNode->getNodes()->getCount());
    
    for (const auto& child : *fhLevelNode->getNodes()) {
        if (!child || !child->getNodes()) continue;
        
        FootholdData fh;
        auto nodes = child->getNodes();
        
        fh.ID = std::stoi(child->getText());
        fh.X1 = GetInt(nodes->operator[]("x1"), 0);
        fh.Y1 = GetInt(nodes->operator[]("y1"), 0);
        fh.X2 = GetInt(nodes->operator[]("x2"), 0);
        fh.Y2 = GetInt(nodes->operator[]("y2"), 0);
        fh.Next = GetInt(nodes->operator[]("next"), 0);
        fh.Prev = GetInt(nodes->operator[]("prev"), 0);
        fh.Force = GetInt(nodes->operator[]("force"), 0);
        fh.Type = GetInt(nodes->operator[]("type"), 0);
        fh.CantThrough = GetBool(nodes->operator[]("cantThrough"), false);
        
        Footholds.push_back(fh);
    }
}

inline void MapData::LoadLife(std::shared_ptr<Wz_Node> lifeNode) {
    if (!lifeNode || !lifeNode->getNodes()) return;
    
    WZ_LOG_DEBUG("MapData::LoadLife: life count=%zu", lifeNode->getNodes()->getCount());
    
    for (const auto& child : *lifeNode->getNodes()) {
        if (!child || !child->getNodes()) continue;
        
        LifeData life;
        auto nodes = child->getNodes();
        
        life.ID = std::stoi(child->getText());
        life.Type = GetString(nodes->operator[]("type"), "");
        life.X = GetInt(nodes->operator[]("x"), 0);
        life.Y = GetInt(nodes->operator[]("y"), 0);
        life.Foothold = GetInt(nodes->operator[]("fh"), 0);
        life.RespawnTime = GetInt(nodes->operator[]("mobTime"), 0);
        life.MobTime = GetInt(nodes->operator[]("mobTime"), 0);
        life.Flip = GetBool(nodes->operator[]("f"), false);
        life.Hide = GetBool(nodes->operator[]("hide"), false);
        life.Cy = GetInt(nodes->operator[]("cy"), 0);
        life.Rx0 = GetInt(nodes->operator[]("rx0"), 0);
        life.Rx1 = GetInt(nodes->operator[]("rx1"), 0);
        
        Lifes.push_back(life);
        
        if (Lifes.size() <= 3) {
            WZ_LOG_DEBUG("MapData::LoadLife: Life %d: type='%s', pos=(%d,%d), foothold=%d, mobTime=%d, flip=%d, hide=%d",
                        life.ID, life.Type.c_str(), life.X, life.Y, life.Foothold, life.MobTime, life.Flip, life.Hide);
        }
    }
}

inline void MapData::LoadReactor(std::shared_ptr<Wz_Node> reactorNode) {
    if (!reactorNode || !reactorNode->getNodes()) return;
    
    for (const auto& child : *reactorNode->getNodes()) {
        if (!child || !child->getNodes()) continue;
        
        ReactorData reactor;
        auto nodes = child->getNodes();
        
        reactor.ID = GetInt(nodes->operator[]("id"), 0);
        reactor.X = GetInt(nodes->operator[]("x"), 0);
        reactor.Y = GetInt(nodes->operator[]("y"), 0);
        reactor.Foothold = GetInt(nodes->operator[]("fh"), 0);
        reactor.Flip = GetBool(nodes->operator[]("f"), false);
        reactor.ReactorTime = GetInt(nodes->operator[]("reactorTime"), 0);
        reactor.Name = GetString(nodes->operator[]("name"), "");
        
        Reactors.push_back(reactor);
    }
}

inline void MapData::LoadPortal(std::shared_ptr<Wz_Node> portalNode) {
    if (!portalNode || !portalNode->getNodes()) return;
    
    WZ_LOG_DEBUG("MapData::LoadPortal: portal count=%zu", portalNode->getNodes()->getCount());
    
    for (const auto& child : *portalNode->getNodes()) {
        if (!child || !child->getNodes()) continue;
        
        PortalData portal;
        auto nodes = child->getNodes();
        
        portal.Index = std::stoi(child->getText());
        portal.Name = GetString(nodes->operator[]("pn"), "");
        portal.Type = GetInt(nodes->operator[]("pt"), 0);
        portal.X = GetInt(nodes->operator[]("x"), 0);
        portal.Y = GetInt(nodes->operator[]("y"), 0);
        portal.ToMap = GetInt(nodes->operator[]("tm"), -1);
        portal.ToName = GetString(nodes->operator[]("tn"), "");
        portal.Script = GetString(nodes->operator[]("script"), "");
        portal.IsHideTooltip = GetBool(nodes->operator[]("hideTooltip"), false);
        portal.Delay = GetInt(nodes->operator[]("delay"), 0);
        portal.VerticalImpact = GetInt(nodes->operator[]("vi"), 0);
        portal.HorizontalImpact = GetInt(nodes->operator[]("hi"), 0);
        portal.HRange = GetInt(nodes->operator[]("hr"), 0);
        portal.VRange = GetInt(nodes->operator[]("vr"), 0);
        
        Portals.push_back(portal);
        
        if (Portals.size() <= 3) {
            WZ_LOG_DEBUG("MapData::LoadPortal: Portal %d: name='%s', type=%d, pos=(%d,%d), toMap=%d, toName='%s'",
                        portal.Index, portal.Name.c_str(), portal.Type, portal.X, portal.Y, portal.ToMap, portal.ToName.c_str());
        }
    }
}

inline void MapData::LoadLadderRope(std::shared_ptr<Wz_Node> ladderRopeNode) {
    if (!ladderRopeNode || !ladderRopeNode->getNodes()) return;
    
    for (const auto& child : *ladderRopeNode->getNodes()) {
        if (!child || !child->getNodes()) continue;
        
        LadderRopeData rope;
        auto nodes = child->getNodes();
        
        rope.X = GetInt(nodes->operator[]("x"), 0);
        rope.Y1 = GetInt(nodes->operator[]("y1"), 0);
        rope.Y2 = GetInt(nodes->operator[]("y2"), 0);
        rope.Page = GetInt(nodes->operator[]("page"), 0);
        rope.Ladder = GetBool(nodes->operator[]("l"), false);
        
        LadderRopes.push_back(rope);
    }
}

inline void MapData::LoadTooltip(std::shared_ptr<Wz_Node> tooltipNode) {
    if (!tooltipNode || !tooltipNode->getNodes()) return;
}

inline void MapData::LoadParticle(std::shared_ptr<Wz_Node> particleNode) {
    if (!particleNode || !particleNode->getNodes()) return;
}

inline void MapData::LoadLight(std::shared_ptr<Wz_Node> lightNode) {
    if (!lightNode || !lightNode->getNodes()) return;
}

inline void MapData::CalcMapSize() {
    if (VRect.Width == 0 || VRect.Height == 0) {
        int left = 0, right = 0, top = 0, bottom = 0;
        
        for (const auto& fh : Footholds) {
            left = std::min({left, fh.X1, fh.X2});
            right = std::max({right, fh.X1, fh.X2});
            top = std::min({top, fh.Y1, fh.Y2});
            bottom = std::max({bottom, fh.Y1, fh.Y2});
        }
        
        left -= 200;
        right += 200;
        top -= 300;
        bottom += 100;
        
        VRect = Rectangle(left, top, right - left, bottom - top);
    }
    
    MapLeft = VRect.X;
    MapTop = VRect.Y;
    MapRight = VRect.X + VRect.Width;
    MapBottom = VRect.Y + VRect.Height;
}

} // namespace WzLibCpp

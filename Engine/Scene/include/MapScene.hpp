#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <entt/entt.hpp>
#include <SDL3/SDL.h>

namespace MapleEngine {

struct ContainerNode {
    struct Slot {
        virtual ~Slot() = default;
        virtual void update(float deltaTime) {}
        virtual void render(SDL_Renderer* renderer, int cameraX, int cameraY) {}
    };
    
    virtual void update(float deltaTime);
    virtual void render(SDL_Renderer* renderer, int cameraX, int cameraY);
    
    std::vector<std::shared_ptr<Slot>> slots_;
};

struct CameraComp {
    int x = 0, y = 0;
    int w = 0, h = 0;
};

struct PositionComp {
    int x = 0, y = 0;
};

struct BackComp {
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
    int x = 0, y = 0;
    int cx = 0, cy = 0;
    int rx = 0, ry = 0;
    int type = 0;
    bool flipX = false;
    bool front = false;
    int alpha = 255;
    int originX = 0, originY = 0;
};

struct TileComp {
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
    int x = 0, y = 0;
    int z = 0;
    int layer = 0;
    int originX = 0, originY = 0;
};

struct ObjComp {
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
    int x = 0, y = 0;
    int z = 0;
    int layer = 0;
    int originX = 0, originY = 0;
};

struct FootholdComp {
    int x1 = 0, y1 = 0;
    int x2 = 0, y2 = 0;
    int prev = 0, next = 0;
    int layer = 0;
};

struct PortalComp {
    int x = 0, y = 0;
    int type = 0;
    std::string target;
};

struct LifeComp {
    int x = 0, y = 0;
    int id = 0;
    int type = 0;
    int foothold = 0;
    int fh = 0;
};

struct BackItem : public ContainerNode::Slot {
    std::string bS;
    int x = 0, y = 0;
    int cx = 0, cy = 0;
    int rx = 0, ry = 0;
    int type = 0;
    int tileMode = 0;
    bool flipX = false;
    bool front = false;
    int alpha = 255;
    int index = 0;
    int animFrame = 0;
    int time = 0;
    int screenMode = 0;
    int no = 0;
    int ani = 0;
    std::string noStr;
    int originX = 0, originY = 0;
    
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
};

struct ObjItem : public ContainerNode::Slot {
    int x = 0, y = 0;
    int z = 0;
    std::string oS;
    std::string l0, l1, l2;
    bool flipX = false;
    bool light = false;
    int questID = 0;
    int animFrame = 0;
    int time = 0;
    int layer = 0;
    int originX = 0, originY = 0;
    
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
};

struct TileItem : public ContainerNode::Slot {
    int x = 0, y = 0;
    int z = 0;
    int u = 0, v = 0;
    int animFrame = 0;
    int layer = 0;
    std::string tilesetName;
    std::string tileNo;
    std::string uStr;
    int originX = 0, originY = 0;
    
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
};

struct LifeItem : public ContainerNode::Slot {
    enum class LifeType { Mob, Npc };
    
    int id = 0;
    LifeType type = LifeType::Mob;
    int x = 0, y = 0;
    int foothold = 0;
    int fh = 0;
    int cy = 0;
    int rx = 0, ry = 0;
    int time = 0;
    int mobTime = 0;
    std::string typeName;
    std::string name;
    std::string funcName;
    std::string interactName;
    std::string script;
    std::vector<int> links;
};

struct PortalItem : public ContainerNode::Slot {
    int x = 0, y = 0;
    int type = 0;
    std::string targetPortalName;
    int targetMapId = 0;
    std::string name;
};

struct MapScene {
    std::vector<BackItem> backs;
    std::vector<TileItem> tiles;
    std::vector<ObjItem> objs;
    std::vector<LifeItem> lifes;
    std::vector<PortalItem> portals;
    
    std::unordered_map<int, std::vector<int>> footholds;
    
    bool showFoothold = false;
    bool showPortal = false;
    bool showLife = false;
    bool showBack = true;
    bool showTile = true;
    bool showObj = true;
};

}
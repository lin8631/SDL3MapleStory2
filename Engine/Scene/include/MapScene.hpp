#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <SDL3/SDL.h>

namespace MapleEngine {

class SceneNode {
public:
    SceneNode() = default;
    virtual ~SceneNode() = default;
    
    void addChild(std::shared_ptr<SceneNode> child) {
        child->parent_ = this;
        children_.push_back(child);
    }
    
    void removeChild(SceneNode* child) {
        children_.erase(
            std::remove_if(children_.begin(), children_.end(),
                [child](const std::shared_ptr<SceneNode>& n) { return n.get() == child; }),
            children_.end()
        );
    }
    
    const std::vector<std::shared_ptr<SceneNode>>& getChildren() const { return children_; }
    SceneNode* getParent() const { return parent_; }
    
    virtual void update(float deltaTime) {}
    virtual void render(SDL_Renderer* renderer, int cameraX, int cameraY) {}
    
private:
    SceneNode* parent_ = nullptr;
    std::vector<std::shared_ptr<SceneNode>> children_;
};

class ContainerNode : public SceneNode {
public:
    struct Slot {
        std::string name;
        int index = 0;
        std::vector<std::string> tags;
        
        virtual ~Slot() = default;
        virtual void update(float deltaTime) {}
        virtual void render(SDL_Renderer* renderer, int cameraX, int cameraY) {}
    };
    
    void addSlot(std::shared_ptr<Slot> slot) {
        slots_.push_back(slot);
    }
    
    void removeSlot(const std::string& name) {
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                [&name](const std::shared_ptr<Slot>& s) { return s->name == name; }),
            slots_.end()
        );
    }
    
    const std::vector<std::shared_ptr<Slot>>& getSlots() const { return slots_; }
    std::shared_ptr<Slot> findSlot(const std::string& name) {
        for (auto& slot : slots_) {
            if (slot->name == name) return slot;
        }
        return nullptr;
    }
    
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer, int cameraX, int cameraY) override;
    
private:
    std::vector<std::shared_ptr<Slot>> slots_;
};

class LayerNode : public SceneNode {
public:
    LayerNode();
    
    ContainerNode& getObj() { return obj_; }
    ContainerNode& getTile() { return tile_; }
    ContainerNode& getReactor() { return reactor_; }
    SceneNode& getFoothold() { return foothold_; }
    
private:
    ContainerNode obj_;
    ContainerNode tile_;
    ContainerNode reactor_;
    SceneNode foothold_;
};

class MapScene : public SceneNode {
public:
    MapScene();
    
    ContainerNode& getBack() { return back_; }
    SceneNode& getLayers() { return layers_; }
    ContainerNode& getFront() { return front_; }
    ContainerNode& getEffect() { return effect_; }
    ContainerNode& getPortal() { return portal_; }
    ContainerNode& getLadderRope() { return ladderRope_; }
    
private:
    ContainerNode back_;
    SceneNode layers_;
    ContainerNode front_;
    ContainerNode effect_;
    ContainerNode portal_;
    ContainerNode ladderRope_;
};

struct BackItem : public ContainerNode::Slot {
    std::string bS;
    int x = 0, y = 0;
    int cx = 0, cy = 0;
    int rx = 0, ry = 0;
    int type = 0;        // 滚动模式 (0-7)
    int tileMode = 0;
    bool flipX = false;
    bool front = false;
    int alpha = 255;
    int index = 0;
    int animFrame = 0;
    int time = 0;
    int screenMode = 0;
    int no = 0;
    int ani = 0;        // 动画类型：0=back(静态), 1=ani(帧动画), 2=spine(骨骼)
    
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
    std::string uStr;     // 瓦片类型字符串（如"bsc", "edD", "enH"等），对应WZ中的U字段
    
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
};

struct LifeItem : public ContainerNode::Slot {
    enum class LifeType { Mob, Npc };
    
    int id = 0;
    LifeType type = LifeType::Mob;
    int x = 0, y = 0;
    int foothold = 0;
    bool flipX = false;
    bool hide = false;
    int respawnTime = 0;
    int mobTime = 0;
    int cy = 0, rx0 = 0, rx1 = 0;
};

struct PortalItem : public ContainerNode::Slot {
    int id = 0;
    std::string name;
    int type = 0;
    int x = 0, y = 0;
    int targetMap = 0;
    std::string targetName;
    std::string script;
    float animTime = 0.0f;
};

struct FootholdItem {
    int id = 0;
    int x1 = 0, y1 = 0;
    int x2 = 0, y2 = 0;
    int next = 0, prev = 0;
    int force = 0;
    int type = 0;
};

struct MiniMapData {
    int width = 0, height = 0;
    int centerX = 0, centerY = 0;
    int mag = 0;
    std::string mark;
    int textureId = -1;
};

} // namespace MapleEngine

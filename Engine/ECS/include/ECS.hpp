#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdint>

#include "entt/entt.hpp"

namespace MapleEngine {

using Entity = entt::entity;
using Registry = entt::registry;

class Scene {
public:
    Scene() = default;
    ~Scene() = default;
    
    Entity createEntity() {
        return registry_.create();
    }
    
    void destroyEntity(Entity entity) {
        registry_.destroy(entity);
    }
    
    template<typename Component>
    Component& addComponent(Entity entity, Component&& component) {
        return registry_.emplace<Component>(entity, std::forward<Component>(component));
    }
    
    template<typename Component>
    Component& getComponent(Entity entity) {
        return registry_.get<Component>(entity);
    }
    
    template<typename Component>
    bool hasComponent(Entity entity) const {
        return registry_.template all_of<Component>(entity);
    }
    
    template<typename Component>
    void removeComponent(Entity entity) {
        registry_.remove<Component>(entity);
    }
    
    template<typename... Components>
    auto view() {
        return registry_.view<Components...>();
    }
    
    Registry& getRegistry() { return registry_; }
    const Registry& getRegistry() const { return registry_; }
    
private:
    Registry registry_;
};

struct TagComponent {
    std::string name;
    TagComponent() = default;
    TagComponent(const std::string& n) : name(n) {}
};

struct TransformComponent {
    float x = 0.0f, y = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f;
    float rotation = 0.0f;
    int zOrder = 0;
    
    TransformComponent() = default;
    TransformComponent(float x, float y) : x(x), y(y) {}
};

struct SpriteComponent {
    int textureId = -1;
    int srcX = 0, srcY = 0;
    int srcWidth = 0, srcHeight = 0;
    int width = 0, height = 0;
    float offsetX = 0.0f, offsetY = 0.0f;
    int alpha = 255;
    bool flipX = false;
    bool visible = true;
};

struct AnimationComponent {
    int currentFrame = 0;
    int frameCount = 0;
    int frameTime = 100;
    uint32_t lastUpdate = 0;
    bool loop = true;
    bool playing = true;
};

} // namespace MapleEngine

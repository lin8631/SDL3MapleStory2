#pragma once

#include <vector>
#include <memory>
#include <SDL3/SDL.h>
#include "MapScene.hpp"
#include "Animation.hpp"

namespace MapleEngine {

enum class RenderBlendMode {
    Alpha,
    Additive,
    Mask
};

struct MeshItem {
    int textureId = -1;
    int srcX = 0, srcY = 0;
    int srcWidth = 0, srcHeight = 0;
    float destX = 0.0f, destY = 0.0f;
    float width = 0.0f, height = 0.0f;
    float originX = 0.0f, originY = 0.0f;
    int alpha = 255;
    bool flipX = false;
    int z0 = 0;
    int z1 = 0;
    RenderBlendMode blendMode = RenderBlendMode::Alpha;
    SDL_Rect* tileRegion = nullptr;
    float tileOffX = 0.0f, tileOffY = 0.0f;
    
    int compareTo(const MeshItem& other) const {
        if (z0 != other.z0) return z0 < other.z0 ? -1 : 1;
        if (z1 != other.z1) return z1 < other.z1 ? -1 : 1;
        return 0;
    }
};

class MeshBatcher {
public:
    MeshBatcher() = default;
    
    void begin(SDL_Renderer* renderer, float offsetX, float offsetY);
    void end();
    
    void draw(const MeshItem& mesh);
    
    void setTexture(int textureId);
    void setBlendMode(RenderBlendMode mode);
    void drawTile(int textureId, int srcX, int srcY, int srcW, int srcH,
                  float destX, float destY, int alpha);
    
private:
    SDL_Renderer* renderer_ = nullptr;
    float offsetX_ = 0.0f;
    float offsetY_ = 0.0f;
    int currentTextureId_ = -1;
    RenderBlendMode currentBlendMode_ = RenderBlendMode::Alpha;
};

} // namespace MapleEngine

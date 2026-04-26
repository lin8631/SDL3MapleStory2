#include "MeshBatcher.hpp"
#include "MapScene.hpp"
#include "Texture.hpp"
#include <algorithm>

namespace MapleEngine {

void MeshBatcher::begin(SDL_Renderer* renderer, float offsetX, float offsetY) {
    renderer_ = renderer;
    offsetX_ = offsetX;
    offsetY_ = offsetY;
    currentTextureId_ = -1;
}

void MeshBatcher::end() {
    if (currentTextureId_ != -1) {
        SDL_RenderPresent(renderer_);
    }
}

void MeshBatcher::draw(const MeshItem& mesh) {
    if (mesh.textureId != currentTextureId_) {
        if (currentTextureId_ != -1) {
            SDL_RenderPresent(renderer_);
        }
        currentTextureId_ = mesh.textureId;
    }
    
    auto texture = TextureManager::getInstance().get(mesh.textureId);
    if (!texture) return;
    
    SDL_Texture* sdlTexture = texture->get();
    SDL_FRect srcRect = {static_cast<float>(mesh.srcX), static_cast<float>(mesh.srcY), 
                         static_cast<float>(mesh.srcWidth), static_cast<float>(mesh.srcHeight)};
    SDL_FRect destRect = {
        static_cast<float>(mesh.destX - offsetX_),
        static_cast<float>(mesh.destY - offsetY_),
        static_cast<float>(mesh.srcWidth),
        static_cast<float>(mesh.srcHeight)
    };
    
    SDL_RenderTextureRotated(renderer_, sdlTexture, &srcRect, &destRect,
                     0, nullptr, mesh.flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void MeshBatcher::setTexture(int textureId) {
    if (textureId != currentTextureId_) {
        if (currentTextureId_ != -1) {
            SDL_RenderPresent(renderer_);
        }
        currentTextureId_ = textureId;
    }
}

void MeshBatcher::setBlendMode(RenderBlendMode mode) {
    if (mode != currentBlendMode_) {
        currentBlendMode_ = mode;
        switch (mode) {
            case RenderBlendMode::Alpha:
                SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
                break;
            case RenderBlendMode::Additive:
                SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_ADD);
                break;
            case RenderBlendMode::Mask:
                SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
                break;
        }
    }
}

void MeshBatcher::drawTile(int textureId, int srcX, int srcY, int srcW, int srcH,
                           float destX, float destY, int alpha) {
    setTexture(textureId);
    
    auto texture = TextureManager::getInstance().get(textureId);
    if (!texture) return;
    
    SDL_FRect srcRect = {static_cast<float>(srcX), static_cast<float>(srcY), 
                         static_cast<float>(srcW), static_cast<float>(srcH)};
    SDL_FRect destRect = {
        static_cast<float>(destX - offsetX_),
        static_cast<float>(destY - offsetY_),
        static_cast<float>(srcW),
        static_cast<float>(srcH)
    };
    
    SDL_SetTextureAlphaMod(texture->get(), static_cast<Uint8>(alpha));
    SDL_RenderTexture(renderer_, texture->get(), &srcRect, &destRect);
}

} // namespace MapleEngine

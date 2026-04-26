#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <SDL3/SDL.h>

namespace MapleEngine {

class Texture {
public:
    Texture() = default;
    Texture(SDL_Texture* texture, int width, int height) 
        : texture_(texture), width_(width), height_(height) {}
    
    ~Texture() {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
    }
    
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    Texture(Texture&& other) noexcept 
        : texture_(other.texture_), width_(other.width_), height_(other.height_) {
        other.texture_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }
    
    Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            if (texture_) SDL_DestroyTexture(texture_);
            texture_ = other.texture_;
            width_ = other.width_;
            height_ = other.height_;
            other.texture_ = nullptr;
            other.width_ = 0;
            other.height_ = 0;
        }
        return *this;
    }
    
    SDL_Texture* get() const { return texture_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    friend class TextureManager;
    
private:
    SDL_Texture* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

class TextureManager {
public:
    static TextureManager& getInstance() {
        static TextureManager instance;
        return instance;
    }
    
    int loadFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
        if (!surface || !renderer) return -1;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) return -1;
        
        int id = nextId_++;
        textures_[id] = std::make_shared<Texture>(texture, surface->w, surface->h);
        return id;
    }
    
    int loadFromPixels(SDL_Renderer* renderer, int width, int height, void* pixels) {
        if (!renderer || !pixels) return -1;
        
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
        if (!texture) return -1;
        
        SDL_UpdateTexture(texture, nullptr, pixels, width * 4);
        
        int id = nextId_++;
        textures_[id] = std::make_shared<Texture>(texture, width, height);
        return id;
    }
    
    std::shared_ptr<Texture> get(int id) {
        auto it = textures_.find(id);
        if (it != textures_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    void remove(int id) {
        textures_.erase(id);
    }
    
    void clear() {
        textures_.clear();
    }
    
private:
    TextureManager() = default;
    
    int nextId_ = 1;
    std::unordered_map<int, std::shared_ptr<Texture>> textures_;
};

struct Rect {
    int x = 0, y = 0;
    int width = 0, height = 0;
    
    Rect() = default;
    Rect(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    
    SDL_FRect toSDLRect() const {
        return {static_cast<float>(x), static_cast<float>(y), 
                static_cast<float>(width), static_cast<float>(height)};
    }
};

struct Color {
    uint8_t r = 255, g = 255, b = 255, a = 255;
    
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    SDL_Color toSDLColor() const {
        return {r, g, b, a};
    }
};

struct Point {
    float x = 0.0f, y = 0.0f;
    
    Point() = default;
    Point(float x, float y) : x(x), y(y) {}
};

} // namespace MapleEngine

#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL.h>

namespace MapleEngine {

struct Vector2 {
    float x = 0.0f, y = 0.0f;
    
    Vector2() = default;
    Vector2(float x, float y) : x(x), y(y) {}
    
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }
    
    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }
    
    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
};

class Camera {
public:
    Camera() = default;
    
    void setViewport(int width, int height) {
        viewportWidth_ = width;
        viewportHeight_ = height;
    }
    
    void setCenter(float x, float y) {
        center_.x = x;
        center_.y = y;
    }
    
    Vector2 getCenter() const { return center_; }
    
    void setBounds(int left, int top, int right, int bottom) {
        boundsLeft_ = left;
        boundsTop_ = top;
        boundsRight_ = right;
        boundsBottom_ = bottom;
        useBounds_ = true;
    }
    
    void clearBounds() {
        useBounds_ = false;
    }
    
    SDL_Rect getClipRect() const {
        if (useBounds_) {
            int left = static_cast<int>(center_.x) - viewportWidth_ / 2;
            int top = static_cast<int>(center_.y) - viewportHeight_ / 2;
            return {left, top, viewportWidth_, viewportHeight_};
        }
        return {0, 0, viewportWidth_, viewportHeight_};
    }
    
    Vector2 getOrigin() const {
        SDL_Rect clip = getClipRect();
        return Vector2(static_cast<float>(clip.x), static_cast<float>(clip.y));
    }
    
    Vector2 worldToScreen(float worldX, float worldY) const {
        return Vector2(worldX - center_.x + viewportWidth_ / 2.0f,
                       worldY - center_.y + viewportHeight_ / 2.0f);
    }
    
    Vector2 screenToWorld(float screenX, float screenY) const {
        return Vector2(screenX + center_.x - viewportWidth_ / 2.0f,
                       screenY + center_.y - viewportHeight_ / 2.0f);
    }
    
    void adjustToRect(int left, int top, int right, int bottom) {
        int halfW = viewportWidth_ / 2;
        int halfH = viewportHeight_ / 2;
        
        if (right - left > viewportWidth_) {
            if (center_.x - halfW < left) center_.x = left + halfW;
            if (center_.x + halfW > right) center_.x = right - halfW;
        } else {
            center_.x = left + (right - left) / 2.0f;
        }
        
        if (bottom - top > viewportHeight_) {
            if (center_.y - halfH < top) center_.y = top + halfH;
            if (center_.y + halfH > bottom) center_.y = bottom - halfH;
        } else {
            center_.y = top + (bottom - top) / 2.0f;
        }
    }
    
private:
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;
    Vector2 center_;
    
    bool useBounds_ = false;
    int boundsLeft_ = 0, boundsTop_ = 0;
    int boundsRight_ = 0, boundsBottom_ = 0;
};

class InputState {
public:
    InputState() {
        keyboardState_ = SDL_GetKeyboardState(&keyboardStateSize_);
        prevKeyboardState_.resize(keyboardStateSize_);
    }
    
    void update() {
        prevKeyboardState_ = currentKeyboardState_;
        const bool* keys = SDL_GetKeyboardState(&keyboardStateSize_);
        currentKeyboardState_.assign(keys, keys + keyboardStateSize_);
        
        mouseButtonsPrev_ = mouseButtons_;
        float x, y;
        mouseButtons_ = SDL_GetMouseState(&x, &y);
        mouseX_ = static_cast<int>(x);
        mouseY_ = static_cast<int>(y);
    }
    
    bool isKeyDown(SDL_Scancode key) const {
        if (key < 0 || key >= SDL_SCANCODE_COUNT) return false;
        if (key >= static_cast<int>(currentKeyboardState_.size())) return false;
        return currentKeyboardState_[key];
    }
    
    bool isKeyPressed(SDL_Scancode key) const {
        if (key < 0 || key >= SDL_SCANCODE_COUNT) return false;
        if (key >= static_cast<int>(currentKeyboardState_.size())) return false;
        if (key >= static_cast<int>(prevKeyboardState_.size())) return false;
        return currentKeyboardState_[key] && !prevKeyboardState_[key];
    }
    
    bool isKeyReleased(SDL_Scancode key) const {
        if (key < 0 || key >= SDL_SCANCODE_COUNT) return false;
        if (key >= static_cast<int>(currentKeyboardState_.size())) return false;
        if (key >= static_cast<int>(prevKeyboardState_.size())) return false;
        return !currentKeyboardState_[key] && prevKeyboardState_[key];
    }
    
    bool isMouseButtonDown(int button) const {
        return (mouseButtons_ & (1 << (button - 1))) != 0;
    }
    
    bool isMouseButtonPressed(int button) const {
        return isMouseButtonDown(button) && (mouseButtonsPrev_ & (1 << (button - 1))) == 0;
    }
    
    int getMouseX() const { return mouseX_; }
    int getMouseY() const { return mouseY_; }
    
private:
    const bool* keyboardState_ = nullptr;
    int keyboardStateSize_ = 0;
    std::vector<char> currentKeyboardState_;
    std::vector<char> prevKeyboardState_;
    
    int mouseButtons_ = 0;
    int mouseButtonsPrev_ = 0;
    int mouseX_ = 0;
    int mouseY_ = 0;
};

} // namespace MapleEngine

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>
#include "Camera.hpp"
#include "MapData.hpp"

namespace MapleEngine {

class UIElement {
public:
    virtual ~UIElement() = default;
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual void update(float deltaTime) {}
    virtual bool handleEvent(const SDL_Event& event) { return false; }
    
    void setPosition(int x, int y) { x_ = x; y_ = y; }
    void setSize(int width, int height) { width_ = width; height_ = height; }
    
    int getX() const { return x_; }
    int getY() const { return y_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    bool isVisible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }
    
    bool containsPoint(int px, int py) const {
        return px >= x_ && px < x_ + width_ && py >= y_ && py < y_ + height_;
    }
    
protected:
    int x_ = 0, y_ = 0;
    int width_ = 0, height_ = 0;
    bool visible_ = true;
};

class MiniMapUI : public UIElement {
public:
    MiniMapUI();
    
    void setMapData(const MiniMapData& data) { mapData_ = data; }
    void setCamera(std::shared_ptr<Camera> camera) { camera_ = camera; }
    
    void render(SDL_Renderer* renderer) override;
    void update(float deltaTime) override;
    
private:
    MiniMapData mapData_;
    std::shared_ptr<Camera> camera_;
    int textureId_ = -1;
};

class TooltipUI : public UIElement {
public:
    TooltipUI();
    
    void setText(const std::string& text);
    void setPosition(int x, int y);
    
    void render(SDL_Renderer* renderer) override;
    void update(float deltaTime) override;
    
private:
    std::string text_;
    int padding_ = 8;
    int fontSize_ = 14;
};

class TopBarUI : public UIElement {
public:
    TopBarUI();
    
    void setMapName(const std::string& name) { mapName_ = name; }
    void setBgm(const std::string& bgm) { bgm_ = bgm; }
    
    void render(SDL_Renderer* renderer) override;
    
private:
    std::string mapName_;
    std::string bgm_;
};

class UIManager {
public:
    static UIManager& getInstance();
    
    void addElement(std::shared_ptr<UIElement> element);
    void removeElement(UIElement* element);
    
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);
    bool handleEvent(const SDL_Event& event);
    
    void setCamera(std::shared_ptr<Camera> camera) { camera_ = camera; }
    void setViewport(int width, int height);
    
private:
    UIManager() = default;
    
    std::vector<std::shared_ptr<UIElement>> elements_;
    std::shared_ptr<Camera> camera_;
    int viewportWidth_ = 1280;
    int viewportHeight_ = 720;
};

} // namespace MapleEngine

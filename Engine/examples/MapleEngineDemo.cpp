#include <iostream>
#include <memory>
#include <string>

#include "Engine.hpp"
#include "Camera.hpp"
#include "MapScene.hpp"
#include "Animation.hpp"
#include "ECS.hpp"

using namespace MapleEngine;

class GameApplication {
public:
    GameApplication() = default;
    
    bool init() {
        if (!Engine::getInstance().init(1280, 720, "MapleEngine Demo")) {
            return false;
        }
        
        Engine::getInstance().setUpdateCallback([this](float dt) { update(dt); });
        Engine::getInstance().setRenderCallback([this]() { render(); });
        Engine::getInstance().setEventCallback([this](SDL_Event& e) { handleEvent(e); });
        
        camera_.setViewport(1280, 720);
        camera_.setCenter(400, 300);
        
        return true;
    }
    
    void run() {
        Engine::getInstance().run();
    }
    
private:
    void update(float deltaTime) {
        input_.update();
        
        const float moveSpeed = 200.0f;
        auto center = camera_.getCenter();
        
        if (input_.isKeyDown(SDL_SCANCODE_W) || input_.isKeyDown(SDL_SCANCODE_UP)) {
            center.y -= moveSpeed * deltaTime;
        }
        if (input_.isKeyDown(SDL_SCANCODE_S) || input_.isKeyDown(SDL_SCANCODE_DOWN)) {
            center.y += moveSpeed * deltaTime;
        }
        if (input_.isKeyDown(SDL_SCANCODE_A) || input_.isKeyDown(SDL_SCANCODE_LEFT)) {
            center.x -= moveSpeed * deltaTime;
        }
        if (input_.isKeyDown(SDL_SCANCODE_D) || input_.isKeyDown(SDL_SCANCODE_RIGHT)) {
            center.x += moveSpeed * deltaTime;
        }
        
        camera_.setCenter(center.x, center.y);
        
        LOG_INFO("Camera: (", center.x, ", ", center.y, ")");
    }
    
    void render() {
        SDL_SetRenderDrawColor(Engine::getInstance().getRenderer(), 50, 50, 50, 255);
        SDL_RenderClear(Engine::getInstance().getRenderer());
        
        auto origin = camera_.getOrigin();
        SDL_SetRenderDrawColor(Engine::getInstance().getRenderer(), 100, 100, 100, 255);
        
        for (int x = static_cast<int>(origin.x) - 100; x < origin.x + 1280 + 100; x += 100) {
            SDL_RenderLine(Engine::getInstance().getRenderer(),
                static_cast<float>(x - static_cast<int>(origin.x)), 0.0f,
                static_cast<float>(x - static_cast<int>(origin.x)), 720.0f);
        }
        
        for (int y = static_cast<int>(origin.y) - 100; y < origin.y + 720 + 100; y += 100) {
            SDL_RenderLine(Engine::getInstance().getRenderer(),
                0.0f, static_cast<float>(y - static_cast<int>(origin.y)),
                1280.0f, static_cast<float>(y - static_cast<int>(origin.y)));
        }
    }
    
    void handleEvent(SDL_Event& event) {
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
            Engine::getInstance().stop();
        }
    }
    
    Camera camera_;
    InputState input_;
};

int main(int argc, char* argv[]) {
    GameApplication app;
    
    if (!app.init()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    LOG_INFO("MapleEngine started!");
    app.run();
    LOG_INFO("MapleEngine stopped!");
    
    return 0;
}

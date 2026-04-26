#include "Engine.hpp"
#include <iostream>
#include <stdexcept>

namespace MapleEngine {

Engine& Engine::getInstance() {
    static Engine instance;
    return instance;
}

Engine::~Engine() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

bool Engine::init(int width, int height, const std::string& title) {
    width_ = width;
    height_ = height;
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR("SDL 初始化失败: ", SDL_GetError());
        return false;
    }
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        width_,
        height_,
        0
    );
    
    if (!window_) {
        LOG_ERROR("窗口创建失败: ", SDL_GetError());
        return false;
    }
    
    renderer_ = SDL_CreateRenderer(window_, 0);
    SDL_SetRenderVSync(renderer_, 1);
    if (!renderer_) {
        LOG_ERROR("渲染器创建失败: ", SDL_GetError());
        return false;
    }
    
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    
    LOG_INFO("引擎初始化成功");
    return true;
}

void Engine::run() {
    if (!window_ || !renderer_) {
        LOG_ERROR("引擎未初始化");
        return;
    }
    
    running_ = true;
    uint32_t lastTime = SDL_GetTicks();
    
    while (running_) {
        uint32_t currentTime = SDL_GetTicks();
        deltaTime_ = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        processEvents();
        update(deltaTime_);
        render();
        
        SDL_RenderPresent(renderer_);
    }
}

void Engine::stop() {
    running_ = false;
}

void Engine::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            stop();
        }
        if (eventCallback_) {
            eventCallback_(event);
        }
    }
}

void Engine::update(float deltaTime) {
    if (updateCallback_) {
        updateCallback_(deltaTime);
    }
}

void Engine::render() {
    SDL_RenderClear(renderer_);
    if (renderCallback_) {
        renderCallback_();
    }
}

} // namespace MapleEngine

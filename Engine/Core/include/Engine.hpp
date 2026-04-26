#pragma once

#include <memory>
#include <string>
#include <functional>
#include <chrono>
#include <iostream>
#include <SDL3/SDL.h>

namespace MapleEngine {

class Engine {
public:
    static Engine& getInstance();
    
    bool init(int width, int height, const std::string& title);
    void run();
    void stop();
    
    SDL_Window* getWindow() const { return window_; }
    SDL_Renderer* getRenderer() const { return renderer_; }
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    float getDeltaTime() const { return deltaTime_; }
    uint32_t getTicks() const { return SDL_GetTicks(); }
    
    using UpdateFunc = std::function<void(float deltaTime)>;
    using RenderFunc = std::function<void()>;
    using EventFunc = std::function<void(SDL_Event&)>;
    
    void setUpdateCallback(UpdateFunc func) { updateCallback_ = func; }
    void setRenderCallback(RenderFunc func) { renderCallback_ = func; }
    void setEventCallback(EventFunc func) { eventCallback_ = func; }

private:
    Engine() = default;
    ~Engine();
    
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    
    void processEvents();
    void update(float deltaTime);
    void render();
    
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    
    int width_ = 0;
    int height_ = 0;
    bool running_ = false;
    float deltaTime_ = 0.0f;
    
    UpdateFunc updateCallback_;
    RenderFunc renderCallback_;
    EventFunc eventCallback_;
};

#define LOG_INFO(...) ::MapleEngine::Logger::info(__VA_ARGS__)
#define LOG_WARN(...) ::MapleEngine::Logger::warn(__VA_ARGS__)
#define LOG_ERROR(...) ::MapleEngine::Logger::error(__VA_ARGS__)

class Logger {
public:
    template<typename... Args>
    static void info(Args&&... args) {
        log("INFO", std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(Args&&... args) {
        log("WARN", std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(Args&&... args) {
        log("ERROR", std::forward<Args>(args)...);
    }
    
private:
    template<typename T>
    static void print(T&& value) {
        std::cout << value;
    }
    
    template<typename T, typename... Args>
    static void print(T&& first, Args&&... rest) {
        std::cout << first;
        print(std::forward<Args>(rest)...);
    }
    
    template<typename... Args>
    static void log(const char* level, Args&&... args) {
        std::cout << "[" << level << "] ";
        print(std::forward<Args>(args)...);
        std::cout << std::endl;
    }
};

} // namespace MapleEngine

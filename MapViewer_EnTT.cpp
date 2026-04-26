#define SDL_MAIN_USE_CALLBACKS 1  // 定义使用SDL3的Main Callbacks方式启动应用

// 标准C++库头文件
#include <iostream>         // 标准输入输出流(cout, cerr, cin)
#include <fstream>          // 文件输入输出流
#include <string>           // std::string字符串类
#include <memory>           // 智能指针(std::shared_ptr, std::unique_ptr)
#include <filesystem>       // C++17文件系统操作(std::filesystem::path)
#include <cstring>          // C字符串操作(memcpy, memset)
#include <cstdio>           // C标准输入输出(printf, sprintf)
#include <unistd.h>         // UNIX标准函数(access, getcwd)

// SDL3图形库头文件
#include <SDL3/SDL.h>          // SDL核心库 - 窗口、渲染器、事件
#include <SDL3/SDL_main.h>     // SDL_main入口点 - 用于Main Callbacks模式

// libpng库 - 用于PNG图像解码
#include <png.h>               // PNG格式读取库

// 标准库容器
#include <vector>              // 动态数组容器
#include <unordered_map>       // 哈希表容器
#include <chrono>              // 时间测量(std::chrono::high_resolution_clock)

// EnTT实体组件系统框架
#include <entt/entt.hpp>

// ImGui图形界面库
#include "imgui.h"                           // ImGui核心API
#include "backends/imgui_impl_sdl3.h"        // ImGui SDL3后端 - 处理SDL事件
#include "backends/imgui_impl_sdlrenderer3.h"// ImGui SDL Renderer3后端 - 渲染ImGui UI

// MapleStory  WZ资源的WzLibCpp解析库
#include "Wz_Structure.hpp"         // WZ结构管理器 - 管理多个WZ文件
#include "Wz_File.hpp"              // WZ文件类 - 单个WZ文件的读写
#include "Wz_Node.hpp"              // WZ节点类 - WZ文件中的树形节点
#include "Wz_Image.hpp"             // WZ图像类 - PNG/Lua等图像数据
#include "Wz_Png.hpp"               // WZ PNG类 - PNG格式压缩数据
#include "Wz_Header.hpp"            // WZ文件头 - 文件版本、偏移等信息
#include "MapData/MapDataFull.hpp"  // 地图完整数据结构
#include "PluginBase/PluginManager.hpp"  // 插件管理器 - 全局WZ节点查询
#include "WzResourceLoader.hpp"     // WZ资源加载器 - 统一的文件加载接口
#include "MapLoader.hpp"            // 地图加载器 - 地图加载功能
#include "MapData.hpp"              // MapData和MapRenderer类
#include "TextureCache.hpp"         // 纹理缓存管理器

// 使用WzLibCpp命名空间，简化类型名称
using namespace WzLibCpp;
using namespace MapleEngine;


/**
 * AppState - 应用状态类
 * 
 * 【设计目的】
 * 在SDL Main Callbacks模式下，应用状态需要在初始化函数和主循环函数之间共享。
 * 使用AppState类来保存所有需要在回调函数间传递的数据，并提供访问方法。
 * 优点：
 * 更好的封装性：相关操作集中在类内部，减少全局函数的数量。
 * 易于维护：新增功能只需修改类内部，外部调用接口稳定。
 * 更符合面向对象设计，利于团队协作和长期演进。
 * 
 * 【成员说明】
 * - window/renderer: SDL窗口和渲染器指针
 * - registry: EnTT实体注册表，存储所有游戏实体
 * - structure: WZ文件结构管理器
 * - mapNode: 当前加载的地图节点
 * - mapLoaded: 地图是否加载成功
 * - mapWidth/mapHeight: 地图尺寸
 * - cameraEntity: 摄像机实体ID
 */
class AppState {
public:
    // 获取单例实例
    static AppState& getInstance() {
        static AppState instance;
        return instance;
    }
    
    // 初始化方法
    bool initialize(int windowWidth = 1068, int windowHeight = 600);
    
    // 处理事件
    bool handleEvent(SDL_Event* event);
    
    // 更新状态
    void update();
    
    // 渲染
    void render();
    
    // 清理资源
    void cleanup();
    
    // 访问器
    SDL_Window* getWindow() const { return window; }
    SDL_Renderer* getRenderer() const { return renderer; }
    entt::registry& getRegistry() { return registry; }
    MapRenderer* getMapRenderer() { return mapRenderer; }
    
    // 成员变量（保持公共以兼容现有代码）
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    entt::registry registry;
    std::shared_ptr<Wz_Structure> structure;
    std::shared_ptr<Wz_Node> mapNode;
    bool mapLoaded = false;
    int mapWidth = 1068;
    int mapHeight = 600;
    entt::entity cameraEntity = entt::null;  // 初始化为null实体
    MapRenderer* mapRenderer = nullptr;
    
public:
    // 构造函数（公共以兼容现有代码）
    AppState() = default;
    ~AppState() = default;
    
    // 禁用拷贝和赋值
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;
    
    // 键盘状态变量（用于边沿检测）
    bool prevZoomIn = false, prevZoomOut = false;
    bool prev1 = false, prev2 = false, prev3 = false;
    bool prev4 = false, prev5 = false, prev6 = false;
    bool showDebugWindow = true;
    
    // 已加载的WZ文件列表
    std::vector<std::shared_ptr<Wz_File>> wzFiles;
    int loadedMapID = 100000000;
    
    // 选中节点
    std::shared_ptr<Wz_Node> selectedNode;
    std::string selectedNodeTitle = "";
    
    // 当前渲染的MapRenderer（使用unique_ptr管理生命周期，避免静态局部变量问题）
    std::unique_ptr<MapRenderer> ownedMapRenderer;
    
private:
    // 私有辅助方法
    bool initSDL(int width, int height);
    bool initImGui();
    void loadTextures(const std::string& wzPath);
    void logMapInfo(const MapData& mapData);
};


// =============================================================================
// 第九节: SDL初始化和渲染函数
// =============================================================================

/**
 * initializeSDL - 初始化SDL视频子系统
 * 
 * @param app AppState指针
 * @param w 窗口宽度
 * @param h 窗口高度
 * @return 成功返回true，失败返回false
 * 
 * 【初始化步骤】
 * 1. SDL_Init: 初始化SDL库和视频子系统
 * 2. SDL_CreateWindow: 创建窗口
 * 3. SDL_CreateRenderer: 创建渲染器
 * 4. 设置渲染器背景色
 */
static bool initializeSDL(AppState* app, int w, int h) {
    // 初始化SDL视频子系统
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    
    // 创建窗口
    app->window = SDL_CreateWindow("Map Viewer", w, h, 0);
    if (!app->window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    // 创建渲染器（使用默认驱动）
    app->renderer = SDL_CreateRenderer(app->window, NULL);
    if (!app->renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return false;
    }
    
    // 设置渲染器清除颜色（天蓝色）
    SDL_SetRenderDrawColor(app->renderer, 135, 206, 235, 255);
    return true;
}


// 第十一节: SDL Main Callbacks 入口点
// =============================================================================

// 前向声明SDL回调函数
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]);
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event);
SDL_AppResult SDL_AppIterate(void* appstate);
void SDL_AppQuit(void* appstate, SDL_AppResult result);

/**
 * SDL_AppInit - 应用初始化回调
 * 
 * @param appstate 输出参数，用于存储应用状态指针
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return SDL_APP_CONTINUE继续运行，SDL_APP_FAILURE失败退出
 * 
 * 【SDL Main Callbacks模式】
 * 与传统main()函数不同，SDL3提供了回调方式的入口点。
 * 这个函数在程序启动时调用，用于初始化。
 * 
 * 【初始化流程】
 * 1. 解析命令行参数
 * 2. 加载WZ资源文件
 * 3. 解析地图数据
 * 4. 初始化SDL窗口和渲染器
 * 5. 初始化ImGui
 * 6. 创建EnTT实体和组件
 * 7. 加载纹理资源
 */
SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char* /*argv*/[]) {
    // 获取应用状态单例
    AppState* app = &AppState::getInstance();
    *appstate = app;    // 返回给SDL

    std::cout << "[MapViewer_EnTT(SDL_AppInit)]: MapViewer_EnTT PNG纹理渲染 (SDL_MAIN_USE_CALLBACKS)" << std::endl;

    // 调用 AppState 的初始化方法
    if (!app->initialize(1068, 600)) {
        return SDL_APP_FAILURE;
    }

    std::cout << "按 WASD/方向键移动，ESC 退出，+/- 调整缩放" << std::endl;
    std::cout << "4=切换背景, 5=切换瓦片, 6=切换对象, 1/2/3=切换其他元素" << std::endl;
    std::cout << "============================================" << std::endl;

    return SDL_APP_CONTINUE;
}

/**
 * SDL_AppEvent - 事件处理回调
 * 
 * @param appstate 应用状态指针
 * @param event SDL事件
 * @return SDL_APP_SUCCESS退出，SDL_APP_CONTINUE继续
 * 
 * 【功能】
 * 处理SDL事件，如键盘、鼠标、窗口事件
 */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    AppState* app = static_cast<AppState*>(appstate);
    
    // 调用 AppState 的事件处理方法
    if (!app->handleEvent(event)) {
        return SDL_APP_SUCCESS;
    }
    
    return SDL_APP_CONTINUE;
}

/**
 * SDL_AppIterate - 主循环迭代回调
 * 
 * @param appstate 应用状态指针
 * @return SDL_APP_CONTINUE继续，SDL_APP_FAILURE失败
 * 
 * 【功能】
 * 每帧调用一次，更新游戏状态并渲染画面
 * 
 * 【更新内容】
 * 1. 读取键盘输入
 * 2. 更新摄像机位置
 * 3. 处理缩放
 * 4. 渲染游戏画面
 * 5. 渲染ImGui界面
 */
SDL_AppResult SDL_AppIterate(void* appstate) {
    AppState* app = static_cast<AppState*>(appstate);
    app->update();
    app->render();
    return SDL_APP_CONTINUE;
}

/**
 * SDL_AppQuit - 应用退出清理回调
 * 
 * @param appstate 应用状态指针
 * @param result 退出结果
 * 
 * 【清理内容】
 * 1. 清理纹理缓存
 * 2. 关闭ImGui
 * 3. 销毁SDL渲染器和窗口
 * 4. 退出SDL
 * 5. 释放应用状态内存
 */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    AppState* app = static_cast<AppState*>(appstate);
    app->cleanup();
}

// =============================================================================
// AppState 方法实现
// =============================================================================

bool AppState::initialize(int windowWidth, int windowHeight) {

    std::cout << "[MapViewer_EnTT(AppState::initialize)]: 初始化应用程序..." << std::endl;

    std::string wzPath = "/home/ltj/MapleStory/072/Data";  // WZ文件所在目录
    int mapID = 100000000;   // 地图ID

    // 使用WzResourceLoader加载所有WZ文件
    auto loadResult = WzResourceLoader::loadFromDirectory(wzPath, true);
    if (!loadResult.success) {
        std::cerr << "加载失败: " << loadResult.errorMessage << std::endl;
        return false;
    }
    structure = loadResult.structure;
    wzFiles = structure->getWzFiles();

    // 注册 WZ 结构管理器到全局插件系统
    PluginBase::PluginManager::RegisterStructures({structure});

    // 创建MapData实例
    MapData mapData;

    // 使用MapLoader加载地图
    MapLoader loader(structure);
    mapNode = loader.loadMap(mapID);
    loadedMapID = mapID;  // 同步到成员变量

    if (!mapNode) {
        std::cerr << "地图加载失败: " << loader.getLastError() << std::endl;
        return false;
    }

    // 加载地图数据到结构体
    mapLoaded = mapData.Load(mapNode);

    if (!mapLoaded) {
        std::cerr << "地图加载失败" << std::endl;
        return false;
    }

    // 输出地图信息
    logMapInfo(mapData);

    // 初始化SDL
    if (!initSDL(windowWidth, windowHeight)) {
        return false;
    }

    // 初始化ImGui
    if (!initImGui()) {
        return false;
    }

    // 创建摄像机实体
    const auto& vrect = mapData.getVRect();
    int camX = vrect.X;
    int camY = vrect.Y;
    int camW = vrect.Width > 0 ? vrect.Width : 1024;
    int camH = vrect.Height > 0 ? vrect.Height : 768;
    
    cameraEntity = registry.create();
    registry.emplace<CameraComp>(cameraEntity, camX, camY, camW, camH);

    // 加载纹理
    loadTextures(wzPath);

    std::cout << "[MapViewer_EnTT(AppState::initialize)]: 初始化完成" << std::endl;
    return true;
}

bool AppState::initSDL(int width, int height) {
    // 初始化SDL视频子系统
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    
    // 创建窗口
    window = SDL_CreateWindow("Map Viewer", width, height, 0);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    // 创建渲染器
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    
    // 设置渲染器清除颜色
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
    return true;
}

bool AppState::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 初始化ImGui的SDL3和SDLRenderer3后端
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    ImGui::StyleColorsDark();
    return true;
}

void AppState::loadTextures(const std::string& wzPath) {
    std::cout << "[MapViewer_EnTT(AppState::loadTextures)]: Loading textures..." << std::endl;
    ownedMapRenderer = std::make_unique<MapRenderer>(registry);
    mapRenderer = ownedMapRenderer.get();
    mapRenderer->setMapData(mapNode, wzPath);
    mapRenderer->loadTextures(renderer);
    
    // 设置TextureCache的ResourceLoader
    TextureCache::getInstance().setResourceLoader(mapRenderer->getResourceLoader());
    
    std::cout << "[MapViewer_EnTT(AppState::loadTextures)]: Textures loaded" << std::endl;
}

void AppState::logMapInfo(const MapData& mapData) {
    std::cout << "[MapViewer_EnTT(AppState::logMapInfo)]: ====" << std::endl;
    std::cout << "[MapInfo] ID=" << mapData.getID()
              << " Name='" << mapData.getName() << "'"
              << " BGM='" << mapData.getBgm() << "'"
              << std::endl;
    std::cout << "  脚踏板: " << mapData.getFootholds().size()
              << ", 传送门: " << mapData.getPortals().size()
              << ", 生命点: " << mapData.getLifes().size()
              << ", 背景: " << mapData.getBacks().size()
              << std::endl;
    std::cout << "=====================================" << std::endl;
}

bool AppState::handleEvent(SDL_Event* event) {
    // 将事件传递给ImGui处理
    ImGui_ImplSDL3_ProcessEvent(event);
    
    // 处理窗口关闭事件
    if (event->type == SDL_EVENT_QUIT) {
        return false; // 退出
    } 
    // 处理键盘按下事件
    else if (event->type == SDL_EVENT_KEY_DOWN) {
        // ESC键退出
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            return false;
        }
        // Tab键切换调试窗口
        else if (event->key.scancode == SDL_SCANCODE_TAB) {
            showDebugWindow = !showDebugWindow;
        }
    }
    return true; // 继续运行
}

void AppState::update() {
    // 空指针保护
    if (!mapRenderer || cameraEntity == entt::null) return;
    
    // 获取键盘状态
    const bool* state = SDL_GetKeyboardState(nullptr);
    
    // 获取摄像机组件引用
    CameraComp& camComp = registry.get<CameraComp>(cameraEntity);
    const int moveSpeed = 6;  // 摄像机移动速度（像素/帧）
    
    // 摄像机移动控制（WASD和方向键）
    if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP]) camComp.y -= moveSpeed;
    if (state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN]) camComp.y += moveSpeed;
    if (state[SDL_SCANCODE_A] || state[SDL_SCANCODE_LEFT]) camComp.x -= moveSpeed;
    if (state[SDL_SCANCODE_D] || state[SDL_SCANCODE_RIGHT]) camComp.x += moveSpeed;

    // 缩放控制（+/-键）
    // 放大（=或+键）
    if ((state[SDL_SCANCODE_EQUALS] || state[SDL_SCANCODE_KP_PLUS]) && !prevZoomIn) {
        float newZoom = (mapRenderer->getZoom() + 0.1f) > 4.0f ? 4.0f : (mapRenderer->getZoom() + 0.1f);
        mapRenderer->setZoom(newZoom);
        prevZoomIn = true;
        std::cout << "Zoom: " << newZoom << std::endl;
    } else if (!(state[SDL_SCANCODE_EQUALS] || state[SDL_SCANCODE_KP_PLUS])) {
        prevZoomIn = false;
    }
    
    // 缩小（-键）
    if (state[SDL_SCANCODE_MINUS] && !prevZoomOut) {
        float newZoom = (mapRenderer->getZoom() - 0.1f) < 0.25f ? 0.25f : (mapRenderer->getZoom() - 0.1f);
        mapRenderer->setZoom(newZoom);
        prevZoomOut = true;
        std::cout << "Zoom: " << newZoom << std::endl;
    } else if (!state[SDL_SCANCODE_MINUS]) {
        prevZoomOut = false;
    }

    // 显示开关（数字键1-6）
    auto mapData = mapRenderer->getMapData();
    if (state[SDL_SCANCODE_1] && !prev1) { mapData->showFoothold = !mapData->showFoothold; prev1 = true; } 
    else if (!state[SDL_SCANCODE_1]) { prev1 = false; }
    
    if (state[SDL_SCANCODE_2] && !prev2) { mapData->showPortal = !mapData->showPortal; prev2 = true; } 
    else if (!state[SDL_SCANCODE_2]) { prev2 = false; }
    
    if (state[SDL_SCANCODE_3] && !prev3) { mapData->showLife = !mapData->showLife; prev3 = true; } 
    else if (!state[SDL_SCANCODE_3]) { prev3 = false; }
    
    if (state[SDL_SCANCODE_4] && !prev4) { mapData->showBack = !mapData->showBack; prev4 = true; } 
    else if (!state[SDL_SCANCODE_4]) { prev4 = false; }
    
    if (state[SDL_SCANCODE_5] && !prev5) { mapData->showTile = !mapData->showTile; prev5 = true; } 
    else if (!state[SDL_SCANCODE_5]) { prev5 = false; }
    
    if (state[SDL_SCANCODE_6] && !prev6) { mapData->showObj = !mapData->showObj; prev6 = true; } 
    else if (!state[SDL_SCANCODE_6]) { prev6 = false; }
}

void AppState::render() {
    // 空指针保护
    if (!mapRenderer || cameraEntity == entt::null) return;
    
    // 渲染游戏画面
    CameraComp& camComp = registry.get<CameraComp>(cameraEntity);
    mapRenderer->render(renderer, camComp, mapWidth, mapHeight);

    // 渲染ImGui界面
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 创建调试窗口
    if (showDebugWindow) {
        ImGui::Begin("WZ Resource Browser", &showDebugWindow, ImGuiWindowFlags_MenuBar);

        // 菜单栏
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Resource Browser", "Tab", &showDebugWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // 三列布局
        ImGui::Columns(3, "trees", true);
        ImGui::SetColumnWidth(0, 250.0f);
        ImGui::SetColumnWidth(1, 250.0f);
        ImGui::SetColumnWidth(2, 250.0f);

        // 第一列：WZ文件列表
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("WZ Files")) {
            std::function<void(std::shared_ptr<Wz_Node>, int, std::string)> renderWzNode = 
            [&](std::shared_ptr<Wz_Node> node, int depth, std::string path) {
                if (!node || depth > 4) return;
                auto nodes = node->getNodes();
                if (nodes && nodes->getCount() > 0) {
                    int maxItems = (depth == 0) ? 20 : (depth == 1) ? 30 : (depth == 2) ? 20 : 10;
                    for (size_t i = 0; i < nodes->getCount() && i < maxItems; i++) {
                        auto child = (*nodes)[i];
                        if (child) {
                            std::string text = child->getText();
                            std::string childPath = path + "/" + text;
                            auto childNodes = child->getNodes();
                            if (childNodes && childNodes->getCount() > 0) {
                                if (ImGui::TreeNode(text.c_str())) {
                                    renderWzNode(child, depth + 1, childPath);
                                    ImGui::TreePop();
                                }
                            } else {
                                // img节点可选中
                                auto wzImg = child->getWzImage();
                                bool isSelected = (selectedNode == child);
                                if (ImGui::Selectable(text.c_str(), isSelected, ImGuiSelectableFlags_None)) {
                                    if (wzImg) {
                                        wzImg->tryExtract();
                                        auto extractedNode = wzImg->getNode();
                                        if (extractedNode) {
                                            selectedNode = extractedNode;
                                            selectedNodeTitle = childPath;
                                        } else {
                                            selectedNode = child;
                                            selectedNodeTitle = childPath;
                                        }
                                    } else {
                                        selectedNode = child;
                                        selectedNodeTitle = childPath;
                                    }
                                }
                            }
                        }
                    }
                }
            };
            
            if (!wzFiles.empty()) {
                int shown = 0;
                for (size_t i = 0; i < wzFiles.size() && shown < 20; i++) {
                    if (!wzFiles[i] || !wzFiles[i]->getHeader()) continue;
                    // 跳过作为子目录加载的WZ文件
                    if (wzFiles[i]->getIsSubDir()) continue;
                    
                    std::string fullPath = wzFiles[i]->getHeader()->getFileName();
                    std::string fileName = std::filesystem::path(fullPath).filename().string();
                    
                    auto wzNode = wzFiles[i]->getNode();
                    if (wzNode && wzNode->getNodes() && wzNode->getNodes()->getCount() > 0) {
                        if (ImGui::TreeNode(fileName.c_str())) {
                            renderWzNode(wzNode, 0, fileName);
                            ImGui::TreePop();
                        }
                    } else {
                        ImGui::Text("%s", fileName.c_str());
                    }
                    shown++;
                }
                ImGui::Text("... Total: %zu files (%d shown)", wzFiles.size(), shown);
            } else {
                ImGui::Text("No WZ files loaded");
            }
            ImGui::TreePop();
        }

        ImGui::NextColumn();

        // 第二列：选中节点的内容
        {
            char title[64];
            if (selectedNodeTitle.empty()) {
                snprintf(title, sizeof(title), "Map\\%d.img", loadedMapID);
            } else {
                snprintf(title, sizeof(title), "%s", selectedNodeTitle.c_str());
            }
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(title)) {
                auto displayNode = selectedNode ? selectedNode : mapNode;
                if (displayNode && displayNode->getNodes()) {
                    std::function<void(std::shared_ptr<Wz_Node>, int)> renderWzNode = 
                    [&](std::shared_ptr<Wz_Node> node, int depth) {
                        if (!node || depth > 4) return;
                        auto nodes = node->getNodes();
                        if (nodes && nodes->getCount() > 0) {
                            int maxItems = (depth == 0) ? 50 : (depth == 1) ? 30 : (depth == 2) ? 20 : 10;
                            for (size_t i = 0; i < nodes->getCount() && i < maxItems; i++) {
                                auto child = (*nodes)[i];
                                if (child) {
                                    std::string text = child->getText();
                                    auto childNodes = child->getNodes();
                                    if (childNodes && childNodes->getCount() > 0) {
                                        if (ImGui::TreeNode(text.c_str())) {
                                            renderWzNode(child, depth + 1);
                                            ImGui::TreePop();
                                        }
                                    } else {
                                        ImGui::Text("%s", text.c_str());
                                    }
                                }
                            }
                        }
                    };
                    renderWzNode(displayNode, 0);
                }
ImGui::TreePop();
            }
        }

        ImGui::NextColumn();

        // 第三列：渲染信息
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Render Info")) {
            ImGui::Text("Zoom: %.2f", mapRenderer->getZoom());
            CameraComp& camComp = registry.get<CameraComp>(cameraEntity);
            ImGui::Text("Camera: (%d, %d)", camComp.x, camComp.y);
            auto mapData = mapRenderer->getMapData();
            ImGui::Text("Show Foothold: %s", mapData->showFoothold ? "Yes" : "No");
            ImGui::Text("Show Portal: %s", mapData->showPortal ? "Yes" : "No");
            ImGui::Text("Show Life: %s", mapData->showLife ? "Yes" : "No");
            ImGui::Text("Show Back: %s", mapData->showBack ? "Yes" : "No");
            ImGui::Text("Show Tile: %s", mapData->showTile ? "Yes" : "No");
            ImGui::Text("Show Obj: %s", mapData->showObj ? "Yes" : "No");
            ImGui::Text("Back: %zu, Tile: %zu, Obj: %zu", 
                        mapData->backs.size(), mapData->tiles.size(), mapData->objs.size());
            ImGui::TreePop();
        }

        ImGui::Columns(1);
        ImGui::End();
    }

    // 执行ImGui渲染
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void AppState::cleanup() {
    // 清理纹理缓存（在clear之前记录数量）
    size_t textureCount = TextureCache::getInstance().size();
    TextureCache::getInstance().clear();
    std::cout << "[MapViewer_EnTT(AppState::cleanup)]: Texture cache cleared, released " << textureCount << " textures" << std::endl;

    // 关闭ImGui
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // 清理ownedMapRenderer（会先销毁其管理的MapRenderer）
    ownedMapRenderer.reset();
    mapRenderer = nullptr;
    
    // 销毁SDL资源
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

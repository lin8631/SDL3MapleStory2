#pragma once

#include <memory>
#include <string>
#include <functional>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

// Forward declarations in WzLibCpp namespace
namespace WzLibCpp {
class Wz_Node;
class Wz_Structure;
class Wz_File;
}

class WzBrowserWindow {
public:
    WzBrowserWindow();
    ~WzBrowserWindow();

    void render(
        std::shared_ptr<WzLibCpp::Wz_Structure> structure,
        std::shared_ptr<WzLibCpp::Wz_Node> mapNode,
        const std::vector<std::shared_ptr<WzLibCpp::Wz_File>>& wzFiles,
        float zoom,
        int cameraX,
        int cameraY,
        bool showFoothold,
        bool showPortal,
        bool showLife,
        bool showBack,
        bool showTile,
        bool showObj,
        size_t backCount,
        size_t tileCount,
        size_t objCount
    );

    void toggle() { visible = !visible; }
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

private:
    bool visible;

    std::shared_ptr<WzLibCpp::Wz_Node> selectedNode;
    std::string selectedNodeTitle;
};
#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

// Forward declarations in WzLibCpp namespace
namespace WzLibCpp {
class Wz_Node;
class Wz_Structure;
class Wz_File;
}

struct WzBrowserState {
    std::shared_ptr<WzLibCpp::Wz_Structure> structure;
    std::shared_ptr<WzLibCpp::Wz_Node> mapNode;
    std::vector<std::shared_ptr<WzLibCpp::Wz_File>> wzFiles;

    float zoom = 1.0f;
    int cameraX = 0;
    int cameraY = 0;

    bool showFoothold = true;
    bool showPortal = true;
    bool showLife = true;
    bool showBack = true;
    bool showTile = true;
    bool showObj = true;

    size_t backCount = 0;
    size_t tileCount = 0;
    size_t objCount = 0;
};

class WzBrowserWindow {
public:
    WzBrowserWindow();
    ~WzBrowserWindow();

    void render(const WzBrowserState& state);

    void toggle() { visible = !visible; }
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

private:
    bool visible;

    std::shared_ptr<WzLibCpp::Wz_Node> selectedNode;
    std::string selectedNodeTitle;
};
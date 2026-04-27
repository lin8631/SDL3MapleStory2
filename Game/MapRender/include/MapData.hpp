#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <entt/entt.hpp>
#include <SDL3/SDL.h>
#include "MapScene.hpp"
#include "Animation.hpp"
#include "Camera.hpp"
#include "Texture.hpp"
#include "Wz_Node.hpp"
#include "Wz_Image.hpp"
#include "Wz_Structure.hpp"
#include "Wz_Png.hpp"

using namespace WzLibCpp;

namespace MapleEngine {

class ResourceLoader;

struct MapRenderData {
    int id = 0;
    std::string name;
    std::string bgm;
    bool isTown = false;
    bool canFly = false;
    bool canSwim = false;
    int returnMap = 999999999;
    bool hideMinimap = false;
    int fieldLimit = 0;
    int link = -1;
    std::string mapMark;
    
    MiniMapData miniMap;
    std::vector<BackItem> backs;
    std::vector<ObjItem> objs;
    std::vector<TileItem> tiles;
    std::vector<FootholdItem> footholds;
    std::vector<LifeItem> lifes;
    std::vector<PortalItem> portals;
    std::vector<std::shared_ptr<SceneNode>> layers;
    ContainerNode back;
    SceneNode sceneLayers;
    ContainerNode portal;
    ContainerNode ladderRope;
    
    int mapLeft = 0, mapTop = 0;
    int mapRight = 0, mapBottom = 0;
    
    int vrLeft = 0, vrTop = 0, vrRight = 0, vrBottom = 0;
    
    std::shared_ptr<Wz_Node> mapImgNode;
    std::string wzPath;
    
    bool load(std::shared_ptr<Wz_Node> mapImgNode);
    void preloadResource(ResourceLoader& loader);
    void calcMapSize();
    
    void loadTextures(SDL_Renderer* renderer, std::shared_ptr<Wz_Structure> structure);
    void unloadTextures();
    
    bool showBack = true;
    bool showTile = true;
    bool showObj = true;
    bool showFoothold = true;
    bool showPortal = true;
    bool showLife = true;
};

class ResourceLoader {
public:
    ResourceLoader() = default;
    
    void setRenderer(SDL_Renderer* renderer);
    SDL_Renderer* getRenderer() const { return renderer_; }
    int loadTexture(const std::string& path);
    std::shared_ptr<Texture> getTexture(int id);
    
    std::shared_ptr<AnimationData> loadAnimationData(std::shared_ptr<Wz_Node> node);
    std::shared_ptr<AnimationData> getAnimationData(const std::string& assetName);
    
    SDL_Texture* loadTextureFromWzPng(SDL_Renderer* renderer, std::shared_ptr<Wz_Png> wzPng);
    SDL_Texture* loadTextureFromPngData(SDL_Renderer* renderer, const std::vector<uint8_t>& pngData);
    SDL_Texture* loadImageTexture(SDL_Renderer* renderer, std::shared_ptr<Wz_Image> img);
    std::shared_ptr<Wz_Image> findAndExtractImage(std::shared_ptr<Wz_Node> imgNode);
    std::shared_ptr<Wz_Node> findObjNode(const std::string& basePath, const std::string& objOS, 
                                         const std::string& l0, const std::string& l1, const std::string& l2);
    std::shared_ptr<Wz_Node> findChildByName(std::shared_ptr<Wz_Node> parent, const std::string& name);
    
    void clear();
    
private:
    int loadPngFromMemory(const std::vector<uint8_t>& data, int width, int height);
    
    SDL_Renderer* renderer_ = nullptr;
    std::unordered_map<int, std::shared_ptr<Texture>> textures_;
    std::unordered_map<std::string, int> pathCache_;
    std::unordered_map<std::string, std::shared_ptr<AnimationData>> animationCache_;
    int nextTextureId_ = 1;
};

class MapRenderer {
public:
    MapRenderer(entt::registry& reg);
    
    bool loadMap(int mapId, const std::string& wzPath);
    void setMapData(std::shared_ptr<Wz_Node> mapNode, const std::string& wzPath);
    void loadTextures(SDL_Renderer* renderer);
    void unloadTextures();
    
    void render(SDL_Renderer* renderer, const CameraComp& cam, int screenW, int screenH);
    
    MapRenderData* getMapData() { return mapData_.get(); }
    ResourceLoader* getResourceLoader() { return resourceLoader_.get(); }
    
    void setZoom(float zoom) { zoom_ = zoom; }
    float getZoom() const { return zoom_; }
    
private:
    void loadBackTexture(SDL_Renderer* renderer, BackItem& back);
    void loadTileTexture(SDL_Renderer* renderer, TileItem& tile);
    void loadObjTexture(SDL_Renderer* renderer, ObjItem& obj);
    
    void renderBacks(SDL_Renderer* renderer, const CameraComp& cam, bool isFront);
    void renderTiles(SDL_Renderer* renderer, const CameraComp& cam, int layer);
    void renderObjs(SDL_Renderer* renderer, const CameraComp& cam, int layer);
    void renderFootholds(SDL_Renderer* renderer, const CameraComp& cam);
    void renderPortals(SDL_Renderer* renderer, const CameraComp& cam);
    void renderLifes(SDL_Renderer* renderer, const CameraComp& cam);
    
    std::unique_ptr<MapRenderData> mapData_;
    std::unique_ptr<ResourceLoader> resourceLoader_;
    entt::registry& registry_;
    Uint32 startTime_ = 0;
    float zoom_ = 1.0f;
};

} // namespace MapleEngine

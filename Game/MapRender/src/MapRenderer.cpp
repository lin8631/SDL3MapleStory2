#include "MapData.hpp"
#include "MapScene.hpp"
#include "Animation.hpp"
#include "Texture.hpp"
#include "Wz_Structure.hpp"
#include "Wz_Png.hpp"
#include "PluginBase/PluginManager.hpp"
#include "MapData/MapDataFull.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

using namespace WzLibCpp;

namespace MapleEngine {

static std::shared_ptr<Wz_Png> findWzPngInNode(std::shared_ptr<Wz_Node> node, int depth = 0) {
    if (!node || depth > 5) return nullptr;
    
    auto png = node->getValue<Wz_Png>();
    if (png) return png;
    
    auto img = node->getValue<Wz_Image>();
    if (img) {
        auto wzFile = std::dynamic_pointer_cast<Wz_File>(img->getWzFile());
        if (wzFile && img->getOffset() == 0) {
            img->setOffset(wzFile->calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset()));
        }
        if (img->tryExtract()) {
            auto extracted = img->getNode();
            if (extracted) {
                return findWzPngInNode(extracted, depth + 1);
            }
        }
        return nullptr;
    }
    
    if (node->getNodes()) {
        for (size_t i = 0; i < node->getNodes()->getCount(); i++) {
            auto child = (*node->getNodes())[i];
            if (child) {
                auto result = findWzPngInNode(child, depth + 1);
                if (result) return result;
            }
        }
    }
    
    return nullptr;
}

MapRenderer::MapRenderer(entt::registry& reg)
    : mapData_(std::make_unique<MapRenderData>())
    , resourceLoader_(std::make_unique<ResourceLoader>())
    , registry_(reg) {
    startTime_ = SDL_GetTicks();
}

bool MapRenderer::loadMap(int mapId, const std::string& wzPath) {
    auto structure = std::make_shared<Wz_Structure>();
    structure->setAutoDetectExtFiles(true);
    
    auto baseWzPath = wzPath + "/Base.wz";
    auto rootNode = std::make_shared<Wz_Node>("Base.wz");
    
    auto file = structure->loadFile(baseWzPath, rootNode, true, false);
    if (!file) {
        SDL_LogWarn(0, "[MapRenderer] Failed to load Base.wz");
        return false;
    }
    
    PluginBase::PluginManager::RegisterStructures({structure});
    
    auto mapNode = structure->getWzNode()->getNodes()->find("Map");
    if (mapNode == structure->getWzNode()->getNodes()->end()) {
        std::cerr << "Cannot find Map node" << std::endl;
        return false;
    }
    
    // 地图路径：Map/Map{folderNum}/{mapId:D9}.img（无subFolder层）
    int folderNum = mapId / 100000000;
    std::string folderName = "Map" + std::to_string(folderNum);
    auto mapFolder = (*mapNode)->getNodes()->find(folderName);
    if (mapFolder == (*mapNode)->getNodes()->end()) {
        std::cerr << "Cannot find Map folder: " << folderName << std::endl;
        return false;
    }
    
    // 使用9位数字格式（补零）
    char imgName[32];
    snprintf(imgName, sizeof(imgName), "%09d.img", mapId);
    auto imgNode = (*mapFolder)->getNodes()->find(imgName);
    if (imgNode == (*mapFolder)->getNodes()->end()) {
        std::cerr << "Cannot find map image: " << imgName << std::endl;
        return false;
    }
    
    // 正确获取Wz_Image：使用getWzImage()方法而不是dynamic_pointer_cast
    std::shared_ptr<Wz_Image> wzImg = (*imgNode)->getWzImage();
    if (!wzImg) {
        std::cerr << "Failed to get Wz_Image" << std::endl;
        return false;
    }
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(wzImg->getWzFile());
    if (wzFile && wzImg->getOffset() == 0) {
        wzImg->setOffset(wzFile->calcOffset(wzImg->getHashedOffsetPosition(), wzImg->getHashedOffset()));
    }
    
    if (!wzImg->tryExtract()) {
        std::cerr << "Failed to extract map image" << std::endl;
        return false;
    }
    
    auto extractedNode = wzImg->getNode();
    if (!extractedNode) {
        std::cerr << "Failed to get extracted node" << std::endl;
        return false;
    }
    
    if (!mapData_->load(extractedNode)) {
        std::cerr << "Failed to load map data" << std::endl;
        return false;
    }
    
    mapData_->wzPath = wzPath;
    startTime_ = SDL_GetTicks();
    return true;
}

void MapRenderer::setMapData(std::shared_ptr<Wz_Node> mapNode, const std::string& wzPath) {
    unloadTextures();
    mapData_ = std::make_unique<MapRenderData>();
    if (!mapData_->load(mapNode)) {
        SDL_LogWarn(0, "[MapRenderer] Failed to load map data");
        return;
    }
    mapData_->wzPath = wzPath;
    startTime_ = SDL_GetTicks();
}

void MapRenderer::loadTextures(SDL_Renderer* renderer) {
    resourceLoader_->setRenderer(renderer);
    
    int loadedBacks = 0, loadedTiles = 0, loadedObjs = 0;
    std::vector<std::string> failedObjs;
    
    for (auto& back : mapData_->backs) {
        int before = static_cast<int>(registry_.view<BackComp>().size());
        loadBackTexture(renderer, back);
        if (static_cast<int>(registry_.view<BackComp>().size()) > before) {
            loadedBacks++;
        }
    }
    
    for (auto& tile : mapData_->tiles) {
        int before = static_cast<int>(registry_.view<TileComp>().size());
        loadTileTexture(renderer, tile);
        if (static_cast<int>(registry_.view<TileComp>().size()) > before) {
            loadedTiles++;
        }
    }
    
    for (auto& obj : mapData_->objs) {
        int before = static_cast<int>(registry_.view<ObjComp>().size());
        loadObjTexture(renderer, obj);
        if (static_cast<int>(registry_.view<ObjComp>().size()) > before) {
            loadedObjs++;
        } else {
            if (!obj.oS.empty()) {
                failedObjs.push_back(obj.oS + "/" + obj.l0 + "/" + obj.l1 + "/" + obj.l2);
            }
        }
    }
    
    std::cerr << "=== 未加载的对象纹理 (" << failedObjs.size() << "个) ===" << std::endl;
    for (size_t i = 0; i < failedObjs.size(); i++) {
        std::cerr << "  [" << i << "] " << failedObjs[i] << std::endl;
    }
    
    for (auto& fh : mapData_->footholds) {
        auto e = registry_.create();
        registry_.emplace<FootholdComp>(e, fh.x1, fh.y1, fh.x2, fh.y2);
    }
    
    for (auto& portal : mapData_->portals) {
        auto e = registry_.create();
        registry_.emplace<PortalComp>(e, portal.x, portal.y);
    }
    
    for (auto& life : mapData_->lifes) {
        auto e = registry_.create();
        registry_.emplace<LifeComp>(e, life.x, life.y);
    }
    
    SDL_Log("Texture loaded - Back:%d/%zu, Tile:%d/%zu, Obj:%d/%zu", 
            loadedBacks, mapData_->backs.size(),
            loadedTiles, mapData_->tiles.size(),
            loadedObjs, mapData_->objs.size());
}

void MapRenderer::unloadTextures() {
    // 销毁纹理的实体也被销毁，防止重复加载时实体累积
    
    // BackComp
    for (auto e : registry_.view<BackComp>()) {
        auto& back = registry_.get<BackComp>(e);
        if (back.texture) {
            SDL_DestroyTexture(back.texture);
            back.texture = nullptr;
        }
    }
    registry_.clear<BackComp>();
    
    // TileComp
    for (auto e : registry_.view<TileComp>()) {
        auto& tile = registry_.get<TileComp>(e);
        if (tile.texture) {
            SDL_DestroyTexture(tile.texture);
            tile.texture = nullptr;
        }
    }
    registry_.clear<TileComp>();
    
    // ObjComp
    for (auto e : registry_.view<ObjComp>()) {
        auto& obj = registry_.get<ObjComp>(e);
        if (obj.texture) {
            SDL_DestroyTexture(obj.texture);
            obj.texture = nullptr;
        }
    }
    registry_.clear<ObjComp>();
    
    registry_.clear<FootholdComp>();
    registry_.clear<PortalComp>();
    registry_.clear<LifeComp>();
}

void MapRenderer::loadBackTexture(SDL_Renderer* renderer, BackItem& back) {
    if (back.bS.empty()) return;
    
    auto backFound = PluginBase::PluginManager::FindWz("Map/Back/" + back.bS + ".img");
    if (!backFound) {
        backFound = PluginBase::PluginManager::FindWz("Map\\Back\\" + back.bS + ".img");
    }
    if (!backFound) return;
    
    auto frameImg = backFound->getValue<Wz_Image>();
    if (!frameImg) return;
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(frameImg->getWzFile());
    if (wzFile && frameImg->getOffset() == 0) {
        frameImg->setOffset(wzFile->calcOffset(frameImg->getHashedOffsetPosition(), frameImg->getHashedOffset()));
    }
    
    if (!frameImg->tryExtract()) return;
    
    auto extracted = frameImg->getNode();
    if (!extracted || !extracted->getNodes()) return;
    
    // C# 中 BackItem.Ani 只有 0（使用 back 目录）和 1（使用 ani 目录）
    // spine 通过单独的 SpineAni 字段处理，不是 ani 字段
    std::string aniDir;
    switch (back.ani) {
        case 0: aniDir = "back"; break;
        case 1: aniDir = "ani"; break;
        default: aniDir = "back"; break;
    }
    
    auto frameNode = extracted->getNodes()->operator[](aniDir);
    if (!frameNode || !frameNode->getNodes()) {
        // 如果指定类型不存在，尝试ani
        frameNode = extracted->getNodes()->operator[]("ani");
        if (!frameNode || !frameNode->getNodes()) {
            frameNode = extracted;
        }
    }
    
    std::string frameNo = back.noStr.empty() ? std::to_string(back.animFrame) : back.noStr;
    auto targetNode = frameNode->getNodes()->operator[](frameNo);
    if (!targetNode) {
        targetNode = frameNode->getNodes()->operator[]("0");
    }
    if (!targetNode) return;
    
    auto wzPng = targetNode->getValue<Wz_Png>();
    if (!wzPng) {
        if (targetNode->getNodes() && targetNode->getNodes()->getCount() > 0) {
            auto pngChild = (*targetNode->getNodes())[0];
            wzPng = pngChild->getValue<Wz_Png>();
        }
    }
    if (!wzPng) return;
    
    // 读取origin偏移
    auto originNode = targetNode->getNodes()->find("origin");
    if (originNode != targetNode->getNodes()->end()) {
        auto originVec = (*originNode)->getValue<Wz_Vector>();
        if (originVec) {
            back.originX = originVec->getX();
            back.originY = originVec->getY();
        }
    }
    
    SDL_Texture* tex = resourceLoader_->loadTextureFromWzPng(renderer, wzPng);
    if (!tex) return;
    
    float w, h;
    SDL_GetTextureSize(tex, &w, &h);
    
    auto e = registry_.create();
    registry_.emplace<BackComp>(e, tex, static_cast<int>(w), static_cast<int>(h),
                             back.x, back.y, back.cx, back.cy, 
                             back.rx, back.ry, back.type, back.flipX, back.front,
                             back.alpha, back.originX, back.originY);
}

void MapRenderer::loadTileTexture(SDL_Renderer* renderer, TileItem& tile) {
    if (tile.tilesetName.empty() || tile.tileNo.empty()) return;
    if (!mapData_->mapImgNode) return;
    
    std::string tilesetPathPrefix = "Map/Tile/" + tile.tilesetName + ".img";
    
    auto tilesetFound = PluginBase::PluginManager::FindWz(tilesetPathPrefix);
    if (!tilesetFound) return;
    
    auto tilesetImg = tilesetFound->getValue<Wz_Image>();
    if (!tilesetImg) return;
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(tilesetImg->getWzFile());
    if (wzFile && tilesetImg->getOffset() == 0) {
        tilesetImg->setOffset(wzFile->calcOffset(tilesetImg->getHashedOffsetPosition(), tilesetImg->getHashedOffset()));
    }
    
    if (!tilesetImg->tryExtract()) return;
    
    auto tilesetRoot = tilesetImg->getNode();
    if (!tilesetRoot || !tilesetRoot->getNodes()) return;
    
// 正确路径：Map\Tile\{TS}.img\{U}\{No}
// 先U（类型分类字符串），再No（编号）
// tile.uStr 是字符串类型（如"bsc", "edD", "enH"）
std::string uKey = tile.uStr.empty() ? std::to_string(tile.u) : tile.uStr;
auto uFolderNode = tilesetRoot->getNodes()->operator[](uKey);
if (!uFolderNode || !uFolderNode->getNodes()) return;

auto tileEntryNode = uFolderNode->getNodes()->operator[](tile.tileNo);
    if (!tileEntryNode) {
        tileEntryNode = uFolderNode->getNodes()->operator[]("0");
    }
    if (!tileEntryNode) return;
    
    auto wzPng = tileEntryNode->getValue<Wz_Png>();
    if (!wzPng) {
        auto tileImg = tileEntryNode->getValue<Wz_Image>();
        if (tileImg) {
            auto tileWzFile = std::dynamic_pointer_cast<Wz_File>(tileImg->getWzFile());
            if (tileWzFile && tileImg->getOffset() == 0) {
                tileImg->setOffset(tileWzFile->calcOffset(tileImg->getHashedOffsetPosition(), tileImg->getHashedOffset()));
            }
            if (tileImg->tryExtract()) {
                auto extracted = tileImg->getNode();
                if (extracted && extracted->getNodes() && extracted->getNodes()->getCount() > 0) {
                    auto first = (*extracted->getNodes())[0];
                    if (first) {
                        wzPng = first->getValue<Wz_Png>();
                    }
                }
            }
        }
    }
    
    if (!wzPng) return;
    
    SDL_Texture* tex = resourceLoader_->loadTextureFromWzPng(renderer, wzPng);
    if (!tex) return;
    
    float w, h;
    SDL_GetTextureSize(tex, &w, &h);
    
    auto e = registry_.create();
    registry_.emplace<TileComp>(e, tex, static_cast<int>(w), static_cast<int>(h),
                             tile.x, tile.y, tile.layer, tile.z,
                             tile.originX, tile.originY);
}

void MapRenderer::loadObjTexture(SDL_Renderer* renderer, ObjItem& obj) {
    std::string pathKey = obj.oS + "/" + obj.l0 + "/" + obj.l1 + "/" + obj.l2;
    
    if (!mapData_->mapImgNode) return;
    if (obj.oS.empty()) return;
    
    auto objImgNode = PluginBase::PluginManager::FindWz("Map/Obj/" + obj.oS + ".img");
    if (!objImgNode) {
        objImgNode = PluginBase::PluginManager::FindWz("Map\\Obj\\" + obj.oS + ".img");
    }
    if (!objImgNode) {
        SDL_Log("[MapRenderer(loadObjTexture)] FAIL: %s - img not found", pathKey.c_str());
        return;
    }
    
    auto wzImg = objImgNode->getValue<Wz_Image>();
    if (!wzImg) return;
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(wzImg->getWzFile());
    if (wzFile && wzImg->getOffset() == 0) {
        wzImg->setOffset(wzFile->calcOffset(wzImg->getHashedOffsetPosition(), wzImg->getHashedOffset()));
    }
    
    if (!wzImg->tryExtract()) {
        SDL_Log("[MapRenderer(loadObjTexture)] FAIL: %s - tryExtract failed", pathKey.c_str());
        return;
    }
    
    auto extracted = wzImg->getNode();
    if (!extracted || !extracted->getNodes()) {
        SDL_Log("[MapRenderer(loadObjTexture)] FAIL: %s - no nodes in extracted", pathKey.c_str());
        return;
    }
    
    auto current = extracted;
    
    std::vector<std::string> pathParts = {obj.l0, obj.l1, obj.l2};
    for (const auto& part : pathParts) {
        // 修复：不再跳过"0"节点，"0"是完全合法的wz节点名称
        if (part.empty()) continue;
        if (!current->getNodes()) {
            SDL_Log("[MapRenderer(loadObjTexture)] FAIL: %s - no children at '%s'", pathKey.c_str(), part.c_str());
            return;
        }
        
        auto child = current->getNodes()->operator[](part);
        if (!child) {
            std::cout << "FAIL: " << pathKey << " - child '" << part << "' not found" << std::endl;
            std::cout.flush();
            return;
        }
        
        auto childImg = child->getValue<Wz_Image>();
        if (childImg) {
            auto childFile = std::dynamic_pointer_cast<Wz_File>(childImg->getWzFile());
            if (childFile) {
                if (childImg->getOffset() == 0) {
                    childImg->setOffset(childFile->calcOffset(childImg->getHashedOffsetPosition(), childImg->getHashedOffset()));
                }
            }
            if (!childImg->tryExtract()) {
                std::cout << "FAIL: " << pathKey << " - child img tryExtract failed at '" << part << "'" << std::endl;
                return;
            }
            
            auto childExtracted = childImg->getNode();
            if (!childExtracted || !childExtracted->getNodes()) {
                std::cout << "FAIL: " << pathKey << " - no nodes in child at '" << part << "'" << std::endl;
                return;
            }
            current = childExtracted;
        } else {
            current = child;
        }
    }
    
    auto wzPng = current->getValue<Wz_Png>();
    if (!wzPng) {
        wzPng = findWzPngInNode(current);
    }
    
    if (!wzPng) {
        std::cout << "FAIL: " << pathKey << " - no Wz_Png found after deep search" << std::endl;
        return;
    }
    
    SDL_Texture* tex = resourceLoader_->loadTextureFromWzPng(renderer, wzPng);
    if (!tex) return;
    
    float w, h;
    SDL_GetTextureSize(tex, &w, &h);
    
    auto e = registry_.create();
    registry_.emplace<ObjComp>(e, tex, static_cast<int>(w), static_cast<int>(h),
                            obj.x, obj.y, obj.layer, obj.z,
                            obj.originX, obj.originY);
}

void MapRenderer::render(SDL_Renderer* renderer, const CameraComp& cam, int screenW, int screenH) {
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
    SDL_RenderClear(renderer);

    if (mapData_->showBack) renderBacks(renderer, cam, false);
    for (int layer = 0; layer <= 7; layer++) {
        if (mapData_->showTile) renderTiles(renderer, cam, layer);
        if (mapData_->showObj) renderObjs(renderer, cam, layer);
    }
    if (mapData_->showBack) renderBacks(renderer, cam, true);
    if (mapData_->showFoothold) renderFootholds(renderer, cam);
    if (mapData_->showPortal) renderPortals(renderer, cam);
    if (mapData_->showLife) renderLifes(renderer, cam);
}

void MapRenderer::renderBacks(SDL_Renderer* renderer, const CameraComp& cam, bool isFront) {
    Uint32 elapsed = SDL_GetTicks() - startTime_;
    int camCenterX = cam.x + cam.w / 2;
    int camCenterY = cam.y + cam.h / 2;

    auto view = registry_.view<BackComp>();
    for (auto e : view) {
        auto& back = registry_.get<BackComp>(e);
        if (!back.texture) continue;
        if (back.front != isFront) continue;
        
        int cx = back.cx > 0 ? back.cx : back.texW;
        int cy = back.cy > 0 ? back.cy : back.texH;
        
        float posX = static_cast<float>(back.x);
        float posY = static_cast<float>(back.y);
        
        int mode = back.type;
        bool horizontal = (mode == 1 || mode == 3 || mode == 4 || mode == 6);
        bool vertical = (mode == 2 || mode == 3 || mode == 5 || mode == 7);
        bool scrollH = (mode == 4 || mode == 6);
        bool scrollV = (mode == 5 || mode == 7);
        
        if (scrollH) {
            // C#: position.X += ((float)back.Rx * 5 * back.View.Time / 1000) % cx;
            posX += static_cast<float>(back.rx) * 5.0f * elapsed / 1000.0f;
            if (back.cx > 0) {
                posX = std::fmod(posX, static_cast<float>(back.cx));
            }
        } else {
            // 修复：正确公式是 (100 + rx) / 100，当rx=0时背景跟随相机
            // C#: position.X += Camera.Center.X * (100 + back.Rx) / 100;
            posX = static_cast<float>(back.x) + static_cast<float>(camCenterX) * (100 + back.rx) / 100.0f;
        }
        
        if (scrollV) {
            posY += static_cast<float>(back.ry) * 5.0f * elapsed / 1000.0f;
            if (back.cy > 0) {
                posY = std::fmod(posY, static_cast<float>(back.cy));
            }
        } else {
            // 修复：正确公式是 (100 + ry) / 100
            posY = static_cast<float>(back.y) + static_cast<float>(camCenterY) * (100 + back.ry) / 100.0f;
        }
        
        posX = std::floor(posX);
        posY = std::floor(posY);
        
        if (horizontal || vertical) {
            int l = 0, t = 0, r = 1, b = 1;
            
            if (horizontal && cx > 0) {
                int camLeft = cam.x;
                int camRight = cam.x + cam.w;
                l = static_cast<int>(std::floor(static_cast<float>(camLeft - posX) / cx)) - 1;
                r = static_cast<int>(std::ceil(static_cast<float>(camRight - posX) / cx)) + 1;
            }
            
            if (vertical && cy > 0) {
                int camTop = cam.y;
                int camBottom = cam.y + cam.h;
                t = static_cast<int>(std::floor(static_cast<float>(camTop - posY) / cy)) - 1;
                b = static_cast<int>(std::ceil(static_cast<float>(camBottom - posY) / cy)) + 1;
            }
            
            for (int ty = t; ty < b; ty++) {
                for (int tx = l; tx < r; tx++) {
                    float drawX = posX + cx * tx - static_cast<float>(back.originX);
                    float drawY = posY + cy * ty - static_cast<float>(back.originY);
                    
                    int sx = static_cast<int>((drawX - cam.x) * zoom_);
                    int sy = static_cast<int>((drawY - cam.y) * zoom_);
                    int sw = static_cast<int>(back.texW * zoom_);
                    int sh = static_cast<int>(back.texH * zoom_);
                    
                    if (sx + sw < 0 || sy + sh < 0 || sx > cam.w || sy > cam.h) continue;
                    
                    SDL_FRect dst{ static_cast<float>(sx), static_cast<float>(sy), 
                                   static_cast<float>(sw), static_cast<float>(sh) };
                    SDL_RenderTexture(renderer, back.texture, nullptr, &dst);
                }
            }
        } else {
            int sx = static_cast<int>((posX - static_cast<float>(back.originX) - cam.x) * zoom_);
            int sy = static_cast<int>((posY - static_cast<float>(back.originY) - cam.y) * zoom_);
            int sw = static_cast<int>(back.texW * zoom_);
            int sh = static_cast<int>(back.texH * zoom_);
            
            if (sx + sw < 0 || sy + sh < 0 || sx > cam.w || sy > cam.h) continue;
            
            SDL_FRect dst{ static_cast<float>(sx), static_cast<float>(sy), 
                           static_cast<float>(sw), static_cast<float>(sh) };
            SDL_RenderTexture(renderer, back.texture, nullptr, &dst);
        }
    }
}

void MapRenderer::renderTiles(SDL_Renderer* renderer, const CameraComp& cam, int layer) {
    auto view = registry_.view<TileComp>();
    for (auto e : view) {
        auto& tile = registry_.get<TileComp>(e);
        if (!tile.texture) continue;
        if (tile.layer != layer) continue;
        
        int sx = static_cast<int>((tile.x - tile.originX - cam.x) * zoom_);
        int sy = static_cast<int>((tile.y - tile.originY - cam.y) * zoom_);
        int sw = static_cast<int>(tile.texW * zoom_);
        int sh = static_cast<int>(tile.texH * zoom_);
        
        if (sx + sw < 0 || sy + sh < 0 || sx > cam.w || sy > cam.h) continue;
        
        SDL_FRect dst{ static_cast<float>(sx), static_cast<float>(sy), 
                       static_cast<float>(sw), static_cast<float>(sh) };
        SDL_RenderTexture(renderer, tile.texture, nullptr, &dst);
    }
}

void MapRenderer::renderObjs(SDL_Renderer* renderer, const CameraComp& cam, int layer) {
    struct SortedObj {
        entt::entity entity;
        int z;
    };
    
    std::vector<SortedObj> sortedObjs;
    auto view = registry_.view<ObjComp>();
    for (auto e : view) {
        auto& obj = registry_.get<ObjComp>(e);
        if (!obj.texture) continue;
        if (obj.layer != layer) continue;
        sortedObjs.push_back({e, obj.z});
    }
    
    std::stable_sort(sortedObjs.begin(), sortedObjs.end(), [](const SortedObj& a, const SortedObj& b) {
        return a.z < b.z;
    });
    
    for (const auto& sorted : sortedObjs) {
        auto e = sorted.entity;
        auto& obj = registry_.get<ObjComp>(e);
        
        int sx = static_cast<int>((obj.x - obj.originX - cam.x) * zoom_);
        int sy = static_cast<int>((obj.y - obj.originY - cam.y) * zoom_);
        int sw = static_cast<int>(obj.texW * zoom_);
        int sh = static_cast<int>(obj.texH * zoom_);
        
        if (sx + sw < 0 || sy + sh < 0 || sx > cam.w || sy > cam.h) continue;
        
        SDL_FRect dst{ static_cast<float>(sx), static_cast<float>(sy), 
                       static_cast<float>(sw), static_cast<float>(sh) };
        SDL_RenderTexture(renderer, obj.texture, nullptr, &dst);
    }
}

void MapRenderer::renderFootholds(SDL_Renderer* renderer, const CameraComp& cam) {
    SDL_SetRenderDrawColor(renderer, 200, 180, 140, 255);
    auto view = registry_.view<FootholdComp>();
    for (auto e : view) {
        auto& fh = registry_.get<FootholdComp>(e);
        int sx1 = static_cast<int>((fh.x1 - cam.x) * zoom_);
        int sy1 = static_cast<int>((fh.y1 - cam.y) * zoom_);
        int sx2 = static_cast<int>((fh.x2 - cam.x) * zoom_);
        int sy2 = static_cast<int>((fh.y2 - cam.y) * zoom_);
        SDL_RenderLine(renderer, sx1, sy1, sx2, sy2);
    }
}

void MapRenderer::renderPortals(SDL_Renderer* renderer, const CameraComp& cam) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    auto view = registry_.view<PortalComp>();
    for (auto e : view) {
        auto& p = registry_.get<PortalComp>(e);
        int sx = static_cast<int>((p.x - cam.x) * zoom_);
        int sy = static_cast<int>((p.y - cam.y) * zoom_);
        SDL_FRect rect{ static_cast<float>(sx - 10), static_cast<float>(sy - 10), 20, 20 };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void MapRenderer::renderLifes(SDL_Renderer* renderer, const CameraComp& cam) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    auto view = registry_.view<LifeComp>();
    for (auto e : view) {
        auto& l = registry_.get<LifeComp>(e);
        int sx = static_cast<int>((l.x - cam.x) * zoom_);
        int sy = static_cast<int>((l.y - cam.y) * zoom_);
        SDL_FRect rect{ static_cast<float>(sx - 8), static_cast<float>(sy - 8), 16, 16 };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void ContainerNode::update(float deltaTime) {
    for (auto& slot : slots_) {
        slot->update(deltaTime);
    }
}

void ContainerNode::render(SDL_Renderer* renderer, int cameraX, int cameraY) {
    for (auto& slot : slots_) {
        slot->render(renderer, cameraX, cameraY);
    }
}

} // namespace MapleEngine

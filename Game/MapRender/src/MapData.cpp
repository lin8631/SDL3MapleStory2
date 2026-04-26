#include "MapData.hpp"
#include "MapScene.hpp"
#include "Animation.hpp"
#include <regex>

using namespace WzLibCpp;

namespace MapleEngine {

inline int GetInt(std::shared_ptr<Wz_Node> node, int defaultValue = 0) {
    if (!node) return defaultValue;
    return node->getInt(defaultValue);
}

inline std::string GetString(std::shared_ptr<Wz_Node> node, const std::string& defaultValue = "") {
    if (!node) return defaultValue;
    return node->getString(defaultValue);
}

bool MapRenderData::load(std::shared_ptr<Wz_Node> imgNode) {
    if (!imgNode) {
        return false;
    }

    this->mapImgNode = imgNode;
    name = imgNode->getText();
    std::regex pattern(R"((\d{9})\.img)");
    std::smatch match;
    if (std::regex_search(name, match, pattern)) {
        id = std::stoi(match[1]);
    }

    auto nodes = imgNode->getNodes();
    if (!nodes) {
        return false;
    }

    auto infoNode = nodes->operator[]("info");
    if (infoNode) {
        auto infoNodes = infoNode->getNodes();
        if (infoNodes) {
            bgm = GetString(infoNodes->operator[]("bgm"), "");
            mapMark = GetString(infoNodes->operator[]("mapMark"), "");
            isTown = GetInt(infoNodes->operator[]("town"), 0) != 0;
            canFly = GetInt(infoNodes->operator[]("fly"), 0) != 0;
            canSwim = GetInt(infoNodes->operator[]("swim"), 0) != 0;
            returnMap = GetInt(infoNodes->operator[]("returnMap"), 999999999);
            hideMinimap = GetInt(infoNodes->operator[]("hideMinimap"), 0) != 0;
            fieldLimit = GetInt(infoNodes->operator[]("fieldLimit"), 0);
            link = GetInt(infoNodes->operator[]("link"), -1);
        }
    }

    auto miniMapNode = nodes->operator[]("miniMap");
    if (miniMapNode) {
        auto miniNodes = miniMapNode->getNodes();
        if (miniNodes) {
            miniMap.width = GetInt(miniNodes->operator[]("width"), 0);
            miniMap.height = GetInt(miniNodes->operator[]("height"), 0);
            miniMap.centerX = GetInt(miniNodes->operator[]("centerX"), 0);
            miniMap.centerY = GetInt(miniNodes->operator[]("centerY"), 0);
            miniMap.mag = GetInt(miniNodes->operator[]("mag"), 0);
        }
    }

    auto backNode = nodes->operator[]("back");
    if (backNode) {
        auto backNodes = backNode->getNodes();
        if (backNodes) {
            for (size_t i = 0; i < backNodes->getCount(); i++) {
                auto child = (*backNodes)[i];
                if (!child) continue;
                
                BackItem item;
                item.name = child->getText();
                item.index = static_cast<int>(i);
                
                auto childNodes = child->getNodes();
                if (childNodes) {
                    item.bS = GetString(childNodes->operator[]("bS"), "");
                    item.animFrame = GetInt(childNodes->operator[]("aS"), 0);
                    item.x = GetInt(childNodes->operator[]("x"), 0);
                    item.y = GetInt(childNodes->operator[]("y"), 0);
                    item.cx = GetInt(childNodes->operator[]("cx"), 0);
                    item.cy = GetInt(childNodes->operator[]("cy"), 0);
                    item.rx = GetInt(childNodes->operator[]("rx"), 0);
                    item.ry = GetInt(childNodes->operator[]("ry"), 0);
                    item.type = GetInt(childNodes->operator[]("type"), 0);
                    item.tileMode = GetInt(childNodes->operator[]("tileMode"), 0);
                    item.flipX = GetInt(childNodes->operator[]("f"), 0) != 0;
                    item.front = GetInt(childNodes->operator[]("front"), 0) != 0;
                    item.alpha = GetInt(childNodes->operator[]("alpha"), 255);
                    item.screenMode = GetInt(childNodes->operator[]("screenMode"), 0);
                }
                backs.push_back(item);
            }
        }
    }

    for (int i = 0; i <= 7; i++) {
        auto layerNode = nodes->operator[](std::to_string(i));
        if (!layerNode) continue;
        
        auto layerNodes = layerNode->getNodes();
        if (!layerNodes) continue;
        
        auto infoNode = layerNodes->operator[]("info");
        std::string tilesetName;
        if (infoNode && infoNode->getNodes()) {
            tilesetName = GetString(infoNode->getNodes()->operator[]("tS"), "");
        }
        
        auto objNode = layerNodes->operator[]("obj");
        if (objNode) {
            auto objNodes = objNode->getNodes();
            if (objNodes) {
                for (size_t j = 0; j < objNodes->getCount(); j++) {
                    auto child = (*objNodes)[j];
                    if (!child) continue;
                    
                    ObjItem item;
                    item.name = child->getText();
                    item.layer = i;
                    
                    auto childNodes = child->getNodes();
                    if (childNodes) {
                        item.x = GetInt(childNodes->operator[]("x"), 0);
                        item.y = GetInt(childNodes->operator[]("y"), 0);
                        item.z = GetInt(childNodes->operator[]("z"), 0);
                        item.oS = GetString(childNodes->operator[]("oS"), "");
                        item.l0 = GetString(childNodes->operator[]("l0"), "");
                        item.l1 = GetString(childNodes->operator[]("l1"), "");
                        item.l2 = GetString(childNodes->operator[]("l2"), "");
                        item.flipX = GetInt(childNodes->operator[]("f"), 0) != 0;
                        item.light = GetInt(childNodes->operator[]("light"), 0) != 0;
                    }
                    objs.push_back(item);
                }
            }
        }
        
        auto tileNode = layerNodes->operator[]("tile");
        if (tileNode) {
            auto tileNodes = tileNode->getNodes();
            if (tileNodes) {
                for (size_t j = 0; j < tileNodes->getCount(); j++) {
                    auto child = (*tileNodes)[j];
                    if (!child) continue;
                    
                    TileItem item;
                    item.name = child->getText();
                    item.layer = i;
                    
                    auto childNodes = child->getNodes();
                    if (childNodes) {
                        item.x = GetInt(childNodes->operator[]("x"), 0);
                        item.y = GetInt(childNodes->operator[]("y"), 0);
                        item.z = GetInt(childNodes->operator[]("z"), 0);
                        item.u = GetInt(childNodes->operator[]("no"), 0);
                        item.v = 0;
                        item.tilesetName = tilesetName;
                        
                        auto uNode = childNodes->operator[]("u");
                        if (uNode) {
                            item.tileNo = uNode->getString("");
                            if (item.tileNo.empty()) {
                                item.tileNo = uNode->getText();
                            }
                        }
                    }
                    tiles.push_back(item);
                }
            }
        }
    }

    auto footholdNode = nodes->operator[]("foothold");
    if (footholdNode) {
        auto fhNodes = footholdNode->getNodes();
        if (fhNodes) {
            for (size_t i = 0; i <= 7; i++) {
                auto levelNode = fhNodes->operator[](std::to_string(i));
                if (!levelNode) continue;
                
                auto levelNodes = levelNode->getNodes();
                if (!levelNodes) continue;
                
                for (size_t j = 0; j < levelNodes->getCount(); j++) {
                    auto child = (*levelNodes)[j];
                    if (!child) continue;
                    
                    FootholdItem item;
                    item.id = std::stoi(child->getText());
                    
                    auto childNodes = child->getNodes();
                    if (childNodes) {
                        item.x1 = GetInt(childNodes->operator[]("x1"), 0);
                        item.y1 = GetInt(childNodes->operator[]("y1"), 0);
                        item.x2 = GetInt(childNodes->operator[]("x2"), 0);
                        item.y2 = GetInt(childNodes->operator[]("y2"), 0);
                        item.next = GetInt(childNodes->operator[]("next"), 0);
                        item.prev = GetInt(childNodes->operator[]("prev"), 0);
                        item.force = GetInt(childNodes->operator[]("force"), 0);
                        item.type = GetInt(childNodes->operator[]("type"), 0);
                    }
                    footholds.push_back(item);
                }
            }
        }
    }

    auto lifeNode = nodes->operator[]("life");
    if (lifeNode) {
        auto lifeNodes = lifeNode->getNodes();
        if (lifeNodes) {
            for (size_t i = 0; i < lifeNodes->getCount(); i++) {
                auto child = (*lifeNodes)[i];
                if (!child) continue;
                
                LifeItem item;
                item.id = std::stoi(child->getText());
                
                auto childNodes = child->getNodes();
                if (childNodes) {
                    std::string typeStr = GetString(childNodes->operator[]("type"), "n");
                    item.type = (typeStr == "m") ? LifeItem::LifeType::Mob : LifeItem::LifeType::Npc;
                    item.x = GetInt(childNodes->operator[]("x"), 0);
                    item.y = GetInt(childNodes->operator[]("y"), 0);
                    item.foothold = GetInt(childNodes->operator[]("fh"), 0);
                    item.flipX = GetInt(childNodes->operator[]("f"), 0) != 0;
                    item.hide = GetInt(childNodes->operator[]("hide"), 0) != 0;
                    item.mobTime = GetInt(childNodes->operator[]("mobTime"), 0);
                    item.respawnTime = item.mobTime;
                    item.cy = GetInt(childNodes->operator[]("cy"), 0);
                    item.rx0 = GetInt(childNodes->operator[]("rx0"), 0);
                    item.rx1 = GetInt(childNodes->operator[]("rx1"), 0);
                }
                lifes.push_back(item);
            }
        }
    }

    auto portalNode = nodes->operator[]("portal");
    if (portalNode) {
        auto portalNodes = portalNode->getNodes();
        if (portalNodes) {
            for (size_t i = 0; i < portalNodes->getCount(); i++) {
                auto child = (*portalNodes)[i];
                if (!child) continue;
                
                PortalItem item;
                item.id = std::stoi(child->getText());
                
                auto childNodes = child->getNodes();
                if (childNodes) {
                    item.name = GetString(childNodes->operator[]("pn"), "");
                    item.type = GetInt(childNodes->operator[]("pt"), 0);
                    item.x = GetInt(childNodes->operator[]("x"), 0);
                    item.y = GetInt(childNodes->operator[]("y"), 0);
                    item.targetMap = GetInt(childNodes->operator[]("tm"), -1);
                    item.targetName = GetString(childNodes->operator[]("tn"), "");
                    item.script = GetString(childNodes->operator[]("script"), "");
                }
                portals.push_back(item);
            }
        }
    }

    calcMapSize();
    return true;
}

void MapRenderData::calcMapSize() {
    int left = 0, right = 0, top = 0, bottom = 0;
    bool initialized = false;
    
    for (const auto& fh : footholds) {
        if (!initialized) {
            left = std::min(fh.x1, fh.x2);
            right = std::max(fh.x1, fh.x2);
            top = std::min(fh.y1, fh.y2);
            bottom = std::max(fh.y1, fh.y2);
            initialized = true;
        } else {
            left = std::min({left, fh.x1, fh.x2});
            right = std::max({right, fh.x1, fh.x2});
            top = std::min({top, fh.y1, fh.y2});
            bottom = std::max({bottom, fh.y1, fh.y2});
        }
    }
    
    left -= 200;
    right += 200;
    top -= 300;
    bottom += 100;
    
    mapLeft = left;
    mapTop = top;
    mapRight = right;
    mapBottom = bottom;
}

void MapRenderData::preloadResource(ResourceLoader& loader) {
    for (auto& back : backs) {
    }
    
    for (auto& obj : objs) {
    }
    
    for (auto& tile : tiles) {
    }
    
    for (auto& life : lifes) {
    }
    
    for (auto& portal : portals) {
    }
}

} // namespace MapleEngine

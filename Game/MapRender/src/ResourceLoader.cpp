#include "MapData.hpp"
#include "MapScene.hpp"
#include "Animation.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include "Wz_Png.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"
#include "PluginBase/PluginManager.hpp"
#include <png.h>

using namespace WzLibCpp;

namespace MapleEngine {

int ResourceLoader::loadTexture(const std::string& path) {
    auto it = pathCache_.find(path);
    if (it != pathCache_.end()) {
        return it->second;
    }
    
    size_t dataSize;
    void* data = SDL_LoadFile(path.c_str(), &dataSize);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return -1;
    }
    
    SDL_Surface* surface = SDL_CreateSurface(0, 0, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_free(data);
        std::cerr << "Failed to create surface" << std::endl;
        return -1;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);
    SDL_free(data);
    
    if (!texture) {
        std::cerr << "Failed to create texture: " << path << std::endl;
        return -1;
    }
    
    int id = nextTextureId_++;
    float w, h;
    SDL_GetTextureSize(texture, &w, &h);
    
    textures_[id] = std::make_shared<Texture>(texture, static_cast<int>(w), static_cast<int>(h));
    pathCache_[path] = id;
    
    return id;
}

std::shared_ptr<Texture> ResourceLoader::getTexture(int id) {
    auto it = textures_.find(id);
    if (it != textures_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<AnimationData> ResourceLoader::loadAnimationData(std::shared_ptr<Wz_Node> node) {
    if (!node) return nullptr;
    
    std::string path = node->getFullPath();
    auto cached = getAnimationData(path);
    if (cached) return cached;
    
    auto animData = std::make_shared<AnimationData>();
    
    auto png = node->getValue<Wz_Png>();
    if (png) {
        Frame frame;
        auto pngData = png->extractPng();
        if (!pngData.empty()) {
            frame.textureId = loadPngFromMemory(pngData, png->getWidth(), png->getHeight());
        }
        frame.width = png->getWidth();
        frame.height = png->getHeight();
        animData->frames.push_back(frame);
        animationCache_[path] = animData;
        return animData;
    }
    
    auto nodes = node->getNodes();
    if (nodes) {
        for (size_t i = 0; i < nodes->getCount(); i++) {
            auto child = (*nodes)[i];
            if (!child) continue;
            
            auto childPng = child->getValue<Wz_Png>();
            if (childPng) {
                Frame frame;
                auto pngData = childPng->extractPng();
                if (!pngData.empty()) {
                    frame.textureId = loadPngFromMemory(pngData, childPng->getWidth(), childPng->getHeight());
                }
                frame.width = childPng->getWidth();
                frame.height = childPng->getHeight();
                frame.delay = 100;
                animData->frames.push_back(frame);
            }
        }
        
        if (!animData->frames.empty()) {
            animData->repeat = node->getBool(true);
            animationCache_[path] = animData;
            return animData;
        }
    }
    
    return nullptr;
}

std::shared_ptr<AnimationData> ResourceLoader::getAnimationData(const std::string& assetName) {
    auto it = animationCache_.find(assetName);
    if (it != animationCache_.end()) {
        return it->second;
    }
    return nullptr;
}

int ResourceLoader::loadPngFromMemory(const std::vector<uint8_t>& data, int width, int height) {
    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
    if (!io) return -1;
    
    SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_CloseIO(io);
        return -1;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);
    SDL_CloseIO(io);
    
    if (!texture) return -1;
    
    int id = nextTextureId_++;
    textures_[id] = std::make_shared<Texture>(texture, width, height);
    return id;
}

void ResourceLoader::clear() {
    textures_.clear();
    pathCache_.clear();
    animationCache_.clear();
}

void ResourceLoader::setRenderer(SDL_Renderer* renderer) {
    renderer_ = renderer;
}

SDL_Texture* ResourceLoader::loadTextureFromWzPng(SDL_Renderer* renderer, std::shared_ptr<Wz_Png> wzPng) {
    if (!wzPng) {
        return nullptr;
    }
    
    auto rawData = wzPng->getRawData();
    if (rawData.empty()) {
        return nullptr;
    }
    
    int width = wzPng->getWidth();
    int height = wzPng->getHeight();
    int format = static_cast<int>(wzPng->getFormat());
    
    SDL_Texture* texture = nullptr;
    
    if (format == 1) {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB4444, SDL_TEXTUREACCESS_STATIC, width, height);
        if (texture) {
            SDL_UpdateTexture(texture, nullptr, rawData.data(), width * 2);
        }
    } 
    else if (format == 2) {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
        if (texture) {
            SDL_UpdateTexture(texture, nullptr, rawData.data(), width * 4);
        }
    }
    
    if (texture) {
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    }
    
    return texture;
}

// =============================================================================
// PNG内存读取支持
// =============================================================================

struct PngReadState {
    const uint8_t* data;
    size_t size;
    size_t pos;
};

static void png_read_fn(png_structp png, png_bytep data, png_size_t length) {
    PngReadState* state = (PngReadState*)png_get_io_ptr(png);
    
    if (state->pos + length > state->size) {
        png_error(png, "Read past end of PNG data");
        return;
    }
    
    std::memcpy(data, state->data + state->pos, length);
    state->pos += length;
}

// =============================================================================
// ResourceLoader 新增函数实现
// =============================================================================

SDL_Texture* ResourceLoader::loadTextureFromPngData(SDL_Renderer* renderer, const std::vector<uint8_t>& pngData) {
    if (pngData.empty()) {
        return nullptr;
    }
    
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        return nullptr;
    }
    
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        return nullptr;
    }
    
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        return nullptr;
    }
    
    PngReadState state = { pngData.data(), pngData.size(), 0 };
    png_set_read_fn(png, &state, png_read_fn);
    
    png_read_info(png, info);
    
    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth = png_get_bit_depth(png, info);
    
    png_set_expand(png);
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB || 
        color_type == PNG_COLOR_TYPE_GRAY || 
        color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || 
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    
    png_read_update_info(png, info);
    
    std::vector<uint8_t> row_buf(width * 4);
    SDL_Surface* surf = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (!surf) {
        png_destroy_read_struct(&png, &info, nullptr);
        return nullptr;
    }
    
    for (int y = 0; y < height; y++) {
        png_read_row(png, row_buf.data(), nullptr);
        uint8_t* dst = (uint8_t*)surf->pixels + y * surf->pitch;
        std::memcpy(dst, row_buf.data(), width * 4);
    }
    
    png_destroy_read_struct(&png, &info, nullptr);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    
    return tex;
}

SDL_Texture* ResourceLoader::loadImageTexture(SDL_Renderer* renderer, std::shared_ptr<Wz_Image> img) {
    if (!img) return nullptr;
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(img->getWzFile());
    if (wzFile && img->getOffset() == 0) {
        img->setOffset(wzFile->calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset()));
    }
    
    if (!img->tryExtract()) return nullptr;
    
    auto pngData = img->extractPng();
    return loadTextureFromPngData(renderer, pngData);
}

std::shared_ptr<Wz_Image> ResourceLoader::findAndExtractImage(std::shared_ptr<Wz_Node> imgNode) {
    if (!imgNode) return nullptr;
    
    auto img = imgNode->getValue<Wz_Image>();
    if (!img) return nullptr;
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(img->getWzFile());
    if (wzFile) {
        int64_t currentOffset = img->getOffset();
        if (currentOffset == 0 || currentOffset > 100000000000LL) {
            img->setOffset(wzFile->calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset()));
        }
    }
    
    if (!img->tryExtract()) return nullptr;
    return img;
}

std::shared_ptr<Wz_Node> ResourceLoader::findObjNode(const std::string& basePath, const std::string& objOS, 
                                         const std::string& l0, const std::string& l1, const std::string& l2) {
    std::string objImgPath = basePath + "/" + objOS + ".img";
    auto objImgNode = PluginBase::PluginManager::FindWz(objImgPath);
    if (!objImgNode) {
        return nullptr;
    }
    
    auto wzImg = objImgNode->getValue<Wz_Image>();
    if (!wzImg) {
        return nullptr;
    }
    
    auto wzFile = std::dynamic_pointer_cast<Wz_File>(wzImg->getWzFile());
    if (wzFile) {
        if (wzImg->getOffset() == 0) {
            wzImg->setOffset(wzFile->calcOffset(wzImg->getHashedOffsetPosition(), wzImg->getHashedOffset()));
        }
    }
    
    if (!wzImg->tryExtract()) {
        return nullptr;
    }
    
    auto extracted = wzImg->getNode();
    if (!extracted || !extracted->getNodes()) {
        return nullptr;
    }
    
    auto current = extracted;
    std::vector<std::string> pathParts = {l0, l1, l2};
    for (const auto& part : pathParts) {
        if (part.empty()) continue;
        if (!current->getNodes()) return nullptr;
        
        auto child = current->getNodes()->operator[](part);
        if (!child) return nullptr;
        
        auto childImg = child->getValue<Wz_Image>();
        if (childImg) {
            auto childFile = std::dynamic_pointer_cast<Wz_File>(childImg->getWzFile());
            if (childFile) {
                if (childImg->getOffset() == 0) {
                    childImg->setOffset(childFile->calcOffset(childImg->getHashedOffsetPosition(), childImg->getHashedOffset()));
                }
            }
            if (!childImg->tryExtract()) return nullptr;
            
            auto childExtracted = childImg->getNode();
            if (!childExtracted || !childExtracted->getNodes()) return nullptr;
            current = childExtracted;
        } else {
            current = child;
        }
    }
    
    return current;
}

std::shared_ptr<Wz_Node> ResourceLoader::findChildByName(std::shared_ptr<Wz_Node> parent, const std::string& name) {
    if (!parent || !parent->getNodes()) return nullptr;
    auto child = parent->getNodes()->operator[](name);
    return child;
}

} // namespace MapleEngine

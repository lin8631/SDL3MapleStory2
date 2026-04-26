#include "TextureCache.hpp"
#include "MapData.hpp" // 包含ResourceLoader定义
#include <png.h>
#include <cstring>
#include <iostream>

namespace MapleEngine {

// =============================================================================
// PNG内存读取支持（从MapViewer_EnTT.cpp复制）
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
// TextureCache 实现
// =============================================================================

TextureCache::TextureCache() = default;

TextureCache::~TextureCache() {
    clear();
}

TextureCache& TextureCache::getInstance() {
    static TextureCache instance;
    return instance;
}

void TextureCache::setResourceLoader(ResourceLoader* loader) {
    resourceLoader_ = loader;
}

SDL_Texture* TextureCache::getTexture(const std::string& key, SDL_Renderer* renderer, const std::vector<uint8_t>& pngData) {
    // 查找缓存
    auto it = cache.find(key);
    if (it != cache.end()) {
        // 找到，增加引用计数
        it->second.refCount++;
        return it->second.texture;
    }

    // 未找到，加载纹理
    SDL_Texture* tex = nullptr;
    if (resourceLoader_) {
        tex = resourceLoader_->loadTextureFromPngData(renderer, pngData);
    } else {
        // 回退到原来的函数
        tex = loadTextureFromPngData(renderer, pngData);
    }
    if (tex) {
        // 添加到缓存
        CacheEntry entry;
        entry.texture = tex;
        entry.refCount = 1;  // 初始引用计数为1
        cache[key] = entry;
    }
    return tex;
}

void TextureCache::releaseTexture(const std::string& key) {
    auto it = cache.find(key);
    if (it != cache.end()) {
        it->second.refCount--;
        // 引用计数为0，释放纹理
        if (it->second.refCount <= 0) {
            if (it->second.texture) {
                SDL_DestroyTexture(it->second.texture);
            }
            cache.erase(it);
        }
    }
}

bool TextureCache::hasTexture(const std::string& key) const {
    return cache.find(key) != cache.end();
}

void TextureCache::clear() {
    for (auto& pair : cache) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
    cache.clear();
}

size_t TextureCache::size() const {
    return cache.size();
}

SDL_Texture* TextureCache::loadTextureFromPngData(SDL_Renderer* renderer, const std::vector<uint8_t>& pngData) {
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

} // namespace MapleEngine
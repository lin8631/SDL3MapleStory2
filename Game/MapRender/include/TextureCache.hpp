#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <SDL3/SDL.h>

namespace MapleEngine {

class ResourceLoader;

/**
 * TextureCache - 纹理缓存单例类
 * 
 * 【设计目的】
 * 避免重复加载相同的纹理，提高性能和资源利用率
 * 
 * 【单例模式】
 * 使用静态局部变量实现线程安全的单例
 * 
 * 【缓存策略】
 * - key-value存储：key是纹理的唯一标识符
 * - 引用计数：跟踪纹理被多少实体使用
 * - 自动释放：当引用计数为0时释放纹理
 */
class TextureCache {
public:
    /**
     * getInstance - 获取TextureCache单例实例
     * 
     * 【实现方式】
     * C++11静态局部变量，在首次调用时构造，之后返回同一实例
     * 线程安全（标准保证）
     */
    static TextureCache& getInstance();

    // 设置ResourceLoader指针，用于纹理加载
    void setResourceLoader(ResourceLoader* loader);

    /**
     * getTexture - 获取或创建纹理
     * 
     * @param key 纹理唯一标识符（通常是路径）
     * @param renderer SDL渲染器
     * @param pngData PNG格式的二进制数据
     * @return SDL纹理指针
     * 
     * 【工作流程】
     * 1. 检查缓存中是否已有该纹理
     * 2. 如果有，增加引用计数并返回
     * 3. 如果没有，加载纹理并添加到缓存
     */
    SDL_Texture* getTexture(const std::string& key, SDL_Renderer* renderer, const std::vector<uint8_t>& pngData);

    /**
     * releaseTexture - 释放纹理引用
     * 
     * @param key 纹理标识符
     * 
     * 【工作流程】
     * 1. 查找纹理
     * 2. 减少引用计数
     * 3. 如果引用计数为0，销毁纹理并从缓存移除
     */
    void releaseTexture(const std::string& key);

    // 检查纹理是否存在
    bool hasTexture(const std::string& key) const;

    // 清空所有缓存纹理
    void clear();

    // 获取缓存中的纹理数量
    size_t size() const;

private:
    /**
     * CacheEntry - 缓存条目结构
     * 
     * 【成员】
     * - texture: SDL纹理指针
     * - refCount: 引用计数
     */
    struct CacheEntry {
        SDL_Texture* texture = nullptr;  // 纹理指针
        int refCount = 0;                 // 引用计数
    };

    // 私有构造函数，实现单例
    TextureCache();
    
    // 析构函数，清理所有纹理
    ~TextureCache();

    // 禁用拷贝构造函数
    TextureCache(const TextureCache&) = delete;
    
    // 禁用赋值运算符
    TextureCache& operator=(const TextureCache&) = delete;

    // 从PNG数据加载纹理（内部方法）
    SDL_Texture* loadTextureFromPngData(SDL_Renderer* renderer, const std::vector<uint8_t>& pngData);

    // 缓存存储：key -> CacheEntry
    std::unordered_map<std::string, CacheEntry> cache;
    
    // ResourceLoader指针，用于纹理加载
    ResourceLoader* resourceLoader_ = nullptr;
};

} // namespace MapleEngine
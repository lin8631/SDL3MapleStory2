# AGENTS.md - MapleStory WZ Texture Rendering Project

## 语言设置
- **思考过程**：在处理任何问题时，全程思考（包括需求分析、逻辑拆解、方案选择、步骤推导等所有内部推理环节）必须使用中文。

- **最终输出**：所有回答内容（包括文字解释、代码注释、步骤说明等）必须全部使用中文，仅代码语法本身的英文关键词除外。

## 技术栈
## - **图形**: SDL3 Renderer
## - **界面**: ImGui
## - **音频**: FFmpeg + SDL3 Audio
 - **资源格式**: WZ文件格式（冒险岛资源格式）

## 代码规范
## - 使用 `SDL_Log` 输出日志信息
## - 错误信息使用 `SDL_LogWarn` 或 `SDL_LogError`
 - 组件使用 EnTT ECS 框架管理

## 角色定位
你是一名资深软件开发专家，精通多种编程语言和技术栈，擅长代码生成、调试、重构、技术方案设计以及技术问答。你的目标是提供准确、高效、易于理解的解决方案。

## 处理不确定信息
- 如果问题描述不清晰、缺少必要信息或存在歧义，请主动指出并请求用户补充说明。
- 当存在多种可行方案时，简要对比优缺点，并给出推荐建议。


## 跨平台
本项目最终实现Windows、macOS 和 Linux 的跨平台构建运行



---

## Goal
实现 MapleStory WZ 文件的 PNG 纹理渲染功能。

## Instructions
- 构建 SDL3_image 静态库并链接到 MapViewer_EnTT
- 从 WZ 文件中提取 PNG 纹理并使用 SDL3_image 渲染

## Discoveries
1. **SDL3_image 版本冲突**: SDL3_image 3.4.0 需要 SDL3 3.4.0，但项目使用 SDL3 3.4.2
2. **PNG 库问题**: SDL3_image CMake 检查 PNG 为共享库，但系统 PNG 是静态库
3. **SDL3 API 变更**: 
   - `SDL_RWFromConstMem` → `SDL_IOFromConstMem` 
   - `SDL_FreeSurface` → `SDL_DestroySurface`
   - `IMG_Load_RW` → `IMG_Load_IO`
   - `SDL_GetKeyboardState` 返回 `const bool*` 而不是 `const Uint8*`
4. **WZ 瓦片结构**: 瓦片节点包含 Wz_Image 子节点用于 x, y, u, no, zM 字段
5. **Tileset 引用**: tileset 名称在 "no" 字段中
6. **背景纹理结构**: 背景纹理在 `ani` 节点下，帧节点包含子节点，第一个子节点是 Wz_Png
7. **对象纹理路径**: ObjItem 的 oS, l0, l1, l2 是字符串类型，对应 `Map/Obj/{oS}.img/l0/l1/l2`

## Accomplished
1. ✅ 构建 SDL3 3.4.2 静态库到 `Engine/SDL3_install/`
2. ✅ 创建 SDL3_image 自定义 CMakeLists.txt 并构建 `libSDL3_image.a`
3. ✅ 修复主 CMakeLists.txt 链接 `SDL3::SDL3-static`
4. ✅ 修复所有 SDL3 API 兼容性问题
5. ✅ 实现纹理加载函数: `loadBackTextures`, `loadTileTextures`, `loadObjTextures`
6. ✅ 修复瓦片字段名从 "t" 到 "no"
7. ✅ 实现 MapRenderer 类到 MapRenderer.cpp
8. ✅ 实现 ResourceLoader::loadTextureFromWzPng 函数
9. ✅ 实现 MapRenderer::loadTileTexture 和 MapRenderer::loadObjTexture
10. ✅ 修复背景纹理加载：在 `ani` 节点中查找帧，帧子节点才是 Wz_Png
11. ✅ 修复对象纹理加载：ObjItem 的 oS, l0, l1, l2 改为 string 类型
12. ✅ 清理调试输出代码

## 当前状态
**纹理加载和渲染功能已完成！**

程序成功运行并加载：
- 背景纹理: 8/8 ✅
- 瓦片纹理: 815/815 ✅
- 对象纹理: 200/250 ✅

渲染顺序正确:
1. Back (front=false)
2. Layers 0-7: Tile + Obj
3. Back (front=true)
4. Foothold, Portal, Life

## Relevant files / directories
- `/home/ltj/MapleStory/MapleStoryW/examples/MapViewer_EnTT.cpp` - 主程序文件
- `/home/ltj/MapleStory/MapleStoryW/Game/MapRender/src/MapRenderer.cpp` - MapRenderer 实现
- `/home/ltj/MapleStory/MapleStoryW/Game/MapRender/src/ResourceLoader.cpp` - ResourceLoader 实现
- `/home/ltj/MapleStory/MapleStoryW/Game/MapRender/src/MapData.cpp` - MapRenderData 加载实现
- `/home/ltj/MapleStory/MapleStoryW/Engine/Scene/include/MapScene.hpp` - ObjItem/BackItem 结构定义
- `/home/ltj/MapleStory/MapleStoryW/build/MapViewer_EnTT` - 编译后的可执行文件

## Next steps
1. ✅ 纹理加载和渲染功能已完成
2. ✅ 验证渲染效果
3. ✅ 清理代码中的调试输出
4. 🔲 优化：剩余 50 个对象纹理未加载（可能是路径不存在或PNG提取失败）


## 调试输出
在调试输出时要求开头保持以下格式：[所属类名(所属方法名)]：
cmake_minimum_required(VERSION 3.18)
project(WzLibCpp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成 compile_commands.json 用于 IDE 跳转
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 兼容旧版 WZLIB_USE_INTERNAL_AES 变量（已弃用）
option(WZLIB_USE_INTERNAL_AES "Internal AES encryption (legacy, unused)" OFF)

# Dependencies
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# EnTT - 优先使用源码构建
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EnTT/CMakeLists.txt")
    add_subdirectory(Engine/EnTT EnTT)
else()
    # 使用单头文件版本
    # EnTT 是 header-only 库，不需要额外配置
endif()

# SDL3 - 优先使用源码构建
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Engine/SDL/CMakeLists.txt")
    # 从源码构建
    set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    add_subdirectory(Engine/SDL SDL3)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Engine/SDL-release-3.4.2/CMakeLists.txt")
    # 从源码构建
    set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    add_subdirectory(Engine/SDL-release-3.4.2 SDL3)
else()
    # 使用预编译的 SDL3
    if(NOT TARGET SDL3::SDL3)
        add_library(SDL3::SDL3 STATIC IMPORTED)
        set_target_properties(SDL3::SDL3 PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Engine/SDL3_install/lib/libSDL3.a
            INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Engine/SDL3_install/include
        )
    endif()
endif()

# SDL3_image 使用预编译静态库
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Engine/SDL3_install/lib/libSDL3_image.a)
    if(NOT TARGET SDL3_image::SDL3_image)
        add_library(SDL3_image::SDL3_image STATIC IMPORTED)
        set_target_properties(SDL3_image::SDL3_image PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Engine/SDL3_install/lib/libSDL3_image.a
        )
        add_dependencies(SDL3_image::SDL3_image SDL3::SDL3)
        target_link_libraries(SDL3_image::SDL3_image INTERFACE SDL3::SDL3)
    endif()
endif()

## Use local ImGui (no network fetch)
## Local ImGui integrated from Engine/imgui

# Removed external wzlib_orig integration. WzLibCpp provides its own AES/mio/zlib based implementation.
# Use local ImGui if available (Engine/imgui)

# Library sources
set(LIB_SOURCES
    Game/WzLib/Wz_Type.cpp
    Game/WzLib/Wz_Header.cpp
    Game/WzLib/Wz_Capabilities.cpp
    Game/WzLib/Wz_Crypto.cpp
    Game/WzLib/Wz_Node.cpp
    Game/WzLib/Wz_Png.cpp
    Game/WzLib/Wz_Structure.cpp
    Game/WzLib/Wz_File.cpp
    Game/WzLib/Wz_Directory.cpp
    Game/WzLib/Wz_Image.cpp
    Game/WzLib/Wz_BinaryReader.cpp
    Game/WzLib/Wz_Types.cpp
    Game/WzLib/MapLoader.cpp
    Game/WzLib/WzResourceLoader.cpp
)

# Include internal AES/MIO/ZLIB sources from WzLib directory
set(LIB_SOURCES ${LIB_SOURCES} Game/WzLib/AES/AES.cpp)

add_library(WzLibCpp STATIC ${LIB_SOURCES})

# Set default visibility to hidden for better encapsulation
set_property(TARGET WzLibCpp PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET WzLibCpp PROPERTY VISIBILITY_INLINES_HIDDEN 1)

# Include directories for WzLibCpp
target_include_directories(WzLibCpp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/WzLib
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/wzlib/mio_include
)

# Link zlib for PNG compression
target_link_libraries(WzLibCpp PRIVATE ZLIB::ZLIB)

# Create example executables
# add_executable(LoadWz examples/LoadWz.cpp)
# target_link_libraries(LoadWz PRIVATE WzLibCpp)

# Map Renderer PPM example (simple image output without external graphics dependencies)
# add_executable(MapRendererPPM examples/MapRendererPPM.cpp)
# target_link_libraries(MapRendererPPM PRIVATE WzLibCpp)

# Test Obj Path
# add_executable(TestObjPath examples/TestObjPath.cpp)
# target_link_libraries(TestObjPath PRIVATE WzLibCpp)

# Map Viewer example (requires SDL3)
# add_executable(MapViewer examples/MapViewer.cpp)
# target_link_libraries(MapViewer PRIVATE WzLibCpp SDL3::SDL3-static)
# target_include_directories(MapViewer PRIVATE 
#     /home/ltj/MapleStory/MSC/WzLibCpp/Engine/SDL3_install/include
# )

# EnTT-based Map Viewer (SDL3 + EnTT) - 仅保留此目标
add_executable(MapViewer_EnTT 
    MapViewer_EnTT.cpp
    Game/MapRender/src/MapRenderer.cpp
    Game/MapRender/src/MapData.cpp
    Game/MapRender/src/ResourceLoader.cpp
    Game/MapRender/src/TextureCache.cpp
)
target_sources(MapViewer_EnTT PRIVATE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Engine/imgui/CMakeLists.txt")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Engine/imgui" imgui_local)
endif()

## Auto-detect ImGui target among common names
set(IMGUI_DETECTED_TARGET "")
foreach(_cand imgui imgui_local WZLIB_IMGUI)
  if(TARGET ${_cand})
    set(IMGUI_DETECTED_TARGET ${_cand})
    break()
  endif()
endforeach()
if(IMGUI_DETECTED_TARGET)
  set(IMGUI_TARGET ${IMGUI_DETECTED_TARGET})
endif()
add_subdirectory(Game/WzLib/mio)
add_subdirectory(Game/WzLib/zlib)

# Add ImGui include directory directly since it doesn't have CMakeLists.txt
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Engine/imgui/imgui.h")
  target_include_directories(MapViewer_EnTT PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Engine/imgui")
endif()

if(IMGUI_TARGET STREQUAL "")
  # 仍未检测到本地 ImGui，则 IMGUI_TARGET 保持为空，后续不参与链接
endif()
if(IMGUI_TARGET)
  target_link_libraries(MapViewer_EnTT PRIVATE WzLibCpp ${IMGUI_TARGET} mio zlibstatic SDL3::SDL3 SDL3_image::SDL3_image png z m pthread)
else()
  target_link_libraries(MapViewer_EnTT PRIVATE WzLibCpp mio zlibstatic SDL3::SDL3 SDL3_image::SDL3_image png z m pthread)
endif()

if(IMGUI_TARGET)
  get_target_property(_imgui_inc ${IMGUI_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
  if(_imgui_inc)
    foreach(_d ${_imgui_inc})
      if(_d)
        message(STATUS "WzLibCpp: adding ImGui include dir: ${_d}")
        target_include_directories(MapViewer_EnTT PRIVATE ${_d})
      endif()
    endforeach()
  endif()
endif()

target_include_directories(MapViewer_EnTT PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine
    # EnTT
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/EnTT/single_include
    # SDL3
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/SDL/include
    # ImGui now provided locally; do not rely on imgui_SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/wzlib/mio_include
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/WzLib/mio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/WzLib/AES
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/WzLib/zlib
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/MapRender/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Renderer/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Animation/include
)

# Unit tests (注释掉)
# enable_testing()
# add_subdirectory(tests)

# Install rules (注释掉)
# install(TARGETS WzLibCpp DESTINATION lib)
# install(DIRECTORY include/ DESTINATION include/WzLibCpp)

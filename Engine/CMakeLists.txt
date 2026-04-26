cmake_minimum_required(VERSION 3.15)
project(MapleEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 静态链接模式
set(BUILD_SHARED_LIBS OFF)

# Find SDL3
set(SDL3_DIR "/home/ltj/MapleStory/MSC/WzLibCpp/Engine/SDL3_install/lib/cmake/SDL3")
find_package(SDL3 REQUIRED)

# 强制使用静态 X11 库
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(X11 REQUIRED)

# 查找所有需要的静态库
set(X11_LIB_DIR "/usr/lib/x86_64-linux-gnu")
find_library(X11_STATIC_LIB NAMES X11 PATHS ${X11_LIB_DIR} NO_DEFAULT_PATH)
find_library(XCB_STATIC_LIB NAMES xcb PATHS ${X11_LIB_DIR} NO_DEFAULT_PATH)
find_library(XEXT_STATIC_LIB NAMES Xext PATHS ${X11_LIB_DIR} NO_DEFAULT_PATH)
find_library(XI_STATIC_LIB NAMES Xi PATHS ${X11_LIB_DIR} NO_DEFAULT_PATH)
find_library(XSS_STATIC_LIB NAMES Xss PATHS ${X11_LIB_DIR} NO_DEFAULT_PATH)
find_library(XAU_STATIC_LIB NAMES Xau PATHS ${X11_LIB_DIR} NO_DEFAULT_PATH)
find_library(XDMCP_STATIC_LIB NAMES Xdmcp PATHS ${X11_LIB_DIR} NO_DEFAULT_PATH)

# X11 静态链接库列表 (顺序重要: X11依赖xcb)
set(X11_STATIC_LIBS
    ${X11_STATIC_LIB}
    ${XCB_STATIC_LIB}
    ${XEXT_STATIC_LIB}
    ${XI_STATIC_LIB}
    ${XSS_STATIC_LIB}
    ${XAU_STATIC_LIB}
    ${XDMCP_STATIC_LIB}
)

# Include directories
set(ENGINE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/Core/include
    ${CMAKE_SOURCE_DIR}/ECS/include
    ${CMAKE_SOURCE_DIR}/Renderer/include
    ${CMAKE_SOURCE_DIR}/Scene/include
    ${CMAKE_SOURCE_DIR}/Animation/include
    ${CMAKE_SOURCE_DIR}/MapRender/include
    ${CMAKE_SOURCE_DIR}/UI/include
    ${CMAKE_SOURCE_DIR}/Entt/single_include
    ${CMAKE_SOURCE_DIR}/../include
)

include_directories(${ENGINE_INCLUDE_DIRS})
include_directories(${SDL3_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIR})

# Source files
set(CORE_SOURCES
    Core/src/Engine.cpp
)

set(RENDERER_SOURCES
    Renderer/src/MeshBatcher.cpp
)

set(MAPRENDER_SOURCES
    MapRender/src/MapData.cpp
    MapRender/src/ResourceLoader.cpp
    MapRender/src/MapRenderer.cpp
)

set(UI_SOURCES
    UI/src/UI.cpp
)

# Main library
add_library(MapleEngine STATIC
    ${CORE_SOURCES}
    ${RENDERER_SOURCES}
    ${MAPRENDER_SOURCES}
    ${UI_SOURCES}
)

# 链接 SDL3 和 X11 (静态)
target_link_libraries(MapleEngine
    SDL3::SDL3-static
    ${X11_STATIC_LIBS}
    pthread
    dl
)

# Demo executable
add_executable(MapleEngineDemo
    examples/MapleEngineDemo.cpp
)

target_link_libraries(MapleEngineDemo
    MapleEngine
    SDL3::SDL3-static
    ${X11_STATIC_LIBS}
    pthread
    dl
)

# 单文件模式: 静态链接 libstdc++ 和 libgcc
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(MapleEngineDemo PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
endif()

enable_testing()

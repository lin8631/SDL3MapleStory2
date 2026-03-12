cmake_minimum_required(VERSION 3.28)
project(sdlMS)

# 自动检测并使用 vcpkg 工具链（如果存在且未指定）
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    message(STATUS "Auto-detected vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/Engine/entt/src")
include_directories("${CMAKE_SOURCE_DIR}/Engine/SDL/include")
include_directories("${CMAKE_SOURCE_DIR}/Game/wzlibcpp")
include_directories("${CMAKE_SOURCE_DIR}/Game/wzlibcpp/include")
include_directories("${CMAKE_SOURCE_DIR}/Game/wzlibcpp/mio/include")

# 查找 FFmpeg 组件（非 Emscripten 平台）
if(NOT EMSCRIPTEN)
    # 判断是否需要静态链接（macOS 或 vcpkg 静态构建）
    set(NEED_STATIC_FFMPEG FALSE)
    if(APPLE)
        set(NEED_STATIC_FFMPEG TRUE)
    elseif(DEFINED ENV{VCPKG_LIBRARY_LINKAGE} AND "$ENV{VCPKG_LIBRARY_LINKAGE}" STREQUAL "static")
        set(NEED_STATIC_FFMPEG TRUE)
    endif()

    if(NEED_STATIC_FFMPEG)
        # 告诉 pkg-config 使用静态库（包含私有依赖）
        set(ENV{PKG_CONFIG_ARGN} "--static")
        message(STATUS "pkg-config will use static libraries (PKG_CONFIG_ARGN=--static)")
    endif()

    # 使用 pkg-config 查找 FFmpeg 组件并创建导入目标
    find_package(PkgConfig REQUIRED)

    set(FFMPEG_COMPONENTS libavformat libavcodec libavutil libswscale libswresample)
    set(FFMPEG_TARGETS "")

    foreach(COMP IN LISTS FFMPEG_COMPONENTS)
        string(REPLACE "lib" "" COMP_SHORT ${COMP})  # 去掉 lib 前缀得到 avformat 等
        pkg_check_modules(${COMP_SHORT} IMPORTED_TARGET ${COMP})   # 目标名：PkgConfig::${COMP_SHORT}
        if(${COMP_SHORT}_FOUND)
            list(APPEND FFMPEG_TARGETS PkgConfig::${COMP_SHORT})
            message(STATUS "Found ${COMP} via pkg-config")
        else()
            message(STATUS "pkg-config could not find ${COMP}")
        endif()
    endforeach()

    list(LENGTH FFMPEG_TARGETS NUM_TARGETS)
    if(NUM_TARGETS EQUAL 5)
        # 所有组件都找到，使用导入目标链接
        set(FFMPEG_LINK_LIBRARIES ${FFMPEG_TARGETS})
    else()
        message(FATAL_ERROR "FFmpeg components not found via pkg-config. Please ensure vcpkg installed them correctly.")
    endif()
endif()

# 处理 Emscripten 平台（保持不变）
if(EMSCRIPTEN)
    if(DEFINED ENV{FFMPEG_ROOT})
        set(FFMPEG_ROOT $ENV{FFMPEG_ROOT})
    endif()
    if(FFMPEG_ROOT AND EXISTS "${FFMPEG_ROOT}/include")
        include_directories("${FFMPEG_ROOT}/include")
        link_directories("${FFMPEG_ROOT}/lib")
    endif()
endif()

file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp)

add_executable(sdlMS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${SOURCE_FILES})

# Windows NOMINMAX
if(WIN32)
    target_compile_definitions(sdlMS PRIVATE NOMINMAX)
endif()

target_link_libraries(sdlMS PRIVATE SDL3::SDL3)
target_link_libraries(sdlMS PRIVATE wzlib)
target_link_libraries(sdlMS PRIVATE freetype)
target_link_libraries(sdlMS PRIVATE EnTT)

if(NOT EMSCRIPTEN)
    # 链接 FFmpeg 导入目标（自动处理私有依赖和系统框架）
    if(FFMPEG_LINK_LIBRARIES)
        target_link_libraries(sdlMS PRIVATE ${FFMPEG_LINK_LIBRARIES})
    else()
        # 回退到手动库列表（理论上不会执行到这里，但保留以防万一）
        target_link_libraries(sdlMS PRIVATE ${FFMPEG_LIBRARIES})
        if(APPLE)
            find_library(VIDEOTOOLBOX_FRAMEWORK VideoToolbox)
            if(VIDEOTOOLBOX_FRAMEWORK)
                target_link_libraries(sdlMS PRIVATE ${VIDEOTOOLBOX_FRAMEWORK})
            endif()
            find_library(COREMEDIA_FRAMEWORK CoreMedia)
            if(COREMEDIA_FRAMEWORK)
                target_link_libraries(sdlMS PRIVATE ${COREMEDIA_FRAMEWORK})
            endif()
            find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
            if(COREFOUNDATION_FRAMEWORK)
                target_link_libraries(sdlMS PRIVATE ${COREFOUNDATION_FRAMEWORK})
            endif()
        endif()
    endif()

    # Brotli 链接（如果需要）
    set(NEED_STATIC_BROTLI FALSE)
    if(APPLE)
        set(NEED_STATIC_BROTLI TRUE)
    elseif(DEFINED ENV{VCPKG_LIBRARY_LINKAGE} AND "$ENV{VCPKG_LIBRARY_LINKAGE}" STREQUAL "static")
        set(NEED_STATIC_BROTLI TRUE)
    endif()

    if(NEED_STATIC_BROTLI)
        find_package(Brotli CONFIG QUIET)
        if(Brotli_FOUND)
            target_link_libraries(sdlMS PRIVATE Brotli::brotlicommon Brotli::brotlidec Brotli::brotlienc)
            message(STATUS "Found Brotli via CMake config")
        else()
            message(STATUS "Brotli CMake config not found, using direct path from vcpkg")
            if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
                set(VCPKG_LIB_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
                set(BROTLI_COMMON_LIB "${VCPKG_LIB_DIR}/libbrotlicommon.a")
                set(BROTLI_DEC_LIB    "${VCPKG_LIB_DIR}/libbrotlidec.a")
                set(BROTLI_ENC_LIB    "${VCPKG_LIB_DIR}/libbrotlienc.a")
                if(EXISTS "${BROTLI_COMMON_LIB}" AND EXISTS "${BROTLI_DEC_LIB}" AND EXISTS "${BROTLI_ENC_LIB}")
                    if(APPLE)
                        target_link_libraries(sdlMS PRIVATE
                            -Wl,-force_load,${BROTLI_COMMON_LIB}
                            -Wl,-force_load,${BROTLI_DEC_LIB}
                            -Wl,-force_load,${BROTLI_ENC_LIB}
                        )
                        message(STATUS "Using Brotli static libraries from vcpkg with -force_load on macOS")
                    else()
                        target_link_libraries(sdlMS PRIVATE
                            -Wl,--whole-archive
                            ${BROTLI_COMMON_LIB}
                            ${BROTLI_DEC_LIB}
                            ${BROTLI_ENC_LIB}
                            -Wl,--no-whole-archive
                        )
                        message(STATUS "Using Brotli static libraries from vcpkg with --whole-archive")
                    endif()
                else()
                    message(FATAL_ERROR "Brotli static libraries not found in vcpkg installation: ${VCPKG_LIB_DIR}")
                endif()
            else()
                message(FATAL_ERROR "VCPKG_INSTALLED_DIR or VCPKG_TARGET_TRIPLET not defined; cannot locate Brotli.")
            endif()
        endif()
    endif()
endif()

if(WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    endif()
    set_target_properties(sdlMS PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(APPLE)
    set_target_properties(sdlMS PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# Emscripten 设置
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    if(EXISTS "${CMAKE_BINARY_DIR}/Font/@Font")
        set_target_properties(sdlMS PROPERTIES LINK_FLAGS "--preload-file ${CMAKE_BINARY_DIR}/Font/@Font/")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH -sUSE_ZLIB -sSTACK_SIZE=32MB")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sINITIAL_MEMORY=64MB")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sFETCH=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sASYNCIFY=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sMAXIMUM_MEMORY=4GB")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -flto")
    set_target_properties(sdlMS PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
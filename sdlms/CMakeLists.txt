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
    # 尝试使用 pkg-config 查找 FFmpeg
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_AVFORMAT QUIET libavformat)
        pkg_check_modules(PC_AVCODEC  QUIET libavcodec)
        pkg_check_modules(PC_AVUTIL   QUIET libavutil)
        pkg_check_modules(PC_SWSCALE  QUIET libswscale)
        pkg_check_modules(PC_SWRESAMPLE QUIET libswresample)

        if(PC_AVFORMAT_FOUND AND PC_AVCODEC_FOUND AND PC_AVUTIL_FOUND AND PC_SWSCALE_FOUND AND PC_SWRESAMPLE_FOUND)
            message(STATUS "Found FFmpeg via pkg-config")
            # 使用传统变量（避免 CMake 导入目标在部分环境失效的问题）
            set(FFMPEG_INCLUDE_DIRS
                ${PC_AVFORMAT_INCLUDE_DIRS}
                ${PC_AVCODEC_INCLUDE_DIRS}
                ${PC_AVUTIL_INCLUDE_DIRS}
                ${PC_SWSCALE_INCLUDE_DIRS}
                ${PC_SWRESAMPLE_INCLUDE_DIRS}
            )
            list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
            include_directories(${FFMPEG_INCLUDE_DIRS})

            set(FFMPEG_LIBRARIES
                ${PC_AVFORMAT_LIBRARIES}
                ${PC_AVCODEC_LIBRARIES}
                ${PC_AVUTIL_LIBRARIES}
                ${PC_SWSCALE_LIBRARIES}
                ${PC_SWRESAMPLE_LIBRARIES}
            )

            set(FFMPEG_LIBRARY_DIRS
                ${PC_AVFORMAT_LIBRARY_DIRS}
                ${PC_AVCODEC_LIBRARY_DIRS}
                ${PC_AVUTIL_LIBRARY_DIRS}
                ${PC_SWSCALE_LIBRARY_DIRS}
                ${PC_SWRESAMPLE_LIBRARY_DIRS}
            )
            list(REMOVE_DUPLICATES FFMPEG_LIBRARY_DIRS)
            link_directories(${FFMPEG_LIBRARY_DIRS})
        else()
            message(STATUS "pkg-config failed to find all FFmpeg components, falling back to manual search")
            set(PKGCONFIG_FAILED TRUE)
        endif()
    else()
        message(STATUS "pkg-config not found, falling back to manual search")
        set(PKGCONFIG_FAILED TRUE)
    endif()

    if(PKGCONFIG_FAILED)
        # 允许用户通过 FFMPEG_ROOT 变量指定 FFmpeg 安装根目录
        if(NOT DEFINED FFMPEG_ROOT)
            set(FFMPEG_ROOT "" CACHE PATH "Path to FFmpeg installation (containing include/ and lib/)")
        endif()

        # 定义 FFmpeg 各组件的头文件和库文件名
        set(FFMPEG_COMPONENTS avformat avcodec avutil swscale swresample)
        set(FFMPEG_POSSIBLE_INCLUDE_SUFFIXES include include/ffmpeg)
        set(FFMPEG_POSSIBLE_LIB_SUFFIXES lib lib64)

        if(WIN32)
            set(FFMPEG_POSSIBLE_BASE_PATHS
                "C:/vcpkg/installed/x64-windows"
                "C:/msys64/mingw64"
                "C:/Program Files/FFmpeg"
                "C:/FFmpeg"
                "$ENV{FFMPEG_ROOT}"
            )
        elseif(APPLE)
            set(FFMPEG_POSSIBLE_BASE_PATHS
                "/usr/local"
                "/opt/homebrew"
                "/opt/local"
                "/usr"
            )
        else() # Linux/Unix
            set(FFMPEG_POSSIBLE_BASE_PATHS
                "/usr/local"
                "/usr"
                "/opt/ffmpeg"
            )
        endif()

        if(FFMPEG_ROOT)
            list(INSERT FFMPEG_POSSIBLE_BASE_PATHS 0 ${FFMPEG_ROOT})
        endif()

        # 循环查找每个组件的头文件和库
        foreach(COMP ${FFMPEG_COMPONENTS})
            string(TOUPPER ${COMP} COMP_UPPER)

            find_path(FFMPEG_${COMP_UPPER}_INCLUDE_DIR
                NAMES lib${COMP}/${COMP}.h
                PATHS ${FFMPEG_POSSIBLE_BASE_PATHS}
                PATH_SUFFIXES ${FFMPEG_POSSIBLE_INCLUDE_SUFFIXES}
                NO_DEFAULT_PATH
            )

            find_library(FFMPEG_${COMP_UPPER}_LIBRARY
                NAMES ${COMP}
                PATHS ${FFMPEG_POSSIBLE_BASE_PATHS}
                PATH_SUFFIXES ${FFMPEG_POSSIBLE_LIB_SUFFIXES}
                NO_DEFAULT_PATH
            )

            if(FFMPEG_${COMP_UPPER}_INCLUDE_DIR AND FFMPEG_${COMP_UPPER}_LIBRARY)
                set(FFMPEG_${COMP_UPPER}_FOUND TRUE)
                list(APPEND FFMPEG_INCLUDE_DIRS ${FFMPEG_${COMP_UPPER}_INCLUDE_DIR})
                list(APPEND FFMPEG_LIBRARIES ${FFMPEG_${COMP_UPPER}_LIBRARY})
            else()
                set(FFMPEG_${COMP_UPPER}_FOUND FALSE)
                message(STATUS "FFmpeg component ${COMP} not found in default paths.")
            endif()
        endforeach()

        # 检查是否所有必需组件都找到
        set(REQUIRED_COMPONENTS avformat avcodec avutil swscale swresample)
        set(ALL_FOUND TRUE)
        foreach(COMP ${REQUIRED_COMPONENTS})
            string(TOUPPER ${COMP} COMP_UPPER)
            if(NOT FFMPEG_${COMP_UPPER}_FOUND)
                set(ALL_FOUND FALSE)
                break()
            endif()
        endforeach()

        if(NOT ALL_FOUND)
            if(WIN32)
                message(STATUS "FFmpeg not found in standard paths, trying hardcoded Windows path")
                # 硬编码路径（根据用户要求）
                set(HARDCODED_FFMPEG_PATH "C:/ffmpeg-master-latest-win64-gpl-shared")
                if(EXISTS "${HARDCODED_FFMPEG_PATH}/include" AND EXISTS "${HARDCODED_FFMPEG_PATH}/lib")
                    include_directories("${HARDCODED_FFMPEG_PATH}/include")
                    link_directories("${HARDCODED_FFMPEG_PATH}/lib")
                    # 设置库变量为硬编码库名
                    set(FFMPEG_LIBRARIES avformat avcodec avutil swscale swresample)
                    message(STATUS "Using hardcoded FFmpeg path: ${HARDCODED_FFMPEG_PATH}")
                else()
                    message(FATAL_ERROR "Required FFmpeg components not found. Please install FFmpeg or set FFMPEG_ROOT to the correct path.")
                endif()
            else()
                message(FATAL_ERROR "Required FFmpeg components not found. Please install FFmpeg or set FFMPEG_ROOT to the correct path.")
            endif()
        else()
            # 所有组件找到，添加头文件路径并去重
            list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
            list(REMOVE_DUPLICATES FFMPEG_LIBRARIES)
            include_directories(${FFMPEG_INCLUDE_DIRS})
        endif()
    endif()
endif()

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

# 为 Windows 平台添加 NOMINMAX 定义，防止 min/max 宏与 std::min/std::max 冲突
if(WIN32)
    target_compile_definitions(sdlMS PRIVATE NOMINMAX)
endif()

target_link_libraries(sdlMS PRIVATE SDL3::SDL3)
target_link_libraries(sdlMS PRIVATE wzlib)
target_link_libraries(sdlMS PRIVATE freetype)
target_link_libraries(sdlMS PRIVATE EnTT)

if(NOT EMSCRIPTEN)
    # 链接 FFmpeg 库（通过 pkg-config 或手动查找得到）
    target_link_libraries(sdlMS PRIVATE ${FFMPEG_LIBRARIES})

    # 判断是否需要静态链接 Brotli
    set(NEED_STATIC_BROTLI FALSE)
    if(APPLE)
        set(NEED_STATIC_BROTLI TRUE)
    elseif(DEFINED ENV{VCPKG_LIBRARY_LINKAGE} AND "$ENV{VCPKG_LIBRARY_LINKAGE}" STREQUAL "static")
        set(NEED_STATIC_BROTLI TRUE)
    endif()

    if(NEED_STATIC_BROTLI)
        # 优先使用 vcpkg 提供的 CMake 配置文件
        find_package(Brotli CONFIG QUIET)
        if(Brotli_FOUND)
            target_link_libraries(sdlMS PRIVATE Brotli::brotlicommon Brotli::brotlidec Brotli::brotlienc)
            message(STATUS "Found Brotli via CMake config")
        else()
            message(STATUS "Brotli CMake config not found, using direct path from vcpkg")
            # 使用 vcpkg 变量直接构造静态库路径
            if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
                set(VCPKG_LIB_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
                set(BROTLI_COMMON_LIB "${VCPKG_LIB_DIR}/libbrotlicommon.a")
                set(BROTLI_DEC_LIB    "${VCPKG_LIB_DIR}/libbrotlidec.a")
                set(BROTLI_ENC_LIB    "${VCPKG_LIB_DIR}/libbrotlienc.a")
                if(EXISTS "${BROTLI_COMMON_LIB}" AND EXISTS "${BROTLI_DEC_LIB}" AND EXISTS "${BROTLI_ENC_LIB}")
                    # 使用 --whole-archive 强制包含所有符号，并按依赖顺序链接
                    target_link_libraries(sdlMS PRIVATE
                        -Wl,--whole-archive
                        ${BROTLI_COMMON_LIB}
                        ${BROTLI_DEC_LIB}
                        ${BROTLI_ENC_LIB}
                        -Wl,--no-whole-archive
                    )
                    message(STATUS "Using Brotli static libraries from vcpkg with --whole-archive")
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
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
endif()

# 设置 Emscripten 编译选项
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

        # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g4 -sNO_DISABLE_EXCEPTION_CATCHING=1")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -flto")

        set_target_properties(sdlMS PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        )
endif()
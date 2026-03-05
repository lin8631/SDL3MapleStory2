cmake_minimum_required(VERSION 3.28)
project(sdlMS)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(Game)
add_subdirectory(sdlms)
add_subdirectory(Engine EXCLUDE_FROM_ALL)

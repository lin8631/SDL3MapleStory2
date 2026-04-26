cmake_minimum_required(VERSION 3.18)
project(WzLibCpp_MioWrapper LANGUAGES CXX)

## Expose a header-only Mio wrapper interface under this path
add_library(mio INTERFACE)
target_include_directories(mio INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mio
  /home/ltj/MapleStory/MSC/WzLibCpp/Engine/wzlib/mio_include/mio
)

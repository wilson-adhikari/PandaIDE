cmake_minimum_required(VERSION 3.20)
project(GoLanguage VERSION 0.1 LANGUAGES CXX)

# Go language provider (header-only for v0.1; a full
# gopls integration would live here).
file(GLOB_RECURSE GO_HDR CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(go-language INTERFACE)
target_include_directories(go-language INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

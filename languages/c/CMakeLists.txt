cmake_minimum_required(VERSION 3.20)
project(CLanguage VERSION 0.1 LANGUAGES CXX)

# C language provider (C11). Compiled as C++ because the
# provider uses the C++ standard library.
file(GLOB_RECURSE CLANG_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE CLANG_HDR CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(c-language STATIC ${CLANG_SRC} ${CLANG_HDR})

target_include_directories(c-language PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

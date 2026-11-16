cmake_minimum_required(VERSION 3.20)
project(RustLanguage VERSION 0.1 LANGUAGES CXX)

# Rust language provider (header-only for v0.1; a full
# rust-analyzer integration would live here).
file(GLOB_RECURSE RUST_HDR CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(rust-language INTERFACE)
target_include_directories(rust-language INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

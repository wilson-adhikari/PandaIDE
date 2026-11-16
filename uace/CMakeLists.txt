cmake_minimum_required(VERSION 3.20)
project(UACE VERSION 0.1 LANGUAGES C CXX)

# UACE static library. The tests/ directory is intentionally NOT
# globbed here; it holds standalone test programs, not library code.
file(GLOB_RECURSE UACE_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/detection/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/x86-64/*.cpp"
)

file(GLOB_RECURSE UACE_HDR CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/detection/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/x86-64/*.h"
)

add_library(uace STATIC ${UACE_SRC} ${UACE_HDR})

target_include_directories(uace PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(uace PRIVATE
    UACE_VERSION_MAJOR=0
    UACE_VERSION_MINOR=1
)

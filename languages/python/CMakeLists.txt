cmake_minimum_required(VERSION 3.20)
project(PythonLanguage VERSION 0.1 LANGUAGES CXX)

# Python language provider (interpreter-based tooling).
file(GLOB_RECURSE PY_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE PY_HDR CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(python-language STATIC ${PY_SRC} ${PY_HDR})

target_include_directories(python-language PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

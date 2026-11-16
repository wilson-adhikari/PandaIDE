cmake_minimum_required(VERSION 3.20)
project(Toolchains VERSION 0.1 LANGUAGES C CXX)

# API headers
file(GLOB_RECURSE API_H
    api/*.h)

# Discovery
file(GLOB_RECURSE DISCOVERY_H
    discovery/*.h)

# Registry
file(GLOB_RECURSE REGISTRY_H
    registry/*.h)

# Builtin toolchains
file(GLOB_RECURSE BUILTIN_CPP
    builtin/*.cpp)
file(GLOB_RECURSE BUILTIN_H
    builtin/*.h)

add_library(toolchains STATIC ${API_H} ${DISCOVERY_H} ${REGISTRY_H} ${BUILTIN_CPP} ${BUILTIN_H})

target_include_directories(toolchains PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
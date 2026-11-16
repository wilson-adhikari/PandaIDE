cmake_minimum_required(VERSION 3.20)
project(PandaIDE VERSION 0.1 LANGUAGES C CXX)

# ============================================================
# PandaIDE Open-Source Project - User Configuration
# ============================================================
# Users can enable/disable components via CMake options:
#
#   cmake -D PANDAIDE_ENABLE_XXX=ON|OFF ..
#
# Qt location is NOT hardcoded here on purpose (open-source friendly).
# Tell CMake where your Qt6 lives, e.g.:
#   cmake -D CMAKE_PREFIX_PATH="E:/Qt/6.8.0/mingw_64" ..
# or set the Qt6_DIR / CMAKE_PREFIX_PATH environment variable.
#
# Default v0.1 build enables core functionality.
# ============================================================

# Global C++ standard (Qt6 requires C++17 or newer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# User-configurable options
option(PANDAIDE_ENABLE_UI "Enable IDE UI module" ON)
option(PANDAIDE_ENABLE_TERMINAL "Enable integrated terminal" ON)
option(PANDAIDE_ENABLE_PLUGINS "Enable plugin system" ON)
option(PANDAIDE_ENABLE_LANGUAGE_C "Enable C language" ON)
option(PANDAIDE_ENABLE_LANGUAGE_CPP "Enable C++ language" OFF)
option(PANDAIDE_ENABLE_LANGUAGE_PYTHON "Enable Python language" ON)
option(PANDAIDE_ENABLE_LANGUAGE_RUST "Enable Rust language" OFF)
option(PANDAIDE_ENABLE_LANGUAGE_GO "Enable Go language" OFF)
option(PANDAIDE_ENABLE_UACE "Enable UACE Assembly Engine" ON)
option(PANDAIDE_ENABLE_UACE_X86 "Enable x86-64 architecture support" ON)
option(PANDAIDE_ENABLE_UACE_ARM "Enable ARM architecture support" OFF)
option(PANDAIDE_ENABLE_UACE_RISCV "Enable RISC-V architecture support" OFF)
option(PANDAIDE_ENABLE_TOOLCHAINS "Enable toolchain manager" ON)
option(PANDAIDE_ENABLE_BUILD_SYSTEM "Enable build system" ON)
option(PANDAIDE_ENABLE_DEBUGGER "Enable debugger" OFF)
option(PANDAIDE_ENABLE_EMULATOR "Enable emulator" OFF)
option(PANDAIDE_ENABLE_HARDWARE "Enable hardware support" OFF)
option(PANDAIDE_ENABLE_BINARY_ENGINE "Enable binary engineering" OFF)

# Print configuration
message(STATUS "PandaIDE v${PROJECT_VERSION} Config: UI=${PANDAIDE_ENABLE_UI} Term=${PANDAIDE_ENABLE_TERMINAL} Lang=C=${PANDAIDE_ENABLE_LANGUAGE_C} Py=${PANDAIDE_ENABLE_LANGUAGE_PYTHON} UACE=${PANDAIDE_ENABLE_UACE} x86=${PANDAIDE_ENABLE_UACE_X86}")

# ============================================================
# Core Application
# ============================================================
add_subdirectory(apps/panda-ide)

# CLI tool (enabled with UI)
if(PANDAIDE_ENABLE_UI)
    add_subdirectory(apps/panda-cli)
endif()

# Languages - conditional
if(PANDAIDE_ENABLE_LANGUAGE_C)
    add_subdirectory(languages/c)
endif()

if(PANDAIDE_ENABLE_LANGUAGE_PYTHON)
    add_subdirectory(languages/python)
endif()

# UACE Assembly Engine (always on for v0.1)
if(PANDAIDE_ENABLE_UACE)
    add_subdirectory(uace)
endif()

# Toolchains
if(PANDAIDE_ENABLE_TOOLCHAINS)
    add_subdirectory(toolchains)
endif()

# UI assets (only if a CMakeLists.txt exists there)
if(PANDAIDE_ENABLE_UI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/CMakeLists.txt")
    add_subdirectory(ui)
endif()

# Build system subsystem (source lives in build_system/ so it never
# collides with the conventional CMake output directory "build/")
if(PANDAIDE_ENABLE_BUILD_SYSTEM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build_system/CMakeLists.txt")
    add_subdirectory(build_system)
endif()

# Optional subsystems (disabled by default for v0.1, added only when
# both enabled AND a CMakeLists.txt exists)
if(PANDAIDE_ENABLE_DEBUGGER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debugger/CMakeLists.txt")
    add_subdirectory(debugger)
endif()

if(PANDAIDE_ENABLE_EMULATOR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/emulator/CMakeLists.txt")
    add_subdirectory(emulator)
endif()

if(PANDAIDE_ENABLE_HARDWARE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hardware/CMakeLists.txt")
    add_subdirectory(hardware)
endif()

if(PANDAIDE_ENABLE_BINARY_ENGINE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/binary/CMakeLists.txt")
    add_subdirectory(binary)
endif()

# Plugins (only if a CMakeLists.txt exists there)
if(PANDAIDE_ENABLE_PLUGINS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/CMakeLists.txt")
    add_subdirectory(plugins)
endif()

# Documentation (only if CMakeLists.txt exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/CMakeLists.txt")
    add_subdirectory(docs)
endif()

# Tests (only if CMakeLists.txt exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

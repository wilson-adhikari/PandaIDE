cmake_minimum_required(VERSION 3.20)

project(PandaIDE-App VERSION 0.1 LANGUAGES CXX)

# ============================================================
# Qt
# ============================================================
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Qml Quick)

# ============================================================
# Qt Automatic Processing
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ============================================================
# C++ Standard
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# MSVC Flags
# ============================================================
if(MSVC)
    # Qt6 requires /permissive- and /Zc:__cplusplus
    add_compile_options(/permissive- /Zc:__cplusplus)
endif()

# ============================================================
# Resources
# ============================================================
set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc")

# ============================================================
# Application Source Files
# ============================================================
file(GLOB_RECURSE APP_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.h"
)

# Main application
add_executable(panda-ide ${APP_SRC} ${RESOURCE_FILE})

target_link_libraries(panda-ide PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
)

# Add defines for the version
target_compile_definitions(panda-ide PRIVATE
    PANDAIDE_VERSION_MAJOR=0
    PANDAIDE_VERSION_MINOR=1
)

# Include directory
target_include_directories(panda-ide PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
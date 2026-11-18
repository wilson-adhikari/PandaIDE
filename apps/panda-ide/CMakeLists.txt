cmake_minimum_required(VERSION 3.20)

project(PandaIDE-App VERSION 0.2.0 LANGUAGES CXX)

# Public client ID of the GitHub OAuth App used for "Sign in with GitHub"
# (device flow). Not a secret. Empty = sign-in shows as not set up.
set(PANDAIDE_GITHUB_CLIENT_ID "" CACHE STRING "GitHub OAuth App client ID for sign-in")

# ============================================================
# Qt
# ============================================================
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Qml Quick Network)

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
    Qt6::Network
)

target_compile_definitions(panda-ide PRIVATE
    PANDAIDE_VERSION="${PROJECT_VERSION}"
    PANDAIDE_GITHUB_CLIENT_ID="${PANDAIDE_GITHUB_CLIENT_ID}"
)

# A desktop app: no console window on Windows, a PandaIDE.app bundle on macOS.
set_target_properties(panda-ide PROPERTIES
    OUTPUT_NAME PandaIDE
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME PandaIDE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.axiomera.pandaide
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

# Include directory
target_include_directories(panda-ide PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ============================================================
# Tests: workspace files/search and the Run pipeline (ctest)
# ============================================================
option(PANDAIDE_BUILD_TESTS "Build the PandaIDE tests" ON)
if(PANDAIDE_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(panda-ide-tests
        tests/workspace_test.cpp
        src/Project/ProjectManager.cpp
        src/Terminal/TerminalService.cpp
    )
    target_include_directories(panda-ide-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(panda-ide-tests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME panda-ide-tests COMMAND panda-ide-tests)
endif()
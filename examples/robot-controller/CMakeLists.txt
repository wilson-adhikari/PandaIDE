cmake_minimum_required(VERSION 3.20)
project(RobotController VERSION 0.1 CXX ASM)

# C++ source
set(CPP_SOURCES src/main.cpp)

# Assembly source
set(ASM_SOURCES asm/fast_math.asm)

# Add executable
add_executable(robot_controller ${CPP_SOURCES} ${ASM_SOURCES})

# UACE integration - compile assembly via UACE
# In a full implementation, UACE would be integrated as a custom command
# For v0.1, we demonstrate the build graph concept

# Target architecture
set(CMAKE_PROCESSOR_x86_64 ON)

# Linker settings
target_link_libraries(robot_controller)

# Add custom build target for UACE assembly
# This would normally call the UACE compiler
add_custom_target(
    uace_compile
    COMMAND ${CMAKE_COMMAND} -E echo "UACE compiling fast_math.asm..."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Dependencies
add_dependencies(robot_controller uace_compile)

# Installation
install(TARGETS robot_controller DESTINATION bin)
install(FILES asm/fast_math.asm DESTINATION share/robot_controller)
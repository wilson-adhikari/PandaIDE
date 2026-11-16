cmake_minimum_required(VERSION 3.20)
project(VisionSystem VERSION 0.1 CXX PYTHON)

# C++ source
set(CPP_SOURCES src/camera.cpp)

# Add executable
add_executable(camera_module ${CPP_SOURCES})

# Python script (for reference, not compiled)
set(PYTHON_SCRIPTS python/main.py)

# Target
set_target_properties(camera_module PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Install
install(TARGETS camera_module DESTINATION bin)
install(FILES python/main.py DESTINATION share/vision_system)
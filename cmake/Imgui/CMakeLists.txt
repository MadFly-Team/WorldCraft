cmake_minimum_required(VERSION 3.27.0)
project(imgui)

# ImGui core sources
set(IMGUI_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
)

# Backend sources for SDL2 + OpenGL3
set(IMGUI_BACKEND_SOURCES
    backends/imgui_impl_sdl2.cpp
    backends/imgui_impl_opengl3.cpp
)

# Create ImGui static library
add_library(imgui STATIC
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
)

# ImGui needs to be built AFTER GLAD is generated
add_dependencies(imgui glad_gl_core_33)

# Include directories
target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
    ${CMAKE_BINARY_DIR}/gladsources/glad_gl_core_33/include  # Add GLAD for OpenGL headers
)

# Find SDL2 (should already be available from parent project)
target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/SDL2/include
)

# Find OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(imgui PUBLIC OpenGL::GL glad_gl_core_33)

# Link to SDL2 (using the same SDL2-static target as WorldCraft)
target_link_libraries(imgui PUBLIC SDL2-static)

# Set C++ standard
target_compile_features(imgui PUBLIC cxx_std_17)

# Tell ImGui to use GLAD as the OpenGL loader
target_compile_definitions(imgui PUBLIC 
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

# make sure the minimum cmake version is compatible with your compiler
cmake_minimum_required(VERSION 3.27.0)

# set a project name and version number
project(WorldCraftSuite VERSION 0.0.1)

# set C Make to use the C++20 Language version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

include(ExternalProject)

find_package(Git REQUIRED)

ExternalProject_Add(
    doctest
    PREFIX ${CMAKE_BINARY_DIR}/doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)


# --- SDL2: build library from submodule ---
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
# Fix for Visual Studio CMake integration: force header detection
# MSVC has these headers, but check_include_file may fail in multi-config generators
if(MSVC)
  set(HAVE_STDINT_H 1 CACHE INTERNAL "Have stdint.h")
  set(HAVE_DINPUT_H 1 CACHE INTERNAL "Have dinput.h")
  set(HAVE_XINPUT_H 1 CACHE INTERNAL "Have xinput.h")
endif()
add_subdirectory(third_party/SDL2)

# --- GLM (CMake project, header-only) ---
add_subdirectory(third_party/glm)

# --- STB (no CMake in upstream): treat as headers ---
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE third_party/stb)


# GLAD submodule located at third_party/glad
add_subdirectory(third_party/glad/cmake glad_cmake)

glad_add_library(glad_gl_core_33 STATIC
  REPRODUCIBLE
  API gl:core=3.3
)

# --- ImGui: UI library ---
add_subdirectory(third_party/imgui)

#-----------------------------------------------------------------------------
# add the subdirectories for the library, IDE and testing framework being used
#-----------------------------------------------------------------------------

add_subdirectory(WorldCraft)

# Set WorldCraft as the Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WorldCraft)


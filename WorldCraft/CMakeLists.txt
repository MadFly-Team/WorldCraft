# CMakeList.txt : CMake project for WorldCraft, include source and define
# project specific logic here.
#

file(GLOB_RECURSE  SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp" "inc/*.h" )

# Add source to this project's executable.
add_executable (WorldCraft ${SOURCE_FILES})

# Set as Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WorldCraft)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET WorldCraft PROPERTY CXX_STANDARD 20)
endif()

# Expose inc/ as the root for all module headers (e.g. #include <Renderer/Shader.h>)
target_include_directories(WorldCraft PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/third_party/SDL2/include
)

# OpenGL itself is provided by the system/driver; find it:
find_package(OpenGL REQUIRED)

target_link_libraries(WorldCraft PRIVATE
    OpenGL::GL
    SDL2main
    SDL2-static
    glad_gl_core_33
    glm::glm
    stb
    imgui
    ZLIB::ZLIB
)

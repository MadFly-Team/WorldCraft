# CMakeList.txt : CMake project for WorldCraft, include source and define
# project specific logic here.
#

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (WorldCraft "*.cpp" "*.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET WorldCraft PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(WorldCraft PRIVATE
    OpenGL::GL
    glfw
    glad::glad
    glm::glm
)

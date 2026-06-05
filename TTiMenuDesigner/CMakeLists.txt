cmake_minimum_required(VERSION 3.27.0)

project(TTiMenuDesigner VERSION 1.0.0)

# Collect source files
set(SOURCES
	src/main.cpp
	src/MenuData.cpp
	src/MenuNavigator.cpp
	src/EmbeddedRenderer.cpp
	src/MenuEditor.cpp
	src/MenuSerializer.cpp
	src/TGR2053Templates.cpp
)

# Collect header files
set(HEADERS
	inc/MenuData.h
	inc/MenuNavigator.h
	inc/EmbeddedRenderer.h
	inc/MenuEditor.h
	inc/MenuSerializer.h
	inc/TGR2053Templates.h
)

# Create executable
add_executable(TTiMenuDesigner
	${SOURCES}
	${HEADERS}
)

# Include directories
target_include_directories(TTiMenuDesigner PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Link CoreLibrary
target_link_libraries(TTiMenuDesigner PRIVATE
	CoreLibrary
	SDL2main
)

# Set C++20 standard
set_property(TARGET TTiMenuDesigner PROPERTY CXX_STANDARD 20)
set_property(TARGET TTiMenuDesigner PROPERTY CXX_STANDARD_REQUIRED ON)

message(STATUS "TTiMenuDesigner configured successfully")

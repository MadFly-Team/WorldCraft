cmake_minimum_required(VERSION 3.27.0)

project(CoreLibrary VERSION 1.0.0)

# Collect all source files
set(CORE_SOURCES
	src/Core/Screen.cpp
	src/Core/Application.cpp
)

set(UI_SOURCES
	src/UI/Window.cpp
	src/UI/Menu.cpp
	src/UI/Dropdown.cpp
	src/UI/TextRenderer.cpp
	src/UI/ImGuiManager.cpp
)

set(GRAPHICS2D_SOURCES
	src/Graphics2D/Renderer2D.cpp
	src/Graphics2D/Shapes.cpp
)

set(SPRITE_SOURCES
	src/Sprite/Sprite.cpp
	src/Sprite/SpriteSheet.cpp
	src/Sprite/SpriteRenderer.cpp
	src/Sprite/stb_impl.cpp
)

# Collect all header files
set(CORE_HEADERS
	inc/Core/Screen.h
	inc/Core/Application.h
)

set(UI_HEADERS
	inc/UI/Window.h
	inc/UI/Menu.h
	inc/UI/Dropdown.h
	inc/UI/TextRenderer.h
	inc/UI/ImGuiManager.h
)

set(GRAPHICS2D_HEADERS
	inc/Graphics2D/Renderer2D.h
	inc/Graphics2D/Shapes.h
)

set(SPRITE_HEADERS
	inc/Sprite/Sprite.h
	inc/Sprite/SpriteSheet.h
	inc/Sprite/SpriteRenderer.h
)

# Create static library
add_library(CoreLibrary STATIC
	${CORE_SOURCES}
	${UI_SOURCES}
	${GRAPHICS2D_SOURCES}
	${SPRITE_SOURCES}
	${CORE_HEADERS}
	${UI_HEADERS}
	${GRAPHICS2D_HEADERS}
	${SPRITE_HEADERS}
)

# Include directories
target_include_directories(CoreLibrary PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Link dependencies
target_link_libraries(CoreLibrary PUBLIC
	SDL2-static
	glad_gl_core_33
	imgui
	glm
	stb
	OpenGL::GL
)

# Set C++20 standard
set_property(TARGET CoreLibrary PROPERTY CXX_STANDARD 20)
set_property(TARGET CoreLibrary PROPERTY CXX_STANDARD_REQUIRED ON)

# Organize files in IDE
source_group("Core\\Headers" FILES ${CORE_HEADERS})
source_group("Core\\Sources" FILES ${CORE_SOURCES})
source_group("UI\\Headers" FILES ${UI_HEADERS})
source_group("UI\\Sources" FILES ${UI_SOURCES})
source_group("Graphics2D\\Headers" FILES ${GRAPHICS2D_HEADERS})
source_group("Graphics2D\\Sources" FILES ${GRAPHICS2D_SOURCES})
source_group("Sprite\\Headers" FILES ${SPRITE_HEADERS})
source_group("Sprite\\Sources" FILES ${SPRITE_SOURCES})

message(STATUS "CoreLibrary configured successfully")

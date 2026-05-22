#pragma once

#include <WorldCraft.h>

namespace Renderer
{

	// Initialise SDL2, create a resizable window and OpenGL context.
	// Returns the window handle, or nullptr on failure.
	SDL_Window* createWindow(int width, int height, const char* title);

	// Create a window in either windowed or startup fullscreen mode with OpenGL context.
	SDL_Window* createWindow(int width, int height, const char* title, bool fullscreen);

	// Destroy the window and its OpenGL context, and shutdown SDL2.
	void destroyWindow(SDL_Window* window);

	// Terminate SDL2 once on final app shutdown.
	void shutdown();

	// Recreate-safe helper: returns the primary display size.
	glm::ivec2 primaryMonitorSize();

	// Toggle fullscreen on the existing window without recreating the OpenGL context.
	void toggleFullscreen(SDL_Window* window);

} // namespace Renderer

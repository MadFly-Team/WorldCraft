#pragma once

#include <SDL.h>

namespace UI
{
	// Manages ImGui initialization, frame handling, and cleanup
	// Integrates ImGui with SDL2 window and OpenGL3 context
	class ImGuiManager
	{
	public:
		ImGuiManager() = default;
		~ImGuiManager();

		// Initialize ImGui with the given SDL window
		// Must be called after OpenGL context is created
		bool initialize(SDL_Window* window);

		// Shutdown ImGui and clean up resources
		void shutdown();

		// Begin a new ImGui frame
		// Call this at the start of each frame before any ImGui calls
		void newFrame();

		// Render ImGui draw data
		// Call this after all ImGui calls and before SDL_GL_SwapWindow
		void render();

		// Process SDL event for ImGui
		// Call this in your event loop for each SDL_Event
		// Returns true if ImGui wants to capture this event
		bool processEvent(const SDL_Event* event);

		// Check if ImGui is initialized
		bool isInitialized() const { return m_initialized; }

	private:
		SDL_Window* m_window = nullptr;
		bool m_initialized = false;
	};
}

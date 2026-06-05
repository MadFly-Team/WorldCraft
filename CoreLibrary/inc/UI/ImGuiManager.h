#pragma once

#include <SDL.h>
#include <imgui.h>
#include <memory>

namespace CoreLib {
namespace UI {

// ---------------------------------------------------------------------------
// ImGuiManager - ImGui initialization and lifecycle management
// ---------------------------------------------------------------------------
class ImGuiManager
{
public:
	ImGuiManager();
	~ImGuiManager();

	// Initialize ImGui with SDL window and OpenGL context
	bool initialize(SDL_Window* window, SDL_GLContext glContext);

	// Shutdown ImGui
	void shutdown();

	// Frame lifecycle
	void newFrame();
	void render();

	// Event processing
	void processEvent(const SDL_Event* event);

	// Style configuration
	void setDarkTheme();
	void setLightTheme();
	void setCustomTheme(const ImVec4& primary, const ImVec4& secondary, const ImVec4& background);

	// State
	bool isInitialized() const { return m_initialized; }

private:
	bool m_initialized;
};

} // namespace UI
} // namespace CoreLib

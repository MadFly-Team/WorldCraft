#pragma once

#include <SDL.h>
#include <string>
#include <memory>

namespace CoreLib {
namespace Core {

// ---------------------------------------------------------------------------
// Screen - Window and OpenGL context management
// ---------------------------------------------------------------------------
class Screen
{
public:
	struct Config
	{
		std::string title = "CoreLibrary Application";
		int width = 1280;
		int height = 720;
		bool fullscreen = false;
		bool vsync = true;
		bool resizable = true;
	};

	Screen();
	~Screen();

	// Initialize screen with configuration
	bool initialize(const Config& config);

	// Shutdown and cleanup
	void shutdown();

	// Window operations
	void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
	void present();
	void setTitle(const std::string& title);
	void toggleFullscreen();
	void setVSync(bool enabled);

	// Getters
	SDL_Window* getWindow() const { return m_window; }
	SDL_GLContext getContext() const { return m_glContext; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	float getAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }
	bool isInitialized() const { return m_initialized; }
	bool isFullscreen() const { return m_fullscreen; }

private:
	SDL_Window* m_window;
	SDL_GLContext m_glContext;
	Config m_config;
	int m_width;
	int m_height;
	bool m_fullscreen;
	bool m_initialized;
};

} // namespace Core
} // namespace CoreLib

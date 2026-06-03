#pragma once

#include <Core/Screen.h>
#include <functional>
#include <memory>

namespace CoreLib {
namespace Core {

// ---------------------------------------------------------------------------
// Application - Main application lifecycle manager
// ---------------------------------------------------------------------------
class Application
{
public:
	using UpdateCallback = std::function<void(float deltaTime)>;
	using RenderCallback = std::function<void()>;
	using EventCallback = std::function<void(const SDL_Event& event)>;

	Application();
	virtual ~Application();

	// Application lifecycle
	bool initialize(const Screen::Config& screenConfig);
	void run();
	void quit();
	void shutdown();

	// Callbacks for application logic
	void setUpdateCallback(UpdateCallback callback) { m_updateCallback = callback; }
	void setRenderCallback(RenderCallback callback) { m_renderCallback = callback; }
	void setEventCallback(EventCallback callback) { m_eventCallback = callback; }

	// Access to screen
	Screen& getScreen() { return *m_screen; }
	const Screen& getScreen() const { return *m_screen; }

	// Time information
	float getDeltaTime() const { return m_deltaTime; }
	float getFPS() const { return m_fps; }
	uint64_t getFrameCount() const { return m_frameCount; }

	// State
	bool isRunning() const { return m_running; }

protected:
	// Virtual methods that can be overridden
	virtual void onInitialize() {}
	virtual void onShutdown() {}
	virtual void onUpdate(float deltaTime) {}
	virtual void onRender() {}
	virtual void onEvent(const SDL_Event& event) {}

private:
	void processEvents();
	void update();
	void render();
	void calculateDeltaTime();

	std::unique_ptr<Screen> m_screen;
	UpdateCallback m_updateCallback;
	RenderCallback m_renderCallback;
	EventCallback m_eventCallback;

	bool m_running;
	bool m_initialized;
	float m_deltaTime;
	float m_fps;
	uint64_t m_lastTime;
	uint64_t m_frameCount;
	uint64_t m_perfFreq;
};

} // namespace Core
} // namespace CoreLib

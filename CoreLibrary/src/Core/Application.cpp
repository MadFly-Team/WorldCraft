#include <Core/Application.h>
#include <glad/gl.h>
#include <iostream>

namespace CoreLib {
namespace Core {

Application::Application()
	: m_screen(std::make_unique<Screen>())
	, m_running(false)
	, m_initialized(false)
	, m_deltaTime(0.0f)
	, m_fps(0.0f)
	, m_lastTime(0)
	, m_frameCount(0)
	, m_perfFreq(0)
{
}

Application::~Application()
{
	shutdown();
}

bool Application::initialize(const Screen::Config& screenConfig)
{
	if (m_initialized)
	{
		std::cerr << "Application already initialized" << std::endl;
		return false;
	}

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
		return false;
	}

	// Initialize screen
	if (!m_screen->initialize(screenConfig))
	{
		std::cerr << "Failed to initialize screen" << std::endl;
		SDL_Quit();
		return false;
	}

	// Initialize timing
	m_perfFreq = SDL_GetPerformanceFrequency();
	m_lastTime = SDL_GetPerformanceCounter();

	m_initialized = true;
	onInitialize();

	std::cout << "Application initialized" << std::endl;
	return true;
}

void Application::run()
{
	if (!m_initialized)
	{
		std::cerr << "Cannot run: Application not initialized" << std::endl;
		return;
	}

	m_running = true;
	std::cout << "Application started" << std::endl;

	while (m_running)
	{
		calculateDeltaTime();
		processEvents();
		update();
		render();
	}

	std::cout << "Application stopped" << std::endl;
}

void Application::quit()
{
	m_running = false;
}

void Application::shutdown()
{
	if (!m_initialized)
		return;

	onShutdown();

	m_screen->shutdown();
	SDL_Quit();

	m_initialized = false;
	std::cout << "Application shutdown complete" << std::endl;
}

void Application::processEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		// Handle quit event
		if (event.type == SDL_QUIT)
		{
			quit();
		}
		// Handle window resize
		else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			int w, h;
			SDL_GetWindowSize(m_screen->getWindow(), &w, &h);
			glViewport(0, 0, w, h);
		}

		// Call custom event handler
		if (m_eventCallback)
			m_eventCallback(event);

		// Call virtual event handler
		onEvent(event);
	}
}

void Application::update()
{
	// Call custom update handler
	if (m_updateCallback)
		m_updateCallback(m_deltaTime);

	// Call virtual update handler
	onUpdate(m_deltaTime);
}

void Application::render()
{
	// Clear screen
	m_screen->clear(0.1f, 0.1f, 0.15f, 1.0f);

	// Call custom render handler
	if (m_renderCallback)
		m_renderCallback();

	// Call virtual render handler
	onRender();

	// Present
	m_screen->present();

	m_frameCount++;
}

void Application::calculateDeltaTime()
{
	uint64_t now = SDL_GetPerformanceCounter();
	m_deltaTime = static_cast<float>(now - m_lastTime) / static_cast<float>(m_perfFreq);
	m_lastTime = now;

	// Calculate FPS (simple moving average over 60 frames)
	static float fpsAccum = 0.0f;
	static int fpsCount = 0;

	if (m_deltaTime > 0.0f)
	{
		fpsAccum += 1.0f / m_deltaTime;
		fpsCount++;

		if (fpsCount >= 60)
		{
			m_fps = fpsAccum / 60.0f;
			fpsAccum = 0.0f;
			fpsCount = 0;
		}
	}
}

} // namespace Core
} // namespace CoreLib

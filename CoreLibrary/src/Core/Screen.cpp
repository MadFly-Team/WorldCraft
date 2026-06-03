#include <Core/Screen.h>
#include <glad/gl.h>
#include <iostream>

namespace CoreLib {
namespace Core {

Screen::Screen()
	: m_window(nullptr)
	, m_glContext(nullptr)
	, m_width(0)
	, m_height(0)
	, m_fullscreen(false)
	, m_initialized(false)
{
}

Screen::~Screen()
{
	shutdown();
}

bool Screen::initialize(const Config& config)
{
	if (m_initialized)
	{
		std::cerr << "Screen already initialized" << std::endl;
		return false;
	}

	m_config = config;
	m_width = config.width;
	m_height = config.height;
	m_fullscreen = config.fullscreen;

	// Initialize SDL video subsystem
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "Failed to initialize SDL video: " << SDL_GetError() << std::endl;
		return false;
	}

	// Set OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// Create window
	Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	if (config.resizable)
		windowFlags |= SDL_WINDOW_RESIZABLE;
	if (config.fullscreen)
		windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	m_window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		m_width,
		m_height,
		windowFlags
	);

	if (!m_window)
	{
		std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		return false;
	}

	// Create OpenGL context
	m_glContext = SDL_GL_CreateContext(m_window);
	if (!m_glContext)
	{
		std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(m_window);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		return false;
	}

	// Initialize GLAD
	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		SDL_GL_DeleteContext(m_glContext);
		SDL_DestroyWindow(m_window);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		return false;
	}

	// Set VSync
	setVSync(config.vsync);

	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_initialized = true;
	std::cout << "Screen initialized: " << m_width << "x" << m_height << std::endl;
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	return true;
}

void Screen::shutdown()
{
	if (!m_initialized)
		return;

	if (m_glContext)
	{
		SDL_GL_DeleteContext(m_glContext);
		m_glContext = nullptr;
	}

	if (m_window)
	{
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	m_initialized = false;
}

void Screen::clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Screen::present()
{
	SDL_GL_SwapWindow(m_window);
}

void Screen::setTitle(const std::string& title)
{
	if (m_window)
	{
		SDL_SetWindowTitle(m_window, title.c_str());
		m_config.title = title;
	}
}

void Screen::toggleFullscreen()
{
	if (!m_window)
		return;

	m_fullscreen = !m_fullscreen;
	SDL_SetWindowFullscreen(m_window, m_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void Screen::setVSync(bool enabled)
{
	SDL_GL_SetSwapInterval(enabled ? 1 : 0);
}

} // namespace Core
} // namespace CoreLib

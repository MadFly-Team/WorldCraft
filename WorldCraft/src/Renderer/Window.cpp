#include <Renderer/Window.h>

namespace Renderer
{

namespace
{

bool g_sdlInitialised = false;
constexpr bool kNoOpenGLIsolationMode = false;

struct WindowState
{
	SDL_GLContext glContext = nullptr;
	bool isFullscreen = false;
	int windowedWidth = 1280;   // Remember windowed mode size
	int windowedHeight = 720;
	int windowedX = SDL_WINDOWPOS_CENTERED;
	int windowedY = SDL_WINDOWPOS_CENTERED;
};

// Store context per window (simplified for single window use)
WindowState g_windowState;

bool ensureSdlInitialised()
{
	if (g_sdlInitialised) return true;
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cerr << "Failed to initialise SDL2: " << SDL_GetError() << "\n";
		return false;
	}
	g_sdlInitialised = true;
	return true;
}

SDL_Window* createWindowInternal(int width, int height, const char* title, bool fullscreen)
{
	// Set OpenGL attributes before window creation
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;

	if (fullscreen)
	{
		// Get desktop display mode for borderless fullscreen
		SDL_DisplayMode displayMode;
		if (SDL_GetDesktopDisplayMode(0, &displayMode) == 0)
		{
			width = displayMode.w;
			height = displayMode.h;
		}
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	SDL_Window* window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		flags
	);

	if (!window)
	{
		std::cerr << "Failed to create SDL2 window: " << SDL_GetError() << "\n";
		return nullptr;
	}

	if (kNoOpenGLIsolationMode)
	{
		return window;
	}

	// Create OpenGL context
	g_windowState.glContext = SDL_GL_CreateContext(window);
	if (!g_windowState.glContext)
	{
		std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << "\n";
		SDL_DestroyWindow(window);
		return nullptr;
	}

	// Make context current
	SDL_GL_MakeCurrent(window, g_windowState.glContext);

	// Load OpenGL functions via GLAD
	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
	{
		std::cerr << "Failed to initialise GLAD\n";
		SDL_GL_DeleteContext(g_windowState.glContext);
		SDL_DestroyWindow(window);
		return nullptr;
	}

	// Enable vsync
	SDL_GL_SetSwapInterval(1);

	g_windowState.isFullscreen = fullscreen;

	// Store initial windowed size if starting in windowed mode
	if (!fullscreen)
	{
		g_windowState.windowedWidth = width;
		g_windowState.windowedHeight = height;
	}

	return window;
}

} // namespace

SDL_Window* createWindow(int width, int height, const char* title)
{
	if (!ensureSdlInitialised()) return nullptr;

	SDL_Window* window = createWindowInternal(width, height, title, false);
	if (!window)
	{
		SDL_Quit();
		return nullptr;
	}

	return window;
}

SDL_Window* createWindow(int width, int height, const char* title, bool fullscreen)
{
	if (!ensureSdlInitialised()) return nullptr;

	return createWindowInternal(width, height, title, fullscreen);
}

void destroyWindow(SDL_Window* window)
{
	if (window)
	{
		if (g_windowState.glContext)
		{
			SDL_GL_DeleteContext(g_windowState.glContext);
			g_windowState.glContext = nullptr;
		}
		SDL_DestroyWindow(window);
	}
}

void shutdown()
{
	if (g_sdlInitialised)
	{
		SDL_Quit();
		g_sdlInitialised = false;
	}
}

glm::ivec2 primaryMonitorSize()
{
	SDL_DisplayMode displayMode;
	if (SDL_GetDesktopDisplayMode(0, &displayMode) != 0)
	{
		return { 0, 0 };
	}
	return { displayMode.w, displayMode.h };
}

void toggleFullscreen(SDL_Window* window)
{
	if (!window) return;

	Uint32 currentFlags = SDL_GetWindowFlags(window);
	bool isCurrentlyFullscreen = (currentFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;

	if (isCurrentlyFullscreen)
	{
		// Switch to windowed mode - restore saved size and position
		SDL_SetWindowFullscreen(window, 0);
		SDL_SetWindowSize(window, g_windowState.windowedWidth, g_windowState.windowedHeight);
		SDL_SetWindowPosition(window, g_windowState.windowedX, g_windowState.windowedY);
		g_windowState.isFullscreen = false;
	}
	else
	{
		// Save current windowed size and position before going fullscreen
		SDL_GetWindowSize(window, &g_windowState.windowedWidth, &g_windowState.windowedHeight);
		SDL_GetWindowPosition(window, &g_windowState.windowedX, &g_windowState.windowedY);

		// Switch to borderless fullscreen
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		g_windowState.isFullscreen = true;
	}

	// Update viewport after toggle
	int w, h;
	SDL_GL_GetDrawableSize(window, &w, &h);
	if (w > 0 && h > 0)
		glViewport(0, 0, w, h);
}

} // namespace Renderer

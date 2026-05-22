#include <UI/ImGuiManager.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

namespace UI
{

ImGuiManager::~ImGuiManager()
{
	shutdown();
}

bool ImGuiManager::initialize(SDL_Window* window)
{
	if (m_initialized)
	{
		std::cerr << "ImGuiManager already initialized\n";
		return false;
	}

	if (!window)
	{
		std::cerr << "Cannot initialize ImGui with null window\n";
		return false;
	}

	m_window = window;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Increase font size for better readability
	io.FontGlobalScale = 1.5f;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Customize style for better visibility
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;
	style.FrameRounding = 3.0f;
	style.ScrollbarRounding = 3.0f;
	style.GrabRounding = 2.0f;
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;

	// Setup Platform/Renderer backends
	const char* glsl_version = "#version 330";  // OpenGL 3.3
	if (!ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext()))
	{
		std::cerr << "Failed to initialize ImGui SDL2 backend\n";
		ImGui::DestroyContext();
		return false;
	}

	if (!ImGui_ImplOpenGL3_Init(glsl_version))
	{
		std::cerr << "Failed to initialize ImGui OpenGL3 backend\n";
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		return false;
	}

	m_initialized = true;
	std::cout << "ImGui initialized successfully\n";
	return true;
}

void ImGuiManager::shutdown()
{
	if (!m_initialized)
		return;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	m_initialized = false;
	m_window = nullptr;
}

void ImGuiManager::newFrame()
{
	if (!m_initialized)
		return;

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::render()
{
	if (!m_initialized)
		return;

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiManager::processEvent(const SDL_Event* event)
{
	if (!m_initialized || !event)
		return false;

	ImGui_ImplSDL2_ProcessEvent(event);

	// Return true if ImGui wants to capture this event
	ImGuiIO& io = ImGui::GetIO();

	if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP || event->type == SDL_TEXTINPUT)
	{
		return io.WantCaptureKeyboard;
	}
	else if (event->type == SDL_MOUSEMOTION || event->type == SDL_MOUSEBUTTONDOWN || 
			 event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEWHEEL)
	{
		return io.WantCaptureMouse;
	}

	return false;
}

}

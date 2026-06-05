#include <UI/ImGuiManager.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

namespace CoreLib {
namespace UI {

ImGuiManager::ImGuiManager()
	: m_initialized(false)
{
}

ImGuiManager::~ImGuiManager()
{
	shutdown();
}

bool ImGuiManager::initialize(SDL_Window* window, SDL_GLContext glContext)
{
	if (m_initialized)
	{
		std::cerr << "ImGuiManager already initialized" << std::endl;
		return false;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init("#version 330");

	// Set default dark theme
	setDarkTheme();

	m_initialized = true;
	std::cout << "ImGuiManager initialized" << std::endl;
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
	std::cout << "ImGuiManager shutdown" << std::endl;
}

void ImGuiManager::newFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::processEvent(const SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

void ImGuiManager::setDarkTheme()
{
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.ScrollbarRounding = 3.0f;

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.15f, 0.95f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.2f, 1.0f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.1f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.3f, 0.5f, 1.0f);
	colors[ImGuiCol_Button] = ImVec4(0.2f, 0.3f, 0.4f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.4f, 0.5f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.1f, 0.5f, 0.7f, 1.0f);
}

void ImGuiManager::setLightTheme()
{
	ImGui::StyleColorsLight();
}

void ImGuiManager::setCustomTheme(const ImVec4& primary, const ImVec4& secondary, const ImVec4& background)
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_WindowBg] = background;
	colors[ImGuiCol_TitleBgActive] = primary;
	colors[ImGuiCol_Button] = secondary;
	colors[ImGuiCol_ButtonHovered] = primary;
	colors[ImGuiCol_ButtonActive] = ImVec4(primary.x * 1.2f, primary.y * 1.2f, primary.z * 1.2f, primary.w);
}

} // namespace UI
} // namespace CoreLib

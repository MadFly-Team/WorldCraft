#include <UI/Window.h>

namespace CoreLib {
namespace UI {

Window::Window(const std::string& title)
	: m_title(title)
	, m_isOpen(true)
	, m_flags(0)
	, m_hasPosition(false)
	, m_position(0, 0)
	, m_hasSize(false)
	, m_size(0, 0)
{
}

void Window::begin()
{
	if (!m_isOpen)
		return;

	// Apply style
	ImGui::PushStyleColor(ImGuiCol_WindowBg, m_style.backgroundColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, m_style.rounding);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, m_style.padding);

	// Set position and size if specified
	if (m_hasPosition)
	{
		ImGui::SetNextWindowPos(m_position, ImGuiCond_FirstUseEver);
	}
	if (m_hasSize)
	{
		ImGui::SetNextWindowSize(m_size, ImGuiCond_FirstUseEver);
	}

	// Begin window
	if (!ImGui::Begin(m_title.c_str(), &m_isOpen, m_flags))
	{
		// Window is collapsed
		ImGui::End();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
		return;
	}

	// Render content
	if (m_contentCallback)
	{
		m_contentCallback();
	}
	else
	{
		renderContent();
	}
}

void Window::end()
{
	if (!m_isOpen)
		return;

	ImGui::End();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
}

void Window::setPosition(float x, float y)
{
	m_hasPosition = true;
	m_position = ImVec2(x, y);
}

void Window::setSize(float width, float height)
{
	m_hasSize = true;
	m_size = ImVec2(width, height);
}

void Window::setPositionAndSize(float x, float y, float width, float height)
{
	setPosition(x, y);
	setSize(width, height);
}

void Window::center()
{
	ImGuiIO& io = ImGui::GetIO();
	float centerX = io.DisplaySize.x * 0.5f;
	float centerY = io.DisplaySize.y * 0.5f;

	if (m_hasSize)
	{
		setPosition(centerX - m_size.x * 0.5f, centerY - m_size.y * 0.5f);
	}
	else
	{
		setPosition(centerX, centerY);
	}
}

} // namespace UI
} // namespace CoreLib

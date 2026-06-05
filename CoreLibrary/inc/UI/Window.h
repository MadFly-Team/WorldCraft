#pragma once

#include <imgui.h>
#include <string>
#include <functional>

namespace CoreLib {
namespace UI {

// ---------------------------------------------------------------------------
// Window - ImGui window wrapper with common functionality
// ---------------------------------------------------------------------------
class Window
{
public:
	struct Style
	{
		ImVec4 backgroundColor = ImVec4(0.1f, 0.1f, 0.15f, 0.95f);
		ImVec4 titleColor = ImVec4(0.0f, 0.8f, 1.0f, 1.0f);
		float rounding = 5.0f;
		ImVec2 padding = ImVec2(10, 10);
	};

	Window(const std::string& title);
	virtual ~Window() = default;

	// Window lifecycle
	void begin();
	void end();
	bool isOpen() const { return m_isOpen; }
	void setOpen(bool open) { m_isOpen = open; }
	void toggle() { m_isOpen = !m_isOpen; }

	// Position and size
	void setPosition(float x, float y);
	void setSize(float width, float height);
	void setPositionAndSize(float x, float y, float width, float height);
	void center();

	// Style
	void setStyle(const Style& style) { m_style = style; }
	const Style& getStyle() const { return m_style; }

	// Flags
	void setFlags(ImGuiWindowFlags flags) { m_flags = flags; }
	ImGuiWindowFlags getFlags() const { return m_flags; }
	void addFlags(ImGuiWindowFlags flags) { m_flags |= flags; }
	void removeFlags(ImGuiWindowFlags flags) { m_flags &= ~flags; }

	// Title
	void setTitle(const std::string& title) { m_title = title; }
	const std::string& getTitle() const { return m_title; }

	// Content callback (for derived classes or lambda-based windows)
	using ContentCallback = std::function<void()>;
	void setContentCallback(ContentCallback callback) { m_contentCallback = callback; }

protected:
	virtual void renderContent() {}

private:
	std::string m_title;
	bool m_isOpen;
	ImGuiWindowFlags m_flags;
	Style m_style;
	ContentCallback m_contentCallback;
	bool m_hasPosition;
	ImVec2 m_position;
	bool m_hasSize;
	ImVec2 m_size;
};

} // namespace UI
} // namespace CoreLib

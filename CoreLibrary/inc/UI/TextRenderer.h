#pragma once

#include <imgui.h>
#include <string>

namespace CoreLib {
namespace UI {

// ---------------------------------------------------------------------------
// TextRenderer - Utility for rendering text with ImGui
// ---------------------------------------------------------------------------
class TextRenderer
{
public:
	// Text rendering
	static void text(const std::string& text);
	static void textColored(const ImVec4& color, const std::string& text);
	static void textWrapped(const std::string& text);
	static void textDisabled(const std::string& text);
	static void bulletText(const std::string& text);

	// Formatted text
	static void textFormatted(const char* fmt, ...);
	static void textColoredFormatted(const ImVec4& color, const char* fmt, ...);

	// Labels
	static void labelText(const std::string& label, const std::string& text);

	// Headings
	static void heading(const std::string& text, const ImVec4& color = ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
	static void subheading(const std::string& text, const ImVec4& color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

	// Separators
	static void separator();
	static void separatorText(const std::string& text);

	// Spacing
	static void spacing(int count = 1);
	static void newLine();
	static void sameLine();

	// Input text
	static bool inputText(const std::string& label, std::string& text, size_t maxLength = 256);
	static bool inputTextMultiline(const std::string& label, std::string& text, const ImVec2& size = ImVec2(0, 0));

	// Tooltips
	static void beginTooltip();
	static void endTooltip();
	static void tooltip(const std::string& text);
	static void setItemTooltip(const std::string& text);
};

} // namespace UI
} // namespace CoreLib

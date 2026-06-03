#include <UI/TextRenderer.h>
#include <cstdarg>
#include <vector>

namespace CoreLib {
namespace UI {

void TextRenderer::text(const std::string& text)
{
	ImGui::TextUnformatted(text.c_str());
}

void TextRenderer::textColored(const ImVec4& color, const std::string& text)
{
	ImGui::TextColored(color, "%s", text.c_str());
}

void TextRenderer::textWrapped(const std::string& text)
{
	ImGui::TextWrapped("%s", text.c_str());
}

void TextRenderer::textDisabled(const std::string& text)
{
	ImGui::TextDisabled("%s", text.c_str());
}

void TextRenderer::bulletText(const std::string& text)
{
	ImGui::BulletText("%s", text.c_str());
}

void TextRenderer::textFormatted(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt, args);
	va_end(args);
}

void TextRenderer::textColoredFormatted(const ImVec4& color, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui::TextColoredV(color, fmt, args);
	va_end(args);
}

void TextRenderer::labelText(const std::string& label, const std::string& text)
{
	ImGui::LabelText(label.c_str(), "%s", text.c_str());
}

void TextRenderer::heading(const std::string& text, const ImVec4& color)
{
	ImGui::PushFont(ImGui::GetFont());
	ImGui::SetWindowFontScale(1.5f);
	ImGui::TextColored(color, "%s", text.c_str());
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopFont();
}

void TextRenderer::subheading(const std::string& text, const ImVec4& color)
{
	ImGui::PushFont(ImGui::GetFont());
	ImGui::SetWindowFontScale(1.2f);
	ImGui::TextColored(color, "%s", text.c_str());
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopFont();
}

void TextRenderer::separator()
{
	ImGui::Separator();
}

void TextRenderer::separatorText(const std::string& text)
{
	ImGui::SeparatorText(text.c_str());
}

void TextRenderer::spacing(int count)
{
	for (int i = 0; i < count; ++i)
	{
		ImGui::Spacing();
	}
}

void TextRenderer::newLine()
{
	ImGui::NewLine();
}

void TextRenderer::sameLine()
{
	ImGui::SameLine();
}

bool TextRenderer::inputText(const std::string& label, std::string& text, size_t maxLength)
{
	std::vector<char> buffer(maxLength + 1);
	std::strncpy(buffer.data(), text.c_str(), maxLength);
	buffer[maxLength] = '\0';

	if (ImGui::InputText(label.c_str(), buffer.data(), buffer.size()))
	{
		text = std::string(buffer.data());
		return true;
	}
	return false;
}

bool TextRenderer::inputTextMultiline(const std::string& label, std::string& text, const ImVec2& size)
{
	std::vector<char> buffer(text.size() + 256);
	std::strncpy(buffer.data(), text.c_str(), buffer.size() - 1);
	buffer[buffer.size() - 1] = '\0';

	if (ImGui::InputTextMultiline(label.c_str(), buffer.data(), buffer.size(), size))
	{
		text = std::string(buffer.data());
		return true;
	}
	return false;
}

void TextRenderer::beginTooltip()
{
	ImGui::BeginTooltip();
}

void TextRenderer::endTooltip()
{
	ImGui::EndTooltip();
}

void TextRenderer::tooltip(const std::string& text)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(text.c_str());
		ImGui::EndTooltip();
	}
}

void TextRenderer::setItemTooltip(const std::string& text)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", text.c_str());
	}
}

} // namespace UI
} // namespace CoreLib

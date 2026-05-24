#include <UI/GameMenu.h>
#include <cmath>
#include <algorithm>

namespace UI
{

GameMenu::GameMenu()
	: m_isVisible(false)
	, m_selectedIndex(0)
	, m_hoveredIndex(-1)
	, m_animTime(0.0f)
	, m_modifiedChunksCount(0)
{
	// Initialize menu items
	m_menuItems = {
		{ "CONTINUE", MenuAction::Continue, ImVec4(0.3f, 1.0f, 0.3f, 1.0f) },
		{ "CREATE NEW WORLD", MenuAction::CreateNewWorld, ImVec4(0.3f, 0.6f, 1.0f, 1.0f) },
		{ "LOAD WORLD", MenuAction::LoadWorld, ImVec4(1.0f, 0.8f, 0.3f, 1.0f) },
		{ "SAVE WORLD", MenuAction::SaveWorld, ImVec4(1.0f, 0.4f, 0.3f, 1.0f) }
	};
}

void GameMenu::render(float deltaTime, int screenWidth, int screenHeight)
{
	if (!m_isVisible) return;

	m_animTime += deltaTime;

	// Dark overlay
	renderDarkOverlay(screenWidth, screenHeight);

	// Calculate center positions
	float centerX = screenWidth * 0.5f;
	float centerY = screenHeight * 0.5f;
	float titleY = centerY - 150.0f;

	// Render title
	renderTitle(centerX, titleY, screenWidth);

	// Render menu items
	renderMenuItems(centerX, centerY, screenWidth, screenHeight);
}

void GameMenu::renderDarkOverlay(int screenWidth, int screenHeight)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	drawList->AddRectFilled(
		ImVec2(0, 0),
		ImVec2(static_cast<float>(screenWidth), static_cast<float>(screenHeight)),
		IM_COL32(0, 0, 0, 200)  // Semi-transparent black
	);
}

void GameMenu::renderTitle(float centerX, float titleY, int screenWidth)
{
	std::string title = "WORLDCRAFT";
	float scale = 3.0f;
	float charWidth = 6.0f * scale;
	float totalWidth = title.length() * charWidth;
	float startX = centerX - totalWidth * 0.5f;

	// Render title with rainbow wave effect
	for (size_t i = 0; i < title.length(); ++i)
	{
		float phase = m_animTime * 2.0f + i * 0.3f;
		float hue = std::fmod(m_animTime * 0.5f + i * 0.1f, 1.0f);

		// HSV to RGB for rainbow effect
		float r, g, b;
		float h = hue * 6.0f;
		float c = 1.0f;
		float x = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));

		if (h < 1.0f) { r = c; g = x; b = 0; }
		else if (h < 2.0f) { r = x; g = c; b = 0; }
		else if (h < 3.0f) { r = 0; g = c; b = x; }
		else if (h < 4.0f) { r = 0; g = x; b = c; }
		else if (h < 5.0f) { r = x; g = 0; b = c; }
		else { r = c; g = 0; b = x; }

		ImVec4 color(r, g, b, 1.0f);
		renderBlockChar(title[i], startX + i * charWidth, titleY, scale, color, phase);
	}
}

void GameMenu::renderMenuItems(float centerX, float centerY, int screenWidth, int screenHeight)
{
	float itemSpacing = 60.0f;
	float startY = centerY - (m_menuItems.size() * itemSpacing * 0.5f);

	m_itemLayouts.clear();

	for (size_t i = 0; i < m_menuItems.size(); ++i)
	{
		const auto& item = m_menuItems[i];
		float y = startY + i * itemSpacing;

		// Calculate item dimensions
		float scale = 2.0f;
		float charWidth = 6.0f * scale;
		float totalWidth = item.text.length() * charWidth;
		float x = centerX - totalWidth * 0.5f;

		// Store layout for mouse interaction
		m_itemLayouts.push_back({ x, y, totalWidth, 8.0f * scale });

		// Determine color based on selection/hover
		ImVec4 color = item.color;
		bool isSelected = (static_cast<int>(i) == m_selectedIndex);
		bool isHovered = (static_cast<int>(i) == m_hoveredIndex);

		if (isSelected || isHovered)
		{
			// Brighten selected/hovered item
			color.x = std::min(1.0f, color.x * 1.5f);
			color.y = std::min(1.0f, color.y * 1.5f);
			color.z = std::min(1.0f, color.z * 1.5f);
			scale = 2.2f;
		}

		// Add pulsing effect to selected item
		if (isSelected)
		{
			float pulse = 0.2f * std::sin(m_animTime * 4.0f);
			scale += pulse;
		}

		// Render text with wobble effect
		float wobblePhase = m_animTime * 3.0f + i * 0.5f;

		// Recalculate position with new scale
		totalWidth = item.text.length() * 6.0f * scale;
		x = centerX - totalWidth * 0.5f;

		renderBlockText(item.text, x, y, scale, color, wobblePhase);

		// Show modified chunks count next to save option
		if (item.action == MenuAction::SaveWorld && m_modifiedChunksCount > 0)
		{
			std::string countText = "(" + std::to_string(m_modifiedChunksCount) + ")";
			float countX = x + totalWidth + 20.0f;
			ImVec4 countColor(1.0f, 1.0f, 0.3f, 1.0f);
			renderBlockText(countText, countX, y, 1.5f, countColor, wobblePhase);
		}
	}
}

void GameMenu::renderBlockText(const std::string& text, float x, float y, float scale, const ImVec4& color, float wobblePhase)
{
	float charWidth = 6.0f * scale;
	for (size_t i = 0; i < text.length(); ++i)
	{
		float charX = x + i * charWidth;

		// Apply wobble offset
		float wobble = std::sin(wobblePhase + i * 0.4f) * 2.0f * scale;

		renderBlockChar(text[i], charX, y + wobble, scale, color, wobblePhase + i * 0.2f);
	}
}

void GameMenu::renderBlockChar(char c, float x, float y, float scale, const ImVec4& color, float wobblePhase)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

	// Each character is 5x7 grid of blocks
	float blockSize = scale;

	for (int py = 0; py < 7; ++py)
	{
		for (int px = 0; px < 5; ++px)
		{
			if (getCharPixel(c, px, py))
			{
				// Add micro-wobble to individual blocks
				float microWobble = std::sin(wobblePhase + px * 0.1f + py * 0.1f) * 0.3f * scale;

				float blockX = x + px * blockSize + microWobble;
				float blockY = y + py * blockSize;

				// Draw block with slight gap for retro effect
				float gap = 0.2f * scale;
				drawList->AddRectFilled(
					ImVec2(blockX + gap, blockY + gap),
					ImVec2(blockX + blockSize - gap, blockY + blockSize - gap),
					col
				);
			}
		}
	}
}

bool GameMenu::getCharPixel(char c, int x, int y) const
{
	// 5x7 block font patterns (each row is a 5-bit pattern)
	// Returns true if pixel should be drawn

	static const unsigned char font[][7] = {
		// A
		{ 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 },
		// B
		{ 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 },
		// C
		{ 0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110 },
		// D
		{ 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 },
		// E
		{ 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 },
		// F
		{ 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 },
		// G
		{ 0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110 },
		// H
		{ 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 },
		// I
		{ 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111 },
		// J
		{ 0b11111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 },
		// K
		{ 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 },
		// L
		{ 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 },
		// M
		{ 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 },
		// N
		{ 0b10001, 0b11001, 0b10101, 0b10101, 0b10011, 0b10001, 0b10001 },
		// O
		{ 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 },
		// P
		{ 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 },
		// Q
		{ 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 },
		// R
		{ 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 },
		// S
		{ 0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110 },
		// T
		{ 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 },
		// U
		{ 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 },
		// V
		{ 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 },
		// W
		{ 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001 },
		// X
		{ 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 },
		// Y
		{ 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 },
		// Z
		{ 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 },
	};

	// Numbers 0-9
	static const unsigned char numbers[][7] = {
		// 0
		{ 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 },
		// 1
		{ 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 },
		// 2
		{ 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 },
		// 3
		{ 0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110 },
		// 4
		{ 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 },
		// 5
		{ 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 },
		// 6
		{ 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 },
		// 7
		{ 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 },
		// 8
		{ 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 },
		// 9
		{ 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 },
	};

	// Special characters
	static const unsigned char space[7] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 };
	static const unsigned char lparen[7] = { 0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010 };
	static const unsigned char rparen[7] = { 0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000 };

	const unsigned char* pattern = nullptr;

	if (c >= 'A' && c <= 'Z')
	{
		pattern = font[c - 'A'];
	}
	else if (c >= 'a' && c <= 'z')
	{
		pattern = font[c - 'a'];  // Use uppercase patterns for lowercase
	}
	else if (c >= '0' && c <= '9')
	{
		pattern = numbers[c - '0'];
	}
	else if (c == ' ')
	{
		pattern = space;
	}
	else if (c == '(')
	{
		pattern = lparen;
	}
	else if (c == ')')
	{
		pattern = rparen;
	}
	else
	{
		return false;  // Unknown character
	}

	if (y < 0 || y >= 7 || x < 0 || x >= 5) return false;

	return (pattern[y] & (1 << (4 - x))) != 0;
}

void GameMenu::handleKeyPress(int key)
{
	if (!m_isVisible) return;

	// SDLK constants
	const int SDLK_UP = 1073741906;
	const int SDLK_DOWN = 1073741905;
	const int SDLK_RETURN = 13;
	const int SDLK_KP_ENTER = 1073741912;

	if (key == SDLK_UP)
	{
		m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_menuItems.size())) % static_cast<int>(m_menuItems.size());
	}
	else if (key == SDLK_DOWN)
	{
		m_selectedIndex = (m_selectedIndex + 1) % static_cast<int>(m_menuItems.size());
	}
	else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
	{
		if (m_actionCallback && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_menuItems.size()))
		{
			m_actionCallback(m_menuItems[m_selectedIndex].action);
		}
	}
}

void GameMenu::handleMouseMove(float mouseX, float mouseY)
{
	if (!m_isVisible) return;

	m_hoveredIndex = -1;
	for (size_t i = 0; i < m_itemLayouts.size(); ++i)
	{
		const auto& layout = m_itemLayouts[i];
		if (mouseX >= layout.x && mouseX <= layout.x + layout.width &&
			mouseY >= layout.y && mouseY <= layout.y + layout.height)
		{
			m_hoveredIndex = static_cast<int>(i);
			m_selectedIndex = static_cast<int>(i);
			break;
		}
	}
}

void GameMenu::handleMouseClick(float mouseX, float mouseY)
{
	if (!m_isVisible) return;

	for (size_t i = 0; i < m_itemLayouts.size(); ++i)
	{
		const auto& layout = m_itemLayouts[i];
		if (mouseX >= layout.x && mouseX <= layout.x + layout.width &&
			mouseY >= layout.y && mouseY <= layout.y + layout.height)
		{
			if (m_actionCallback)
			{
				m_actionCallback(m_menuItems[i].action);
			}
			break;
		}
	}
}

} // namespace UI

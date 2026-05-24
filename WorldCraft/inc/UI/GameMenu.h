#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace UI
{
	enum class MenuAction
	{
		None,
		Continue,
		CreateNewWorld,
		LoadWorld,
		SaveWorld
	};

	// Retro-style game menu with custom block font and effects
	class GameMenu
	{
	public:
		using MenuActionCallback = std::function<void(MenuAction)>;

		GameMenu();

		// Render the menu (call every frame when active)
		void render(float deltaTime, int screenWidth, int screenHeight);

		// Toggle menu visibility
		void show() { m_isVisible = true; m_selectedIndex = 0; }
		void hide() { m_isVisible = false; }
		void toggle() { m_isVisible ? hide() : show(); }
		bool isVisible() const { return m_isVisible; }

		// Handle input
		void handleKeyPress(int key);
		void handleMouseMove(float mouseX, float mouseY);
		void handleMouseClick(float mouseX, float mouseY);

		// Set callback for menu actions
		void setActionCallback(MenuActionCallback callback) { m_actionCallback = callback; }

		// Update modified chunks count for save option
		void setModifiedChunksCount(int count) { m_modifiedChunksCount = count; }

	private:
		struct MenuItem
		{
			std::string text;
			MenuAction action;
			ImVec4 color;
		};

		// Block font rendering
		void renderBlockText(const std::string& text, float x, float y, float scale, const ImVec4& color, float wobblePhase);
		void renderBlockChar(char c, float x, float y, float scale, const ImVec4& color, float wobblePhase);
		bool getCharPixel(char c, int x, int y) const;

		// Menu rendering
		void renderDarkOverlay(int screenWidth, int screenHeight);
		void renderMenuItems(float centerX, float centerY, int screenWidth, int screenHeight);
		void renderTitle(float centerX, float titleY, int screenWidth);

		// Menu state
		bool m_isVisible;
		int m_selectedIndex;
		int m_hoveredIndex;
		float m_animTime;
		int m_modifiedChunksCount;

		std::vector<MenuItem> m_menuItems;
		MenuActionCallback m_actionCallback;

		// Layout cached for mouse interaction
		struct ItemLayout
		{
			float x, y, width, height;
		};
		std::vector<ItemLayout> m_itemLayouts;
	};
}

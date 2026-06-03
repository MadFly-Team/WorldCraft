#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace CoreLib {
namespace UI {

// ---------------------------------------------------------------------------
// Menu - Menu bar and menu items wrapper
// ---------------------------------------------------------------------------
class Menu
{
public:
	struct MenuItem
	{
		std::string label;
		std::string shortcut;
		std::function<void()> action;
		bool enabled = true;
		bool separator = false;
		std::vector<MenuItem> submenu;

		MenuItem() = default;
		MenuItem(const std::string& lbl, std::function<void()> act = nullptr, const std::string& sc = "")
			: label(lbl), shortcut(sc), action(act), enabled(true), separator(false) {}

		static MenuItem Separator() 
		{ 
			MenuItem item;
			item.separator = true;
			return item;
		}
	};

	Menu();

	// Menu bar rendering
	void beginMenuBar();
	void endMenuBar();

	// Add menu items
	void addMenu(const std::string& label, const std::vector<MenuItem>& items);
	void clearMenus();

	// Direct rendering helpers
	static bool beginMenu(const std::string& label, bool enabled = true);
	static void endMenu();
	static bool menuItem(const std::string& label, const std::string& shortcut = "", bool selected = false, bool enabled = true);
	static void separator();

	// Render all registered menus (with BeginMenuBar/EndMenuBar)
	void render();

	// Render all registered menus (without Begin/End, for use inside an existing menu bar)
	void renderMenus();

private:
	struct MenuData
	{
		std::string label;
		std::vector<MenuItem> items;
	};

	std::vector<MenuData> m_menus;

	void renderMenuItem(const MenuItem& item);
};

} // namespace UI
} // namespace CoreLib

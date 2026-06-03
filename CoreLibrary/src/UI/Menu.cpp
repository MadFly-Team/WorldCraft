#include <UI/Menu.h>

namespace CoreLib {
namespace UI {

Menu::Menu()
{
}

void Menu::beginMenuBar()
{
	ImGui::BeginMenuBar();
}

void Menu::endMenuBar()
{
	ImGui::EndMenuBar();
}

void Menu::addMenu(const std::string& label, const std::vector<MenuItem>& items)
{
	MenuData menu;
	menu.label = label;
	menu.items = items;
	m_menus.push_back(menu);
}

void Menu::clearMenus()
{
	m_menus.clear();
}

bool Menu::beginMenu(const std::string& label, bool enabled)
{
	return ImGui::BeginMenu(label.c_str(), enabled);
}

void Menu::endMenu()
{
	ImGui::EndMenu();
}

bool Menu::menuItem(const std::string& label, const std::string& shortcut, bool selected, bool enabled)
{
	return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
}

void Menu::separator()
{
	ImGui::Separator();
}

void Menu::render()
{
	if (ImGui::BeginMenuBar())
	{
		for (const auto& menu : m_menus)
		{
			if (ImGui::BeginMenu(menu.label.c_str()))
			{
				for (const auto& item : menu.items)
				{
					renderMenuItem(item);
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenuBar();
	}
}

void Menu::renderMenus()
{
	for (const auto& menu : m_menus)
	{
		if (ImGui::BeginMenu(menu.label.c_str()))
		{
			for (const auto& item : menu.items)
			{
				renderMenuItem(item);
			}
			ImGui::EndMenu();
		}
	}
}

void Menu::renderMenuItem(const MenuItem& item)
{
	if (item.separator)
	{
		ImGui::Separator();
		return;
	}

	if (!item.submenu.empty())
	{
		if (ImGui::BeginMenu(item.label.c_str(), item.enabled))
		{
			for (const auto& subitem : item.submenu)
			{
				renderMenuItem(subitem);
			}
			ImGui::EndMenu();
		}
	}
	else
	{
		if (ImGui::MenuItem(item.label.c_str(), 
						   item.shortcut.empty() ? nullptr : item.shortcut.c_str(), 
						   false, 
						   item.enabled))
		{
			if (item.action)
			{
				item.action();
			}
		}
	}
}

} // namespace UI
} // namespace CoreLib

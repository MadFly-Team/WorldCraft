#include <UI/Dropdown.h>

namespace CoreLib {
namespace UI {

// ---------------------------------------------------------------------------
// Dropdown
// ---------------------------------------------------------------------------

Dropdown::Dropdown(const std::string& label)
	: m_label(label)
	, m_selectedIndex(-1)
{
}

void Dropdown::setItems(const std::vector<std::string>& items)
{
	m_items = items;
	if (m_selectedIndex >= static_cast<int>(m_items.size()))
	{
		m_selectedIndex = m_items.empty() ? -1 : 0;
	}
}

void Dropdown::addItem(const std::string& item)
{
	m_items.push_back(item);
	if (m_selectedIndex < 0 && !m_items.empty())
	{
		m_selectedIndex = 0;
	}
}

void Dropdown::clearItems()
{
	m_items.clear();
	m_selectedIndex = -1;
}

void Dropdown::setSelectedIndex(int index)
{
	if (index >= 0 && index < static_cast<int>(m_items.size()))
	{
		m_selectedIndex = index;
	}
}

std::string Dropdown::getSelectedValue() const
{
	if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
	{
		return m_items[m_selectedIndex];
	}
	return "";
}

bool Dropdown::render()
{
	if (m_items.empty())
		return false;

	const char* preview = (m_selectedIndex >= 0) ? m_items[m_selectedIndex].c_str() : "";

	bool changed = false;
	if (ImGui::BeginCombo(m_label.c_str(), preview))
	{
		for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
		{
			bool isSelected = (i == m_selectedIndex);
			if (ImGui::Selectable(m_items[i].c_str(), isSelected))
			{
				m_selectedIndex = i;
				changed = true;

				if (m_callback)
				{
					m_callback(i, m_items[i]);
				}
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	return changed;
}

// ---------------------------------------------------------------------------
// ListBox
// ---------------------------------------------------------------------------

ListBox::ListBox(const std::string& label, float height)
	: m_label(label)
	, m_selectedIndex(-1)
	, m_height(height)
{
}

void ListBox::setItems(const std::vector<std::string>& items)
{
	m_items = items;
	if (m_selectedIndex >= static_cast<int>(m_items.size()))
	{
		m_selectedIndex = m_items.empty() ? -1 : 0;
	}
}

void ListBox::addItem(const std::string& item)
{
	m_items.push_back(item);
	if (m_selectedIndex < 0 && !m_items.empty())
	{
		m_selectedIndex = 0;
	}
}

void ListBox::clearItems()
{
	m_items.clear();
	m_selectedIndex = -1;
}

void ListBox::setSelectedIndex(int index)
{
	if (index >= 0 && index < static_cast<int>(m_items.size()))
	{
		m_selectedIndex = index;
	}
}

std::string ListBox::getSelectedValue() const
{
	if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
	{
		return m_items[m_selectedIndex];
	}
	return "";
}

bool ListBox::render()
{
	if (m_items.empty())
		return false;

	bool changed = false;
	if (ImGui::BeginListBox(m_label.c_str(), ImVec2(-1, m_height)))
	{
		for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
		{
			bool isSelected = (i == m_selectedIndex);
			if (ImGui::Selectable(m_items[i].c_str(), isSelected))
			{
				m_selectedIndex = i;
				changed = true;

				if (m_callback)
				{
					m_callback(i, m_items[i]);
				}
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndListBox();
	}

	return changed;
}

} // namespace UI
} // namespace CoreLib

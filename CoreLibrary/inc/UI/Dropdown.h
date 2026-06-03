#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace CoreLib {
namespace UI {

// ---------------------------------------------------------------------------
// Dropdown - Combo box / dropdown menu wrapper
// ---------------------------------------------------------------------------
class Dropdown
{
public:
	using SelectionCallback = std::function<void(int index, const std::string& value)>;

	Dropdown(const std::string& label);

	// Items management
	void setItems(const std::vector<std::string>& items);
	void addItem(const std::string& item);
	void clearItems();

	// Selection
	void setSelectedIndex(int index);
	int getSelectedIndex() const { return m_selectedIndex; }
	std::string getSelectedValue() const;

	// Callbacks
	void setSelectionCallback(SelectionCallback callback) { m_callback = callback; }

	// Rendering
	bool render();

private:
	std::string m_label;
	std::vector<std::string> m_items;
	int m_selectedIndex;
	SelectionCallback m_callback;
};

// ---------------------------------------------------------------------------
// ListBox - List box wrapper
// ---------------------------------------------------------------------------
class ListBox
{
public:
	using SelectionCallback = std::function<void(int index, const std::string& value)>;

	ListBox(const std::string& label, float height = -1.0f);

	// Items management
	void setItems(const std::vector<std::string>& items);
	void addItem(const std::string& item);
	void clearItems();

	// Selection
	void setSelectedIndex(int index);
	int getSelectedIndex() const { return m_selectedIndex; }
	std::string getSelectedValue() const;

	// Callbacks
	void setSelectionCallback(SelectionCallback callback) { m_callback = callback; }

	// Rendering
	bool render();

private:
	std::string m_label;
	std::vector<std::string> m_items;
	int m_selectedIndex;
	float m_height;
	SelectionCallback m_callback;
};

} // namespace UI
} // namespace CoreLib

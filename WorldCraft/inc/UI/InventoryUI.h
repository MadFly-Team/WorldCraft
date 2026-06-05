#pragma once

#include <Inventory/Inventory.h>

namespace UI
{

// ---------------------------------------------------------------------------
// InventoryUI - Simple inventory display screen (I key)
// Shows all inventory slots with their contents
// ---------------------------------------------------------------------------
class InventoryUI
{
public:
	InventoryUI();

	// Show/hide the inventory UI
	void show() { m_isOpen = true; }
	void hide() { m_isOpen = false; }
	void close() { m_isOpen = false; }
	bool isOpen() const { return m_isOpen; }
	void toggle() { m_isOpen = !m_isOpen; }

	// Render the inventory UI
	void render(Inventory::PlayerInventory& inventory);

private:
	bool m_isOpen;
};

} // namespace UI

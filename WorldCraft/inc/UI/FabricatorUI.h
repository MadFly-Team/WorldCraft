#pragma once

#include <Inventory/Inventory.h>

namespace UI
{

// ---------------------------------------------------------------------------
// FabricatorUI - Full inventory management screen (Right-click in air)
// ---------------------------------------------------------------------------
class FabricatorUI
{
public:
	FabricatorUI();

	// Show/hide the fabricator UI
	void show();
	void hide();
	bool isOpen() const { return m_isOpen; }
	void toggle() { m_isOpen = !m_isOpen; }

	// Render the fabricator UI
	void render(Inventory::PlayerInventory& inventory);

private:
	bool m_isOpen;
	Inventory::MaterialCategory m_selectedCategory;

	// UI state
	int m_hoveredMaterialIndex;
	int m_hoveredHotbarSlot;

	// Render category list
	void renderCategoryList();

	// Render material grid for selected category
	void renderMaterialGrid(Inventory::PlayerInventory& inventory);

	// Render hotbar slots at bottom
	void renderHotbarSlots(Inventory::PlayerInventory& inventory);
};

} // namespace UI

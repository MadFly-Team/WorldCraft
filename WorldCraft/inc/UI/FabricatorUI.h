#pragma once

#include <Inventory/Inventory.h>
#include <Crafting/Blueprint.h>

namespace UI
{

// ---------------------------------------------------------------------------
// FabricatorUI - Blueprint crafting and inventory management screen
// ---------------------------------------------------------------------------
class FabricatorUI
{
public:
	FabricatorUI();

	// Show/hide the fabricator UI
	void show();
	void hide();
	void close() { m_isOpen = false; }
	bool isOpen() const { return m_isOpen; }
	void toggle() { m_isOpen = !m_isOpen; }

	// Render the fabricator UI
	void render(Inventory::PlayerInventory& inventory);

private:
	bool m_isOpen;
	Crafting::BlueprintRarity m_selectedRarity;

	// UI state
	int m_hoveredBlueprintIndex;
	const Crafting::Blueprint* m_selectedBlueprint;

	// Render rarity tier list
	void renderRarityList();

	// Render blueprint grid for selected rarity
	void renderBlueprintGrid(Inventory::PlayerInventory& inventory);

	// Render crafting panel (selected blueprint + craft button)
	void renderCraftingPanel(Inventory::PlayerInventory& inventory);
};

} // namespace UI


#pragma once

#include <Inventory/Inventory.h>

namespace UI
{

// ---------------------------------------------------------------------------
// Hotbar HUD - Displays 9-slot inventory bar at bottom of screen
// ---------------------------------------------------------------------------
class Hotbar
{
public:
	Hotbar();

	// Render the hotbar (call every frame)
	void render(const Inventory::PlayerInventory& inventory, int screenWidth, int screenHeight);

	// Show/hide the hotbar
	void show() { m_visible = true; }
	void hide() { m_visible = false; }
	bool isVisible() const { return m_visible; }

private:
	bool m_visible;

	// Visual settings
	static constexpr float SLOT_SIZE = 50.0f;
	static constexpr float SLOT_PADDING = 5.0f;
	static constexpr float HOTBAR_PADDING_BOTTOM = 20.0f;

	// Render a single hotbar slot
	void renderSlot(int slotIndex, const Inventory::Material& material, 
					bool isSelected, float posX, float posY);
};

} // namespace UI

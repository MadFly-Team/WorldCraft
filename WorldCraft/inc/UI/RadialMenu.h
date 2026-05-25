#pragma once

#include <Inventory/Inventory.h>
#include <glm/glm.hpp>

namespace UI
{

// ---------------------------------------------------------------------------
// Radial Menu - Quick material selection (Hold Q)
// ---------------------------------------------------------------------------
class RadialMenu
{
public:
	RadialMenu();

	// Show/hide menu
	void show();
	void hide();
	bool isVisible() const { return m_visible; }

	// Update (call every frame while visible)
	// Returns true if a material was selected
	bool update(Inventory::PlayerInventory& inventory, float mouseX, float mouseY, int screenWidth, int screenHeight);

	// Attempt to select the currently hovered material and equip it to the hotbar
	// Returns true if a material was selected
	bool selectHoveredMaterial(Inventory::PlayerInventory& inventory);

	// Render the menu
	void render(int screenWidth, int screenHeight);

private:
	bool m_visible;

	// Selected category and material indices
	int m_hoveredCategoryIndex;
	int m_hoveredMaterialIndex;

	// Menu layout
	static constexpr float INNER_RADIUS = 80.0f;
	static constexpr float OUTER_RADIUS = 200.0f;
	static constexpr float MATERIAL_RING_RADIUS = 280.0f;
	static constexpr int NUM_CATEGORIES = static_cast<int>(Inventory::MaterialCategory::COUNT);

	// Helper to get category angle range
	void getCategoryAngleRange(int categoryIndex, float& startAngle, float& endAngle) const;

	// Check if mouse is hovering over a category
	int getCategoryUnderMouse(float mouseX, float mouseY, float centerX, float centerY) const;

	// Check if mouse is hovering over a material in the outer ring
	int getMaterialUnderMouse(float mouseX, float mouseY, float centerX, float centerY,
							  const std::vector<Inventory::Material>& materials) const;

	// Current category materials cache
	std::vector<Inventory::Material> m_currentCategoryMaterials;
};

} // namespace UI

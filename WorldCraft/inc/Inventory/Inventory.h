#pragma once

#include <Voxel/BlockTypes.h>
#include <array>
#include <string>
#include <vector>

namespace Inventory
{

// ---------------------------------------------------------------------------
// Sci-Fi Material Categories
// ---------------------------------------------------------------------------
enum class MaterialCategory
{
	Structure,   // Hull plates, bulkheads, frames
	Energy,      // Conduits, reactors, power cells
	Tech,        // Computers, terminals, machinery
	Defensive,   // Force fields, armor plating
	Lighting,    // Glow panels, strip lights
	Natural,     // Terrain blocks (dirt, stone, etc.)
	Special,     // Unique/rare materials
	COUNT
};

// ---------------------------------------------------------------------------
// Material Definition - Wraps a BlockID with display info
// ---------------------------------------------------------------------------
struct Material
{
	Voxel::BlockID blockID;
	std::string name;
	std::string description;
	MaterialCategory category;

	Material()
		: blockID(Voxel::BlockID::Air)
		, name("Empty")
		, description("")
		, category(MaterialCategory::Natural)
	{}

	Material(Voxel::BlockID id, const std::string& n, const std::string& desc, MaterialCategory cat)
		: blockID(id), name(n), description(desc), category(cat)
	{}

	bool isEmpty() const { return blockID == Voxel::BlockID::Air; }
};

// ---------------------------------------------------------------------------
// Player Inventory - Manages hotbar slots and material selection
// ---------------------------------------------------------------------------
class PlayerInventory
{
public:
	static constexpr int HOTBAR_SIZE = 9;

	PlayerInventory();

	// Hotbar management
	int getSelectedSlot() const { return m_selectedSlot; }
	void setSelectedSlot(int slot);
	void selectNextSlot();
	void selectPrevSlot();

	// Get material in a hotbar slot
	const Material& getMaterial(int slot) const;
	Material& getMaterial(int slot);

	// Get currently selected material
	const Material& getSelectedMaterial() const;
	Voxel::BlockID getSelectedBlockID() const;

	// Set material in a hotbar slot
	void setMaterial(int slot, const Material& material);

	// Get all available materials (for radial menu / fabricator UI)
	static const std::vector<Material>& getAllMaterials();

	// Get materials by category
	static std::vector<Material> getMaterialsByCategory(MaterialCategory category);

	// Get category name
	static std::string getCategoryName(MaterialCategory category);

	// Save/load hotbar state to/from metadata
	std::vector<uint16_t> exportHotbar() const;
	void importHotbar(const std::vector<uint16_t>& blockIDs);

private:
	std::array<Material, HOTBAR_SIZE> m_hotbar;
	int m_selectedSlot;

	// Initialize default hotbar materials
	void initializeDefaultHotbar();

	// Static material database
	static std::vector<Material> s_allMaterials;
	static void initializeMaterialDatabase();
};

} // namespace Inventory

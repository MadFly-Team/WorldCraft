#pragma once

#include <Voxel/BlockTypes.h>
#include <Crafting/Blueprint.h>
#include <array>
#include <string>
#include <vector>
#include <map>

// Forward declaration for persistence
namespace Persistence { struct WorldMetadata; }

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
// Inventory Slot - Tracks block type and stack count (for pickup system)
// ---------------------------------------------------------------------------
struct InventorySlot
{
	Voxel::BlockID blockID;
	int stackCount;  // 0-100, 0 = empty slot

	InventorySlot()
		: blockID(Voxel::BlockID::Air)
		, stackCount(0)
	{}

	InventorySlot(Voxel::BlockID id, int count = 1)
		: blockID(id)
		, stackCount(count)
	{}

	bool isEmpty() const { return stackCount == 0 || blockID == Voxel::BlockID::Air; }
	bool isFull() const { return stackCount >= 100; }
	bool canStack(Voxel::BlockID otherBlock) const 
	{ 
		return !isEmpty() && blockID == otherBlock && stackCount < 100; 
	}
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
	static constexpr int BASE_SLOTS = 10;        // Starting inventory size
	static constexpr int MAX_SLOTS = 40;         // Maximum expandable slots
	static constexpr int HOTBAR_SIZE = BASE_SLOTS; // Compatibility alias

	PlayerInventory();

	// Inventory capacity management
	int getCurrentCapacity() const { return m_currentCapacity; }
	int getMaxCapacity() const { return MAX_SLOTS; }
	bool canExpandInventory() const { return m_currentCapacity < MAX_SLOTS; }
	bool expandInventory(int additionalSlots = 5);  // Returns true if successful

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

	// ---- New pickup/collection system ----
	// Try to add picked-up items to inventory (stacks to 100)
	// Returns number of items successfully added (may be < count if inventory full)
	int addPickedUpItem(Voxel::BlockID blockID, int count = 1);

	// Get inventory slot info (for new inventory UI)
	const InventorySlot& getSlot(int index) const;
	InventorySlot& getSlot(int index);

	// Check if inventory has space for an item
	bool hasSpaceFor(Voxel::BlockID blockID, int count = 1) const;

	// Consume items from the selected slot when placing blocks
	// Returns true if item was consumed, false if slot is empty
	bool consumeSelectedItem(int count = 1);

	// ---- Blueprint/Crafting system ----
	// Get all resources in inventory as a map (for crafting checks)
	std::map<Voxel::BlockID, int> getResourceMap() const;

	// Craft a blueprint (consumes resources, adds result to inventory)
	// Returns number of items successfully crafted (may be 0 if resources insufficient)
	int craftBlueprint(const Crafting::Blueprint& blueprint);

	// Access blueprint database
	Crafting::BlueprintDatabase& getBlueprintDatabase() { return m_blueprints; }
	const Crafting::BlueprintDatabase& getBlueprintDatabase() const { return m_blueprints; }

	// Persistence: Save/load inventory and blueprints to/from world metadata
	void saveToMetadata(Persistence::WorldMetadata& metadata) const;
	void loadFromMetadata(const Persistence::WorldMetadata& metadata);

	// Legacy: Save/load hotbar state to/from metadata (deprecated, kept for compatibility)
	std::vector<uint16_t> exportHotbar() const;
	void importHotbar(const std::vector<uint16_t>& blockIDs);

private:
	std::array<Material, MAX_SLOTS> m_hotbar;          // Materials for all possible slots
	std::array<InventorySlot, MAX_SLOTS> m_slots;     // Stack tracking for all slots
	int m_selectedSlot;
	int m_currentCapacity;  // Current number of usable slots (10-40)

	Crafting::BlueprintDatabase m_blueprints;  // Crafting blueprints

	// Initialize default hotbar materials
	void initializeDefaultHotbar();

	// Static material database
	static std::vector<Material> s_allMaterials;
	static void initializeMaterialDatabase();
};

} // namespace Inventory

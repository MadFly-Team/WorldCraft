#include <Inventory/Inventory.h>
#include <Persistence/WorldMetadata.h>
#include <algorithm>
#include <iostream>

namespace Inventory
{

// Static material database initialization
std::vector<Material> PlayerInventory::s_allMaterials;

PlayerInventory::PlayerInventory()
	: m_selectedSlot(0)
	, m_currentCapacity(BASE_SLOTS)  // Start with 10 slots
{
	initializeMaterialDatabase();
	initializeDefaultHotbar();

	// Unlock all blueprints by default for testing/gameplay
	m_blueprints.unlockAllBlueprints();
}

bool PlayerInventory::expandInventory(int additionalSlots)
{
	if (!canExpandInventory())
		return false;

	int newCapacity = std::min(m_currentCapacity + additionalSlots, MAX_SLOTS);

	if (newCapacity > m_currentCapacity)
	{
		m_currentCapacity = newCapacity;
		return true;
	}

	return false;
}

void PlayerInventory::setSelectedSlot(int slot)
{
	if (slot >= 0 && slot < m_currentCapacity)  // Use dynamic capacity
	{
		m_selectedSlot = slot;
	}
}

void PlayerInventory::selectNextSlot()
{
	m_selectedSlot = (m_selectedSlot + 1) % m_currentCapacity;
}

void PlayerInventory::selectPrevSlot()
{
	m_selectedSlot = (m_selectedSlot - 1 + m_currentCapacity) % m_currentCapacity;
}

const Material& PlayerInventory::getMaterial(int slot) const
{
	if (slot >= 0 && slot < m_currentCapacity)
		return m_hotbar[slot];

	static Material empty;
	return empty;
}

Material& PlayerInventory::getMaterial(int slot)
{
	static Material empty;
	if (slot >= 0 && slot < m_currentCapacity)
		return m_hotbar[slot];
	return empty;
}

const Material& PlayerInventory::getSelectedMaterial() const
{
	return m_hotbar[m_selectedSlot];
}

Voxel::BlockID PlayerInventory::getSelectedBlockID() const
{
	return m_hotbar[m_selectedSlot].blockID;
}

void PlayerInventory::setMaterial(int slot, const Material& material)
{
	if (slot >= 0 && slot < HOTBAR_SIZE)
	{
		m_hotbar[slot] = material;
	}
}

const std::vector<Material>& PlayerInventory::getAllMaterials()
{
	if (s_allMaterials.empty())
		initializeMaterialDatabase();
	return s_allMaterials;
}

std::vector<Material> PlayerInventory::getMaterialsByCategory(MaterialCategory category)
{
	std::vector<Material> filtered;
	const auto& all = getAllMaterials();

	for (const auto& mat : all)
	{
		if (mat.category == category)
			filtered.push_back(mat);
	}

	return filtered;
}

std::string PlayerInventory::getCategoryName(MaterialCategory category)
{
	switch (category)
	{
	case MaterialCategory::Structure:  return "Structure";
	case MaterialCategory::Energy:     return "Energy";
	case MaterialCategory::Tech:       return "Tech";
	case MaterialCategory::Defensive:  return "Defensive";
	case MaterialCategory::Lighting:   return "Lighting";
	case MaterialCategory::Natural:    return "Natural";
	case MaterialCategory::Special:    return "Special";
	default: return "Unknown";
	}
}

void PlayerInventory::initializeDefaultHotbar()
{
	// Start with some common materials in the first 10 slots
	m_hotbar[0] = Material(Voxel::BlockID::Stone,    "Durasteel Panel",  "Standard structural plating", MaterialCategory::Structure);
	m_hotbar[1] = Material(Voxel::BlockID::Dirt,     "Composite Block",  "Basic building material",     MaterialCategory::Structure);
	m_hotbar[2] = Material(Voxel::BlockID::Grass,    "Bio-Surface",      "Living terrain surface",      MaterialCategory::Natural);
	m_hotbar[3] = Material(Voxel::BlockID::Wood,     "Reinforced Frame", "Support structure",           MaterialCategory::Structure);
	m_hotbar[4] = Material(Voxel::BlockID::Sand,     "Silicate Block",   "Sandy composite",             MaterialCategory::Natural);
	m_hotbar[5] = Material(Voxel::BlockID::IronOre,  "Alloy Ore",        "Raw metal composite",         MaterialCategory::Natural);
	m_hotbar[6] = Material(Voxel::BlockID::GoldOre,  "Energy Ore",       "Power-infused crystal",       MaterialCategory::Energy);
	m_hotbar[7] = Material(Voxel::BlockID::Water,    "Liquid Tank",      "Contained fluid block",       MaterialCategory::Tech);
	m_hotbar[8] = Material(Voxel::BlockID::Torch,    "Plasma Emitter",   "Light source",                MaterialCategory::Lighting);
	m_hotbar[9] = Material(Voxel::BlockID::Obsidian, "Hardened Alloy",   "Ultra-dense material",        MaterialCategory::Structure);

	// Initialize all inventory slots as empty (will be filled by pickups)
	for (int i = 0; i < MAX_SLOTS; ++i)
	{
		m_slots[i] = InventorySlot();
	}
}

void PlayerInventory::initializeMaterialDatabase()
{
	if (!s_allMaterials.empty())
		return;

	// Structure Category - Sci-fi themed names for building blocks
	s_allMaterials.emplace_back(Voxel::BlockID::Stone,    "Durasteel Panel",    "Standard structural plating",        MaterialCategory::Structure);
	s_allMaterials.emplace_back(Voxel::BlockID::Dirt,     "Composite Block",    "Basic building material",            MaterialCategory::Structure);
	s_allMaterials.emplace_back(Voxel::BlockID::Wood,     "Reinforced Frame",   "Support structure beam",             MaterialCategory::Structure);
	s_allMaterials.emplace_back(Voxel::BlockID::Gravel,   "Ablative Plating",   "Heat-resistant armor",               MaterialCategory::Structure);
	s_allMaterials.emplace_back(Voxel::BlockID::Bedrock,  "Titanium Core",      "Indestructible foundation",          MaterialCategory::Structure);
	s_allMaterials.emplace_back(Voxel::BlockID::Obsidian, "Hardened Alloy",     "Ultra-dense structural material",    MaterialCategory::Structure);

	// Energy Category
	s_allMaterials.emplace_back(Voxel::BlockID::GoldOre,     "Energy Crystal",     "Power-infused crystal ore",       MaterialCategory::Energy);
	s_allMaterials.emplace_back(Voxel::BlockID::RedstoneOre, "Plasma Conduit",     "Energy transfer system",          MaterialCategory::Energy);
	s_allMaterials.emplace_back(Voxel::BlockID::LapisOre,    "Quantum Cell",       "Advanced power storage",          MaterialCategory::Energy);
	s_allMaterials.emplace_back(Voxel::BlockID::EmeraldOre,  "Fusion Core",        "High-output reactor block",       MaterialCategory::Energy);

	// Tech Category
	s_allMaterials.emplace_back(Voxel::BlockID::IronOre,    "Alloy Processor",    "Metal refinement block",          MaterialCategory::Tech);
	s_allMaterials.emplace_back(Voxel::BlockID::CoalOre,    "Carbon Module",      "Processing component",            MaterialCategory::Tech);
	s_allMaterials.emplace_back(Voxel::BlockID::DiamondOre, "Nanotech Matrix",    "Advanced circuitry block",        MaterialCategory::Tech);
	s_allMaterials.emplace_back(Voxel::BlockID::Water,      "Liquid Tank",        "Contained fluid storage",         MaterialCategory::Tech);

	// Defensive Category
	s_allMaterials.emplace_back(Voxel::BlockID::Rock1,      "Armor Plate A",      "Standard defense plating",        MaterialCategory::Defensive);
	s_allMaterials.emplace_back(Voxel::BlockID::Rock2,      "Armor Plate B",      "Reinforced defense plating",      MaterialCategory::Defensive);
	s_allMaterials.emplace_back(Voxel::BlockID::Rock3,      "Armor Plate C",      "Heavy defense plating",           MaterialCategory::Defensive);
	s_allMaterials.emplace_back(Voxel::BlockID::SnowBlock,  "Cryo-Shield",        "Temperature-resistant barrier",   MaterialCategory::Defensive);

	// Lighting Category
	s_allMaterials.emplace_back(Voxel::BlockID::Torch,     "Plasma Emitter",     "Standard light source",           MaterialCategory::Lighting);
	s_allMaterials.emplace_back(Voxel::BlockID::Mushroom,  "Bio-Luminescent",    "Organic light source",            MaterialCategory::Lighting);

	// Natural Category (keep some original terrain for realism)
	s_allMaterials.emplace_back(Voxel::BlockID::Grass,     "Bio-Surface",        "Living terrain surface",          MaterialCategory::Natural);
	s_allMaterials.emplace_back(Voxel::BlockID::Sand,      "Silicate Block",     "Sandy composite material",        MaterialCategory::Natural);
	s_allMaterials.emplace_back(Voxel::BlockID::Leaf,      "Bio-Foliage",        "Organic plant matter",            MaterialCategory::Natural);
	s_allMaterials.emplace_back(Voxel::BlockID::Grass1,    "Bio-Surface V1",     "Terrain variant 1",               MaterialCategory::Natural);
	s_allMaterials.emplace_back(Voxel::BlockID::Grass2,    "Bio-Surface V2",     "Terrain variant 2",               MaterialCategory::Natural);
	s_allMaterials.emplace_back(Voxel::BlockID::Grass3,    "Bio-Surface V3",     "Terrain variant 3",               MaterialCategory::Natural);

	// Special Category
	s_allMaterials.emplace_back(Voxel::BlockID::Rock4,     "Exotic Material A",  "Rare composite",                  MaterialCategory::Special);
	s_allMaterials.emplace_back(Voxel::BlockID::Rock5,     "Exotic Material B",  "Rare composite",                  MaterialCategory::Special);
	s_allMaterials.emplace_back(Voxel::BlockID::Rock6,     "Exotic Material C",  "Rare composite",                  MaterialCategory::Special);
}

std::vector<uint16_t> PlayerInventory::exportHotbar() const
{
	std::vector<uint16_t> blockIDs;
	blockIDs.reserve(HOTBAR_SIZE);

	for (int i = 0; i < HOTBAR_SIZE; ++i)
	{
		blockIDs.push_back(static_cast<uint16_t>(m_hotbar[i].blockID));
	}

	return blockIDs;
}

void PlayerInventory::importHotbar(const std::vector<uint16_t>& blockIDs)
{
	// If empty or wrong size, use defaults
	if (blockIDs.size() != HOTBAR_SIZE)
	{
		initializeDefaultHotbar();
		return;
	}

	// Import each block ID and find matching material
	for (int i = 0; i < HOTBAR_SIZE; ++i)
	{
		Voxel::BlockID blockID = static_cast<Voxel::BlockID>(blockIDs[i]);

		// Find matching material in database
		bool found = false;
		for (const Material& mat : s_allMaterials)
		{
			if (mat.blockID == blockID)
			{
				m_hotbar[i] = mat;
				found = true;
				break;
			}
		}

		// If not found, create a basic material wrapper
		if (!found)
		{
			m_hotbar[i] = Material(blockID, "Unknown Material", "Undefined block", MaterialCategory::Structure);
		}
	}
}

// ---- New pickup/collection system implementation ----

int PlayerInventory::addPickedUpItem(Voxel::BlockID blockID, int count)
{
	if (blockID == Voxel::BlockID::Air || count <= 0)
		return 0;

	int remainingToAdd = count;

	// First pass: try to stack with existing slots (within current capacity)
	for (int i = 0; i < m_currentCapacity && remainingToAdd > 0; ++i)
	{
		if (m_slots[i].canStack(blockID))
		{
			int spaceInSlot = 100 - m_slots[i].stackCount;
			int toAdd = std::min(remainingToAdd, spaceInSlot);
			m_slots[i].stackCount += toAdd;
			remainingToAdd -= toAdd;
		}
	}

	// Second pass: fill empty slots (within current capacity)
	for (int i = 0; i < m_currentCapacity && remainingToAdd > 0; ++i)
	{
		if (m_slots[i].isEmpty())
		{
			int toAdd = std::min(remainingToAdd, 100);
			m_slots[i] = InventorySlot(blockID, toAdd);

			// Also update the material reference for that slot
			for (const auto& mat : s_allMaterials)
			{
				if (mat.blockID == blockID)
				{
					m_hotbar[i] = mat;
					break;
				}
			}

			remainingToAdd -= toAdd;
		}
	}

	return count - remainingToAdd;  // Return how many were actually added
}

const InventorySlot& PlayerInventory::getSlot(int index) const
{
	static InventorySlot empty;
	if (index >= 0 && index < MAX_SLOTS)
		return m_slots[index];
	return empty;
}

InventorySlot& PlayerInventory::getSlot(int index)
{
	static InventorySlot empty;
	if (index >= 0 && index < MAX_SLOTS)
		return m_slots[index];
	return empty;
}

bool PlayerInventory::hasSpaceFor(Voxel::BlockID blockID, int count) const
{
	int availableSpace = 0;

	// Count space in existing stacks (within current capacity)
	for (int i = 0; i < m_currentCapacity; ++i)
	{
		if (m_slots[i].canStack(blockID))
		{
			availableSpace += (100 - m_slots[i].stackCount);
		}
		else if (m_slots[i].isEmpty())
		{
			availableSpace += 100;
		}
	}

	return availableSpace >= count;
}

bool PlayerInventory::consumeSelectedItem(int count)
{
	if (m_selectedSlot < 0 || m_selectedSlot >= m_currentCapacity)
		return false;

	auto& slot = m_slots[m_selectedSlot];

	if (slot.isEmpty() || slot.stackCount < count)
		return false;

	slot.stackCount -= count;

	// Clear slot if empty
	if (slot.stackCount <= 0)
	{
		slot = InventorySlot();
		m_hotbar[m_selectedSlot] = Material(); // Clear material too
	}

	return true;
}

void PlayerInventory::clear()
{
	// Clear all inventory slots
	for (auto& slot : m_slots)
	{
		slot = InventorySlot();
	}

	// Reset capacity to base and selected slot
	m_currentCapacity = BASE_SLOTS;
	m_selectedSlot = 0;

	// Reinitialize default hotbar materials
	initializeDefaultHotbar();

	// Unlock all blueprints (reset crafting)
	m_blueprints.unlockAllBlueprints();

	std::cout << "[Inventory] Cleared inventory for new world" << std::endl;
}

// ---- Blueprint/Crafting system ----

std::map<Voxel::BlockID, int> PlayerInventory::getResourceMap() const
{
	std::map<Voxel::BlockID, int> resources;

	for (int i = 0; i < m_currentCapacity; ++i)
	{
		if (!m_slots[i].isEmpty())
		{
			resources[m_slots[i].blockID] += m_slots[i].stackCount;
		}
	}

	return resources;
}

int PlayerInventory::craftBlueprint(const Crafting::Blueprint& blueprint)
{
	if (!blueprint.isUnlocked)
		return 0;

	// Check if we have enough resources
	auto resources = getResourceMap();
	if (!blueprint.canCraft(resources))
		return 0;

	// Consume resources from inventory
	for (const auto& cost : blueprint.costs)
	{
		int remainingToConsume = cost.quantity;

		for (int i = 0; i < m_currentCapacity && remainingToConsume > 0; ++i)
		{
			if (m_slots[i].blockID == cost.resourceType && m_slots[i].stackCount > 0)
			{
				int toConsume = std::min(remainingToConsume, m_slots[i].stackCount);
				m_slots[i].stackCount -= toConsume;
				remainingToConsume -= toConsume;

				// Clear slot if empty
				if (m_slots[i].stackCount <= 0)
				{
					m_slots[i] = InventorySlot();
					m_hotbar[i] = Material();
				}
			}
		}
	}

	// Add crafted items to inventory
	int crafted = addPickedUpItem(blueprint.resultBlock, blueprint.craftYield);

	return crafted;
}

void PlayerInventory::saveToMetadata(Persistence::WorldMetadata& metadata) const
{
	// Save inventory capacity and selected slot
	metadata.inventoryCapacity = m_currentCapacity;
	metadata.selectedSlot = m_selectedSlot;

	// Save all inventory slots
	metadata.inventorySlots.clear();
	metadata.inventorySlots.reserve(m_currentCapacity);

	for (int i = 0; i < m_currentCapacity; ++i)
	{
		Persistence::WorldMetadata::InventorySlotData slotData;
		slotData.blockID = static_cast<uint16_t>(m_slots[i].blockID);
		slotData.stackCount = m_slots[i].stackCount;
		metadata.inventorySlots.push_back(slotData);
	}

	// Save unlocked blueprints
	metadata.unlockedBlueprints.clear();
	const auto& allBlueprints = m_blueprints.getAllBlueprints();
	for (const auto& bp : allBlueprints)
	{
		if (bp.isUnlocked)
		{
			metadata.unlockedBlueprints.push_back(static_cast<uint16_t>(bp.resultBlock));
		}
	}
}

void PlayerInventory::loadFromMetadata(const Persistence::WorldMetadata& metadata)
{
	// Restore inventory capacity
	m_currentCapacity = metadata.inventoryCapacity;
	if (m_currentCapacity < BASE_SLOTS) m_currentCapacity = BASE_SLOTS;
	if (m_currentCapacity > MAX_SLOTS) m_currentCapacity = MAX_SLOTS;

	// Restore selected slot
	m_selectedSlot = metadata.selectedSlot;
	if (m_selectedSlot < 0 || m_selectedSlot >= m_currentCapacity)
		m_selectedSlot = 0;

	// Clear current inventory
	for (int i = 0; i < MAX_SLOTS; ++i)
	{
		m_slots[i] = InventorySlot();
		m_hotbar[i] = Material();
	}

	// Restore inventory slots
	int slotsToLoad = std::min(static_cast<int>(metadata.inventorySlots.size()), m_currentCapacity);
	for (int i = 0; i < slotsToLoad; ++i)
	{
		const auto& slotData = metadata.inventorySlots[i];
		Voxel::BlockID blockID = static_cast<Voxel::BlockID>(slotData.blockID);
		int stackCount = slotData.stackCount;

		// Only restore non-empty slots
		if (blockID != Voxel::BlockID::Air && stackCount > 0)
		{
			m_slots[i] = InventorySlot(blockID, stackCount);

			// Find matching material in database
			bool found = false;
			for (const Material& mat : s_allMaterials)
			{
				if (mat.blockID == blockID)
				{
					m_hotbar[i] = mat;
					found = true;
					break;
				}
			}

			// If not found in database, create a basic material wrapper
			if (!found)
			{
				m_hotbar[i] = Material(blockID, "Unknown Material", "Undefined block", MaterialCategory::Structure);
			}
		}
	}

	// Restore unlocked blueprints
	if (!metadata.unlockedBlueprints.empty())
	{
		// Clear all unlocks first
		const auto& allBlueprints = m_blueprints.getAllBlueprints();
		for (auto& bp : allBlueprints)
		{
			const_cast<Crafting::Blueprint&>(bp).isUnlocked = false;
		}

		// Unlock saved blueprints
		for (uint16_t blockID : metadata.unlockedBlueprints)
		{
			m_blueprints.unlockBlueprint(static_cast<Voxel::BlockID>(blockID));
		}
	}
	else
	{
		// If no blueprint data found, unlock all by default (backward compatibility)
		m_blueprints.unlockAllBlueprints();
	}
}

} // namespace Inventory

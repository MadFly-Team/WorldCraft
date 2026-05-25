#include <Inventory/Inventory.h>
#include <algorithm>

namespace Inventory
{

// Static material database initialization
std::vector<Material> PlayerInventory::s_allMaterials;

PlayerInventory::PlayerInventory()
	: m_selectedSlot(0)
{
	initializeMaterialDatabase();
	initializeDefaultHotbar();
}

void PlayerInventory::setSelectedSlot(int slot)
{
	if (slot >= 0 && slot < HOTBAR_SIZE)
	{
		m_selectedSlot = slot;
	}
}

void PlayerInventory::selectNextSlot()
{
	m_selectedSlot = (m_selectedSlot + 1) % HOTBAR_SIZE;
}

void PlayerInventory::selectPrevSlot()
{
	m_selectedSlot = (m_selectedSlot - 1 + HOTBAR_SIZE) % HOTBAR_SIZE;
}

const Material& PlayerInventory::getMaterial(int slot) const
{
	if (slot >= 0 && slot < HOTBAR_SIZE)
		return m_hotbar[slot];

	static Material empty;
	return empty;
}

Material& PlayerInventory::getMaterial(int slot)
{
	static Material empty;
	if (slot >= 0 && slot < HOTBAR_SIZE)
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
	// Start with some common materials in the hotbar
	m_hotbar[0] = Material(Voxel::BlockID::Stone,    "Durasteel Panel",  "Standard structural plating", MaterialCategory::Structure);
	m_hotbar[1] = Material(Voxel::BlockID::Dirt,     "Composite Block",  "Basic building material",     MaterialCategory::Structure);
	m_hotbar[2] = Material(Voxel::BlockID::Grass,    "Bio-Surface",      "Living terrain surface",      MaterialCategory::Natural);
	m_hotbar[3] = Material(Voxel::BlockID::Wood,     "Reinforced Frame", "Support structure",           MaterialCategory::Structure);
	m_hotbar[4] = Material(Voxel::BlockID::Sand,     "Silicate Block",   "Sandy composite",             MaterialCategory::Natural);
	m_hotbar[5] = Material(Voxel::BlockID::IronOre,  "Alloy Ore",        "Raw metal composite",         MaterialCategory::Natural);
	m_hotbar[6] = Material(Voxel::BlockID::GoldOre,  "Energy Ore",       "Power-infused crystal",       MaterialCategory::Energy);
	m_hotbar[7] = Material(Voxel::BlockID::Water,    "Liquid Tank",      "Contained fluid block",       MaterialCategory::Tech);
	m_hotbar[8] = Material(Voxel::BlockID::Torch,    "Plasma Emitter",   "Light source",                MaterialCategory::Lighting);
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

} // namespace Inventory

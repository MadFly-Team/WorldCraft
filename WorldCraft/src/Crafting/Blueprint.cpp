#include <Crafting/Blueprint.h>
#include <imgui.h>
#include <algorithm>

namespace Crafting
{

// Check if player has enough resources to craft
bool Blueprint::canCraft(const std::map<Voxel::BlockID, int>& playerResources) const
{
	if (!isUnlocked)
		return false;

	for (const auto& cost : costs)
	{
		auto it = playerResources.find(cost.resourceType);
		if (it == playerResources.end() || it->second < cost.quantity)
		{
			return false;  // Missing resource or not enough
		}
	}

	return true;
}

ImVec4 Blueprint::getRarityColor(BlueprintRarity rarity)
{
	switch (rarity)
	{
	case BlueprintRarity::Common:    return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // Gray
	case BlueprintRarity::Uncommon:  return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);  // Green
	case BlueprintRarity::Rare:      return ImVec4(0.2f, 0.5f, 1.0f, 1.0f);  // Blue
	case BlueprintRarity::Epic:      return ImVec4(0.7f, 0.2f, 1.0f, 1.0f);  // Purple
	case BlueprintRarity::Legendary: return ImVec4(1.0f, 0.6f, 0.0f, 1.0f);  // Orange
	default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

std::string Blueprint::getRarityName(BlueprintRarity rarity)
{
	switch (rarity)
	{
	case BlueprintRarity::Common:    return "Common";
	case BlueprintRarity::Uncommon:  return "Uncommon";
	case BlueprintRarity::Rare:      return "Rare";
	case BlueprintRarity::Epic:      return "Epic";
	case BlueprintRarity::Legendary: return "Legendary";
	default: return "Unknown";
	}
}

// ---------------------------------------------------------------------------
// BlueprintDatabase Implementation
// ---------------------------------------------------------------------------

BlueprintDatabase::BlueprintDatabase()
{
	initializeBlueprints();
}

std::vector<const Blueprint*> BlueprintDatabase::getBlueprintsByRarity(BlueprintRarity rarity) const
{
	std::vector<const Blueprint*> result;
	for (const auto& bp : m_blueprints)
	{
		if (bp.rarity == rarity)
		{
			result.push_back(&bp);
		}
	}
	return result;
}

std::vector<const Blueprint*> BlueprintDatabase::getUnlockedBlueprints() const
{
	std::vector<const Blueprint*> result;
	for (const auto& bp : m_blueprints)
	{
		if (bp.isUnlocked)
		{
			result.push_back(&bp);
		}
	}
	return result;
}

const Blueprint* BlueprintDatabase::findBlueprintByBlock(Voxel::BlockID blockType) const
{
	for (const auto& bp : m_blueprints)
	{
		if (bp.resultBlock == blockType)
		{
			return &bp;
		}
	}
	return nullptr;
}

void BlueprintDatabase::unlockBlueprint(Voxel::BlockID blockType)
{
	for (auto& bp : m_blueprints)
	{
		if (bp.resultBlock == blockType)
		{
			bp.isUnlocked = true;
			return;
		}
	}
}

void BlueprintDatabase::unlockAllBlueprints()
{
	for (auto& bp : m_blueprints)
	{
		bp.isUnlocked = true;
	}
}

void BlueprintDatabase::initializeBlueprints()
{
	// COMMON TIER - Basic materials (free or cheap)
	// Stone/Durasteel Panel
	{
		Blueprint bp(Voxel::BlockID::Stone, "Durasteel Panel", 
					 "Standard structural plating. Basic building material.", 
					 BlueprintRarity::Common, 4);
		bp.addCost(Voxel::BlockID::Dirt, 2);   // 2 dirt → 4 stone
		bp.isUnlocked = true;  // Start unlocked
		m_blueprints.push_back(bp);
	}

	// Dirt/Composite Block
	{
		Blueprint bp(Voxel::BlockID::Dirt, "Composite Block", 
					 "Basic compressed earth material.", 
					 BlueprintRarity::Common, 1);
		// Free - always craftable from nothing (or found in world)
		bp.isUnlocked = true;
		m_blueprints.push_back(bp);
	}

	// Wood/Reinforced Frame
	{
		Blueprint bp(Voxel::BlockID::Wood, "Reinforced Frame", 
					 "Structural support beam.", 
					 BlueprintRarity::Common, 2);
		bp.addCost(Voxel::BlockID::Dirt, 3);
		bp.isUnlocked = true;
		m_blueprints.push_back(bp);
	}

	// Sand/Silicate Block
	{
		Blueprint bp(Voxel::BlockID::Sand, "Silicate Block", 
					 "Sandy composite material.", 
					 BlueprintRarity::Common, 2);
		bp.addCost(Voxel::BlockID::Dirt, 2);
		bp.isUnlocked = true;
		m_blueprints.push_back(bp);
	}

	// UNCOMMON TIER - Refined materials
	// Grass/Bio-Surface
	{
		Blueprint bp(Voxel::BlockID::Grass, "Bio-Surface", 
					 "Living terrain surface with organic coating.", 
					 BlueprintRarity::Uncommon, 1);
		bp.addCost(Voxel::BlockID::Dirt, 1);
		bp.addCost(Voxel::BlockID::Wood, 1);
		bp.isUnlocked = false;  // Must discover
		m_blueprints.push_back(bp);
	}

	// Gravel/Ablative Plating
	{
		Blueprint bp(Voxel::BlockID::Gravel, "Ablative Plating", 
					 "Heat-resistant armor material.", 
					 BlueprintRarity::Uncommon, 2);
		bp.addCost(Voxel::BlockID::Stone, 2);
		bp.addCost(Voxel::BlockID::Sand, 1);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// RARE TIER - Advanced materials (need ore)
	// Iron Ore/Alloy Processor
	{
		Blueprint bp(Voxel::BlockID::IronOre, "Alloy Processor", 
					 "Metal refinement block with embedded ore.", 
					 BlueprintRarity::Rare, 1);
		bp.addCost(Voxel::BlockID::Stone, 4);
		bp.addCost(Voxel::BlockID::Wood, 2);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// Gold Ore/Energy Crystal
	{
		Blueprint bp(Voxel::BlockID::GoldOre, "Energy Crystal", 
					 "Power-infused crystal ore for energy systems.", 
					 BlueprintRarity::Rare, 1);
		bp.addCost(Voxel::BlockID::Stone, 6);
		bp.addCost(Voxel::BlockID::IronOre, 2);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// Redstone Ore/Plasma Conduit
	{
		Blueprint bp(Voxel::BlockID::RedstoneOre, "Plasma Conduit", 
					 "Energy transfer system block.", 
					 BlueprintRarity::Rare, 1);
		bp.addCost(Voxel::BlockID::IronOre, 3);
		bp.addCost(Voxel::BlockID::Stone, 4);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// Lapis Ore/Quantum Cell
	{
		Blueprint bp(Voxel::BlockID::LapisOre, "Quantum Cell", 
					 "Advanced power storage unit.", 
					 BlueprintRarity::Rare, 1);
		bp.addCost(Voxel::BlockID::GoldOre, 2);
		bp.addCost(Voxel::BlockID::IronOre, 2);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// EPIC TIER - High-tech materials
	// Diamond Ore/Fusion Matrix
	{
		Blueprint bp(Voxel::BlockID::DiamondOre, "Fusion Matrix", 
					 "High-density crystalline reactor component.", 
					 BlueprintRarity::Epic, 1);
		bp.addCost(Voxel::BlockID::GoldOre, 4);
		bp.addCost(Voxel::BlockID::LapisOre, 2);
		bp.addCost(Voxel::BlockID::RedstoneOre, 2);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// Emerald Ore/Fusion Core
	{
		Blueprint bp(Voxel::BlockID::EmeraldOre, "Fusion Core", 
					 "High-output reactor block for power generation.", 
					 BlueprintRarity::Epic, 1);
		bp.addCost(Voxel::BlockID::DiamondOre, 2);
		bp.addCost(Voxel::BlockID::GoldOre, 4);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// LEGENDARY TIER - Ultimate materials
	// Obsidian/Hardened Alloy
	{
		Blueprint bp(Voxel::BlockID::Obsidian, "Hardened Alloy", 
					 "Ultra-dense structural material. Nearly indestructible.", 
					 BlueprintRarity::Legendary, 1);
		bp.addCost(Voxel::BlockID::DiamondOre, 4);
		bp.addCost(Voxel::BlockID::IronOre, 8);
		bp.addCost(Voxel::BlockID::Stone, 10);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// Bedrock/Titanium Core
	{
		Blueprint bp(Voxel::BlockID::Bedrock, "Titanium Core", 
					 "Indestructible foundation material. Ultimate defense.", 
					 BlueprintRarity::Legendary, 1);
		bp.addCost(Voxel::BlockID::Obsidian, 4);
		bp.addCost(Voxel::BlockID::EmeraldOre, 2);
		bp.addCost(Voxel::BlockID::DiamondOre, 4);
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}

	// UTILITY TIER (Rare/Epic)
	// Torch/Plasma Emitter
	{
		Blueprint bp(Voxel::BlockID::Torch, "Plasma Emitter", 
					 "High-intensity light source.", 
					 BlueprintRarity::Uncommon, 4);
		bp.addCost(Voxel::BlockID::Wood, 1);
		bp.addCost(Voxel::BlockID::Stone, 1);
		bp.isUnlocked = true;
		m_blueprints.push_back(bp);
	}

	// Water/Liquid Tank (special - cannot be crafted in survival, found only)
	{
		Blueprint bp(Voxel::BlockID::Water, "Liquid Tank", 
					 "Contained fluid block. Cannot be crafted.", 
					 BlueprintRarity::Rare, 1);
		// No costs - not craftable
		bp.isUnlocked = false;
		m_blueprints.push_back(bp);
	}
}

} // namespace Crafting

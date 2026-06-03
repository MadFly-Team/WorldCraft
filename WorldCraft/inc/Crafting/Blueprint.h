#pragma once

#include <Voxel/BlockTypes.h>
#include <string>
#include <vector>
#include <map>

// Forward declare ImGui types
struct ImVec4;

namespace Crafting
{

// ---------------------------------------------------------------------------
// Blueprint Rarity Tiers
// ---------------------------------------------------------------------------
enum class BlueprintRarity
{
	Common,      // Basic structural materials (dirt, stone, wood)
	Uncommon,    // Refined materials (iron, glass, brick)
	Rare,        // Advanced alloys (gold, redstone, lapis)
	Epic,        // High-tech materials (diamond, emerald)
	Legendary    // Ultimate materials (obsidian, bedrock equivalents)
};

// ---------------------------------------------------------------------------
// Resource Cost - What materials are needed to craft
// ---------------------------------------------------------------------------
struct ResourceCost
{
	Voxel::BlockID resourceType;  // What resource is needed
	int quantity;                  // How many of that resource

	ResourceCost(Voxel::BlockID type, int qty)
		: resourceType(type), quantity(qty)
	{}
};

// ---------------------------------------------------------------------------
// Blueprint Definition - Describes a craftable block
// ---------------------------------------------------------------------------
struct Blueprint
{
	Voxel::BlockID resultBlock;           // What block this blueprint creates
	std::string name;                     // Display name
	std::string description;              // Lore/description
	BlueprintRarity rarity;               // Rarity tier
	std::vector<ResourceCost> costs;      // Resources required to craft
	int craftYield;                       // How many blocks created per craft (default 1)
	bool isUnlocked;                      // Whether player has discovered this blueprint

	Blueprint()
		: resultBlock(Voxel::BlockID::Air)
		, name("Unknown")
		, description("")
		, rarity(BlueprintRarity::Common)
		, craftYield(1)
		, isUnlocked(false)
	{}

	Blueprint(Voxel::BlockID result, const std::string& n, const std::string& desc,
			  BlueprintRarity r, int yield = 1)
		: resultBlock(result)
		, name(n)
		, description(desc)
		, rarity(r)
		, craftYield(yield)
		, isUnlocked(false)
	{}

	// Add a resource cost
	void addCost(Voxel::BlockID resource, int quantity)
	{
		costs.emplace_back(resource, quantity);
	}

	// Check if player has enough resources to craft
	bool canCraft(const std::map<Voxel::BlockID, int>& playerResources) const;

	// Get rarity color for UI
	static ImVec4 getRarityColor(BlueprintRarity rarity);
	static std::string getRarityName(BlueprintRarity rarity);
};

// ---------------------------------------------------------------------------
// Blueprint Database - Manages all craftable blueprints
// ---------------------------------------------------------------------------
class BlueprintDatabase
{
public:
	BlueprintDatabase();

	// Get all blueprints
	const std::vector<Blueprint>& getAllBlueprints() const { return m_blueprints; }

	// Get blueprints by rarity
	std::vector<const Blueprint*> getBlueprintsByRarity(BlueprintRarity rarity) const;

	// Get blueprints that are unlocked
	std::vector<const Blueprint*> getUnlockedBlueprints() const;

	// Find blueprint by result block type
	const Blueprint* findBlueprintByBlock(Voxel::BlockID blockType) const;

	// Unlock a blueprint
	void unlockBlueprint(Voxel::BlockID blockType);

	// Unlock all blueprints (for testing/creative mode)
	void unlockAllBlueprints();

private:
	std::vector<Blueprint> m_blueprints;

	// Initialize the blueprint database
	void initializeBlueprints();
};

} // namespace Crafting

#pragma once

#include <Voxel/BlockTypes.h>
#include <cstdint>

namespace WorldPhysics {

// Material properties for a block type
struct BlockMaterial
{
	float mass;           // Base mass of the block (50-1000)
	float hardness;       // Resistance to destruction (0.0-1.0)
	float friction;       // Energy loss on deflection (0.0-1.0)
	float density;        // For water displacement calculations
	bool isFluid;         // Special handling for water/liquids
	bool isDestructible;  // Can this block be destroyed by impact?
};

// Registry of mass and material properties for all block types
class BlockMassRegistry
{
public:
	// Get material properties for a block type
	static const BlockMaterial& getMaterial(Voxel::BlockID blockType);

	// Quick access helpers
	static float getMass(Voxel::BlockID blockType);
	static bool isFluid(Voxel::BlockID blockType);
	static bool isDestructible(Voxel::BlockID blockType);

	// Calculate energy dampening factor when blockA impacts blockB
	// Returns value 0.0-1.0 representing energy retained
	static float calculateDampening(Voxel::BlockID fallingBlock, Voxel::BlockID struckBlock);

	// Check if impact energy exceeds destruction threshold
	// Threshold is 2x the struck block's mass
	static bool shouldDestroy(float impactEnergy, Voxel::BlockID struckBlock);

private:
	static void initializeMaterials();
	static BlockMaterial s_materials[static_cast<size_t>(Voxel::BlockID::COUNT)];
	static bool s_initialized;
};

} // namespace WorldPhysics

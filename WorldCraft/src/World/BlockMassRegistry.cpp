#include <World/BlockMassRegistry.h>
#include <algorithm>
#include <cmath>

namespace WorldPhysics {

BlockMaterial BlockMassRegistry::s_materials[static_cast<size_t>(Voxel::BlockID::COUNT)];
bool BlockMassRegistry::s_initialized = false;

void BlockMassRegistry::initializeMaterials()
{
	if (s_initialized)
		return;

	// Initialize all to default (air-like)
	for (size_t i = 0; i < static_cast<size_t>(Voxel::BlockID::COUNT); ++i)
	{
		s_materials[i] = { 0.0f, 0.0f, 0.0f, 0.0f, false, false };
	}

	// Air - no mass, no physics
	s_materials[static_cast<size_t>(Voxel::BlockID::Air)] = 
		{ 0.0f, 0.0f, 0.0f, 0.0f, false, false };

	// Water - lightest, fluid, not destructible (displaced instead)
	s_materials[static_cast<size_t>(Voxel::BlockID::Water)] = 
		{ 50.0f, 0.1f, 0.9f, 1.0f, true, false };

	// Grass variants - light organic material
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass1)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass2)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass3)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass4)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass5)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass6)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass7)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Grass8)] = 
		{ 100.0f, 0.3f, 0.6f, 0.8f, false, true };

	// Dirt - slightly heavier than grass
	s_materials[static_cast<size_t>(Voxel::BlockID::Dirt)] = 
		{ 120.0f, 0.4f, 0.7f, 1.2f, false, true };

	// Sand - medium weight, loose material
	s_materials[static_cast<size_t>(Voxel::BlockID::Sand)] = 
		{ 150.0f, 0.4f, 0.5f, 1.4f, false, true };

	// Gravel - similar to sand but slightly heavier
	s_materials[static_cast<size_t>(Voxel::BlockID::Gravel)] = 
		{ 160.0f, 0.5f, 0.5f, 1.5f, false, true };

	// Snow - light but compact
	s_materials[static_cast<size_t>(Voxel::BlockID::SnowBlock)] = 
		{ 80.0f, 0.2f, 0.8f, 0.5f, false, true };

	// Wood - medium density organic
	s_materials[static_cast<size_t>(Voxel::BlockID::Wood)] = 
		{ 200.0f, 0.6f, 0.6f, 0.7f, false, true };

	// Leaf - very light
	s_materials[static_cast<size_t>(Voxel::BlockID::Leaf)] = 
		{ 30.0f, 0.1f, 0.4f, 0.3f, false, true };

	// Stone - heavy, hard
	s_materials[static_cast<size_t>(Voxel::BlockID::Stone)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };

	// Rock variants - similar to stone
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock1)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock2)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock3)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock4)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock5)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock6)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock7)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Rock8)] = 
		{ 500.0f, 0.8f, 0.4f, 2.5f, false, true };

	// Ore blocks - heavy, very hard
	s_materials[static_cast<size_t>(Voxel::BlockID::CoalOre)] = 
		{ 600.0f, 0.85f, 0.4f, 2.8f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::IronOre)] = 
		{ 700.0f, 0.9f, 0.3f, 3.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::GoldOre)] = 
		{ 800.0f, 0.85f, 0.3f, 4.0f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::DiamondOre)] = 
		{ 900.0f, 0.95f, 0.2f, 3.5f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::RedstoneOre)] = 
		{ 650.0f, 0.85f, 0.4f, 3.0f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::LapisOre)] = 
		{ 650.0f, 0.85f, 0.4f, 3.0f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::EmeraldOre)] = 
		{ 850.0f, 0.95f, 0.2f, 3.5f, false, true };

	// Obsidian - very heavy, extremely hard
	s_materials[static_cast<size_t>(Voxel::BlockID::Obsidian)] = 
		{ 1000.0f, 0.98f, 0.2f, 3.0f, false, true };

	// Bedrock - indestructible
	s_materials[static_cast<size_t>(Voxel::BlockID::Bedrock)] = 
		{ 10000.0f, 1.0f, 0.1f, 5.0f, false, false };

	// Decorative blocks - very light, fragile
	s_materials[static_cast<size_t>(Voxel::BlockID::Mushroom)] = 
		{ 10.0f, 0.1f, 0.8f, 0.2f, false, true };
	s_materials[static_cast<size_t>(Voxel::BlockID::Torch)] = 
		{ 5.0f, 0.1f, 0.9f, 0.1f, false, true };

	s_initialized = true;
}

const BlockMaterial& BlockMassRegistry::getMaterial(Voxel::BlockID blockType)
{
	if (!s_initialized)
		initializeMaterials();

	size_t index = static_cast<size_t>(blockType);
	if (index >= static_cast<size_t>(Voxel::BlockID::COUNT))
		index = 0; // Return Air properties for invalid

	return s_materials[index];
}

float BlockMassRegistry::getMass(Voxel::BlockID blockType)
{
	return getMaterial(blockType).mass;
}

bool BlockMassRegistry::isFluid(Voxel::BlockID blockType)
{
	return getMaterial(blockType).isFluid;
}

bool BlockMassRegistry::isDestructible(Voxel::BlockID blockType)
{
	return getMaterial(blockType).isDestructible;
}

float BlockMassRegistry::calculateDampening(Voxel::BlockID fallingBlock, Voxel::BlockID struckBlock)
{
	const BlockMaterial& fallingMat = getMaterial(fallingBlock);
	const BlockMaterial& struckMat = getMaterial(struckBlock);

	// Base dampening from struck block's hardness
	float baseDampening = struckMat.hardness;

	// Friction from both materials
	float frictionLoss = (fallingMat.friction + struckMat.friction) * 0.5f;

	// Mass ratio effect - heavier struck blocks dampen more
	float massRatio = struckMat.mass / (fallingMat.mass + 1.0f); // +1 to avoid divide by zero
	float massDampening = std::min(1.0f, massRatio * 0.3f);

	// Combined dampening (energy retained = 1.0 - dampening)
	float totalDampening = baseDampening + frictionLoss + massDampening;
	totalDampening = std::clamp(totalDampening, 0.0f, 0.95f); // Never dampen 100%

	// Return energy retention factor
	return 1.0f - totalDampening;
}

bool BlockMassRegistry::shouldDestroy(float impactEnergy, Voxel::BlockID struckBlock)
{
	const BlockMaterial& material = getMaterial(struckBlock);

	// Can't destroy indestructible blocks
	if (!material.isDestructible)
		return false;

	// Fluids are displaced, not destroyed
	if (material.isFluid)
		return false;

	// Destruction threshold is 2x the block's mass
	float destructionThreshold = material.mass * 2.0f;

	return impactEnergy >= destructionThreshold;
}

} // namespace WorldPhysics

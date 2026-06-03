#pragma once

#include <Voxel/BlockTypes.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

// Forward declarations
namespace Chunk {
	class ChunkWorld;
}

namespace WorldPhysics {

// Water ripple effect
struct WaterRipple
{
	glm::ivec3 center;
	float strength;
	float radius;
	float age;
};

// Specialized water behavior handler
class WaterPhysics
{
public:
	explicit WaterPhysics(Chunk::ChunkWorld* world);

	// Update water simulation
	void update(float deltaTime);

	// Handle impact creating ripples
	void createRipple(const glm::ivec3& position, float impactEnergy);

	// Displace water from an impact point
	void displaceWater(const glm::ivec3& position, float energy);

	// Check if a position is underwater
	bool isUnderwater(const glm::ivec3& position) const;

	// Get water level at X,Z coordinates (returns Y coordinate)
	int getWaterLevel(int worldX, int worldZ) const;

	// Fill gaps in water (water seeks equilibrium)
	void fillWaterGaps();

	// Process water flow from a specific source position
	void processWaterFlow(const glm::ivec3& sourcePos, int maxDepth = 10);

	// Remove settled rocks underwater
	void removeUnderwaterRocks();

	// Clear all water physics state
	void clear();

private:
	// Update a single ripple
	void updateRipple(WaterRipple& ripple, float deltaTime);

	// Propagate water displacement
	void propagateDisplacement(const glm::ivec3& center, float strength, int radius);

	// Find water surface level at a column
	int findWaterSurface(int worldX, int worldZ) const;

	// Check if block is water
	bool isWater(const glm::ivec3& pos) const;

	// Get block at position
	Voxel::BlockID getBlock(const glm::ivec3& pos) const;

	// Set block at position
	void setBlock(const glm::ivec3& pos, Voxel::BlockID blockType);

	Chunk::ChunkWorld* m_world;
	std::vector<WaterRipple> m_ripples;

	// Water level cache for faster lookups
	mutable std::unordered_map<uint64_t, int> m_waterLevelCache;

	// Constants
	static constexpr float RIPPLE_SPEED = 5.0f;           // Blocks per second
	static constexpr float RIPPLE_DECAY = 2.0f;           // Strength loss per second
	static constexpr float MIN_RIPPLE_STRENGTH = 0.1f;    // Below this, ripple disappears
	static constexpr int MAX_RIPPLE_RADIUS = 8;           // Maximum ripple spread
	static constexpr float DISPLACEMENT_THRESHOLD = 50.0f; // Min energy for displacement
};

} // namespace WorldPhysics

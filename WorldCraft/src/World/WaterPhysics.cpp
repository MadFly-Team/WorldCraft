#include <World/WaterPhysics.h>
#include <World/BlockMassRegistry.h>
#include <Chunk/ChunkWorld.h>
#include <algorithm>
#include <cmath>

namespace WorldPhysics {

// Helper to pack X,Z coordinates into a single key
static uint64_t packCoord(int x, int z)
{
	return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint64_t>(static_cast<uint32_t>(z));
}

WaterPhysics::WaterPhysics(Chunk::ChunkWorld* world)
	: m_world(world)
{
}

void WaterPhysics::update(float deltaTime)
{
	// Update all active ripples
	for (auto it = m_ripples.begin(); it != m_ripples.end(); )
	{
		updateRipple(*it, deltaTime);

		// Remove ripples that have decayed
		if (it->strength < MIN_RIPPLE_STRENGTH)
		{
			it = m_ripples.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Periodically clear water level cache (every few seconds in real usage)
	static float cacheTimer = 0.0f;
	cacheTimer += deltaTime;
	if (cacheTimer > 5.0f)
	{
		m_waterLevelCache.clear();
		cacheTimer = 0.0f;
	}
}

void WaterPhysics::createRipple(const glm::ivec3& position, float impactEnergy)
{
	// Only create ripples for significant impacts
	if (impactEnergy < DISPLACEMENT_THRESHOLD)
		return;

	// Check if position is water or near water surface
	if (!isWater(position))
	{
		// Check if water is nearby
		bool nearWater = false;
		for (int dy = -1; dy <= 1 && !nearWater; ++dy)
		{
			glm::ivec3 checkPos = position + glm::ivec3(0, dy, 0);
			if (isWater(checkPos))
				nearWater = true;
		}

		if (!nearWater)
			return;
	}

	// Create ripple
	WaterRipple ripple;
	ripple.center = position;
	ripple.strength = std::min(impactEnergy / 100.0f, 10.0f); // Scale and cap
	ripple.radius = 0.0f;
	ripple.age = 0.0f;

	m_ripples.push_back(ripple);
}

void WaterPhysics::displaceWater(const glm::ivec3& position, float energy)
{
	if (energy < DISPLACEMENT_THRESHOLD)
		return;

	// Calculate displacement radius based on energy
	int radius = static_cast<int>(std::sqrt(energy / 50.0f));
	radius = std::clamp(radius, 1, MAX_RIPPLE_RADIUS);

	// Propagate displacement
	propagateDisplacement(position, energy, radius);

	// Create visual ripple
	createRipple(position, energy);
}

bool WaterPhysics::isUnderwater(const glm::ivec3& position) const
{
	// Check if block at position is water
	if (isWater(position))
		return true;

	// Check if below water surface
	int waterLevel = getWaterLevel(position.x, position.z);
	return position.y <= waterLevel;
}

int WaterPhysics::getWaterLevel(int worldX, int worldZ) const
{
	// Check cache first
	uint64_t key = packCoord(worldX, worldZ);
	auto it = m_waterLevelCache.find(key);
	if (it != m_waterLevelCache.end())
		return it->second;

	// Find water surface
	int level = findWaterSurface(worldX, worldZ);

	// Cache result
	m_waterLevelCache[key] = level;

	return level;
}

void WaterPhysics::fillWaterGaps()
{
	// Water flow simulation: water spreads horizontally and flows downward
	// This creates a flood-fill effect where water seeks its level

	if (!m_world)
		return;

	// We'll process water flow in chunks to avoid performance issues
	// For now, we'll scan a limited area and apply simple flow rules

	// Collect all water source blocks that can spread
	std::vector<glm::ivec3> activeWaterBlocks;

	// Scan loaded chunks for water blocks
	// Note: In a real implementation, you'd iterate through loaded chunks
	// For now, we'll use a simplified approach focusing on modified areas

	// Instead of scanning everything, we'll use a different approach:
	// Each frame, we'll process a limited number of water blocks
	// This spreads the computational load over multiple frames

	// TODO: Implement proper chunk-based water scanning
	// For now, this is a placeholder that will be activated when water is modified
}

// New method: Process water flow from a specific source position
void WaterPhysics::processWaterFlow(const glm::ivec3& sourcePos, int maxDepth)
{
	if (!m_world)
		return;

	// Check if source is actually water
	if (!isWater(sourcePos))
		return;

	// Use a queue-based flood fill to spread water
	std::vector<glm::ivec3> queue;
	std::vector<glm::ivec3> processed;

	queue.push_back(sourcePos);

	while (!queue.empty() && processed.size() < 100) // Limit per-frame processing
	{
		glm::ivec3 current = queue.back();
		queue.pop_back();

		// Skip if already processed
		bool alreadyProcessed = false;
		for (const auto& p : processed)
		{
			if (p == current)
			{
				alreadyProcessed = true;
				break;
			}
		}
		if (alreadyProcessed)
			continue;

		processed.push_back(current);

		// Water flows down first (priority)
		glm::ivec3 below = current + glm::ivec3(0, -1, 0);
		Voxel::BlockID belowBlock = getBlock(below);

		if (belowBlock == Voxel::BlockID::Air)
		{
			// Flow downward - water always tries to go down
			setBlock(below, Voxel::BlockID::Water);
			queue.push_back(below);

			// Invalidate cache
			uint64_t key = packCoord(below.x, below.z);
			m_waterLevelCache.erase(key);
			continue; // Prioritize downward flow
		}

		// If can't flow down, spread horizontally
		// Only spread if we're at the surface or there's a solid block below
		if (belowBlock != Voxel::BlockID::Air)
		{
			// Try to spread to adjacent blocks (4 cardinal directions)
			glm::ivec3 directions[] = {
				{1, 0, 0},   // East
				{-1, 0, 0},  // West
				{0, 0, 1},   // North
				{0, 0, -1}   // South
			};

			for (const auto& dir : directions)
			{
				glm::ivec3 neighbor = current + dir;
				Voxel::BlockID neighborBlock = getBlock(neighbor);

				if (neighborBlock == Voxel::BlockID::Air)
				{
					// Check if neighbor has solid block below (water doesn't float)
					glm::ivec3 neighborBelow = neighbor + glm::ivec3(0, -1, 0);
					Voxel::BlockID neighborBelowBlock = getBlock(neighborBelow);

					if (neighborBelowBlock != Voxel::BlockID::Air)
					{
						// Safe to spread water here
						setBlock(neighbor, Voxel::BlockID::Water);
						queue.push_back(neighbor);

						// Invalidate cache
						uint64_t key = packCoord(neighbor.x, neighbor.z);
						m_waterLevelCache.erase(key);
					}
				}
			}
		}
	}
}

void WaterPhysics::removeUnderwaterRocks()
{
	// Scan for non-water, non-air blocks that are below water surface
	// This would typically be called after physics settling

	// In a real implementation, this would:
	// 1. Find settled blocks underwater
	// 2. Remove them (they've "sunk")
	// 3. Optionally spawn item entities

	// Placeholder - will be expanded based on actual usage patterns
}

void WaterPhysics::clear()
{
	m_ripples.clear();
	m_waterLevelCache.clear();
}

void WaterPhysics::updateRipple(WaterRipple& ripple, float deltaTime)
{
	// Age the ripple
	ripple.age += deltaTime;

	// Expand radius
	ripple.radius += RIPPLE_SPEED * deltaTime;

	// Decay strength
	ripple.strength -= RIPPLE_DECAY * deltaTime;
	ripple.strength = std::max(0.0f, ripple.strength);

	// Optionally apply visual effects to water blocks in ripple radius
	// (This would be done in rendering, not here)
}

void WaterPhysics::propagateDisplacement(const glm::ivec3& center, float strength, int radius)
{
	// Displace water in a spherical pattern
	for (int dx = -radius; dx <= radius; ++dx)
	{
		for (int dy = -radius; dy <= radius; ++dy)
		{
			for (int dz = -radius; dz <= radius; ++dz)
			{
				glm::ivec3 offset(dx, dy, dz);
				float distance = glm::length(glm::vec3(offset));

				if (distance > static_cast<float>(radius))
					continue;

				glm::ivec3 pos = center + offset;

				// Only displace actual water blocks
				if (!isWater(pos))
					continue;

				// Calculate displacement strength (stronger at center)
				float localStrength = strength * (1.0f - distance / static_cast<float>(radius));

				// For strong displacement, temporarily remove water
				if (localStrength > 200.0f && distance < 2.0f)
				{
					// Create temporary air pocket (water will flow back)
					setBlock(pos, Voxel::BlockID::Air);

					// Invalidate water level cache for this column
					uint64_t key = packCoord(pos.x, pos.z);
					m_waterLevelCache.erase(key);
				}
			}
		}
	}
}

int WaterPhysics::findWaterSurface(int worldX, int worldZ) const
{
	// Scan from top down to find highest water block
	// In a real implementation, this would use chunk height maps

	const int MAX_HEIGHT = 256;
	const int MIN_HEIGHT = 0;

	for (int y = MAX_HEIGHT - 1; y >= MIN_HEIGHT; --y)
	{
		glm::ivec3 pos(worldX, y, worldZ);
		if (isWater(pos))
			return y;
	}

	return MIN_HEIGHT - 1; // No water found
}

bool WaterPhysics::isWater(const glm::ivec3& pos) const
{
	return getBlock(pos) == Voxel::BlockID::Water;
}

Voxel::BlockID WaterPhysics::getBlock(const glm::ivec3& pos) const
{
	if (!m_world)
		return Voxel::BlockID::Air;

	return m_world->getBlockAt(
		static_cast<float>(pos.x) + 0.5f,
		static_cast<float>(pos.y) + 0.5f,
		static_cast<float>(pos.z) + 0.5f
	);
}

void WaterPhysics::setBlock(const glm::ivec3& pos, Voxel::BlockID blockType)
{
	if (!m_world)
		return;

	m_world->setBlockAt(
		static_cast<float>(pos.x) + 0.5f,
		static_cast<float>(pos.y) + 0.5f,
		static_cast<float>(pos.z) + 0.5f,
		blockType
	);
}

} // namespace WorldPhysics

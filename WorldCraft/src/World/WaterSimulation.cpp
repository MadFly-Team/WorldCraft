#include <World/WaterSimulation.h>
#include <algorithm>
#include <cmath>

namespace World
{

int WaterSimulation::update(Chunk::ChunkWorld* world, const glm::vec3& playerPos, float maxDistance)
{
	if (!world || m_pendingUpdates.empty())
		return 0;

	// Limit updates per tick to avoid performance spikes
	constexpr int kMaxUpdatesPerTick = 100;
	int updatesProcessed = 0;
	int blocksModified = 0;

	std::vector<WaterUpdate> newUpdates;
	newUpdates.reserve(m_pendingUpdates.size() * 6); // Worst case: 6 neighbors per block

	// Process pending updates
	auto it = m_pendingUpdates.begin();
	while (it != m_pendingUpdates.end() && updatesProcessed < kMaxUpdatesPerTick)
	{
		const WaterUpdate& update = *it;

		// Only process water blocks within range of player
		float dx = static_cast<float>(update.x) - playerPos.x;
		float dy = static_cast<float>(update.y) - playerPos.y;
		float dz = static_cast<float>(update.z) - playerPos.z;
		float distSq = dx * dx + dy * dy + dz * dz;

		if (distSq <= maxDistance * maxDistance)
		{
			// Check if this position still has water
			if (isWater(world, update.x, update.y, update.z))
			{
				tryFlowFrom(world, update.x, update.y, update.z, newUpdates);
			}
		}

		it = m_pendingUpdates.erase(it);
		++updatesProcessed;
	}

	// Add new water blocks to pending updates for next tick
	for (const auto& newUpdate : newUpdates)
	{
		m_pendingUpdates.insert(newUpdate);
		++blocksModified;
	}

	return blocksModified;
}

void WaterSimulation::markForUpdate(int x, int y, int z)
{
	m_pendingUpdates.insert(WaterUpdate{ x, y, z });
}

void WaterSimulation::clear()
{
	m_pendingUpdates.clear();
}

void WaterSimulation::tryFlowFrom(Chunk::ChunkWorld* world, int x, int y, int z, 
								  std::vector<WaterUpdate>& newUpdates)
{
	// Water flow priority:
	// 1. Down first (gravity)
	// 2. Then horizontally in all 4 directions

	// Try flowing down
	if (isAir(world, x, y - 1, z))
	{
		world->setBlockAt(static_cast<float>(x), 
						 static_cast<float>(y - 1), 
						 static_cast<float>(z), 
						 Voxel::BlockID::Water);
		newUpdates.push_back(WaterUpdate{ x, y - 1, z });
		return; // Water flows down exclusively if possible
	}

	// If can't flow down, try horizontal spread
	// Check all 4 horizontal neighbors
	const int dx[] = { 1, -1, 0, 0 };
	const int dz[] = { 0, 0, 1, -1 };

	for (int i = 0; i < 4; ++i)
	{
		int nx = x + dx[i];
		int nz = z + dz[i];

		if (isAir(world, nx, y, nz))
		{
			// Check if there's a solid block below (water doesn't spread into thin air)
			if (!isAir(world, nx, y - 1, nz))
			{
				world->setBlockAt(static_cast<float>(nx), 
								 static_cast<float>(y), 
								 static_cast<float>(nz), 
								 Voxel::BlockID::Water);
				newUpdates.push_back(WaterUpdate{ nx, y, nz });
			}
		}
	}
}

bool WaterSimulation::isAir(Chunk::ChunkWorld* world, int x, int y, int z) const
{
	if (y < 0 || y >= 256) // Out of world bounds
		return false;

	Voxel::BlockID block = world->getBlockAt(static_cast<float>(x), 
											 static_cast<float>(y), 
											 static_cast<float>(z));
	return block == Voxel::BlockID::Air;
}

bool WaterSimulation::isWater(Chunk::ChunkWorld* world, int x, int y, int z) const
{
	if (y < 0 || y >= 256)
		return false;

	Voxel::BlockID block = world->getBlockAt(static_cast<float>(x), 
											 static_cast<float>(y), 
											 static_cast<float>(z));
	return block == Voxel::BlockID::Water;
}

} // namespace World

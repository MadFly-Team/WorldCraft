#pragma once

#include <Chunk/ChunkWorld.h>
#include <Voxel/BlockTypes.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>

namespace World
{

// Simple water flow simulation system
// Water spreads from source blocks into adjacent air spaces
class WaterSimulation
{
public:
	WaterSimulation() = default;

	// Update water simulation for one tick
	// Returns number of blocks modified
	int update(Chunk::ChunkWorld* world, const glm::vec3& playerPos, float maxDistance);

	// Mark a position for water flow check (called when blocks are removed near water)
	void markForUpdate(int x, int y, int z);

	// Clear all pending updates
	void clear();

private:
	struct WaterUpdate
	{
		int x, y, z;
		bool operator==(const WaterUpdate& other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
	};

	struct WaterUpdateHash
	{
		std::size_t operator()(const WaterUpdate& u) const
		{
			// Simple hash combining x, y, z
			return std::hash<int>()(u.x) ^ 
				   (std::hash<int>()(u.y) << 1) ^ 
				   (std::hash<int>()(u.z) << 2);
		}
	};

	// Pending water updates (positions where water might flow)
	std::unordered_set<WaterUpdate, WaterUpdateHash> m_pendingUpdates;

	// Try to flow water from (x,y,z) to adjacent air blocks
	void tryFlowFrom(Chunk::ChunkWorld* world, int x, int y, int z, 
					 std::vector<WaterUpdate>& newUpdates);

	// Check if a position is air (can accept water flow)
	bool isAir(Chunk::ChunkWorld* world, int x, int y, int z) const;

	// Check if a position is water
	bool isWater(Chunk::ChunkWorld* world, int x, int y, int z) const;
};

} // namespace World

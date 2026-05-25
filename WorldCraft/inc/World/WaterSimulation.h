#pragma once

#include <Chunk/ChunkWorld.h>
#include <Voxel/BlockTypes.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>

namespace World
{

// Realistic water flow simulation system
// Each source produces 15 water blocks that flow dynamically
// Water flows down first, then horizontally, and stops when blocked or hits other water
class WaterSimulation
{
public:
	WaterSimulation() = default;

	// Update water simulation for one tick
	// Returns number of blocks that moved
	// maxNewSources: maximum number of new sources to discover per update (0 = unlimited)
	int update(Chunk::ChunkWorld* world, const glm::vec3& playerPos, float maxDistance, int maxNewSources = 5);

	// Register a new water source block (player-placed water)
	void registerSource(int x, int y, int z);

	// Check if a position already has a source
	bool hasSource(int x, int y, int z) const;

	// Remove a water source and all its flowing water
	void removeSource(int x, int y, int z, Chunk::ChunkWorld* world);

	// Notify water simulation that a block was removed/changed near water
	// This will reactivate nearby settled water blocks
	void notifyBlockChange(int x, int y, int z);

	// Clear all water simulation state
	void clear();

private:
	struct Position
	{
		int x, y, z;

		bool operator==(const Position& other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}

		bool operator<(const Position& other) const
		{
			if (x != other.x) return x < other.x;
			if (y != other.y) return y < other.y;
			return z < other.z;
		}
	};

	struct PositionHash
	{
		std::size_t operator()(const Position& p) const
		{
			return std::hash<int>()(p.x) ^ 
				   (std::hash<int>()(p.y) << 1) ^ 
				   (std::hash<int>()(p.z) << 2);
		}
	};

	// Represents a single flowing water block
	struct FlowingWater
	{
		Position pos;           // Current position
		Position sourcePos;     // Which source this came from
		int blockNumber;        // Which block (0-4) from the source
		bool isSettled;         // Has this block stopped moving?
		Position pushedFrom;    // Position of the block pushing this one (or itself if source)
		bool hasPush;           // Is this block being pushed?
		int lastDx;             // Last horizontal direction moved (-1, 0, or 1)
		int lastDz;             // Last horizontal direction moved (-1, 0, or 1)
	};

	// Each source tracks its flowing water blocks
	struct WaterSource
	{
		Position pos;
		std::vector<FlowingWater> flowingBlocks;  // 15 blocks per source
		bool needsUpdate;
	};

	// Map of source positions to their water data
	std::unordered_map<Position, WaterSource, PositionHash> m_sources;

	// Map of all water block positions with count (for quick lookup)
	std::unordered_map<Position, int, PositionHash> m_waterPositions;

	// Try to move a single water block (returns true if it moved)
	bool tryMoveWater(FlowingWater& water, Chunk::ChunkWorld* world);

	// Check if a position is air (can accept water flow)
	bool isAir(Chunk::ChunkWorld* world, int x, int y, int z) const;

	// Check if a position has water (including from any source)
	bool hasWater(const Position& pos) const;
};

} // namespace World

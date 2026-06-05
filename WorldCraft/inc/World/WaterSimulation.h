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

	// NEW: Update delayed water scans (call each frame with deltaTime)
	// Processes delayed scans whose timers have expired
	void updateDelayedScans(float deltaTime, Chunk::ChunkWorld* world, int maxBlocksPerTick = 20);

	// Register a new water source block (player-placed water)
	void registerSource(int x, int y, int z);

	// Check if a position already has a source
	bool hasSource(int x, int y, int z) const;

	// Remove a water source and all its flowing water
	void removeSource(int x, int y, int z, Chunk::ChunkWorld* world);

	// Notify water simulation that a block was removed/changed near water
	// This will reactivate nearby settled water blocks
	void notifyBlockChange(int x, int y, int z);

	// NEW: Queue a delayed water scan at the given position
	// The scan will execute after WATER_SCAN_DELAY seconds
	void queueDelayedScan(int x, int y, int z);

	// NEW: Scan-based water fill - scans entire connected cavity, then fills bottom-up
	// Most robust approach: finds all air blocks, sorts by Y, fills from bottom
	// Returns number of blocks filled this call
	int scanAndFillWater(int x, int y, int z, Chunk::ChunkWorld* world, int maxBlocksPerTick = 20);

	// NEW: Continue existing scan-and-fill operation without starting a new scan
	// Only processes blocks already in the scan list - safe to call every tick
	// Returns number of blocks filled this call
	int continueScanFill(Chunk::ChunkWorld* world, int maxBlocksPerTick = 20);

	// Check if a scan-and-fill operation is currently in progress
	bool isScanInProgress() const { return !m_scanFillList.empty() && m_scanFillIndex < m_scanFillList.size(); }

	// NEW: Simple column-based water fill (more predictable, less memory-intensive)
	// Fills water in vertical columns from bottom to sea level
	// Returns number of blocks filled
	int fillWaterColumn(int x, int y, int z, Chunk::ChunkWorld* world, int maxBlocksPerTick = 10);

	// NEW: Flood-fill water into a gap starting from a position
	// Returns number of blocks filled
	// maxBlocksPerTick: how many water blocks to place per call (controls fill speed)
	int fillWaterGap(int x, int y, int z, Chunk::ChunkWorld* world, int maxBlocksPerTick = 10);

	// NEW: Process existing fill queue (continue filling without starting new fill)
	// Returns number of blocks filled
	int processFillQueue(Chunk::ChunkWorld* world, int maxBlocksPerTick = 10);

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

	// NEW: Active flood-fill queue (positions waiting to be filled)
	std::queue<Position> m_fillQueue;

	// NEW: Set of positions already in fill queue (prevents duplicates)
	std::unordered_set<Position, PositionHash> m_inFillQueue;

	// NEW: Maximum Y level for current fill operation (prevents filling above sea level)
	int m_fillMaxY = -1;

	// NEW: Fill origin (for distance limiting)
	Position m_fillOrigin{ 0, 0, 0 };

	// NEW: Scan-based fill state (sorted list of positions to fill, processed bottom-up)
	std::vector<Position> m_scanFillList;
	size_t m_scanFillIndex = 0;  // Current position in the fill list

	// NEW: Queue for follow-up scans (to avoid recursive scanning)
	std::vector<Position> m_pendingFollowUpScans;

	// NEW: Cache of recently scanned positions that produced zero fills
	// Prevents infinite loops when repeatedly scanning sea-level air pockets
	std::unordered_set<Position, PositionHash> m_recentZeroFillScans;
	static constexpr size_t MAX_ZERO_FILL_CACHE = 100;  // Limit cache size

	// NEW: Delayed scan queue - positions waiting for delayed water fill check
	struct DelayedScan
	{
		Position pos;
		float timeRemaining;  // Seconds until scan should trigger
	};
	std::vector<DelayedScan> m_delayedScans;
	static constexpr float WATER_SCAN_DELAY = 1.0f;  // 1 second delay before water fills

	// NEW: Maximum fill radius (prevents runaway fills)
	static constexpr int MAX_FILL_RADIUS = 20;  // Don't fill more than 20 blocks from origin
	static constexpr size_t MAX_QUEUE_SIZE = 1000;  // Limit queue size to prevent memory explosion


	// Try to move a single water block (returns true if it moved)
	bool tryMoveWater(FlowingWater& water, Chunk::ChunkWorld* world);

	// Check if a position is air (can accept water flow)
	bool isAir(Chunk::ChunkWorld* world, int x, int y, int z) const;

	// Check if a position has water (including from any source)
	bool hasWater(const Position& pos) const;
};

} // namespace World

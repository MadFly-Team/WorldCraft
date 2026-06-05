#include <World/WaterSimulation.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

namespace World
{

int WaterSimulation::update(Chunk::ChunkWorld* world, const glm::vec3& playerPos, float maxDistance, int maxNewSources)
{
	if (!world || m_sources.empty())
		return 0;

	int blocksMoved = 0;

	// Process each source's flowing water blocks
	for (auto& [sourcePos, source] : m_sources)
	{
		// Skip sources too far from player
		float dx = static_cast<float>(sourcePos.x) - playerPos.x;
		float dy = static_cast<float>(sourcePos.y) - playerPos.y;
		float dz = static_cast<float>(sourcePos.z) - playerPos.z;
		float distSq = dx * dx + dy * dy + dz * dz;

		if (distSq > maxDistance * maxDistance)
			continue;

		// SIMPLIFIED: Only reactivate settled water if there's EMPTY SPACE BELOW
		// This prevents the constant churn and allows water to reach equilibrium
		for (FlowingWater& water : source.flowingBlocks)
		{
			if (water.isSettled)
			{
				Position down{ water.pos.x, water.pos.y - 1, water.pos.z };

				// ONLY reactivate if air appears directly below (gravity)
				if (isAir(world, down.x, down.y, down.z) && !hasWater(down))
				{
					water.isSettled = false;
					water.hasPush = true;
					water.pushedFrom = water.pos;
				}
			}
		}

		// Try to move each unsettled water block
		for (FlowingWater& water : source.flowingBlocks)
		{
			// Water at source always has push
			if (water.pos == water.sourcePos)
			{
				water.hasPush = true;
				water.isSettled = false;
			}

			if (!water.isSettled)
			{
				if (tryMoveWater(water, world))
				{
					++blocksMoved;
				}
			}
		}
	}

	return blocksMoved;
}

void WaterSimulation::updateDelayedScans(float deltaTime, Chunk::ChunkWorld* world, int maxBlocksPerTick)
{
	if (m_delayedScans.empty())
		return;

	std::vector<DelayedScan> remainingScans;
	int scansProcessed = 0;

	for (auto& scan : m_delayedScans)
	{
		scan.timeRemaining -= deltaTime;

		if (scan.timeRemaining <= 0.0f)
		{
			// Time expired - trigger the scan
			std::cout << "[WaterSim] Delayed scan triggered at (" 
					  << scan.pos.x << ", " << scan.pos.y << ", " << scan.pos.z 
					  << ") after 1 second delay" << std::endl;

			scanAndFillWater(scan.pos.x, scan.pos.y, scan.pos.z, world, maxBlocksPerTick);
			scansProcessed++;
		}
		else
		{
			// Still waiting - keep it in the queue
			remainingScans.push_back(scan);
		}
	}

	m_delayedScans = std::move(remainingScans);

	if (scansProcessed > 0)
	{
		std::cout << "[WaterSim] Processed " << scansProcessed << " delayed scans, " 
				  << m_delayedScans.size() << " remaining" << std::endl;
	}
}

void WaterSimulation::queueDelayedScan(int x, int y, int z)
{
	Position pos{ x, y, z };

	// Check if this position is already in the delayed queue
	for (const auto& scan : m_delayedScans)
	{
		if (scan.pos == pos)
		{
			// Already queued - don't add duplicate
			return;
		}
	}

	// Add new delayed scan
	DelayedScan newScan;
	newScan.pos = pos;
	newScan.timeRemaining = WATER_SCAN_DELAY;

	m_delayedScans.push_back(newScan);

	std::cout << "[WaterSim] Queued delayed water scan at (" << x << ", " << y << ", " << z 
			  << ") - will trigger in " << WATER_SCAN_DELAY << " seconds" << std::endl;
}

void WaterSimulation::registerSource(int x, int y, int z)
{
	Position sourcePos{ x, y, z };

	// Check if a source already exists at this position
	if (m_sources.find(sourcePos) != m_sources.end())
	{
		// Source already exists, don't recreate it
		return;
	}

	// SAFETY: Limit total number of active water sources to prevent memory issues
	const size_t MAX_ACTIVE_SOURCES = 100; // Reduced from 500 - more conservative limit
	if (m_sources.size() >= MAX_ACTIVE_SOURCES)
	{
		static bool hasWarnedAboutLimit = false;
		if (!hasWarnedAboutLimit)
		{
			std::cout << "[WaterSim] WARNING: Maximum water source limit (" << MAX_ACTIVE_SOURCES 
					  << ") reached. Water will not flow from new sources until existing ones settle." << std::endl;
			hasWarnedAboutLimit = true;
		}
		return;
	}

	// Create new source with 8 flowing water blocks (reduced from 15 for stability)
	WaterSource source;
	source.pos = sourcePos;
	source.needsUpdate = true;

	// Create water blocks that will flow from this source
	// Initially they all start at the source position
	for (int i = 0; i < 8; ++i)
	{
		FlowingWater water;
		water.pos = sourcePos;
		water.sourcePos = sourcePos;
		water.blockNumber = i;
		water.isSettled = false;
		water.pushedFrom = sourcePos;  // Source pushes itself (always has push)
		water.hasPush = true;           // Source blocks always have push
		water.lastDx = 0;               // No initial direction
		water.lastDz = 0;
		source.flowingBlocks.push_back(water);
	}

	m_sources[sourcePos] = source;
	m_waterPositions[sourcePos] = 8; // 8 blocks start at source
}

bool WaterSimulation::hasSource(int x, int y, int z) const
{
	Position pos{ x, y, z };
	return m_sources.find(pos) != m_sources.end();
}

void WaterSimulation::removeSource(int x, int y, int z, Chunk::ChunkWorld* world)
{
	Position sourcePos{ x, y, z };

	auto it = m_sources.find(sourcePos);
	if (it == m_sources.end())
		return;

	// Remove all water blocks belonging to this source
	for (const FlowingWater& water : it->second.flowingBlocks)
	{
		// Decrease count at position
		auto posIt = m_waterPositions.find(water.pos);
		if (posIt != m_waterPositions.end())
		{
			if (--posIt->second <= 0)
			{
				m_waterPositions.erase(posIt);
				// Remove water block from world if this was the last water block here
				world->setBlockAt(static_cast<float>(water.pos.x),
								 static_cast<float>(water.pos.y),
								 static_cast<float>(water.pos.z),
								 Voxel::BlockID::Air);
			}
		}
	}

	m_sources.erase(it);
}

void WaterSimulation::notifyBlockChange(int x, int y, int z)
{
	// When a block is removed/changed, check all water within 3 blocks (including diagonals)
	// and reactivate any settled water that might now be able to flow
	Position changedPos{ x, y, z };

	// Also clear the zero-fill cache for nearby positions since the block change
	// may have created new fillable cavities or removed obstacles
	// Check a small radius around the changed position
	const int CACHE_CLEAR_RADIUS = 5;
	std::vector<Position> positionsToRemove;

	for (const auto& cachedPos : m_recentZeroFillScans)
	{
		int dx = cachedPos.x - x;
		int dy = cachedPos.y - y;
		int dz = cachedPos.z - z;
		int distSq = dx * dx + dy * dy + dz * dz;

		if (distSq <= CACHE_CLEAR_RADIUS * CACHE_CLEAR_RADIUS)
		{
			positionsToRemove.push_back(cachedPos);
		}
	}

	for (const auto& pos : positionsToRemove)
	{
		m_recentZeroFillScans.erase(pos);
	}

	if (!positionsToRemove.empty())
	{
		std::cout << "[WaterSim] Cleared " << positionsToRemove.size() 
				  << " cached zero-fill positions near block change at (" 
				  << x << "," << y << "," << z << ")" << std::endl;
	}

	for (auto& [sourcePos, source] : m_sources)
	{
		for (FlowingWater& water : source.flowingBlocks)
		{
			// Calculate distance to changed block
			int dx = water.pos.x - changedPos.x;
			int dy = water.pos.y - changedPos.y;
			int dz = water.pos.z - changedPos.z;
			int distSq = dx * dx + dy * dy + dz * dz;

			// If within 3 blocks (distSq <= 9), check if this water should reactivate
			if (distSq <= 9)
			{
				// Always reactivate settled water near a block change
				if (water.isSettled)
				{
					water.isSettled = false;
					water.hasPush = true;
				}
				// Also ensure unsettled water stays active
				else
				{
					water.hasPush = true;
				}
			}
		}
	}
}

int WaterSimulation::scanAndFillWater(int x, int y, int z, Chunk::ChunkWorld* world, int maxBlocksPerTick)
{
	// SCAN-AND-FILL APPROACH:
	// 1. If we have an existing fill list, continue filling from it
	// 2. Otherwise, scan the entire connected cavity for all air blocks
	// 3. Sort by Y coordinate (bottom-up)
	// 4. Fill incrementally over multiple calls

	// PHASE 1: Continue filling from existing scan if we have one
	if (!m_scanFillList.empty() && m_scanFillIndex < m_scanFillList.size())
	{
		int blocksFilled = 0;

		while (m_scanFillIndex < m_scanFillList.size() && blocksFilled < maxBlocksPerTick)
		{
			Position pos = m_scanFillList[m_scanFillIndex];
			m_scanFillIndex++;

			// Double-check this is still air (might have been filled by spreading)
			Voxel::BlockID block = world->getBlockAt(
				static_cast<float>(pos.x),
				static_cast<float>(pos.y),
				static_cast<float>(pos.z)
			);

			if (block == Voxel::BlockID::Air)
			{
				world->setBlockAt(
					static_cast<float>(pos.x),
					static_cast<float>(pos.y),
					static_cast<float>(pos.z),
					Voxel::BlockID::Water
				);
				blocksFilled++;
			}
		}

		if (blocksFilled > 0)
		{
			std::cout << "[ScanFill] Filled " << blocksFilled << " blocks, progress: " 
					  << m_scanFillIndex << "/" << m_scanFillList.size() << std::endl;
		}

		// If we finished the list, clear it
		if (m_scanFillIndex >= m_scanFillList.size())
		{
			std::cout << "[ScanFill] Complete! Total blocks filled: " << m_scanFillList.size() << std::endl;
			m_scanFillList.clear();
			m_scanFillIndex = 0;
		}

		return blocksFilled;
	}

	// PHASE 2: Start a new scan
	Voxel::BlockID blockAtPos = world->getBlockAt(
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(z)
	);

	// Only start scan if this is air
	if (blockAtPos != Voxel::BlockID::Air)
		return 0;

	// Check if we recently scanned this exact position and found 0 blocks to fill
	// This prevents infinite loops when callbacks repeatedly trigger at sea level
	Position scanPos{x, y, z};
	if (m_recentZeroFillScans.count(scanPos) > 0)
	{
		std::cout << "[ScanFill] Skipping scan at (" << x << "," << y << "," << z 
				  << ") - recently scanned with zero fills" << std::endl;
		return 0;
	}

	// Check if adjacent to water
	const int dx[] = { -1, 1, 0, 0, 0, 0 };
	const int dy[] = { 0, 0, -1, 1, 0, 0 };
	const int dz[] = { 0, 0, 0, 0, -1, 1 };

	bool adjacentToWater = false;
	int maxWaterY = -1;

	// Quick adjacency check
	for (int i = 0; i < 6; ++i)
	{
		Voxel::BlockID neighbor = world->getBlockAt(
			static_cast<float>(x + dx[i]),
			static_cast<float>(y + dy[i]),
			static_cast<float>(z + dz[i])
		);
		if (neighbor == Voxel::BlockID::Water)
		{
			adjacentToWater = true;
			if (y + dy[i] > maxWaterY)
				maxWaterY = y + dy[i];
		}
	}

	if (!adjacentToWater)
		return 0;

	std::cout << "[ScanFill] Starting scan at (" << x << "," << y << "," << z << ")" << std::endl;

	// PHASE 3: Scan the entire connected cavity using BFS
	std::queue<Position> scanQueue;
	std::unordered_set<Position, PositionHash> scanned;

	scanQueue.push(Position{x, y, z});
	scanned.insert(Position{x, y, z});

	// Also scan for water to find the true sea level
	const int SCAN_RADIUS = 5;
	for (int sy = -2; sy <= 5; ++sy)
	{
		for (int sx = -SCAN_RADIUS; sx <= SCAN_RADIUS; ++sx)
		{
			for (int sz = -SCAN_RADIUS; sz <= SCAN_RADIUS; ++sz)
			{
				Voxel::BlockID block = world->getBlockAt(
					static_cast<float>(x + sx),
					static_cast<float>(y + sy),
					static_cast<float>(z + sz)
				);
				if (block == Voxel::BlockID::Water && (y + sy) > maxWaterY)
				{
					maxWaterY = y + sy;
				}
			}
		}
	}

	int seaLevel = maxWaterY;
	std::cout << "[ScanFill] Detected sea level at Y=" << seaLevel << std::endl;

	// BFS to find all connected air blocks (limited by radius and sea level)
	const int MAX_SCAN_SIZE = 500;  // Prevent memory bloat - max 500 blocks per scan
	while (!scanQueue.empty() && m_scanFillList.size() < MAX_SCAN_SIZE)
	{
		Position current = scanQueue.front();
		scanQueue.pop();

		// Distance check
		int dist = std::max({
			abs(current.x - x),
			abs(current.y - y),
			abs(current.z - z)
		});

		if (dist > MAX_FILL_RADIUS)
			continue;

		// Don't scan above sea level - STRICT enforcement
		if (seaLevel >= 0 && current.y > seaLevel)
			continue;

		// Add to fill list
		m_scanFillList.push_back(current);

		// Scan all 6 neighbors
		for (int i = 0; i < 6; ++i)
		{
			Position neighbor{
				current.x + dx[i],
				current.y + dy[i],
				current.z + dz[i]
			};

			// Skip if already scanned
			if (scanned.find(neighbor) != scanned.end())
				continue;

			// Skip if out of bounds
			if (neighbor.y < 0 || neighbor.y >= 256)
				continue;

			// Check if it's air
			Voxel::BlockID block = world->getBlockAt(
				static_cast<float>(neighbor.x),
				static_cast<float>(neighbor.y),
				static_cast<float>(neighbor.z)
			);

			if (block == Voxel::BlockID::Air)
			{
				scanQueue.push(neighbor);
				scanned.insert(neighbor);
			}
		}
	}

	// PHASE 4: Sort by Y coordinate (bottom-up fill)
	std::sort(m_scanFillList.begin(), m_scanFillList.end(),
		[](const Position& a, const Position& b) {
			return a.y < b.y;  // Lowest Y first
		});

	std::cout << "[ScanFill] Scan complete! Found " << m_scanFillList.size() 
			  << " air blocks to fill (bottom-up)" << std::endl;

	// If zero blocks found, cache this scan position to prevent re-scanning
	if (m_scanFillList.empty())
	{
		Position scanStart{x, y, z};
		m_recentZeroFillScans.insert(scanStart);

		// Limit cache size to prevent unbounded growth
		if (m_recentZeroFillScans.size() > MAX_ZERO_FILL_CACHE)
		{
			// Clear the oldest half of the cache (simple eviction strategy)
			m_recentZeroFillScans.clear();
		}

		std::cout << "[ScanFill] Cached zero-fill position (" << scanStart.x << "," 
				  << scanStart.y << "," << scanStart.z << ") to prevent re-scan" << std::endl;

		return 0;  // No blocks to fill, exit early
	}

	// Start filling immediately
	m_scanFillIndex = 0;
	return scanAndFillWater(x, y, z, world, maxBlocksPerTick);  // Recursive call to start filling
}

int WaterSimulation::continueScanFill(Chunk::ChunkWorld* world, int maxBlocksPerTick)
{
	// CONTINUE-ONLY METHOD: Only processes existing scan list, never starts a new scan
	// Safe to call every tick - will return 0 if no work to do

	if (m_scanFillList.empty() || m_scanFillIndex >= m_scanFillList.size())
		return 0;  // No work to do

	int blocksFilled = 0;

	while (m_scanFillIndex < m_scanFillList.size() && blocksFilled < maxBlocksPerTick)
	{
		Position pos = m_scanFillList[m_scanFillIndex];
		m_scanFillIndex++;

		// Double-check this is still air (might have been filled by spreading)
		Voxel::BlockID block = world->getBlockAt(
			static_cast<float>(pos.x),
			static_cast<float>(pos.y),
			static_cast<float>(pos.z)
		);

		if (block == Voxel::BlockID::Air)
		{
			world->setBlockAt(
				static_cast<float>(pos.x),
				static_cast<float>(pos.y),
				static_cast<float>(pos.z),
				Voxel::BlockID::Water
			);
			blocksFilled++;
		}
	}

	if (blocksFilled > 0)
	{
		std::cout << "[ScanFill] Continue: Filled " << blocksFilled << " blocks, progress: " 
				  << m_scanFillIndex << "/" << m_scanFillList.size() << std::endl;
	}

	// If we finished the list, check for any remaining nearby air pockets
	if (m_scanFillIndex >= m_scanFillList.size())
	{
		std::cout << "[ScanFill] Complete! Total blocks filled: " << m_scanFillList.size() << std::endl;

		// Only check for follow-ups if we actually filled something
		// If 0 blocks were filled, the air pocket is likely above sea level or invalid
		size_t totalFilled = m_scanFillList.size();

		// Before clearing, save the last filled position to check for nearby air pockets
		glm::ivec3 lastFilledPos(0, 0, 0);
		int maxFilledY = -1;  // Track the highest Y we filled (= sea level)

		if (!m_scanFillList.empty())
		{
			// Use the middle position from the scan as reference
			size_t midIndex = m_scanFillList.size() / 2;
			lastFilledPos = glm::ivec3(
				m_scanFillList[midIndex].x,
				m_scanFillList[midIndex].y,
				m_scanFillList[midIndex].z
			);

			// Find the maximum Y coordinate in the filled list (this is the sea level)
			for (const auto& pos : m_scanFillList)
			{
				if (pos.y > maxFilledY)
					maxFilledY = pos.y;
			}
		}

		m_scanFillList.clear();
		m_scanFillIndex = 0;

		// Check for remaining air pockets ONLY if we filled at least 1 block
		// This prevents infinite loops when scanning air above sea level
		if (totalFilled > 0 && lastFilledPos != glm::ivec3(0, 0, 0) && m_pendingFollowUpScans.empty())
		{
			// Increased radius to 7 for very large destruction zones
			// This helps catch all disconnected pockets in massive blast areas
			const int CHECK_RADIUS = 7;
			std::vector<glm::ivec3> foundPockets;  // Collect all air pockets first

			for (int dy = -CHECK_RADIUS; dy <= CHECK_RADIUS; ++dy)
			{
				for (int dx = -CHECK_RADIUS; dx <= CHECK_RADIUS; ++dx)
				{
					for (int dz = -CHECK_RADIUS; dz <= CHECK_RADIUS; ++dz)
					{
						glm::ivec3 checkPos = lastFilledPos + glm::ivec3(dx, dy, dz);

						// Skip positions above sea level (maxFilledY + 1 layer tolerance)
						// Allow checking one layer above the highest filled Y to catch edge cases
						// where the top layer wasn't included in the initial scan
						if (maxFilledY >= 0 && checkPos.y > maxFilledY + 1)
							continue;

						Voxel::BlockID block = world->getBlockAt(
							static_cast<float>(checkPos.x),
							static_cast<float>(checkPos.y),
							static_cast<float>(checkPos.z)
						);

						if (block == Voxel::BlockID::Air)
						{
							// Check if adjacent to water
							static const glm::ivec3 adjacentOffsets[] = {
								{-1, 0, 0}, {1, 0, 0},
								{0, -1, 0}, {0, 1, 0},
								{0, 0, -1}, {0, 0, 1}
							};

							bool hasWaterNeighbor = false;
							bool hasWaterBelow = false;

							for (const auto& offset : adjacentOffsets)
							{
								glm::ivec3 neighborPos = checkPos + offset;
								Voxel::BlockID neighbor = world->getBlockAt(
									static_cast<float>(neighborPos.x),
									static_cast<float>(neighborPos.y),
									static_cast<float>(neighborPos.z)
								);

								if (neighbor == Voxel::BlockID::Water)
								{
									hasWaterNeighbor = true;
									if (offset.y < 0)  // Check if water is below
										hasWaterBelow = true;
								}
							}

							// Only queue if there's water nearby AND either:
							// 1. Water is below (submerged air pocket), OR
							// 2. The position is not above maxFilledY (within the already-filled region)
							if (hasWaterNeighbor && (hasWaterBelow || checkPos.y <= maxFilledY))
							{
								// Found a remaining air pocket - add to list for queueing
								foundPockets.push_back(checkPos);
							}
						}
					}
				}
			}

			// Queue all found pockets (increased limit for large destruction zones)
			// Allow more follow-ups to handle massive blast zones properly
			const int MAX_FOLLOWUPS_PER_SCAN = 10;
			int queuedCount = 0;
			for (const auto& pocket : foundPockets)
			{
				if (queuedCount >= MAX_FOLLOWUPS_PER_SCAN)
					break;

				std::cout << "[ScanFill] Found remaining air pocket at ("
						  << pocket.x << ", " << pocket.y << ", " << pocket.z
						  << "), queuing for follow-up scan..." << std::endl;
				m_pendingFollowUpScans.push_back(Position{pocket.x, pocket.y, pocket.z});
				queuedCount++;
			}

			if (foundPockets.size() > MAX_FOLLOWUPS_PER_SCAN)
			{
				std::cout << "[ScanFill] Found " << foundPockets.size() << " air pockets, queued "
						  << MAX_FOLLOWUPS_PER_SCAN << " (limited)" << std::endl;
			}
		}

		// If we have a pending follow-up, start it now (but only if the previous scan was productive)
		if (!m_pendingFollowUpScans.empty() && totalFilled > 0)
		{
			Position followUp = m_pendingFollowUpScans.back();
			m_pendingFollowUpScans.pop_back();
			std::cout << "[ScanFill] Starting follow-up scan at (" 
					  << followUp.x << ", " << followUp.y << ", " << followUp.z << ")" << std::endl;
			return scanAndFillWater(followUp.x, followUp.y, followUp.z, world, maxBlocksPerTick);
		}
		else if (!m_pendingFollowUpScans.empty())
		{
			// If totalFilled == 0, the follow-up pocket is invalid (above sea level or already filled)
			// Clear the pending queue to prevent infinite loops
			std::cout << "[ScanFill] Discarding " << m_pendingFollowUpScans.size() 
					  << " pending follow-up scan(s) - no blocks filled in previous scan" << std::endl;
			m_pendingFollowUpScans.clear();
		}
	}

	return blocksFilled;
}

int WaterSimulation::fillWaterColumn(int x, int y, int z, Chunk::ChunkWorld* world, int maxBlocksPerTick)
{
	// Enhanced column fill with horizontal spreading and proper sea level detection

	Voxel::BlockID blockAtPos = world->getBlockAt(
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(z)
	);

	// Only start fill if this is air (a gap)
	if (blockAtPos != Voxel::BlockID::Air)
		return 0;

	// Check if immediately adjacent (6 directions) to water
	const int dx[] = { -1, 1, 0, 0, 0, 0 };
	const int dy[] = { 0, 0, -1, 1, 0, 0 };
	const int dz[] = { 0, 0, 0, 0, -1, 1 };

	bool adjacentToWater = false;
	int maxWaterY = -1;  // Find the highest water level nearby

	// PHASE 0: Scan horizontally around this position to find the true sea level
	// This prevents filling above the actual water surface
	const int SCAN_RADIUS = 5;
	for (int sy = -2; sy <= 5; ++sy)  // Look a bit below and well above
	{
		for (int sx = -SCAN_RADIUS; sx <= SCAN_RADIUS; ++sx)
		{
			for (int sz = -SCAN_RADIUS; sz <= SCAN_RADIUS; ++sz)
			{
				int checkX = x + sx;
				int checkY = y + sy;
				int checkZ = z + sz;

				Voxel::BlockID block = world->getBlockAt(
					static_cast<float>(checkX),
					static_cast<float>(checkY),
					static_cast<float>(checkZ)
				);

				if (block == Voxel::BlockID::Water)
				{
					// This is the highest water we've found
					if (checkY > maxWaterY)
						maxWaterY = checkY;

					// Check if this water is adjacent to our position
					if (abs(sx) <= 1 && abs(sy) <= 1 && abs(sz) <= 1)
						adjacentToWater = true;
				}
			}
		}
	}

	if (!adjacentToWater)
		return 0;

	// The sea level is the highest water block we found
	int seaLevel = maxWaterY;

	if (seaLevel < 0)
	{
		// No water found nearby, don't fill
		return 0;
	}

	std::cout << "[WaterColumn] Starting fill at (" << x << "," << y << "," << z 
			  << ") with sea level Y=" << seaLevel << std::endl;

	// Fill the column from current position down to solid ground, then up to sea level
	int blocksFilled = 0;

	// PHASE 1: Fill downward to find the floor
	for (int checkY = y - 1; checkY >= 0 && blocksFilled < maxBlocksPerTick; --checkY)
	{
		Voxel::BlockID block = world->getBlockAt(
			static_cast<float>(x),
			static_cast<float>(checkY),
			static_cast<float>(z)
		);

		if (block == Voxel::BlockID::Air)
		{
			world->setBlockAt(
				static_cast<float>(x),
				static_cast<float>(checkY),
				static_cast<float>(z),
				Voxel::BlockID::Water
			);
			blocksFilled++;
		}
		else
		{
			// Hit solid ground or water, stop going down
			break;
		}
	}

	// PHASE 2: Fill current position
	world->setBlockAt(
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(z),
		Voxel::BlockID::Water
	);
	blocksFilled++;

	// PHASE 3: Fill upward to sea level (but NOT above it)
	for (int checkY = y + 1; checkY <= seaLevel && blocksFilled < maxBlocksPerTick; ++checkY)
	{
		Voxel::BlockID block = world->getBlockAt(
			static_cast<float>(x),
			static_cast<float>(checkY),
			static_cast<float>(z)
		);

		if (block == Voxel::BlockID::Air)
		{
			world->setBlockAt(
				static_cast<float>(x),
				static_cast<float>(checkY),
				static_cast<float>(z),
				Voxel::BlockID::Water
			);
			blocksFilled++;
		}
		else if (block != Voxel::BlockID::Water)
		{
			// Hit solid block, stop going up
			break;
		}
	}

	// PHASE 4: Horizontal spreading in a small radius (fills connected gaps)
	// This ensures nearby horizontal gaps get filled without runaway memory usage
	const int SPREAD_RADIUS = 3;  // Only spread 3 blocks horizontally

	for (int spreadY = y - 1; spreadY <= seaLevel && blocksFilled < maxBlocksPerTick; ++spreadY)
	{
		for (int spreadX = -SPREAD_RADIUS; spreadX <= SPREAD_RADIUS && blocksFilled < maxBlocksPerTick; ++spreadX)
		{
			for (int spreadZ = -SPREAD_RADIUS; spreadZ <= SPREAD_RADIUS && blocksFilled < maxBlocksPerTick; ++spreadZ)
			{
				// Skip the center column (already filled)
				if (spreadX == 0 && spreadZ == 0)
					continue;

				int fillX = x + spreadX;
				int fillZ = z + spreadZ;

				Voxel::BlockID neighborBlock = world->getBlockAt(
					static_cast<float>(fillX),
					static_cast<float>(spreadY),
					static_cast<float>(fillZ)
				);

				// Only fill if it's air
				if (neighborBlock == Voxel::BlockID::Air)
				{
					// Make sure it's adjacent to water (or will be after we fill)
					bool nextToWater = false;
					for (int i = 0; i < 6; ++i)
					{
						int adjX = fillX + dx[i];
						int adjY = spreadY + dy[i];
						int adjZ = fillZ + dz[i];

						Voxel::BlockID adjBlock = world->getBlockAt(
							static_cast<float>(adjX),
							static_cast<float>(adjY),
							static_cast<float>(adjZ)
						);

						if (adjBlock == Voxel::BlockID::Water)
						{
							nextToWater = true;
							break;
						}
					}

					if (nextToWater)
					{
						world->setBlockAt(
							static_cast<float>(fillX),
							static_cast<float>(spreadY),
							static_cast<float>(fillZ),
							Voxel::BlockID::Water
						);
						blocksFilled++;
					}
				}
			}
		}
	}

	if (blocksFilled > 0)
	{
		std::cout << "[WaterColumn] Filled " << blocksFilled << " blocks (vertical + horizontal spread) at (" 
				  << x << "," << y << "," << z << "), sea level Y=" << seaLevel << std::endl;
	}

	return blocksFilled;
}

int WaterSimulation::fillWaterGap(int x, int y, int z, Chunk::ChunkWorld* world, int maxBlocksPerTick)
{
	// If this position is already queued or is not adjacent to water, don't start a fill
	Voxel::BlockID blockAtPos = world->getBlockAt(
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(z)
	);

	// Only start fill if this is air (a gap)
	if (blockAtPos != Voxel::BlockID::Air)
		return 0;

	const int dx[] = { -1, 1, 0, 0, 0, 0 };
	const int dy[] = { 0, 0, -1, 1, 0, 0 };
	const int dz[] = { 0, 0, 0, 0, -1, 1 };

	// FIRST: Check if immediately adjacent (6 directions) to water
	// This ensures water can only flow into gaps that are directly connected
	bool immediatelyAdjacentToWater = false;
	for (int i = 0; i < 6; ++i)
	{
		int checkX = x + dx[i];
		int checkY = y + dy[i];
		int checkZ = z + dz[i];

		Voxel::BlockID neighborBlock = world->getBlockAt(
			static_cast<float>(checkX),
			static_cast<float>(checkY),
			static_cast<float>(checkZ)
		);

		if (neighborBlock == Voxel::BlockID::Water)
		{
			immediatelyAdjacentToWater = true;
			break;
		}
	}

	// If not immediately adjacent to water, don't start a fill
	if (!immediatelyAdjacentToWater)
		return 0;

	// SECOND: Find the sea level by scanning for water at/above this position
	// Only scan horizontally and upward (water flows down, not up through solid blocks)
	int maxWaterY = -1;
	const int SCAN_RADIUS = 5;

	for (int sy = 0; sy <= SCAN_RADIUS; ++sy)  // Only scan at or above current position
	{
		for (int sx = -SCAN_RADIUS; sx <= SCAN_RADIUS; ++sx)
		{
			for (int sz = -SCAN_RADIUS; sz <= SCAN_RADIUS; ++sz)
			{
				int checkX = x + sx;
				int checkY = y + sy;
				int checkZ = z + sz;

				Voxel::BlockID neighborBlock = world->getBlockAt(
					static_cast<float>(checkX),
					static_cast<float>(checkY),
					static_cast<float>(checkZ)
				);

				if (neighborBlock == Voxel::BlockID::Water)
				{
					// Track the highest water level found
					if (checkY > maxWaterY)
						maxWaterY = checkY;
				}
			}
		}
	}

	// Add to queue if not already queued
	Position pos{ x, y, z };
	if (m_inFillQueue.find(pos) == m_inFillQueue.end())
	{
		// Set the fill origin for distance limiting
		m_fillOrigin = pos;

		m_fillQueue.push(pos);
		m_inFillQueue.insert(pos);

		// Set or update the max Y level for this fill operation
		if (maxWaterY > m_fillMaxY)
		{
			m_fillMaxY = maxWaterY;
			std::cout << "[WaterFill] Starting fill at (" << x << "," << y << "," << z 
					  << ") with sea level Y=" << m_fillMaxY << ", max radius=" << MAX_FILL_RADIUS << std::endl;
		}
	}

	// Process the fill queue (flood-fill algorithm)
	int blocksFilled = 0;

	while (!m_fillQueue.empty() && blocksFilled < maxBlocksPerTick)
	{
		Position current = m_fillQueue.front();
		m_fillQueue.pop();
		m_inFillQueue.erase(current);

		// CRITICAL: Don't fill above the detected sea level
		if (current.y > m_fillMaxY)
		{
			continue;
		}

		// Check if this position is still air (might have been filled already)
		Voxel::BlockID currentBlock = world->getBlockAt(
			static_cast<float>(current.x),
			static_cast<float>(current.y),
			static_cast<float>(current.z)
		);

		if (currentBlock != Voxel::BlockID::Air)
			continue;

		// PRIORITY 1: Check if there's air below - if so, don't fill here yet
		// Water should fill from bottom up
		Position below{ current.x, current.y - 1, current.z };
		if (below.y >= 0)
		{
			Voxel::BlockID blockBelow = world->getBlockAt(
				static_cast<float>(below.x),
				static_cast<float>(below.y),
				static_cast<float>(below.z)
			);

			if (blockBelow == Voxel::BlockID::Air)
			{
				// Add the position below to queue instead (only if not above sea level)
				if (below.y <= m_fillMaxY && m_inFillQueue.find(below) == m_inFillQueue.end())
				{
					m_fillQueue.push(below);
					m_inFillQueue.insert(below);
				}
				continue; // Don't fill this position yet
			}
		}

		// PRIORITY 2: Fill this position with water
		world->setBlockAt(
			static_cast<float>(current.x),
			static_cast<float>(current.y),
			static_cast<float>(current.z),
			Voxel::BlockID::Water
		);
		blocksFilled++;

		// PRIORITY 3: Add horizontal neighbors and position above to queue
		// Check all 6 adjacent positions
		for (int i = 0; i < 6; ++i)
		{
			Position neighbor{ current.x + dx[i], current.y + dy[i], current.z + dz[i] };

			// Bounds check
			if (neighbor.y < 0 || neighbor.y >= 256)
				continue;

			// CRITICAL: Don't queue positions above sea level
			if (neighbor.y > m_fillMaxY)
				continue;

			// Check if neighbor is air
			Voxel::BlockID neighborBlock = world->getBlockAt(
				static_cast<float>(neighbor.x),
				static_cast<float>(neighbor.y),
				static_cast<float>(neighbor.z)
			);

			if (neighborBlock == Voxel::BlockID::Air)
			{
				// Add to queue if not already there
				if (m_inFillQueue.find(neighbor) == m_inFillQueue.end())
				{
					m_fillQueue.push(neighbor);
					m_inFillQueue.insert(neighbor);
				}
			}
		}
	}

	if (blocksFilled > 0)
	{
		std::cout << "[WaterFill] Filled " << blocksFilled << " blocks, " 
				  << m_fillQueue.size() << " positions remaining in queue (sea level Y=" << m_fillMaxY << ")" << std::endl;
	}

	// If queue is empty, reset max Y for next fill operation
	if (m_fillQueue.empty())
	{
		m_fillMaxY = -1;
	}

	return blocksFilled;
}

int WaterSimulation::processFillQueue(Chunk::ChunkWorld* world, int maxBlocksPerTick)
{
	// Just process the existing queue without validation
	// This is called each tick to continue filling
	if (m_fillQueue.empty())
		return 0;

	const int dx[] = { -1, 1, 0, 0, 0, 0 };
	const int dy[] = { 0, 0, -1, 1, 0, 0 };
	const int dz[] = { 0, 0, 0, 0, -1, 1 };

	int blocksFilled = 0;

	while (!m_fillQueue.empty() && blocksFilled < maxBlocksPerTick)
	{
		// Safety check: if queue is too large, abort
		if (m_fillQueue.size() > MAX_QUEUE_SIZE)
		{
			std::cout << "[WaterFill] ERROR: Queue size exceeded " << MAX_QUEUE_SIZE 
					  << ", aborting fill to prevent memory explosion" << std::endl;
			// Clear the queue to prevent further issues
			while (!m_fillQueue.empty())
				m_fillQueue.pop();
			m_inFillQueue.clear();
			m_fillMaxY = -1;
			return blocksFilled;
		}

		Position current = m_fillQueue.front();
		m_fillQueue.pop();
		m_inFillQueue.erase(current);

		// CRITICAL: Distance check - don't fill too far from origin
		int dx_origin = abs(current.x - m_fillOrigin.x);
		int dy_origin = abs(current.y - m_fillOrigin.y);
		int dz_origin = abs(current.z - m_fillOrigin.z);
		int maxDist = std::max({dx_origin, dy_origin, dz_origin});

		if (maxDist > MAX_FILL_RADIUS)
		{
			continue;  // Skip this position, it's too far from the origin
		}

		// CRITICAL: Don't fill above the detected sea level
		if (m_fillMaxY >= 0 && current.y > m_fillMaxY)
		{
			continue;
		}

		// Check if this position is still air (might have been filled already)
		Voxel::BlockID currentBlock = world->getBlockAt(
			static_cast<float>(current.x),
			static_cast<float>(current.y),
			static_cast<float>(current.z)
		);

		if (currentBlock != Voxel::BlockID::Air)
			continue;

		// PRIORITY 1: Check if there's air below - if so, don't fill here yet
		// Water should fill from bottom up
		Position below{ current.x, current.y - 1, current.z };
		if (below.y >= 0)
		{
			Voxel::BlockID blockBelow = world->getBlockAt(
				static_cast<float>(below.x),
				static_cast<float>(below.y),
				static_cast<float>(below.z)
			);

			if (blockBelow == Voxel::BlockID::Air)
			{
				// Add the position below to queue instead (only if not above sea level and within radius)
				int dx_b = abs(below.x - m_fillOrigin.x);
				int dy_b = abs(below.y - m_fillOrigin.y);
				int dz_b = abs(below.z - m_fillOrigin.z);
				int maxDist_b = std::max({dx_b, dy_b, dz_b});

				if (maxDist_b <= MAX_FILL_RADIUS && 
					(m_fillMaxY < 0 || below.y <= m_fillMaxY) && 
					m_inFillQueue.find(below) == m_inFillQueue.end() &&
					m_fillQueue.size() < MAX_QUEUE_SIZE)
				{
					m_fillQueue.push(below);
					m_inFillQueue.insert(below);
				}
				continue; // Don't fill this position yet
			}
		}

		// PRIORITY 2: Fill this position with water
		world->setBlockAt(
			static_cast<float>(current.x),
			static_cast<float>(current.y),
			static_cast<float>(current.z),
			Voxel::BlockID::Water
		);
		blocksFilled++;

		// PRIORITY 3: Add horizontal neighbors and position above to queue
		// Check all 6 adjacent positions
		for (int i = 0; i < 6; ++i)
		{
			Position neighbor{ current.x + dx[i], current.y + dy[i], current.z + dz[i] };

			// Bounds check
			if (neighbor.y < 0 || neighbor.y >= 256)
				continue;

			// Distance check from origin
			int dx_n = abs(neighbor.x - m_fillOrigin.x);
			int dy_n = abs(neighbor.y - m_fillOrigin.y);
			int dz_n = abs(neighbor.z - m_fillOrigin.z);
			int maxDist_n = std::max({dx_n, dy_n, dz_n});

			if (maxDist_n > MAX_FILL_RADIUS)
				continue;

			// CRITICAL: Don't queue positions above sea level
			if (m_fillMaxY >= 0 && neighbor.y > m_fillMaxY)
				continue;

			// Queue size check
			if (m_fillQueue.size() >= MAX_QUEUE_SIZE)
				continue;

			// Check if neighbor is air
			Voxel::BlockID neighborBlock = world->getBlockAt(
				static_cast<float>(neighbor.x),
				static_cast<float>(neighbor.y),
				static_cast<float>(neighbor.z)
			);

			if (neighborBlock == Voxel::BlockID::Air)
			{
				// Add to queue if not already there
				if (m_inFillQueue.find(neighbor) == m_inFillQueue.end())
				{
					m_fillQueue.push(neighbor);
					m_inFillQueue.insert(neighbor);
				}
			}
		}
	}

	if (blocksFilled > 0)
	{
		std::cout << "[WaterFill] Processing queue: " << blocksFilled << " blocks filled, " 
				  << m_fillQueue.size() << " remaining (sea level Y=" << m_fillMaxY << ")" << std::endl;
	}

	// If queue is empty, reset max Y for next fill operation
	if (m_fillQueue.empty())
	{
		m_fillMaxY = -1;
		std::cout << "[WaterFill] Queue empty, fill complete" << std::endl;
	}

	return blocksFilled;
}

void WaterSimulation::clear()
{
	m_sources.clear();
	m_waterPositions.clear();

	// Clear fill queue
	while (!m_fillQueue.empty())
		m_fillQueue.pop();
	m_inFillQueue.clear();

	// Reset sea level tracker
	m_fillMaxY = -1;

	// Clear scan-fill state
	m_scanFillList.clear();
	m_scanFillIndex = 0;

	// Clear zero-fill scan cache
	m_recentZeroFillScans.clear();
}

bool WaterSimulation::tryMoveWater(FlowingWater& water, Chunk::ChunkWorld* world)
{
	Position oldPos = water.pos;
	int currentCount = m_waterPositions[oldPos];

	const int dx[] = { 1, -1, 0, 0 };
	const int dz[] = { 0, 0, 1, -1 };

	// SIMPLIFIED WATER MOVEMENT: Just gravity + basic spreading
	// No complex leveling, no neighbor reactivation, no gap detection

	// PRIORITY 1: Try to fall down
	Position down{ water.pos.x, water.pos.y - 1, water.pos.z };

	if (isAir(world, down.x, down.y, down.z))
	{
		int downCount = m_waterPositions[down];

		// Only fall if there's room below (not full)
		if (downCount < 8) // Match our 8-blocks-per-source limit
		{
			// Fall down
			if (--m_waterPositions[oldPos] <= 0)
			{
				m_waterPositions.erase(oldPos);
				world->setBlockAt(static_cast<float>(oldPos.x),
								 static_cast<float>(oldPos.y),
								 static_cast<float>(oldPos.z),
								 Voxel::BlockID::Air);
			}

			water.pos = down;
			water.lastDx = 0;
			water.lastDz = 0;

			if (m_waterPositions[down]++ == 0)
			{
				world->setBlockAt(static_cast<float>(down.x),
								 static_cast<float>(down.y),
								 static_cast<float>(down.z),
								 Voxel::BlockID::Water);
			}

			return true; // Fell down
		}
	}

	// PRIORITY 2: Spread sideways (only if can't fall)
	// Water will spread as long as it has push (from source or recent movement)
	// Don't check if at source - let water spread freely

	// Try to spread in last direction first (momentum)
	if (water.lastDx != 0 || water.lastDz != 0)
	{
		Position next{ water.pos.x + water.lastDx, water.pos.y, water.pos.z + water.lastDz };

		if (isAir(world, next.x, next.y, next.z))
		{
			int nextCount = m_waterPositions[next];

			// Continue spreading if neighbor is emptier OR if this water has push
			if ((nextCount < currentCount || water.hasPush) && nextCount < 8)
			{
				if (--m_waterPositions[oldPos] <= 0)
				{
					m_waterPositions.erase(oldPos);
					world->setBlockAt(static_cast<float>(oldPos.x),
									 static_cast<float>(oldPos.y),
									 static_cast<float>(oldPos.z),
									 Voxel::BlockID::Air);
				}

				water.pos = next;

				// Gradually lose push as water spreads away from source
				if (water.hasPush && water.pos != water.sourcePos)
				{
					// Random chance to lose push (allows spreading but prevents infinite flow)
					// 10% chance per move - allows water to spread ~10 blocks before losing push
					if (rand() % 10 == 0)
					{
						water.hasPush = false;
					}
				}

				if (m_waterPositions[next]++ == 0)
				{
					world->setBlockAt(static_cast<float>(next.x),
									 static_cast<float>(next.y),
									 static_cast<float>(next.z),
									 Voxel::BlockID::Water);
				}

				return true;
			}
		}
	}

	// Try all 4 directions
	for (int i = 0; i < 4; ++i)
	{
		Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };

		if (isAir(world, neighbor.x, neighbor.y, neighbor.z))
		{
			int neighborCount = m_waterPositions[neighbor];

			// Spread if neighbor is emptier OR if this water has push from source
			if ((neighborCount < currentCount || water.hasPush) && neighborCount < 8)
			{
				if (--m_waterPositions[oldPos] <= 0)
				{
					m_waterPositions.erase(oldPos);
					world->setBlockAt(static_cast<float>(oldPos.x),
									 static_cast<float>(oldPos.y),
									 static_cast<float>(oldPos.z),
									 Voxel::BlockID::Air);
				}

				water.pos = neighbor;
				water.lastDx = dx[i];
				water.lastDz = dz[i];

				// Gradually lose push as water spreads away from source
				if (water.hasPush && water.pos != water.sourcePos)
				{
					// Random chance to lose push (allows spreading but prevents infinite flow)
					// 10% chance per move - allows water to spread ~10 blocks before losing push
					if (rand() % 10 == 0)
					{
						water.hasPush = false;
					}
				}

				if (m_waterPositions[neighbor]++ == 0)
				{
					world->setBlockAt(static_cast<float>(neighbor.x),
									 static_cast<float>(neighbor.y),
									 static_cast<float>(neighbor.z),
									 Voxel::BlockID::Water);
				}

				return true;
			}
		}
	}

	// Can't move - settle
	water.isSettled = true;
	return false;
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

bool WaterSimulation::hasWater(const Position& pos) const
{
	return m_waterPositions.find(pos) != m_waterPositions.end();
}

} // namespace World


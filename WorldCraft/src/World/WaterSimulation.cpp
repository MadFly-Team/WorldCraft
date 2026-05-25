#include <World/WaterSimulation.h>
#include <algorithm>
#include <cmath>
#include <random>

namespace World
{

int WaterSimulation::update(Chunk::ChunkWorld* world, const glm::vec3& playerPos, float maxDistance, int maxNewSources)
{
	if (!world || m_sources.empty())
		return 0;

	int blocksMoved = 0;
	int newSourcesDiscovered = 0; // Track how many new sources we've discovered this update

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

		// First pass: check if any settled water should be reactivated
		for (FlowingWater& water : source.flowingBlocks)
		{
			if (water.isSettled)
			{
				Position down{ water.pos.x, water.pos.y - 1, water.pos.z };
				Position above{ water.pos.x, water.pos.y + 1, water.pos.z };

				// Reactivate if:
				// 1. Air appears below (can fall)
				// 2. Water appears above (gets pushed)
				// 3. Air appears beside (can spread or cascade)
				bool shouldReactivate = false;

				// Check below for falling
				if (isAir(world, down.x, down.y, down.z) && !hasWater(down))
				{
					shouldReactivate = true; // Can fall
				}
				// Check above for compression
				else if (hasWater(above))
				{
					shouldReactivate = true; // Gets pushed by water above
				}
				// Check all 4 horizontal directions for gaps/edges
				else
				{
					const int offsetDx[] = { 1, -1, 0, 0 };
					const int offsetDz[] = { 0, 0, 1, -1 };
					for (int i = 0; i < 4; ++i)
					{
						Position neighbor{ water.pos.x + offsetDx[i], water.pos.y, water.pos.z + offsetDz[i] };
						// Any air space with fewer water blocks should trigger flow
						if (isAir(world, neighbor.x, neighbor.y, neighbor.z) && 
							m_waterPositions[neighbor] < m_waterPositions[water.pos])
						{
							shouldReactivate = true;
							break;
						}
					}
				}

				if (shouldReactivate)
				{
					water.isSettled = false;
					water.hasPush = true;
					water.pushedFrom = water.pos; // Push from current position
				}
			}
		}

		// Second pass: try to move each unsettled water block
		for (FlowingWater& water : source.flowingBlocks)
		{
			if (!water.isSettled)
			{
				if (tryMoveWater(water, world))
				{
					++blocksMoved;
				}
			}
		}

		// Third pass: discover and register nearby world-generated water
		// This allows lakes/oceans to activate progressively as flowing water reaches them
		// Throttled by maxNewSources to prevent instant activation of entire lakes
		if (maxNewSources == 0 || newSourcesDiscovered < maxNewSources)
		{
			for (const FlowingWater& water : source.flowingBlocks)
			{
				// Check neighbors of each water block
				const int dx[] = { 1, -1, 0, 0 };
				const int dz[] = { 0, 0, 1, -1 };
				for (int i = 0; i < 4; ++i)
				{
					Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };

					// Check if there's a water block in the world at this position
					Voxel::BlockID block = world->getBlockAt(
						static_cast<float>(neighbor.x),
						static_cast<float>(neighbor.y),
						static_cast<float>(neighbor.z)
					);

					// If it's water and doesn't have a source yet, register it
					// This progressively activates the lake as water spreads
					if (block == Voxel::BlockID::Water && !hasSource(neighbor.x, neighbor.y, neighbor.z))
					{
						registerSource(neighbor.x, neighbor.y, neighbor.z);
						newSourcesDiscovered++;

						// Stop discovering if we've hit the limit
						if (maxNewSources > 0 && newSourcesDiscovered >= maxNewSources)
							break;
					}
				}

				// Break outer loop too if we've hit the limit
				if (maxNewSources > 0 && newSourcesDiscovered >= maxNewSources)
					break;
			}
		}
	}

	return blocksMoved;
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

	// Create new source with 15 flowing water blocks (increased for better coverage)
	WaterSource source;
	source.pos = sourcePos;
	source.needsUpdate = true;

	// Create 15 water blocks that will flow from this source
	// Initially they all start at the source position
	for (int i = 0; i < 15; ++i)
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
	m_waterPositions[sourcePos] = 15; // All 15 blocks start at source
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

void WaterSimulation::clear()
{
	m_sources.clear();
	m_waterPositions.clear();
}

bool WaterSimulation::tryMoveWater(FlowingWater& water, Chunk::ChunkWorld* world)
{
	Position oldPos = water.pos;

	// Get current position's water count
	int currentCount = m_waterPositions[oldPos];

	// Reusable direction arrays
	const int dx[] = { 1, -1, 0, 0 };
	const int dz[] = { 0, 0, 1, -1 };

	// Priority 0: Aggressive leveling - if current position has multiple blocks and neighbor has none, always level
	// This ensures water spreads evenly before doing anything else
	if (currentCount > 2) // Only level if we have water to spare
	{
		for (int i = 0; i < 4; ++i)
		{
			Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };
			int neighborCount = m_waterPositions[neighbor];

			// If neighbor is air and has significantly less water (or none), level into it
			if (isAir(world, neighbor.x, neighbor.y, neighbor.z) && neighborCount <= currentCount - 2)
			{
				// Move water to neighbor
				if (--m_waterPositions[oldPos] <= 0)
				{
					m_waterPositions.erase(oldPos);
					world->setBlockAt(static_cast<float>(oldPos.x),
									 static_cast<float>(oldPos.y),
									 static_cast<float>(oldPos.z),
									 Voxel::BlockID::Air);
				}

				water.pos = neighbor;
				water.pushedFrom = oldPos;
				water.hasPush = true;
				water.lastDx = dx[i];
				water.lastDz = dz[i];

				if (m_waterPositions[neighbor]++ == 0)
				{
					world->setBlockAt(static_cast<float>(neighbor.x),
									 static_cast<float>(neighbor.y),
									 static_cast<float>(neighbor.z),
									 Voxel::BlockID::Water);
				}

				return true; // Water leveled
			}
		}
	}

	// Priority 1: Try to flow down (gravity) - ALWAYS check this first
	Position down{ water.pos.x, water.pos.y - 1, water.pos.z };

	if (isAir(world, down.x, down.y, down.z))
	{
		int downCount = m_waterPositions[down];

		// Fall if the position below has room (< 15 water blocks)
		if (downCount < 15)
		{
			// Air below with room - fall
			// Decrease count at old position
			if (--m_waterPositions[oldPos] <= 0)
			{
				m_waterPositions.erase(oldPos);
				// Remove water block from world if this was the last water block here
				world->setBlockAt(static_cast<float>(oldPos.x),
								 static_cast<float>(oldPos.y),
								 static_cast<float>(oldPos.z),
								 Voxel::BlockID::Air);
			}

			// Place water at new position
			water.pos = down;
			water.pushedFrom = oldPos;  // Remember where we came from
			water.hasPush = true;        // Falling water always has push (gravity)

			// Reset horizontal direction when falling
			water.lastDx = 0;
			water.lastDz = 0;

			// Increase count at new position
			if (m_waterPositions[down]++ == 0)
			{
				// First water block at this position, place in world
				world->setBlockAt(static_cast<float>(down.x),
								 static_cast<float>(down.y),
								 static_cast<float>(down.z),
								 Voxel::BlockID::Water);
			}

			return true; // Water moved
		}
		else
		{
			// Position below is full (5 water blocks) - spread horizontally instead
			water.hasPush = true;
			water.pushedFrom = oldPos;
			// Fall through to horizontal spread logic
		}
	}

	// Check if this water can spread horizontally
	// Water can spread if:
	// 1. It has push from behind, OR
	// 2. There's water above it (being compressed), OR
	// 3. It's at the source position (always active), OR
	// 4. There's any air space beside it (can flow into gaps/over edges), OR
	// 5. There's an adjacent position with fewer water blocks (leveling)
	Position above{ water.pos.x, water.pos.y + 1, water.pos.z };
	bool hasWaterAbove = hasWater(above);
	bool isAtSource = (water.pos.x == water.sourcePos.x && 
					   water.pos.y == water.sourcePos.y && 
					   water.pos.z == water.sourcePos.z);

	// Check for any neighboring air space (or unsaturated water position)
	bool hasAirNeighbor = false;
	bool canLevel = false;  // Can level out to adjacent position with fewer blocks
	for (int i = 0; i < 4; ++i)
	{
		Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };
		if (isAir(world, neighbor.x, neighbor.y, neighbor.z))
		{
			int neighborCount = m_waterPositions[neighbor];
			if (neighborCount < 15)
			{
				hasAirNeighbor = true;
			}
			// Check if we can level (neighbor has fewer blocks than us)
			if (neighborCount < currentCount)
			{
				canLevel = true;
			}
		}
	}

	bool canSpread = water.hasPush || hasWaterAbove || isAtSource || hasAirNeighbor || canLevel;

	if (!canSpread)
	{
		// Before settling, check if we should activate neighboring water sources
		// This allows water to spread beyond the 5-block-per-source limit
		bool activatedNeighbor = false;
		for (int i = 0; i < 4; ++i)
		{
			Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };
			// Look for water blocks in neighboring positions that might need activation
			if (hasWater(neighbor))
			{
				int neighborCount = m_waterPositions[neighbor];
				// If neighbor has water and is not saturated, activate it to keep spreading
				if (neighborCount > 0 && neighborCount < 15)
				{
					// Reactivate all sources that have water at this neighbor position
					for (auto& [srcPos, src] : m_sources)
					{
						for (FlowingWater& otherWater : src.flowingBlocks)
						{
							if (otherWater.pos == neighbor && otherWater.isSettled)
							{
								otherWater.isSettled = false;
								otherWater.hasPush = true;
								activatedNeighbor = true;
							}
						}
					}
				}
			}
		}

		// If we activated neighbors, don't settle yet - let them spread first
		if (activatedNeighbor)
		{
			water.hasPush = false; // This water can rest while neighbors spread
			return false;
		}

		// No push, no water above, not at source, no air neighbors, no leveling - water settles
		water.isSettled = true;
		water.hasPush = false;
		return false;
	}

	// If there's water above OR at source OR air neighbor OR can level, push outward
	if (hasWaterAbove || isAtSource || hasAirNeighbor || canLevel)
	{
		water.hasPush = true;
		water.pushedFrom = water.pos; // Push from current position
	}

	// Priority 2: Fill any empty gaps before cascading (ensures continuous water body)
	// If current position has water and there's an empty air space beside it, fill that first
	if (currentCount > 1) // Only if we have water to spare (more than 1 block here)
	{
		for (int i = 0; i < 4; ++i)
		{
			Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };
			int neighborCount = m_waterPositions[neighbor];

			// Fill completely empty positions first (gap filling)
			if (isAir(world, neighbor.x, neighbor.y, neighbor.z) && neighborCount == 0)
			{
				// Check if this is surrounded by water (it's a gap)
				int waterNeighbors = 0;
				for (int j = 0; j < 4; ++j)
				{
					Position check{ neighbor.x + dx[j], neighbor.y, neighbor.z + dz[j] };
					if (hasWater(check))
						waterNeighbors++;
				}

				// If at least 2 water neighbors, it's a gap - fill it
				if (waterNeighbors >= 2)
				{
					// Move water to fill the gap
					if (--m_waterPositions[oldPos] <= 0)
					{
						m_waterPositions.erase(oldPos);
						world->setBlockAt(static_cast<float>(oldPos.x),
										 static_cast<float>(oldPos.y),
										 static_cast<float>(oldPos.z),
										 Voxel::BlockID::Air);
					}

					water.pos = neighbor;
					water.pushedFrom = oldPos;
					water.hasPush = true;
					water.lastDx = dx[i];
					water.lastDz = dz[i];

					if (m_waterPositions[neighbor]++ == 0)
					{
						world->setBlockAt(static_cast<float>(neighbor.x),
										 static_cast<float>(neighbor.y),
										 static_cast<float>(neighbor.z),
										 Voxel::BlockID::Water);
					}

					return true; // Water moved to fill gap
				}
			}
		}
	}

	// Priority 3: Check for edges to cascade over (air beside with air below = waterfall)

	for (int i = 0; i < 4; ++i)
	{
		Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };
		Position belowNeighbor{ neighbor.x, neighbor.y - 1, neighbor.z };

		int neighborCount = m_waterPositions[neighbor];

		// Check if this is an edge (air horizontally AND air below that = waterfall)
		// Always move to empty spaces (count=0), or to positions with fewer blocks (leveling)
		if (isAir(world, neighbor.x, neighbor.y, neighbor.z) && 
			(neighborCount == 0 || neighborCount < currentCount) && // Empty or lower level
			neighborCount < 15 && // Don't overfill positions
			isAir(world, belowNeighbor.x, belowNeighbor.y, belowNeighbor.z) && 
			m_waterPositions[belowNeighbor] < 15)
		{
			// Flow over the edge
			if (--m_waterPositions[oldPos] <= 0)
			{
				m_waterPositions.erase(oldPos);
				world->setBlockAt(static_cast<float>(oldPos.x),
								 static_cast<float>(oldPos.y),
								 static_cast<float>(oldPos.z),
								 Voxel::BlockID::Air);
			}

			water.pos = neighbor;
			water.pushedFrom = oldPos;
			water.hasPush = true;
			water.lastDx = dx[i];
			water.lastDz = dz[i];

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

	// Priority 2.5: Fill gaps in water body (air with solid/water below, surrounded by water)
	for (int i = 0; i < 4; ++i)
	{
		Position neighbor{ water.pos.x + dx[i], water.pos.y, water.pos.z + dz[i] };
		Position belowNeighbor{ neighbor.x, neighbor.y - 1, neighbor.z };

		int neighborCount = m_waterPositions[neighbor];

		// Check if this is a gap (air at same level with solid or water below)
		// Always move to empty spaces (count=0), or to positions with fewer blocks (leveling)
		if (isAir(world, neighbor.x, neighbor.y, neighbor.z) && 
			(neighborCount == 0 || neighborCount < currentCount) && // Empty or lower level
			neighborCount < 15)
		{
			// Gap must have something below it (not a waterfall edge)
			if (!isAir(world, belowNeighbor.x, belowNeighbor.y, belowNeighbor.z) || hasWater(belowNeighbor))
			{
				// Fill the gap
				if (--m_waterPositions[oldPos] <= 0)
				{
					m_waterPositions.erase(oldPos);
					world->setBlockAt(static_cast<float>(oldPos.x),
									 static_cast<float>(oldPos.y),
									 static_cast<float>(oldPos.z),
									 Voxel::BlockID::Air);
				}

				water.pos = neighbor;
				water.pushedFrom = oldPos;
				water.hasPush = true;
				water.lastDx = dx[i];
				water.lastDz = dz[i];

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

	// Priority 3: If can't flow down or cascade, try horizontal spread (only if being pushed)
	// First try to continue in the last direction (momentum/inertia)
	if (water.lastDx != 0 || water.lastDz != 0)
	{
		Position preferred{ water.pos.x + water.lastDx, water.pos.y, water.pos.z + water.lastDz };
		int preferredCount = m_waterPositions[preferred];

		if (isAir(world, preferred.x, preferred.y, preferred.z) && 
			(preferredCount == 0 || preferredCount < currentCount) && // Empty or lower level
			preferredCount < 15)
		{
			// Decrease count at old position
			if (--m_waterPositions[oldPos] <= 0)
			{
				m_waterPositions.erase(oldPos);
				// Remove water block from world if this was the last water block here
				world->setBlockAt(static_cast<float>(oldPos.x),
								 static_cast<float>(oldPos.y),
								 static_cast<float>(oldPos.z),
								 Voxel::BlockID::Air);
			}

			// Place water at new position (continue in same direction)
			water.pos = preferred;
			water.pushedFrom = oldPos;  // Remember where we came from
			water.hasPush = true;        // Keep push when spreading
			// Keep lastDx and lastDz - continue flowing in same direction

			// Increase count at new position
			if (m_waterPositions[preferred]++ == 0)
			{
				// First water block at this position, place in world
				world->setBlockAt(static_cast<float>(preferred.x),
								 static_cast<float>(preferred.y),
								 static_cast<float>(preferred.z),
								 Voxel::BlockID::Water);
			}

			return true; // Water moved
		}
	}

	// Can't continue in last direction, try all 4 horizontal directions
	// Prioritize edges (air below) over flat spreading
	std::vector<int> directions = { 0, 1, 2, 3 };
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::shuffle(directions.begin(), directions.end(), gen);

	// First pass: try to find an edge (has air below) to cascade over
	for (int idx : directions)
	{
		Position neighbor{ water.pos.x + dx[idx], water.pos.y, water.pos.z + dz[idx] };
		Position belowNeighbor{ neighbor.x, neighbor.y - 1, neighbor.z };
		int neighborCount = m_waterPositions[neighbor];

		// Prefer edges: air beside AND air below
		if (isAir(world, neighbor.x, neighbor.y, neighbor.z) && 
			(neighborCount == 0 || neighborCount < currentCount) && 
			neighborCount < 15 &&
			isAir(world, belowNeighbor.x, belowNeighbor.y, belowNeighbor.z) &&
			m_waterPositions[belowNeighbor] < 15)
		{
			// Move to edge
			if (--m_waterPositions[oldPos] <= 0)
			{
				m_waterPositions.erase(oldPos);
				world->setBlockAt(static_cast<float>(oldPos.x),
								 static_cast<float>(oldPos.y),
								 static_cast<float>(oldPos.z),
								 Voxel::BlockID::Air);
			}

			water.pos = neighbor;
			water.pushedFrom = oldPos;
			water.hasPush = true;
			water.lastDx = dx[idx];
			water.lastDz = dz[idx];

			if (m_waterPositions[neighbor]++ == 0)
			{
				world->setBlockAt(static_cast<float>(neighbor.x),
								 static_cast<float>(neighbor.y),
								 static_cast<float>(neighbor.z),
								 Voxel::BlockID::Water);
			}

			return true; // Water moved to edge
		}
	}

	// Second pass: if no edges found, spread flat (only if solid/water below)
	for (int idx : directions)
	{
		Position neighbor{ water.pos.x + dx[idx], water.pos.y, water.pos.z + dz[idx] };
		Position belowNeighbor{ neighbor.x, neighbor.y - 1, neighbor.z };
		int neighborCount = m_waterPositions[neighbor];

		// Only spread flat if there's solid ground or water below (not an edge)
		if (isAir(world, neighbor.x, neighbor.y, neighbor.z) && 
			(neighborCount == 0 || neighborCount < currentCount) && 
			neighborCount < 15 &&
			(!isAir(world, belowNeighbor.x, belowNeighbor.y, belowNeighbor.z) || hasWater(belowNeighbor)))
		{
			// Move flat
			if (--m_waterPositions[oldPos] <= 0)
			{
				m_waterPositions.erase(oldPos);
				world->setBlockAt(static_cast<float>(oldPos.x),
								 static_cast<float>(oldPos.y),
								 static_cast<float>(oldPos.z),
								 Voxel::BlockID::Air);
			}

			water.pos = neighbor;
			water.pushedFrom = oldPos;
			water.hasPush = true;
			water.lastDx = dx[idx];
			water.lastDz = dz[idx];

			if (m_waterPositions[neighbor]++ == 0)
			{
				world->setBlockAt(static_cast<float>(neighbor.x),
								 static_cast<float>(neighbor.y),
								 static_cast<float>(neighbor.z),
								 Voxel::BlockID::Water);
			}

			return true; // Water moved flat
		}
	}

	// Can't move in any direction - water is settled
	water.isSettled = true;
	water.hasPush = false;
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

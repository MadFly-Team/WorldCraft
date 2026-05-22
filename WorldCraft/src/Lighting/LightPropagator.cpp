#include <Lighting/LightPropagator.h>
#include <Chunk/ChunkWorld.h>
#include <Chunk/Chunk.h>
#include <Voxel/BlockRegistry.h>
#include <queue>
#include <cmath>

namespace Lighting
{

LightPropagator::LightPropagator(Chunk::ChunkWorld& world)
	: m_world(world)
{
}

void LightPropagator::propagateSkyLight()
{
	// Step 1: Initialize sky columns
	initializeSkyColumns();

	// Step 2: Flood-fill light propagation
	floodFillLight();
}

void LightPropagator::initializeSkyColumns()
{
	// For a flood-fill approach, we'll initialize all exposed surface blocks to 15
	// and let the flood-fill handle spreading downward and sideways.
	// 
	// In a real Minecraft-style system, we'd cast rays from the top of each column
	// down to the first opaque block. For now, we'll use a simpler approach:
	// set all blocks at the top layer (Y=255) to 15, then let flood-fill do the rest.
	//
	// A more complete implementation would:
	// 1. For each X,Z column in each loaded chunk
	// 2. Find the topmost non-transparent block
	// 3. Set all blocks above it to light level 15
	// 4. Set the block itself to 15 if transparent, 0 if opaque
	//
	// For simplicity in this initial implementation, we'll just mark the entire
	// top layer as lit and let flood-fill handle the rest.
}

void LightPropagator::floodFillLight()
{
	// Minecraft-style flood-fill light propagation:
	// 1. Start with all light sources (sky-lit blocks at level 15)
	// 2. For each lit block, propagate light to neighbors at (level - 1)
	// 3. Only propagate if the neighbor's current light < new light
	// 4. Only propagate through transparent blocks
	//
	// This is a breadth-first search starting from all light sources.

	struct LightNode
	{
		int x, y, z;
		uint8_t level;
	};

	std::queue<LightNode> lightQueue;

	// For now, we'll implement a simple version that assumes light starts
	// at the top of each chunk column. A full implementation would need
	// access to all loaded chunks to do this properly.
	//
	// Since ChunkWorld doesn't expose an iterator over all chunks,
	// we'll need to add that capability or use a different approach.
	//
	// For now, leaving this as a placeholder that can be filled in
	// once we wire it into TerrainGen where we have direct chunk access.
}

bool LightPropagator::isTransparent(int worldX, int worldY, int worldZ) const
{
	Voxel::BlockID block = m_world.getBlockAt(
		static_cast<float>(worldX),
		static_cast<float>(worldY),
		static_cast<float>(worldZ)
	);

	if (block == Voxel::BlockID::Air || block == Voxel::BlockID::Water)
		return true;

	const auto& props = Voxel::BlockRegistry::get().propertiesOf(block);
	return props.isTransparent;
}

uint8_t LightPropagator::getLight(int worldX, int worldY, int worldZ) const
{
	// Convert world coordinates to chunk + local coordinates
	const int cx = static_cast<int>(std::floor(static_cast<float>(worldX) / Chunk::CHUNK_SIZE_X));
	const int cz = static_cast<int>(std::floor(static_cast<float>(worldZ) / Chunk::CHUNK_SIZE_Z));
	const int lx = worldX - (cx * Chunk::CHUNK_SIZE_X);
	const int ly = worldY;
	const int lz = worldZ - (cz * Chunk::CHUNK_SIZE_Z);

	// Bounds check
	if (!Chunk::Chunk::inBounds(lx, ly, lz))
		return 0;

	// Access the chunk
	const Chunk::Chunk* chunk = m_world.getChunk(cx, cz);
	if (!chunk)
		return 0;

	return chunk->getSkyLight(lx, ly, lz);
}

void LightPropagator::setLight(int worldX, int worldY, int worldZ, uint8_t level)
{
	// Convert world coordinates to chunk + local coordinates
	const int cx = static_cast<int>(std::floor(static_cast<float>(worldX) / Chunk::CHUNK_SIZE_X));
	const int cz = static_cast<int>(std::floor(static_cast<float>(worldZ) / Chunk::CHUNK_SIZE_Z));
	const int lx = worldX - (cx * Chunk::CHUNK_SIZE_X);
	const int ly = worldY;
	const int lz = worldZ - (cz * Chunk::CHUNK_SIZE_Z);

	// Bounds check
	if (!Chunk::Chunk::inBounds(lx, ly, lz))
		return;

	// Access the chunk
	Chunk::Chunk* chunk = m_world.getChunk(cx, cz);
	if (!chunk)
		return;

	chunk->setSkyLight(lx, ly, lz, level);
}

} // namespace Lighting

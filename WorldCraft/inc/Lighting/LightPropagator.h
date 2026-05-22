#pragma once

#include <cstdint>

namespace Chunk { class ChunkWorld; }

namespace Lighting
{

// Sky light propagation engine using flood-fill algorithm.
// Initializes sky light values for all chunks after terrain generation.
class LightPropagator
{
public:
	explicit LightPropagator(Chunk::ChunkWorld& world);

	// Propagate sky light across all loaded chunks.
	// Should be called after terrain generation is complete.
	void propagateSkyLight();

private:
	Chunk::ChunkWorld& m_world;

	// Initialize light by casting rays down from the top of each column
	void initializeSkyColumns();

	// Flood-fill light propagation step
	void floodFillLight();

	// Check if a block at world coordinates is transparent to light
	bool isTransparent(int worldX, int worldY, int worldZ) const;

	// Get light level at world coordinates (returns 0 if chunk not loaded)
	uint8_t getLight(int worldX, int worldY, int worldZ) const;

	// Set light level at world coordinates (no-op if chunk not loaded)
	void setLight(int worldX, int worldY, int worldZ, uint8_t level);
};

} // namespace Lighting

#pragma once

#include <Chunk/Chunk.h>
#include <WorldGen/WorldSettings.h>

namespace WorldGen
{

	// ---------------------------------------------------------------------------
	// Biome types — chosen per XZ column via low-frequency Perlin sampling.
	// ---------------------------------------------------------------------------
	enum class Biome
	{
		Plains,     // gentle rolling hills, grass/dirt
		Desert,     // flat-ish, sand surface, no trees
		Forest,     // tall hills, dense trees, grass
		Mountains,  // dramatic elevation, stone peaks, snow caps
		Tundra,     // low flat terrain, snow surface
		Ocean,      // deep below sea level, sandy floor, no trees
	};

	// ---------------------------------------------------------------------------
	// TerrainGen — fills one 16×128×16 chunk with a procedural Minecraft-style
	// landscape including:
	//   • Bedrock at y=0
	//   • Stone fill up to heightmap
	//   • Ore veins (Coal, Iron, Gold, Lapis, Redstone, Diamond, Emerald)
	//   • Cave networks (worm-cave Perlin threshold)
	//   • Biome-dependent surface (Grass/Dirt, Sand, Snow)
	//   • Gravel pockets near cave floors
	//   • Oak trees in Forest / Plains biomes
	//   • Water fill below sea level (y=64)
	//   • Obsidian at lava/water boundaries (future-ready stub)
	// ---------------------------------------------------------------------------
	class TerrainGen
	{
	public:
		// World seed and settings — controls all noise and generation parameters.
		explicit TerrainGen(const WorldSettings& settings);

		// Fill 'chunk' completely.  chunk.chunkX/Z are the world-space chunk
		// grid coordinates (each chunk spans 16 voxels).
		void generate(Chunk::Chunk& chunk, int chunkX, int chunkZ) const;

	private:
		WorldSettings m_settings;

		// Biome classification for a world-space XZ column.
		Biome getBiome(float wx, float wz) const;

		// Surface height (in voxels, 1-based) for a world-space XZ column.
		int getSurfaceHeight(float wx, float wz, Biome biome) const;

		// NEW CAVE SYSTEM: Mountain entrances + Deep caverns (no worm caves)
		// Returns true if the voxel should be carved out by cave/cavern generation.
		bool isCave(float wx, float wy, float wz) const;

		// Mountain entrance cave detection - large openings on mountainsides only
		bool isMountainCave(float wx, float wy, float wz) const;

		// Deep underground cavern detection - massive sealed chambers
		bool isDeepCavern(float wx, float wy, float wz) const;

		// Ore substitution — returns the ore block to place at depth y, or
		// Stone if no ore should spawn here (based on a hash of the position).
		Voxel::BlockID oreAt(int wx, int wy, int wz) const;

		// Attempt to plant an oak tree with trunk base at (wx, wy, wz).
		void plantTree(Chunk::Chunk& chunk, int lx, int ly, int lz) const;
	};

} // namespace WorldGen

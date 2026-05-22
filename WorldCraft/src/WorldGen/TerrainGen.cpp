#include <WorldGen/TerrainGen.h>

// stb_perlin — implementation already compiled in BlockTextures.cpp.
// Include header-only here (no STB_PERLIN_IMPLEMENTATION).
#include <stb_perlin.h>

#include <Voxel/BlockRegistry.h>

#include <cmath>
#include <cstdlib>
#include <unordered_set>
#include <utility>
#include <cstdio>
#include <vector>

namespace WorldGen
{

// Hash function for std::pair<int, int> for cave debug logging
struct PairHash
{
	std::size_t operator()(const std::pair<int, int>& p) const
	{
		return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
	}
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TerrainGen::TerrainGen(const WorldSettings& settings)
	: m_settings(settings)
{
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Seamless 3-D Perlin with a seed offset baked in.
static float pnoise(float x, float y, float z, float seed)
{
	return stb_perlin_noise3(x, y + seed, z, 0, 0, 0);
}

// Map [-1,1] noise to [0,1].
static float p01(float x, float y, float z, float seed)
{
	return (pnoise(x, y, z, seed) + 1.0f) * 0.5f;
}

// Fast integer hash (for deterministic ore placement).
static unsigned int hash3(int x, int y, int z)
{
	unsigned int h = static_cast<unsigned int>(x * 1619 + y * 31337 + z * 6971);
	h ^= h >> 16;
	h *= 0x45d9f3b;
	h ^= h >> 16;
	return h;
}

// ---------------------------------------------------------------------------
// Biome
// ---------------------------------------------------------------------------

Biome TerrainGen::getBiome(float wx, float wz) const
{
	const float seed = static_cast<float>(m_settings.seed);
	const float biomeFreq = 1.0f / m_settings.biomeScale;
	// Low frequency 2D noise for biome blending.
	float temp       = p01(wx * biomeFreq, 0.0f, wz * biomeFreq, seed + 1000.0f);
	float moisture   = p01(wx * biomeFreq, 0.0f, wz * biomeFreq, seed + 2000.0f);
	// Continental noise: low values = ocean regions.
	float continental = p01(wx * biomeFreq * 0.7f, 0.0f, wz * biomeFreq * 0.7f, seed + 9000.0f);

	if (continental < 0.35f) return Biome::Ocean;
	if (temp < 0.22f)  return Biome::Tundra;
	if (temp < 0.55f)  return (moisture < 0.6f) ? Biome::Mountains : Biome::Forest;
	if (temp > 0.78f)  return Biome::Desert;
	return (moisture > 0.55f) ? Biome::Forest : Biome::Plains;
}

// ---------------------------------------------------------------------------
// Surface height
// ---------------------------------------------------------------------------

// Compute the raw (unquantised) height for a single biome.
// Returns base elevation + variation based on noise.
static float biomeHeight(Biome biome, float base, float detail, float rough, float ridge, float amplitudeScale)
{
	// Base elevation for each biome (always applied)
	float baseElevation = 0.0f;
	// Variation components (scaled by amplitude)
	float variation = 0.0f;

	switch (biome)
	{
		case Biome::Plains:
			baseElevation = 60.0f;
			variation = base * 14.0f + detail *  8.0f + rough * 6.0f + ridge *  4.0f;
			break;
		case Biome::Desert:
			baseElevation = 58.0f;
			variation = base * 12.0f + detail *  6.0f + rough * 5.0f + ridge *  3.0f;
			break;
		case Biome::Forest:
			baseElevation = 62.0f;
			variation = base * 18.0f + detail * 12.0f + rough * 8.0f + ridge *  5.0f;
			break;
		case Biome::Mountains:
			baseElevation = 50.0f;
			variation = base * 60.0f + detail * 28.0f + rough * 14.0f + ridge * 18.0f;
			break;
		case Biome::Tundra:
			baseElevation = 60.0f;
			variation = base * 10.0f + detail *  7.0f + rough * 6.0f + ridge *  4.0f;
			break;
		case Biome::Ocean:
			baseElevation = 42.0f;
			variation = base *  8.0f + detail *  3.0f + rough * 1.0f + ridge *  1.0f;
			break;
	}

	return baseElevation + variation * amplitudeScale;
}

int TerrainGen::getSurfaceHeight(float wx, float wz, Biome /*biome*/) const
{
	const float seed = static_cast<float>(m_settings.seed);
	const float terrainFreq = 1.0f / m_settings.terrainScale;
	const float biomeFreq = 1.0f / m_settings.biomeScale;
	const float amp = m_settings.terrainAmplitude / 80.0f;  // Scale factor relative to default

	// Shared noise layers.
	float base   = p01(wx * terrainFreq, 0.0f, wz * terrainFreq, seed +   0.0f);
	float detail = p01(wx * terrainFreq * 4.0f,  0.0f, wz * terrainFreq * 4.0f,  seed + 500.0f);
	float rough  = p01(wx * terrainFreq * 16.0f,  0.0f, wz * terrainFreq * 16.0f,  seed + 750.0f);
	// Ridged noise: |noise| inverted so peaks are sharp ridges.
	float ridgeRaw = std::abs(pnoise(wx * terrainFreq * 7.0f, 0.0f, wz * terrainFreq * 7.0f, seed + 1500.0f));
	float ridge    = 1.0f - ridgeRaw;  // 1 at ridge crest, 0 in valleys

	// Continuous biome noise values in [0,1] — same frequencies as getBiome.
	float temp         = p01(wx * biomeFreq, 0.0f, wz * biomeFreq, seed + 1000.0f);
	float moisture     = p01(wx * biomeFreq, 0.0f, wz * biomeFreq, seed + 2000.0f);
	float continental  = p01(wx * biomeFreq * 0.7f, 0.0f, wz * biomeFreq * 0.7f, seed + 9000.0f);

	// Compute per-biome heights with amplitude scaling.
	float hPlains    = biomeHeight(Biome::Plains,    base, detail, rough, ridge, amp);
	float hDesert    = biomeHeight(Biome::Desert,    base, detail, rough, ridge, amp);
	float hForest    = biomeHeight(Biome::Forest,    base, detail, rough, ridge, amp);
	float hMountains = biomeHeight(Biome::Mountains, base, detail, rough, ridge, amp);
	float hTundra    = biomeHeight(Biome::Tundra,    base, detail, rough, ridge, amp);
	float hOcean     = biomeHeight(Biome::Ocean,     base, detail, rough, ridge, amp);

	// Smoothstep helper — maps x in [edge0,edge1] to [0,1] with smooth ends.
	auto smoothstep = [](float edge0, float edge1, float x) -> float {
		float t = (x - edge0) / (edge1 - edge0);
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		return t * t * (3.0f - 2.0f * t);
	};

	// Ocean weight — strongest where continental is low; fades out above 0.45.
	float wOcean     = 1.0f - smoothstep(0.30f, 0.48f, continental);
	// Land weight — inverse of ocean, used to scale all land biomes.
	float landWeight = 1.0f - wOcean;

	// Land biome weights (continental biomes only active on land).
	float wTundra    = landWeight * (1.0f - smoothstep(0.10f, 0.30f, temp));
	float wMountains = landWeight * smoothstep(0.20f, 0.40f, temp)
					 * (1.0f - smoothstep(0.50f, 0.65f, temp))
					 * (1.0f - smoothstep(0.45f, 0.65f, moisture));
	float wForest    = landWeight * smoothstep(0.35f, 0.55f, temp)
					 * (1.0f - smoothstep(0.72f, 0.85f, temp))
					 * smoothstep(0.40f, 0.60f, moisture);
	float wDesert    = landWeight * smoothstep(0.68f, 0.82f, temp);
	float wPlains    = landWeight * smoothstep(0.40f, 0.58f, temp)
					 * (1.0f - smoothstep(0.68f, 0.82f, temp))
					 * (1.0f - smoothstep(0.50f, 0.70f, moisture));

	// Normalise weights so they always sum to 1.
	float total = wOcean + wTundra + wMountains + wForest + wDesert + wPlains;
	if (total < 1e-4f) total = 1e-4f;
	wOcean     /= total;
	wTundra    /= total;
	wMountains /= total;
	wForest    /= total;
	wDesert    /= total;
	wPlains    /= total;

	float h = wOcean     * hOcean
			+ wTundra    * hTundra
			+ wMountains * hMountains
			+ wForest    * hForest
			+ wDesert    * hDesert
			+ wPlains    * hPlains;

	// Cap mountains at 128 to keep them reasonable while world height is 256
	if (wMountains > 0.5f && h > 128.0f)
	{
		h = 128.0f;
	}

	int height = static_cast<int>(h);
	if (height < 1)   height = 1;
	if (height > 254) height = 254;  // Keep below 256 to leave room for sky
	return height;
}

// ---------------------------------------------------------------------------
// NEW CAVE SYSTEM: Mountain entrance caves + Deep underground caverns
// ---------------------------------------------------------------------------

// Mountain entrance cave detection - rare, large openings on mountainsides
bool TerrainGen::isMountainCave(float wx, float wy, float wz) const
{
	if (!m_settings.enableMountainCaves)
		return false;

	const float seed = static_cast<float>(m_settings.seed);
	const float rarity = m_settings.mountainCaveRarity;
	const float caveSize = m_settings.mountainCaveSize;

	// Snap to a grid to define cave center locations
	// Each grid cell is rarity × rarity blocks
	const int gridX = static_cast<int>(std::floor(wx / rarity));
	const int gridZ = static_cast<int>(std::floor(wz / rarity));

	// Use grid coordinates to determine if this cell gets a cave
	// Hash-based: deterministic, spreads caves out
	unsigned int cellHash = hash3(gridX, 0, gridZ);
	float cellValue = static_cast<float>(cellHash & 0xFFFF) / 65536.0f;

	// ~50% of grid cells get a cave (with rarity=800, caves are about 800 blocks apart)
	if (cellValue > 0.5f)
		return false;

	// Cave center is at the center of this grid cell
	float centerX = (gridX + 0.5f) * rarity;
	float centerZ = (gridZ + 0.5f) * rarity;

	// Check if this cave center is in a mountain biome
	Biome caveBiome = getBiome(centerX, centerZ);
	if (caveBiome != Biome::Mountains)
		return false;  // Only mountain caves!

	// Get the surface height at the cave center
	int surfaceHeight = getSurfaceHeight(centerX, centerZ, caveBiome);

	// Cave entrance should be high on the mountain - just below the peak
	// Place it 80-90% up the mountain from sea level
	const int seaLevel = m_settings.seaLevel;
	int mountainHeight = surfaceHeight - seaLevel;  // Height above sea level
	int entranceHeight = seaLevel + static_cast<int>(mountainHeight * 0.85f);  // 85% up the mountain

	// Make sure entrance is not too low
	if (entranceHeight < surfaceHeight - 20)
		entranceHeight = surfaceHeight - 20;  // At most 20 blocks below surface

	// Debug: Log when we find a valid mountain cave location
	static std::unordered_set<std::pair<int, int>, PairHash> loggedCaves;
	std::pair<int, int> caveKey = {gridX, gridZ};
	if (loggedCaves.find(caveKey) == loggedCaves.end())
	{
		loggedCaves.insert(caveKey);
		printf("[MOUNTAIN CAVE] Grid(%d, %d) -> Center(%.1f, %.1f) SurfaceH=%d SeaLevel=%d EntranceY=%d Size=%.1f\n",
			gridX, gridZ, centerX, centerZ, surfaceHeight, seaLevel, entranceHeight, caveSize);
	}

	// Calculate horizontal distance from cave center
	float dx = wx - centerX;
	float dz = wz - centerZ;
	float horizontalDist = std::sqrt(dx * dx + dz * dz);

	// Only carve if we're within horizontal range (reduced from 1.5x to 1.0x)
	if (horizontalDist > caveSize * 1.0f)
		return false;

	// Get the terrain height at THIS specific location (not just center)
	int localSurfaceHeight = getSurfaceHeight(wx, wz, getBiome(wx, wz));

	// Cave opening should be visible on the mountainside
	// Only carve blocks that are:
	// 1. Below the local surface (inside the mountain)
	// 2. Near the entrance height level

	// Vertical distance from entrance level
	float dy = wy - static_cast<float>(entranceHeight);

	// Add noise for organic cave walls
	float wallNoise = pnoise(wx * 0.1f, wy * 0.1f, wz * 0.1f, seed + 8000.0f) * 5.0f;

	// Horizontal tunnel extending INTO the mountain
	// Gets wider as you go deeper
	float depthFactor = std::min(horizontalDist / (caveSize * 1.0f), 1.0f);
	float tunnelRadius = caveSize * 0.4f * (1.0f - depthFactor * 0.3f) + wallNoise;  // Smaller radius, slightly tapers

	// Gradually sloping floor - drops as you go deeper
	float floorOffset = -horizontalDist * 0.3f;  // 30% downward slope
	float adjustedFloorY = static_cast<float>(entranceHeight) + floorOffset;

	// Height of the tunnel (taller near entrance, shorter deeper in)
	float tunnelHeight = caveSize * 0.8f * (1.0f + depthFactor * 0.5f);

	// Check if this block is within the tunnel
	// At the entrance (near center), be VERY aggressive about carving to create visible opening
	// Deeper in, require blocks to be below surface (inside mountain)
	float entranceDistance = caveSize * 0.4f;  // First 40% of cave radius is the "entrance"
	bool isNearEntrance = (horizontalDist < entranceDistance);

	bool belowSurface;
	if (isNearEntrance)
	{
		// At entrance: carve aggressively - allow up to 10 blocks ABOVE local surface
		// This ensures the opening is visible but not massive
		belowSurface = (wy < localSurfaceHeight + 10);
	}
	else
	{
		// Deeper in: must be well below surface (inside mountain)
		belowSurface = (wy < localSurfaceHeight - 2);
	}

	bool withinRadius = (std::abs(dy) < tunnelRadius);
	bool withinHeight = (wy > adjustedFloorY && wy < adjustedFloorY + tunnelHeight);

	return belowSurface && withinRadius && withinHeight;
}

bool TerrainGen::isCave(float wx, float wy, float wz) const
{
	// NEW SYSTEM: Check mountain caves and deep caverns
	// (Old worm cave system removed!)

	if (isMountainCave(wx, wy, wz))
		return true;

	if (isDeepCavern(wx, wy, wz))
		return true;

	return false;
}

// Deep underground cavern detection - massive sealed chambers deep underground
bool TerrainGen::isDeepCavern(float wx, float wy, float wz) const
{
	if (!m_settings.enableDeepCaverns)
		return false;

	// Only generate caverns deep underground
	if (wy < m_settings.cavernMinDepth || wy > m_settings.cavernMaxDepth)
		return false;

	const float seed = static_cast<float>(m_settings.seed);
	const float cavernSize = m_settings.cavernSize;

	// Use low-frequency 3D noise to create large "bubble" chambers
	// Smaller scale = RARER and LARGER individual chambers
	const float scale = 0.003f;  // VERY low frequency = extremely rare, larger chambers

	// Primary cavern noise - defines the main chamber structure
	float n1 = pnoise(wx * scale, wy * scale, wz * scale, seed + 7000.0f);
	float n2 = pnoise(wx * scale, wy * scale, wz * scale, seed + 7100.0f);
	float n3 = pnoise(wx * scale, wy * scale, wz * scale, seed + 7200.0f);

	// Combine noise values to create 3D "blobs"
	// When all three are near zero, we're at a cavern center
	float cavernDist = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);

	// MUCH stricter threshold - very few, smaller caverns
	// Higher threshold = fewer caverns, smaller size
	float threshold = 0.25f - (cavernSize / 300.0f);  // Much tighter

	if (cavernDist > threshold)
		return false;  // Not in a cavern

	// No additional noise - just use the base threshold for box-like chambers
	return true;
}

// ---------------------------------------------------------------------------
// Ore placement
// ---------------------------------------------------------------------------

Voxel::BlockID TerrainGen::oreAt(int wx, int wy, int wz) const
{
	// If ores disabled, return stone
	if (!m_settings.enableOres || m_settings.oreAbundance <= 0.0f)
		return Voxel::BlockID::Stone;

	// Each ore type gets a separate noise pass so their veins are independent.
	const float seed = static_cast<float>(m_settings.seed);
	const float fx = static_cast<float>(wx);
	const float fy = static_cast<float>(wy);
	const float fz = static_cast<float>(wz);
	const float abundance = m_settings.oreAbundance;

	// Helper lambda: returns true when a high-freq noise blob appears here.
	// Threshold is reduced by abundance multiplier to spawn more ore.
	auto oreNoise = [&](float oreSeed, float freq) {
		float n = pnoise(fx * freq, fy * freq, fz * freq, oreSeed);
		float threshold = 0.55f - (abundance - 1.0f) * 0.15f;  // More abundance = lower threshold
		return n > threshold;
	};

	// Diamond — rare, only below y=16.
	if (wy <= 16 && oreNoise(seed + 5100.0f, 0.18f))
		return Voxel::BlockID::DiamondOre;

	// Emerald — very rare, mountains-depth, below y=29.
	if (wy <= 29)
	{
		unsigned int h = hash3(wx, wy, wz);
		unsigned int emeraldChance = static_cast<unsigned int>(400 * abundance);  // ~0.6% * abundance
		if ((h & 0xFFFF) < emeraldChance)
			return Voxel::BlockID::EmeraldOre;
	}

	// Lapis — below y=30.
	if (wy <= 30 && oreNoise(seed + 5200.0f, 0.20f))
		return Voxel::BlockID::LapisOre;

	// Redstone — below y=16.
	if (wy <= 16 && oreNoise(seed + 5300.0f, 0.22f))
		return Voxel::BlockID::RedstoneOre;

	// Gold — below y=32.
	if (wy <= 32 && oreNoise(seed + 5400.0f, 0.16f))
		return Voxel::BlockID::GoldOre;

	// Iron — below y=64.
	if (wy <= 64 && oreNoise(seed + 5500.0f, 0.14f))
		return Voxel::BlockID::IronOre;

	// Coal — below y=80.
	if (wy <= 80 && oreNoise(seed + 5600.0f, 0.12f))
		return Voxel::BlockID::CoalOre;

	return Voxel::BlockID::Stone;
}

// ---------------------------------------------------------------------------
// Tree planting (local chunk coordinates)
// ---------------------------------------------------------------------------

void TerrainGen::plantTree(Chunk::Chunk& chunk, int lx, int ly, int lz) const
{
	// Trunk: 4 blocks tall.
	const int trunkH = 4;
	for (int i = 0; i < trunkH; ++i)
		chunk.setBlock(lx, ly + i, lz, Voxel::BlockID::Wood);

	// Leaf crown — standard Minecraft oak style:
	//   Bottom two crown layers: 3x3 with corners trimmed
	//   Top two crown layers:    3x3 solid (smaller cap)
	const int crownY = ly + trunkH - 1; // first leaf layer overlaps top of trunk

	for (int dy = 0; dy <= 3; ++dy)
	{
		int radius = (dy <= 1) ? 1 : 0; // dy 0,1 → 3x3; dy 2,3 → only 1x1 plus cross
		for (int dx = -1; dx <= 1; ++dx)
		for (int dz = -1; dz <= 1; ++dz)
		{
			// Don't overwrite the trunk in lower layers.
			if (dx == 0 && dz == 0 && dy <= 1) continue;
			chunk.setBlock(lx + dx, crownY + dy, lz + dz, Voxel::BlockID::Leaf);
		}
		// For the top two layers only place the centre + cardinal neighbours.
		if (dy >= 2)
		{
			chunk.setBlock(lx,     crownY + dy, lz,     Voxel::BlockID::Leaf);
			chunk.setBlock(lx + 1, crownY + dy, lz,     Voxel::BlockID::Leaf);
			chunk.setBlock(lx - 1, crownY + dy, lz,     Voxel::BlockID::Leaf);
			chunk.setBlock(lx,     crownY + dy, lz + 1, Voxel::BlockID::Leaf);
			chunk.setBlock(lx,     crownY + dy, lz - 1, Voxel::BlockID::Leaf);
		}
	}
}

// ---------------------------------------------------------------------------
// Main generation pass
// ---------------------------------------------------------------------------

void TerrainGen::generate(Chunk::Chunk& chunk, int chunkX, int chunkZ) const
{
	const float seed = static_cast<float>(m_settings.seed);
	const int kSeaLevel = m_settings.seaLevel;

	// Pre-compute per-column biome & height (XZ pass).
	Biome   biome [Chunk::CHUNK_SIZE_X][Chunk::CHUNK_SIZE_Z];
	int     surfH [Chunk::CHUNK_SIZE_X][Chunk::CHUNK_SIZE_Z];

	for (int lz = 0; lz < Chunk::CHUNK_SIZE_Z; ++lz)
	for (int lx = 0; lx < Chunk::CHUNK_SIZE_X; ++lx)
	{
		const float wx = static_cast<float>(chunkX * Chunk::CHUNK_SIZE_X + lx);
		const float wz = static_cast<float>(chunkZ * Chunk::CHUNK_SIZE_Z + lz);
		biome[lx][lz]  = getBiome(wx, wz);
		surfH[lx][lz]  = getSurfaceHeight(wx, wz, biome[lx][lz]);
	}


	// ---------------------------------------------------------------------------
	// Y pass - fill blocks column by column.
	// ---------------------------------------------------------------------------
	for (int lz = 0; lz < Chunk::CHUNK_SIZE_Z; ++lz)
	for (int lx = 0; lx < Chunk::CHUNK_SIZE_X; ++lx)
	{
		const int wx = chunkX * Chunk::CHUNK_SIZE_X + lx;
		const int wz = chunkZ * Chunk::CHUNK_SIZE_Z + lz;
		const Biome b = biome[lx][lz];
		const int   h = surfH[lx][lz];
		const float fwx = static_cast<float>(wx);
		const float fwz = static_cast<float>(wz);

		// Grass patch variant: two noise octaves produce large smooth colour
		// patches. Each biome selects from its own sub-range of Grass1-8:
		//   Tundra  : Grass1-2  (pale yellowish)
		//   Plains  : Grass2-5  (mid bright-green)
		//   Forest  : Grass5-8  (dark lush green)
		//   Other   : Grass3-5  (neutral mid-green)
		float gPatch  = p01(fwx * 0.04f, 0.0f, fwz * 0.04f, seed + 8100.0f);
		float gDetail = p01(fwx * 0.12f, 0.0f, fwz * 0.12f, seed + 8200.0f);
		float gBlend  = gPatch * 0.75f + gDetail * 0.25f;

		int grassBase, grassRange;
		switch (b)
		{
			case Biome::Tundra:  grassBase = 0; grassRange = 2; break;
			case Biome::Plains:  grassBase = 1; grassRange = 4; break;
			case Biome::Forest:  grassBase = 4; grassRange = 4; break;
			default:             grassBase = 2; grassRange = 3; break;
		}
		int grassIdx = grassBase + static_cast<int>(gBlend * static_cast<float>(grassRange));
		if (grassIdx > 7) grassIdx = 7;
		const Voxel::BlockID grassBlock = static_cast<Voxel::BlockID>(
			static_cast<int>(Voxel::BlockID::Grass1) + grassIdx);

		// Rock layer variant: Y depth drives geological strata. A low-freq wobble
		// breaks perfectly horizontal seams. Biome shifts the palette:
		//   Mountains : Rock1-4   Tundra : Rock2-5   Plains : Rock3-6
		//   Forest    : Rock4-7   Desert : Rock5-8
		float rWobble = pnoise(fwx * 0.018f, 0.0f, fwz * 0.018f, seed + 9100.0f) * 6.0f;
		int biomRockShift;
		switch (b)
		{
			case Biome::Mountains: biomRockShift = 0; break;
			case Biome::Tundra:    biomRockShift = 1; break;
			case Biome::Plains:    biomRockShift = 2; break;
			case Biome::Forest:    biomRockShift = 3; break;
			case Biome::Desert:    biomRockShift = 4; break;
			default:               biomRockShift = 0; break;
		}
		auto rockAtDepth = [&](int wy) -> Voxel::BlockID {
			int strata = static_cast<int>((static_cast<float>(wy) + rWobble + 8.0f) / 16.0f);
			strata += biomRockShift;
			if (strata < 0) strata = 0;
			if (strata > 7) strata = 7;
			return static_cast<Voxel::BlockID>(
				static_cast<int>(Voxel::BlockID::Rock1) + strata);
		};

		// Sub-surface fill layers (dirt or sand).
		const bool isWetBiome = (b == Biome::Desert || b == Biome::Ocean || h <= kSeaLevel);
		const int subLayers   = isWetBiome ? 4 : 3;
		const Voxel::BlockID subBlock = isWetBiome
			? Voxel::BlockID::Sand : Voxel::BlockID::Dirt;

		for (int ly = 0; ly < Chunk::CHUNK_SIZE_Y; ++ly)
		{
			Voxel::BlockID block = Voxel::BlockID::Air;

			if (ly == 0)
			{
				block = Voxel::BlockID::Bedrock;
			}
			else if (ly < h - subLayers)
			{
				// Deep underground: layered rock with ores, or carved out by caves
				const float fwy = static_cast<float>(ly);

				// Check if this voxel should be carved out by cave system
				if (isCave(fwx, fwy, fwz))
				{
					block = Voxel::BlockID::Air;  // Cave carving
				}
				else
				{
					// Solid rock with ore veins
					Voxel::BlockID ore = oreAt(wx, ly, wz);
					block = (ore != Voxel::BlockID::Stone) ? ore : rockAtDepth(ly);
				}
			}
			else if (ly >= h - subLayers && ly < h)
			{
				// Subsurface layers - but check for caves first!
				const float fwy = static_cast<float>(ly);
				if (isCave(fwx, fwy, fwz))
				{
					block = Voxel::BlockID::Air;  // Cave carving
				}
				else
				{
					block = subBlock;
				}
			}
			else if (ly == h)
			{
				// Surface layer - check for caves first!
				const float fwy = static_cast<float>(ly);
				if (isCave(fwx, fwy, fwz))
				{
					block = Voxel::BlockID::Air;  // Cave entrance carving!
				}
				else if (h <= kSeaLevel)
				{
					block = Voxel::BlockID::Sand;
				}
				else
				{
					switch (b)
					{
						case Biome::Desert:
						case Biome::Ocean:
							block = Voxel::BlockID::Sand;
							break;
						case Biome::Tundra:
							block = Voxel::BlockID::SnowBlock;
							break;
						case Biome::Mountains:
							block = (h >= 105) ? Voxel::BlockID::SnowBlock
											  : rockAtDepth(h);
							break;
						default:
							block = (h >= 85) ? rockAtDepth(h) : grassBlock;
							break;
					}
				}
			}

			// Water fill: surface depressions above terrain, below sea level.
			// Don't fill caves - only fill surface depressions
			// Fill 1 block below sea level so waves stay below land surface
			if (block == Voxel::BlockID::Air && ly > h && ly < kSeaLevel)
			{
				block = Voxel::BlockID::Water;
			}

			chunk.setBlock(lx, ly, lz, block);
		}
	}

	// ---------------------------------------------------------------------------
	// Tree placement - done after the full column fill to avoid overwriting.
	// ---------------------------------------------------------------------------
	if (m_settings.enableTrees && m_settings.treeDensity > 0.0f)
	{
		for (int lz = 1; lz < Chunk::CHUNK_SIZE_Z - 2; ++lz)
		for (int lx = 1; lx < Chunk::CHUNK_SIZE_X - 2; ++lx)
		{
			const Biome b = biome[lx][lz];
			if (b != Biome::Forest && b != Biome::Plains)
				continue;
			if (surfH[lx][lz] <= kSeaLevel)
				continue;

			const int wx = chunkX * Chunk::CHUNK_SIZE_X + lx;
			const int wz = chunkZ * Chunk::CHUNK_SIZE_Z + lz;

			unsigned int th = hash3(wx, 0, wz);
			// Apply tree density multiplier - forest gets more trees
			float baseDensity = (b == Biome::Forest) ? 0.008f : 0.002f;
			float density = baseDensity * m_settings.treeDensity * 10.0f;  // Scale treeDensity range
			if ((th & 0xFFFF) > static_cast<unsigned int>(density * 65536.0f))
				continue;

			const int sy = surfH[lx][lz];
			Voxel::BlockID top = chunk.getBlock(lx, sy, lz);
			if (static_cast<int>(top) >= static_cast<int>(Voxel::BlockID::Grass1) &&
				static_cast<int>(top) <= static_cast<int>(Voxel::BlockID::Grass8))
				plantTree(chunk, lx, sy + 1, lz);
		}
	}

	// ---------------------------------------------------------------------------
	// Mountain cave markers - DEBUG: Place visible beacons at cave locations
	// ---------------------------------------------------------------------------
	if (m_settings.enableMountainCaves)
	{
		const float rarity = m_settings.mountainCaveRarity;

		// Check if any mountain cave centers fall within this chunk
		for (int lx = 0; lx < Chunk::CHUNK_SIZE_X; ++lx)
		for (int lz = 0; lz < Chunk::CHUNK_SIZE_Z; ++lz)
		{
			const float wx = static_cast<float>(chunkX * Chunk::CHUNK_SIZE_X + lx);
			const float wz = static_cast<float>(chunkZ * Chunk::CHUNK_SIZE_Z + lz);

			// Calculate which grid cell this block is in
			const int gridX = static_cast<int>(std::floor(wx / rarity));
			const int gridZ = static_cast<int>(std::floor(wz / rarity));

			// Calculate cave center for this grid cell
			float centerX = (gridX + 0.5f) * rarity;
			float centerZ = (gridZ + 0.5f) * rarity;

			// Check if we're close to the center (within 2 blocks)
			float dx = wx - centerX;
			float dz = wz - centerZ;
			if (std::abs(dx) <= 2.0f && std::abs(dz) <= 2.0f)
			{
				// Check if this grid cell has a cave
				unsigned int cellHash = hash3(gridX, 0, gridZ);
				float cellValue = static_cast<float>(cellHash & 0xFFFF) / 65536.0f;

				if (cellValue <= 0.5f)  // 50% chance
				{
					// Check if it's in a mountain biome
					Biome caveBiome = getBiome(centerX, centerZ);
					if (caveBiome == Biome::Mountains)
					{
						// Get surface height and place a tall marker beacon
						int surfaceHeight = getSurfaceHeight(centerX, centerZ, caveBiome);

						// Place emerald blocks in a vertical pillar from surface to sky
						// This will be VERY visible from the air!
						for (int markerY = surfaceHeight + 1; markerY < Chunk::CHUNK_SIZE_Y && markerY < surfaceHeight + 50; ++markerY)
						{
							chunk.setBlock(lx, markerY, lz, Voxel::BlockID::EmeraldOre);
						}

						// Also place a bright base marker at surface level
						chunk.setBlock(lx, surfaceHeight, lz, Voxel::BlockID::DiamondOre);
					}
				}
			}
		}
	}

	// ---------------------------------------------------------------------------
	// Sky light propagation - improved with horizontal spreading
	// ---------------------------------------------------------------------------
	const auto& blockReg = Voxel::BlockRegistry::get();

	// Step 1: Initialize sky columns (direct sunlight from above)
	for (int lx = 0; lx < Chunk::CHUNK_SIZE_X; ++lx)
	for (int lz = 0; lz < Chunk::CHUNK_SIZE_Z; ++lz)
	{
		uint8_t lightLevel = 15;  // Start at max sky light at the top

		// Scan from top to bottom
		for (int ly = Chunk::CHUNK_SIZE_Y - 1; ly >= 0; --ly)
		{
			Voxel::BlockID block = chunk.getBlock(lx, ly, lz);

			// Set the current light level
			chunk.setSkyLight(lx, ly, lz, lightLevel);

			// Check if this block blocks light
			if (block != Voxel::BlockID::Air)
			{
				const auto& props = blockReg.propertiesOf(block);
				if (!props.isTransparent)
				{
					// Opaque block - stop light from going below
					lightLevel = 0;
				}
				else
				{
					// Transparent block - reduce light slightly
					if (lightLevel > 0)
						lightLevel = static_cast<uint8_t>(lightLevel > 1 ? lightLevel - 1 : 0);
				}
			}
		}
	}

	// Step 2: Horizontal light propagation (flood-fill within chunk)
	// This spreads light into caves and around corners
	// Use a queue-based flood fill for better light distribution
	struct LightNode { int x, y, z; };
	std::vector<LightNode> lightQueue;

	// Seed the queue with all lit blocks
	for (int ly = 0; ly < Chunk::CHUNK_SIZE_Y; ++ly)
	for (int lx = 0; lx < Chunk::CHUNK_SIZE_X; ++lx)
	for (int lz = 0; lz < Chunk::CHUNK_SIZE_Z; ++lz)
	{
		if (chunk.getSkyLight(lx, ly, lz) > 0)
			lightQueue.push_back({lx, ly, lz});
	}

	// Process queue: spread light from bright to dim areas
	// Multiple passes ensure light reaches far enough
	for (int pass = 0; pass < 5; ++pass)  // 5 full propagation passes (was 3)
	{
		std::vector<LightNode> nextQueue;

		for (const auto& node : lightQueue)
		{
			uint8_t currentLight = chunk.getSkyLight(node.x, node.y, node.z);
			if (currentLight <= 1) continue;

			// Check all 6 neighbors
			const int dx[] = {1, -1, 0, 0, 0, 0};
			const int dy[] = {0, 0, 1, -1, 0, 0};
			const int dz[] = {0, 0, 0, 0, 1, -1};

			for (int dir = 0; dir < 6; ++dir)
			{
				int nx = node.x + dx[dir];
				int ny = node.y + dy[dir];
				int nz = node.z + dz[dir];

				// Check bounds
				if (!Chunk::Chunk::inBounds(nx, ny, nz))
					continue;

				Voxel::BlockID neighborBlock = chunk.getBlock(nx, ny, nz);

				// Can only spread light through air or transparent blocks
				if (neighborBlock != Voxel::BlockID::Air)
				{
					const auto& props = blockReg.propertiesOf(neighborBlock);
					if (!props.isTransparent)
						continue;  // Opaque block blocks light
				}

				uint8_t neighborLight = chunk.getSkyLight(nx, ny, nz);
				// Very slow decay: only lose 1 light level every 3 blocks (was 2)
				uint8_t newLight;
				if (currentLight > 3)
					newLight = currentLight - 1;
				else if (currentLight > 1)
					newLight = 2;  // Keep at 2 for a while
				else
					newLight = 1;

				// Update neighbor if our light is brighter
				if (newLight > neighborLight)
				{
					chunk.setSkyLight(nx, ny, nz, newLight);
					nextQueue.push_back({nx, ny, nz});
				}
			}
		}

		lightQueue = std::move(nextQueue);
	}
}

} // namespace WorldGen

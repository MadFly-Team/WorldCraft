#pragma once

namespace WorldGen
{
	// World generation settings that can be configured via UI
	struct WorldSettings
	{
		// Basic settings
		int   seed          = 42;
		int   renderDistance = 20;  // in chunks - balanced default view distance

		// Terrain generation
		float terrainScale     = 100.0f;   // Larger = smoother terrain
		float terrainAmplitude = 80.0f;    // Larger = taller mountains
		int   seaLevel         = 64;       // Y coordinate of sea level

		// Biome settings
		float biomeScale       = 300.0f;   // Larger = bigger biome regions
		float biomeBlend       = 0.3f;     // 0-1, controls biome blending

		// Cave generation - NEW SYSTEM: Mountain entrances + Deep caverns (no worm caves!)
		bool  enableMountainCaves = true;      // Large cave entrances on mountainsides
		float mountainCaveRarity  = 800.0f;    // Distance between mountain caves (larger = rarer)
		float mountainCaveSize    = 25.0f;     // Size of mountain cave entrance/tunnels

		bool  enableDeepCaverns   = true;      // Massive underground chambers
		float cavernSize          = 35.0f;     // Size of deep cavern spaces (larger = bigger caverns)
		int   cavernMinDepth      = 10;        // Don't spawn caverns above this Y level
		int   cavernMaxDepth      = 50;        // Caverns most common below this Y level

		// Ore generation
		bool  enableOres       = true;
		float oreAbundance     = 1.0f;     // Multiplier for ore spawn rates

		// Trees
		bool  enableTrees      = true;
		float treeDensity      = 0.02f;    // 0-1, probability per suitable block

		// Water simulation
		bool  enableWaterFlow  = true;     // Enable water spreading simulation
		bool  enableWaterWaves = true;     // Enable visual wave effect on water surfaces
		float waterFlowRate    = 5.0f;     // Ticks per second for water updates (higher = faster flow)

		// Persistence settings
		bool  enableAutoSave   = false;    // Auto-save every 5 minutes (default: OFF)
		bool  loadLastWorldOnStartup = true;  // Load most recent world on startup (default: ON for testing)

		// World name (for future save/load)
		char  worldName[64]    = "New World";

		// Start position for camera navigation
		float startX = 0.0f;
		float startY = 100.0f;
		float startZ = 0.0f;

		// Helper to create default settings
		static WorldSettings createDefault()
		{
			return WorldSettings{};
		}

		// Preset configurations
		static WorldSettings createFlat()
		{
			WorldSettings s;
			s.terrainAmplitude = 3.0f;    // Minimal variation - keeps base elevations but nearly flat
			s.terrainScale = 300.0f;      // Very smooth/gradual changes
			s.seaLevel = 50;              // Lower than most base elevations (58-62)
			s.enableTrees = false;
			s.enableMountainCaves = false;
			s.enableDeepCaverns = false;
			return s;
		}

		static WorldSettings createMountainous()
		{
			WorldSettings s;
			s.terrainAmplitude = 120.0f;
			s.terrainScale = 80.0f;
			return s;
		}

		static WorldSettings createIslands()
		{
			WorldSettings s;
			s.terrainAmplitude = 60.0f;
			s.seaLevel = 80;
			s.biomeScale = 200.0f;
			return s;
		}

		static WorldSettings createCaveWorld()
		{
			WorldSettings s;
			s.enableMountainCaves = true;
			s.mountainCaveRarity = 400.0f;   // More frequent mountain caves
			s.mountainCaveSize = 35.0f;      // Larger entrances
			s.enableDeepCaverns = true;
			s.cavernSize = 50.0f;            // Massive underground spaces
			s.cavernMaxDepth = 80;           // Caverns reach higher up
			s.enableTrees = false;
			return s;
		}
	};
}

#pragma once

#include <Persistence/WorldMetadata.h>
#include <Persistence/RegionFile.h>
#include <Chunk/Chunk.h>
#include <Util/PairHash.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace Persistence
{

	// WorldPersistence manages all save/load operations for a world
	class WorldPersistence
	{
	public:
		explicit WorldPersistence(const std::string& savesDirectory = "saves");
		~WorldPersistence();

		// Create a new world with the given metadata
		bool createWorld(const WorldMetadata& metadata);

		// Load an existing world by name
		// Returns true on success, fills outMetadata
		bool loadWorld(const std::string& worldName, WorldMetadata& outMetadata);

		// Save current world metadata
		bool saveMetadata(const WorldMetadata& metadata);

		// Load current world metadata
		bool loadMetadata(WorldMetadata& outMetadata) const;

		// Check if a chunk exists on disk
		bool hasChunk(int chunkX, int chunkZ);

		// Load a chunk from disk
		// Returns nullptr if chunk doesn't exist or fails to load
		std::unique_ptr<Chunk::Chunk> loadChunk(int chunkX, int chunkZ);

		// Save a chunk to disk
		// Returns true on success
		bool saveChunk(int chunkX, int chunkZ, const Chunk::Chunk& chunk);

		// Save multiple chunks (batch operation)
		// Returns number of chunks successfully saved
		int saveChunks(const std::vector<std::pair<int, int>>& chunkCoords,
					   const std::unordered_map<std::pair<int, int>, const Chunk::Chunk*>& chunks);

		// Get world directory path
		std::string getWorldDirectory() const { return m_worldDir; }

		// Get region directory path
		std::string getRegionDirectory() const { return m_worldDir + "/region"; }

		// Check if a world is currently open
		bool isWorldOpen() const { return !m_worldDir.empty(); }

		// Get current world name
		std::string getCurrentWorldName() const { return m_currentWorldName; }

		// Switch to a different world (closes current, opens new)
		bool switchWorld(const std::string& worldName);

		// Close current world and release resources
		void closeWorld();

		// List all available worlds in the saves directory
		std::vector<WorldMetadata> listWorlds() const;

		// Delete a world (removes all files and directories)
		// Returns false if world doesn't exist or is currently open
		bool deleteWorld(const std::string& worldName);

	private:
		std::string m_savesDir;
		std::string m_worldDir;  // Current world directory
		std::string m_currentWorldName;  // Current world name

		// Cache of open region files
		std::unordered_map<RegionCoord, std::unique_ptr<RegionFile>, RegionCoordHash> m_regionFiles;
		mutable std::mutex m_regionMutex;

		// Get or create a region file for the given chunk coordinates
		RegionFile* getOrCreateRegionFile(int chunkX, int chunkZ);

		// Close all open region files
		void closeAllRegionFiles();

		// Hash function for pair<int, int> to use with unordered_map
		struct PairHash
		{
			size_t operator()(const std::pair<int, int>& p) const
			{
				return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
			}
		};
	};

} // namespace Persistence

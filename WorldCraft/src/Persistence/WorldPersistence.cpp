#include <Persistence/WorldPersistence.h>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace Persistence
{

WorldPersistence::WorldPersistence(const std::string& savesDirectory)
	: m_savesDir(savesDirectory)
{
	// Create saves directory if it doesn't exist
	std::filesystem::create_directories(m_savesDir);
}

WorldPersistence::~WorldPersistence()
{
	closeWorld();
}

bool WorldPersistence::createWorld(const WorldMetadata& metadata)
{
	// Close any existing world
	closeWorld();

	// Sanitize world name for directory
	std::string sanitizedName = metadata.worldName;
	std::replace_if(sanitizedName.begin(), sanitizedName.end(),
					[](char c) { return c == '/' || c == '\\' || c == ':' || c == '*' || 
								 c == '?' || c == '"' || c == '<' || c == '>' || c == '|'; },
					'_');

	// Create world directory with UUID for uniqueness
	m_worldDir = m_savesDir + "/" + sanitizedName + "_" + metadata.worldUUID.substr(0, 8);

	if (std::filesystem::exists(m_worldDir))
		return false;  // World already exists

	// Create world and region directories
	if (!std::filesystem::create_directories(m_worldDir))
		return false;

	m_currentWorldName = metadata.worldName;

	if (!std::filesystem::create_directories(getRegionDirectory()))
		return false;

	// Save metadata
	return saveMetadata(metadata);
}

bool WorldPersistence::loadWorld(const std::string& worldName, WorldMetadata& outMetadata)
{
	// Close any existing world
	closeWorld();

	// Search for world directory matching the name
	if (!std::filesystem::exists(m_savesDir))
		return false;

	for (const auto& entry : std::filesystem::directory_iterator(m_savesDir))
	{
		if (entry.is_directory())
		{
			std::string dirName = entry.path().filename().string();

			// Check if this directory starts with the world name
			if (dirName.find(worldName) == 0)
			{
				m_worldDir = entry.path().string();

				// Try to load metadata
				if (loadMetadata(outMetadata))
				{
					// Verify the world name matches
					if (outMetadata.worldName == worldName)
					{
						m_currentWorldName = worldName;
						return true;
					}
				}
			}
		}
	}

	m_worldDir.clear();
	m_currentWorldName.clear();
	return false;
}

bool WorldPersistence::saveMetadata(const WorldMetadata& metadata)
{
	if (m_worldDir.empty())
		return false;

	std::string metadataPath = m_worldDir + "/world.dat";
	return metadata.saveToFile(metadataPath);
}

bool WorldPersistence::loadMetadata(WorldMetadata& outMetadata) const
{
	if (m_worldDir.empty())
		return false;

	std::string metadataPath = m_worldDir + "/world.dat";
	return outMetadata.loadFromFile(metadataPath);
}

void WorldPersistence::closeWorld()
{
	closeAllRegionFiles();
	m_worldDir.clear();
	m_currentWorldName.clear();
}

void WorldPersistence::closeAllRegionFiles()
{
	std::lock_guard<std::mutex> lock(m_regionMutex);

	for (auto& pair : m_regionFiles)
	{
		if (pair.second)
			pair.second->close();
	}

	m_regionFiles.clear();
}

RegionFile* WorldPersistence::getOrCreateRegionFile(int chunkX, int chunkZ)
{
	if (m_worldDir.empty())
		return nullptr;

	RegionCoord rc = RegionCoord::fromChunkCoord(chunkX, chunkZ);

	std::lock_guard<std::mutex> lock(m_regionMutex);

	// Check if already open
	auto it = m_regionFiles.find(rc);
	if (it != m_regionFiles.end())
		return it->second.get();

	// Create new region file
	auto regionFile = std::make_unique<RegionFile>(getRegionDirectory(), rc);
	if (!regionFile->open())
		return nullptr;

	RegionFile* ptr = regionFile.get();
	m_regionFiles[rc] = std::move(regionFile);
	return ptr;
}

bool WorldPersistence::hasChunk(int chunkX, int chunkZ)
{
	RegionFile* rf = getOrCreateRegionFile(chunkX, chunkZ);
	if (!rf)
		return false;

	return rf->hasChunk(chunkX, chunkZ);
}

std::unique_ptr<Chunk::Chunk> WorldPersistence::loadChunk(int chunkX, int chunkZ)
{
	RegionFile* rf = getOrCreateRegionFile(chunkX, chunkZ);
	if (!rf)
		return nullptr;

	return rf->readChunk(chunkX, chunkZ);
}

bool WorldPersistence::saveChunk(int chunkX, int chunkZ, const Chunk::Chunk& chunk)
{
	RegionFile* rf = getOrCreateRegionFile(chunkX, chunkZ);
	if (!rf)
		return false;

	return rf->writeChunk(chunkX, chunkZ, chunk);
}

int WorldPersistence::saveChunks(const std::vector<std::pair<int, int>>& chunkCoords,
								  const std::unordered_map<std::pair<int, int>, const Chunk::Chunk*>& chunks)
{
	int savedCount = 0;

	for (const auto& coord : chunkCoords)
	{
		auto it = chunks.find(coord);
		if (it != chunks.end() && it->second != nullptr)
		{
			if (saveChunk(coord.first, coord.second, *it->second))
				savedCount++;
		}
	}

	return savedCount;
}

std::vector<WorldMetadata> WorldPersistence::listWorlds() const
{
	std::vector<WorldMetadata> worlds;

	if (!std::filesystem::exists(m_savesDir))
	{
		std::cout << "[WorldPersistence] Saves directory does not exist: " << m_savesDir << std::endl;
		return worlds;
	}

	std::cout << "[WorldPersistence] Scanning for worlds in: " << m_savesDir << std::endl;

	for (const auto& entry : std::filesystem::directory_iterator(m_savesDir))
	{
		if (entry.is_directory())
		{
			std::string dirName = entry.path().filename().string();
			std::string metadataPath = entry.path().string() + "/world.dat";

			std::cout << "[WorldPersistence]   Checking directory: " << dirName << std::endl;

			if (std::filesystem::exists(metadataPath))
			{
				WorldMetadata meta;
				if (meta.loadFromFile(metadataPath))
				{
					std::cout << "[WorldPersistence]     ✓ Loaded world: " << meta.worldName << std::endl;
					worlds.push_back(meta);
				}
				else
				{
					std::cout << "[WorldPersistence]     ✗ Failed to load metadata from: " << metadataPath << std::endl;
				}
			}
			else
			{
				std::cout << "[WorldPersistence]     - No world.dat found" << std::endl;
			}
		}
	}

	std::cout << "[WorldPersistence] Found " << worlds.size() << " world(s)" << std::endl;

	// Sort by last played time (most recent first)
	std::sort(worlds.begin(), worlds.end(),
			  [](const WorldMetadata& a, const WorldMetadata& b) {
				  return a.lastPlayedTime > b.lastPlayedTime;
			  });

	return worlds;
}

bool WorldPersistence::switchWorld(const std::string& worldName)
{
	WorldMetadata metadata;
	return loadWorld(worldName, metadata);
}

bool WorldPersistence::deleteWorld(const std::string& worldName)
{
	// Don't allow deleting the currently open world
	if (m_currentWorldName == worldName)
		return false;

	// Find the world directory
	if (!std::filesystem::exists(m_savesDir))
		return false;

	for (const auto& entry : std::filesystem::directory_iterator(m_savesDir))
	{
		if (entry.is_directory())
		{
			std::string dirName = entry.path().filename().string();

			// Check if this directory matches the world name
			if (dirName.find(worldName) == 0)
			{
				// Try to load metadata to verify it's the right world
				std::string metadataPath = entry.path().string() + "/world.dat";
				WorldMetadata meta;
				if (meta.loadFromFile(metadataPath) && meta.worldName == worldName)
				{
					// Delete the entire world directory
					try
					{
						std::filesystem::remove_all(entry.path());
						return true;
					}
					catch (const std::filesystem::filesystem_error&)
					{
						return false;
					}
				}
			}
		}
	}

	return false;
}

} // namespace Persistence

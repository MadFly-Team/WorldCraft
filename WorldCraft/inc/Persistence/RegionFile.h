#pragma once

#include <Chunk/Chunk.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <fstream>

namespace Persistence
{

	// Region dimensions - each region contains 32x32 chunks
	constexpr int REGION_SIZE = 32;
	constexpr int CHUNKS_PER_REGION = REGION_SIZE * REGION_SIZE;

	// Sector size for region file storage (4KB)
	constexpr int SECTOR_SIZE = 4096;

	// Header size (offset table for all chunks)
	constexpr int HEADER_SIZE = SECTOR_SIZE;  // 1024 uint32_t values (4 bytes each)

	// Convert chunk coordinates to region coordinates
	struct RegionCoord
	{
		int x = 0;
		int z = 0;

		bool operator==(const RegionCoord& o) const { return x == o.x && z == o.z; }

		// Convert chunk coordinates to region coordinates
		static RegionCoord fromChunkCoord(int chunkX, int chunkZ);

		// Get local chunk index within region (0-1023)
		static int getLocalChunkIndex(int chunkX, int chunkZ);
	};

	struct RegionCoordHash
	{
		std::size_t operator()(const RegionCoord& c) const noexcept
		{
			auto h1 = std::hash<int>{}(c.x);
			auto h2 = std::hash<int>{}(c.z);
			return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL + 0x6c62272e07bb0142ULL + (h1 << 6) + (h1 >> 2));
		}
	};

	// Region file handler for reading/writing chunks to disk
	class RegionFile
	{
	public:
		RegionFile(const std::string& regionDirectory, const RegionCoord& coord);
		~RegionFile();

		// Open the region file (creates if doesn't exist)
		bool open();

		// Close the region file
		void close();

		// Check if a chunk exists in this region file
		bool hasChunk(int chunkX, int chunkZ) const;

		// Read a chunk from the region file
		// Returns nullptr if chunk doesn't exist or read fails
		std::unique_ptr<Chunk::Chunk> readChunk(int chunkX, int chunkZ);

		// Write a chunk to the region file
		// Returns true on success
		bool writeChunk(int chunkX, int chunkZ, const Chunk::Chunk& chunk);

		// Get file path for this region
		std::string getFilePath() const;

		// Check if region file is open
		bool isOpen() const { return m_file.is_open(); }

	private:
		std::string m_regionDir;
		RegionCoord m_coord;
		std::fstream m_file;

		// Offset table: stores location and size of each chunk
		// Each entry: sector offset (24 bits) + sector count (8 bits)
		uint32_t m_offsets[CHUNKS_PER_REGION];

		// Track which sectors are in use for allocation
		std::vector<bool> m_usedSectors;

		// Load the offset table from file
		bool loadOffsetTable();

		// Save the offset table to file
		bool saveOffsetTable();

		// Allocate sectors for chunk data
		int allocateSectors(int count);

		// Free sectors used by a chunk
		void freeSectors(int offset, int count);

		// Read raw chunk data from file
		bool readChunkData(int offset, int sectorCount, std::vector<uint8_t>& outData);

		// Write raw chunk data to file
		bool writeChunkData(int offset, int sectorCount, const std::vector<uint8_t>& data);
	};

} // namespace Persistence

#include <Persistence/RegionFile.h>
#include <Persistence/ChunkSerializer.h>
#include <filesystem>
#include <cstring>
#include <algorithm>

namespace Persistence
{

RegionCoord RegionCoord::fromChunkCoord(int chunkX, int chunkZ)
{
	RegionCoord rc;
	rc.x = chunkX >> 5;  // Divide by 32
	rc.z = chunkZ >> 5;

	// Handle negative coordinates correctly
	if (chunkX < 0 && (chunkX & 31) != 0) rc.x--;
	if (chunkZ < 0 && (chunkZ & 31) != 0) rc.z--;

	return rc;
}

int RegionCoord::getLocalChunkIndex(int chunkX, int chunkZ)
{
	// Get local coordinates within region (0-31)
	int localX = chunkX & 31;  // Modulo 32
	int localZ = chunkZ & 31;

	// Handle negative coordinates
	if (localX < 0) localX += 32;
	if (localZ < 0) localZ += 32;

	return localX + localZ * REGION_SIZE;
}

RegionFile::RegionFile(const std::string& regionDirectory, const RegionCoord& coord)
	: m_regionDir(regionDirectory)
	, m_coord(coord)
{
	std::memset(m_offsets, 0, sizeof(m_offsets));
}

RegionFile::~RegionFile()
{
	close();
}

std::string RegionFile::getFilePath() const
{
	return m_regionDir + "/r." + std::to_string(m_coord.x) + "." + std::to_string(m_coord.z) + ".wcr";
}

bool RegionFile::open()
{
	// Create region directory if it doesn't exist
	std::filesystem::create_directories(m_regionDir);

	std::string filepath = getFilePath();
	bool fileExists = std::filesystem::exists(filepath);

	if (fileExists)
	{
		// Open existing file for reading and writing
		m_file.open(filepath, std::ios::in | std::ios::out | std::ios::binary);
		if (!m_file.is_open())
			return false;

		// Load existing offset table
		if (!loadOffsetTable())
		{
			m_file.close();
			return false;
		}
	}
	else
	{
		// Create new file
		m_file.open(filepath, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
		if (!m_file.is_open())
			return false;

		// Initialize with empty offset table
		std::memset(m_offsets, 0, sizeof(m_offsets));
		if (!saveOffsetTable())
		{
			m_file.close();
			return false;
		}
	}

	// Initialize sector usage tracking
	// Sector 0 is used by the header
	m_usedSectors.clear();
	m_usedSectors.resize(1, true);

	// Mark sectors used by existing chunks
	for (int i = 0; i < CHUNKS_PER_REGION; i++)
	{
		uint32_t entry = m_offsets[i];
		if (entry != 0)
		{
			int offset = (entry >> 8) & 0xFFFFFF;  // 24-bit offset
			int count = entry & 0xFF;               // 8-bit count

			// Ensure we have enough space in the tracking vector
			if (offset + count > static_cast<int>(m_usedSectors.size()))
				m_usedSectors.resize(offset + count, false);

			// Mark sectors as used
			for (int s = 0; s < count; s++)
				m_usedSectors[offset + s] = true;
		}
	}

	return true;
}

void RegionFile::close()
{
	if (m_file.is_open())
	{
		m_file.close();
	}
}

bool RegionFile::loadOffsetTable()
{
	m_file.seekg(0, std::ios::beg);
	m_file.read(reinterpret_cast<char*>(m_offsets), sizeof(m_offsets));
	return m_file.good();
}

bool RegionFile::saveOffsetTable()
{
	m_file.seekp(0, std::ios::beg);
	m_file.write(reinterpret_cast<const char*>(m_offsets), sizeof(m_offsets));
	m_file.flush();
	return m_file.good();
}

bool RegionFile::hasChunk(int chunkX, int chunkZ) const
{
	int index = RegionCoord::getLocalChunkIndex(chunkX, chunkZ);
	return m_offsets[index] != 0;
}

int RegionFile::allocateSectors(int count)
{
	if (count <= 0)
		return -1;

	// Find contiguous free sectors
	int start = -1;
	int consecutive = 0;

	for (size_t i = 1; i < m_usedSectors.size(); i++)  // Skip sector 0 (header)
	{
		if (!m_usedSectors[i])
		{
			if (start == -1)
				start = static_cast<int>(i);
			consecutive++;

			if (consecutive >= count)
			{
				// Found enough space - mark as used
				for (int j = 0; j < count; j++)
					m_usedSectors[start + j] = true;
				return start;
			}
		}
		else
		{
			start = -1;
			consecutive = 0;
		}
	}

	// Need to expand file - allocate at end
	start = static_cast<int>(m_usedSectors.size());
	for (int i = 0; i < count; i++)
		m_usedSectors.push_back(true);

	return start;
}

void RegionFile::freeSectors(int offset, int count)
{
	for (int i = 0; i < count && (offset + i) < static_cast<int>(m_usedSectors.size()); i++)
		m_usedSectors[offset + i] = false;
}

bool RegionFile::readChunkData(int offset, int sectorCount, std::vector<uint8_t>& outData)
{
	if (offset <= 0 || sectorCount <= 0)
		return false;

	// Seek to chunk data location
	m_file.seekg(static_cast<std::streamoff>(offset) * SECTOR_SIZE, std::ios::beg);

	// Read first 4 bytes to get actual data size
	uint32_t dataSize;
	m_file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

	if (!m_file.good() || dataSize == 0 || dataSize > static_cast<uint32_t>(sectorCount * SECTOR_SIZE - 4))
		return false;

	// Read actual chunk data
	outData.resize(dataSize);
	m_file.read(reinterpret_cast<char*>(outData.data()), dataSize);

	return m_file.good();
}

bool RegionFile::writeChunkData(int offset, int sectorCount, const std::vector<uint8_t>& data)
{
	if (offset <= 0 || sectorCount <= 0 || data.empty())
		return false;

	// Seek to chunk data location
	m_file.seekp(static_cast<std::streamoff>(offset) * SECTOR_SIZE, std::ios::beg);

	// Write data size first
	uint32_t dataSize = static_cast<uint32_t>(data.size());
	m_file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

	// Write actual chunk data
	m_file.write(reinterpret_cast<const char*>(data.data()), data.size());

	// Pad to sector boundary with zeros
	size_t totalWritten = sizeof(dataSize) + data.size();
	size_t remainder = totalWritten % SECTOR_SIZE;
	if (remainder != 0)
	{
		size_t padding = SECTOR_SIZE - remainder;
		std::vector<uint8_t> zeros(padding, 0);
		m_file.write(reinterpret_cast<const char*>(zeros.data()), padding);
	}

	m_file.flush();
	return m_file.good();
}

std::unique_ptr<Chunk::Chunk> RegionFile::readChunk(int chunkX, int chunkZ)
{
	int index = RegionCoord::getLocalChunkIndex(chunkX, chunkZ);
	uint32_t entry = m_offsets[index];

	if (entry == 0)
		return nullptr;  // Chunk doesn't exist

	int offset = (entry >> 8) & 0xFFFFFF;
	int count = entry & 0xFF;

	std::vector<uint8_t> data;
	if (!readChunkData(offset, count, data))
		return nullptr;

	// Deserialize chunk data
	return ChunkSerializer::deserialize(data);
}

bool RegionFile::writeChunk(int chunkX, int chunkZ, const Chunk::Chunk& chunk)
{
	int index = RegionCoord::getLocalChunkIndex(chunkX, chunkZ);

	// Serialize chunk data
	std::vector<uint8_t> data;
	if (!ChunkSerializer::serialize(chunk, data))
		return false;

	if (data.empty())
		return false;

	// Calculate sectors needed
	int sectorsNeeded = static_cast<int>((data.size() + sizeof(uint32_t) + SECTOR_SIZE - 1) / SECTOR_SIZE);

	// Free old sectors if chunk already exists
	uint32_t oldEntry = m_offsets[index];
	if (oldEntry != 0)
	{
		int oldOffset = (oldEntry >> 8) & 0xFFFFFF;
		int oldCount = oldEntry & 0xFF;
		freeSectors(oldOffset, oldCount);
	}

	// Allocate new sectors
	int newOffset = allocateSectors(sectorsNeeded);
	if (newOffset < 0)
		return false;

	// Write chunk data
	if (!writeChunkData(newOffset, sectorsNeeded, data))
	{
		freeSectors(newOffset, sectorsNeeded);
		return false;
	}

	// Update offset table
	m_offsets[index] = (static_cast<uint32_t>(newOffset) << 8) | (sectorsNeeded & 0xFF);

	// Save offset table
	return saveOffsetTable();
}

} // namespace Persistence

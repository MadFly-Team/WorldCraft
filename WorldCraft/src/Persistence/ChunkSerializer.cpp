#include <Persistence/ChunkSerializer.h>
#include <Voxel/BlockTypes.h>
#include <zlib.h>
#include <cstring>

namespace Persistence
{

// Chunk data format:
// [Header: 16 bytes]
//   uint32: format version
//   uint32: uncompressed data size
//   uint32: compressed data size
//   uint32: CRC32 checksum (of uncompressed data)
// [Compressed Data]
//   Block data: CHUNK_VOLUME bytes (16×256×16 = 65,536 blocks)
//   Sky light: CHUNK_VOLUME/2 bytes (4 bits per block, packed)

bool ChunkSerializer::serialize(const Chunk::Chunk& chunk, std::vector<uint8_t>& outData)
{
	// Prepare uncompressed data
	constexpr size_t BLOCK_DATA_SIZE = Chunk::CHUNK_VOLUME;
	constexpr size_t LIGHT_DATA_SIZE = Chunk::CHUNK_VOLUME / 2;
	constexpr size_t UNCOMPRESSED_SIZE = BLOCK_DATA_SIZE + LIGHT_DATA_SIZE;

	std::vector<uint8_t> uncompressed;
	uncompressed.reserve(UNCOMPRESSED_SIZE);

	// Copy block data
	for (int y = 0; y < Chunk::CHUNK_SIZE_Y; y++)
	{
		for (int z = 0; z < Chunk::CHUNK_SIZE_Z; z++)
		{
			for (int x = 0; x < Chunk::CHUNK_SIZE_X; x++)
			{
				Voxel::BlockID block = chunk.getBlock(x, y, z);
				uncompressed.push_back(static_cast<uint8_t>(block));
			}
		}
	}

	// Copy sky light data
	for (int y = 0; y < Chunk::CHUNK_SIZE_Y; y++)
	{
		for (int z = 0; z < Chunk::CHUNK_SIZE_Z; z++)
		{
			for (int x = 0; x < Chunk::CHUNK_SIZE_X; x += 2)
			{
				uint8_t light1 = chunk.getSkyLight(x, y, z);
				uint8_t light2 = (x + 1 < Chunk::CHUNK_SIZE_X) ? chunk.getSkyLight(x + 1, y, z) : 0;
				uint8_t packed = (light1 & 0x0F) | ((light2 & 0x0F) << 4);
				uncompressed.push_back(packed);
			}
		}
	}

	// Calculate checksum
	uint32_t checksum = calculateCRC32(uncompressed.data(), uncompressed.size());

	// Compress data
	std::vector<uint8_t> compressed;
	if (!compress(uncompressed, compressed))
		return false;

	// Build final output with header
	outData.clear();
	outData.reserve(16 + compressed.size());

	// Write header
	uint32_t version = CHUNK_FORMAT_VERSION;
	uint32_t uncompSize = static_cast<uint32_t>(uncompressed.size());
	uint32_t compSize = static_cast<uint32_t>(compressed.size());

	outData.insert(outData.end(), reinterpret_cast<uint8_t*>(&version), reinterpret_cast<uint8_t*>(&version) + 4);
	outData.insert(outData.end(), reinterpret_cast<uint8_t*>(&uncompSize), reinterpret_cast<uint8_t*>(&uncompSize) + 4);
	outData.insert(outData.end(), reinterpret_cast<uint8_t*>(&compSize), reinterpret_cast<uint8_t*>(&compSize) + 4);
	outData.insert(outData.end(), reinterpret_cast<uint8_t*>(&checksum), reinterpret_cast<uint8_t*>(&checksum) + 4);

	// Write compressed data
	outData.insert(outData.end(), compressed.begin(), compressed.end());

	return true;
}

std::unique_ptr<Chunk::Chunk> ChunkSerializer::deserialize(const std::vector<uint8_t>& data)
{
	// Validate minimum size (header)
	if (data.size() < 16)
		return nullptr;

	// Read header
	uint32_t version, uncompSize, compSize, checksum;
	std::memcpy(&version, &data[0], 4);
	std::memcpy(&uncompSize, &data[4], 4);
	std::memcpy(&compSize, &data[8], 4);
	std::memcpy(&checksum, &data[12], 4);

	// Validate version
	if (version != CHUNK_FORMAT_VERSION)
		return nullptr;

	// Validate sizes
	if (data.size() < 16 + compSize)
		return nullptr;

	// Decompress data
	std::vector<uint8_t> uncompressed;
	std::vector<uint8_t> compressed(data.begin() + 16, data.end());

	if (!decompress(compressed, uncompSize, uncompressed))
		return nullptr;

	// Validate checksum
	uint32_t actualChecksum = calculateCRC32(uncompressed.data(), uncompressed.size());
	if (actualChecksum != checksum)
		return nullptr;

	// Create chunk and load data
	auto chunk = std::make_unique<Chunk::Chunk>();

	size_t offset = 0;

	// Load block data
	for (int y = 0; y < Chunk::CHUNK_SIZE_Y; y++)
	{
		for (int z = 0; z < Chunk::CHUNK_SIZE_Z; z++)
		{
			for (int x = 0; x < Chunk::CHUNK_SIZE_X; x++)
			{
				if (offset >= uncompressed.size())
					return nullptr;

				Voxel::BlockID block = static_cast<Voxel::BlockID>(uncompressed[offset++]);
				chunk->setBlock(x, y, z, block);
			}
		}
	}

	// Load sky light data
	for (int y = 0; y < Chunk::CHUNK_SIZE_Y; y++)
	{
		for (int z = 0; z < Chunk::CHUNK_SIZE_Z; z++)
		{
			for (int x = 0; x < Chunk::CHUNK_SIZE_X; x += 2)
			{
				if (offset >= uncompressed.size())
					return nullptr;

				uint8_t packed = uncompressed[offset++];
				uint8_t light1 = packed & 0x0F;
				uint8_t light2 = (packed >> 4) & 0x0F;

				chunk->setSkyLight(x, y, z, light1);
				if (x + 1 < Chunk::CHUNK_SIZE_X)
					chunk->setSkyLight(x + 1, y, z, light2);
			}
		}
	}

	return chunk;
}

bool ChunkSerializer::compress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output)
{
	// Allocate output buffer (worst case: input size + 0.1% + 12 bytes)
	uLongf compressedSize = compressBound(static_cast<uLong>(input.size()));
	output.resize(compressedSize);

	// Compress using zlib
	int result = ::compress2(
		output.data(),
		&compressedSize,
		input.data(),
		static_cast<uLong>(input.size()),
		Z_DEFAULT_COMPRESSION
	);

	if (result != Z_OK)
		return false;

	// Resize to actual compressed size
	output.resize(compressedSize);
	return true;
}

bool ChunkSerializer::decompress(const std::vector<uint8_t>& input, uint32_t uncompressedSize, std::vector<uint8_t>& output)
{
	output.resize(uncompressedSize);

	uLongf destLen = uncompressedSize;
	int result = ::uncompress(
		output.data(),
		&destLen,
		input.data(),
		static_cast<uLong>(input.size())
	);

	if (result != Z_OK || destLen != uncompressedSize)
		return false;

	return true;
}

uint32_t ChunkSerializer::calculateCRC32(const uint8_t* data, size_t length)
{
	// Use zlib's CRC32 implementation
	return static_cast<uint32_t>(crc32(0L, data, static_cast<uInt>(length)));
}

} // namespace Persistence

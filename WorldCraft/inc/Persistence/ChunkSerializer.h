#pragma once

#include <Chunk/Chunk.h>
#include <vector>
#include <cstdint>
#include <memory>

namespace Persistence
{

	// Chunk serialization format version
	constexpr uint32_t CHUNK_FORMAT_VERSION = 1;

	// ChunkSerializer handles conversion between Chunk objects and binary data
	class ChunkSerializer
	{
	public:
		// Serialize a chunk to binary format (with compression)
		// Returns true on success, fills outData with compressed chunk data
		static bool serialize(const Chunk::Chunk& chunk, std::vector<uint8_t>& outData);

		// Deserialize a chunk from binary format (with decompression)
		// Returns nullptr on failure
		static std::unique_ptr<Chunk::Chunk> deserialize(const std::vector<uint8_t>& data);

	private:
		// Compress data using zlib
		static bool compress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);

		// Decompress data using zlib
		static bool decompress(const std::vector<uint8_t>& input, uint32_t uncompressedSize, std::vector<uint8_t>& output);

		// Calculate CRC32 checksum for data validation
		static uint32_t calculateCRC32(const uint8_t* data, size_t length);
	};

} // namespace Persistence

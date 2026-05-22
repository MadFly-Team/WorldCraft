#pragma once

#include <Voxel/BlockTypes.h>
#include <array>

namespace Chunk
{

	// Chunk dimensions (voxels per axis).
	// X/Z = 16 matches Minecraft; Y = 256 gives extended world height.
	inline constexpr int CHUNK_SIZE_X = 16;
	inline constexpr int CHUNK_SIZE_Y = 256;
	inline constexpr int CHUNK_SIZE_Z = 16;
	inline constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

	// A 16x256x16 grid of block IDs.
	// The mesh is regenerated whenever isDirty is true.
	class Chunk
	{
	public:
		Chunk();

		// Read the block at local coordinates — returns Air for out-of-bounds.
		Voxel::BlockID getBlock(int x, int y, int z) const;

		// Write a block at local coordinates (silently ignores out-of-bounds).
		// Marks the chunk dirty on success.
		void setBlock(int x, int y, int z, Voxel::BlockID id);

		// Bounds check.
		static bool inBounds(int x, int y, int z)
		{
			return x >= 0 && x < CHUNK_SIZE_X
				&& y >= 0 && y < CHUNK_SIZE_Y
				&& z >= 0 && z < CHUNK_SIZE_Z;
		}

		bool isDirty() const { return m_dirty; }
		void clearDirty()    { m_dirty = false; }

		// Fill every voxel with the given ID (useful for testing / terrain gen).
		void fill(Voxel::BlockID id);

	private:
		// Convert 3-D local coordinates to a flat array index.
		static int index(int x, int y, int z);

		std::array<Voxel::BlockID, CHUNK_VOLUME> m_blocks;
		bool m_dirty = true;
	};

} // namespace Chunk

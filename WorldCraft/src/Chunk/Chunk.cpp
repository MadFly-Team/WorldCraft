#include <Chunk/Chunk.h>

namespace Chunk
{

Chunk::Chunk()
{
	m_blocks.fill(Voxel::BlockID::Air);
}

int Chunk::index(int x, int y, int z)
{
	return x + CHUNK_SIZE_X * (y + CHUNK_SIZE_Y * z);
}

Voxel::BlockID Chunk::getBlock(int x, int y, int z) const
{
	if (!inBounds(x, y, z)) return Voxel::BlockID::Air;
	return m_blocks[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, Voxel::BlockID id)
{
	if (!inBounds(x, y, z)) return;
	m_blocks[index(x, y, z)] = id;
	m_dirty = true;
}

void Chunk::fill(Voxel::BlockID id)
{
	m_blocks.fill(id);
	m_dirty = true;
}

} // namespace Chunk

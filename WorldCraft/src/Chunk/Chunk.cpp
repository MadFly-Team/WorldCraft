#include <Chunk/Chunk.h>

namespace Chunk
{

Chunk::Chunk()
{
	m_blocks.fill(Voxel::BlockID::Air);
	m_skyLight.fill(0);  // Initialize all light to 0
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

uint8_t Chunk::getSkyLight(int x, int y, int z) const
{
	if (!inBounds(x, y, z)) return 0;
	int idx = index(x, y, z);
	int byteIdx = idx / 2;
	// Even indices use lower 4 bits, odd indices use upper 4 bits
	if (idx % 2 == 0)
		return m_skyLight[byteIdx] & 0x0F;
	else
		return (m_skyLight[byteIdx] >> 4) & 0x0F;
}

void Chunk::setSkyLight(int x, int y, int z, uint8_t level)
{
	if (!inBounds(x, y, z)) return;
	level = level > 15 ? 15 : level;  // Clamp to 0-15
	int idx = index(x, y, z);
	int byteIdx = idx / 2;
	if (idx % 2 == 0)
	{
		// Lower 4 bits
		m_skyLight[byteIdx] = (m_skyLight[byteIdx] & 0xF0) | level;
	}
	else
	{
		// Upper 4 bits
		m_skyLight[byteIdx] = (m_skyLight[byteIdx] & 0x0F) | (level << 4);
	}
	m_dirty = true;
}

} // namespace Chunk

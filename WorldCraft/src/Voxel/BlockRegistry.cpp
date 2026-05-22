#include <Voxel/BlockRegistry.h>
#include <Texture/BlockTextures.h>

namespace Voxel
{

// Convenience: fill all six face slots of an entry with the same layer index.
static void setAllFaces(BlockProperties& p, int layer)
{
	for (int f = 0; f < static_cast<int>(FaceDir::COUNT); ++f)
		p.texLayer[f] = layer;
}

// Convenience: set top (PosY), bottom (NegY), and four side faces separately.
static void setFaces(BlockProperties& p, int top, int bottom, int side)
{
	p.texLayer[static_cast<int>(FaceDir::PosX)] = side;
	p.texLayer[static_cast<int>(FaceDir::NegX)] = side;
	p.texLayer[static_cast<int>(FaceDir::PosY)] = top;
	p.texLayer[static_cast<int>(FaceDir::NegY)] = bottom;
	p.texLayer[static_cast<int>(FaceDir::PosZ)] = side;
	p.texLayer[static_cast<int>(FaceDir::NegZ)] = side;
}

BlockRegistry::BlockRegistry()
{
	using T = Texture::TextureArray; // shorthand for layer constants

	// Air — no texture needed (never rendered)
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Air)];
		p.isSolid = false; p.isTransparent = false; p.name = "Air";
		setAllFaces(p, 0);
	}

	// Stone — same texture on every face
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Stone)];
		p.isSolid = true; p.isTransparent = false; p.name = "Stone";
		setAllFaces(p, Texture::LAYER_STONE);
	}

	// Dirt — same texture on every face
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Dirt)];
		p.isSolid = true; p.isTransparent = false; p.name = "Dirt";
		setAllFaces(p, Texture::LAYER_DIRT);
	}

	// Grass — top: grass, bottom: dirt, sides: grass-side
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Grass)];
		p.isSolid = true; p.isTransparent = false; p.name = "Grass";
		setFaces(p, Texture::LAYER_GRASS_TOP, Texture::LAYER_DIRT, Texture::LAYER_GRASS_SIDE);
	}

	// Sand — same texture on every face
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Sand)];
		p.isSolid = true; p.isTransparent = false; p.name = "Sand";
		setAllFaces(p, Texture::LAYER_SAND);
	}

	// Wood — top: ring pattern, bottom: ring pattern, sides: vertical grain
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Wood)];
		p.isSolid = true; p.isTransparent = false; p.name = "Wood";
		setFaces(p, Texture::LAYER_WOOD_TOP, Texture::LAYER_WOOD_TOP, Texture::LAYER_WOOD_SIDE);
	}

	// Leaf — same semi-transparent texture on every face
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Leaf)];
		p.isSolid = true; p.isTransparent = true; p.name = "Leaf";
		setAllFaces(p, Texture::LAYER_LEAF);
	}

	// Bedrock — uniform dark texture on all faces
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Bedrock)];
		p.isSolid = true; p.isTransparent = false; p.name = "Bedrock";
		setAllFaces(p, Texture::LAYER_BEDROCK);
	}

	// Gravel — same coarse pebble texture all faces
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Gravel)];
		p.isSolid = true; p.isTransparent = false; p.name = "Gravel";
		setAllFaces(p, Texture::LAYER_GRAVEL);
	}

	// SnowBlock — snow top, snow sides; we reuse snow for all faces
	{
		auto& p = m_table[static_cast<size_t>(BlockID::SnowBlock)];
		p.isSolid = true; p.isTransparent = false; p.name = "Snow Block";
		setFaces(p, Texture::LAYER_SNOW, Texture::LAYER_DIRT, Texture::LAYER_SNOW);
	}

	// Water — semi-transparent blue on all faces
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Water)];
		p.isSolid = false; p.isTransparent = true; p.name = "Water";
		setAllFaces(p, Texture::LAYER_WATER);
	}

	// Coal Ore
	{
		auto& p = m_table[static_cast<size_t>(BlockID::CoalOre)];
		p.isSolid = true; p.isTransparent = false; p.name = "Coal Ore";
		setAllFaces(p, Texture::LAYER_COAL_ORE);
	}

	// Iron Ore
	{
		auto& p = m_table[static_cast<size_t>(BlockID::IronOre)];
		p.isSolid = true; p.isTransparent = false; p.name = "Iron Ore";
		setAllFaces(p, Texture::LAYER_IRON_ORE);
	}

	// Gold Ore
	{
		auto& p = m_table[static_cast<size_t>(BlockID::GoldOre)];
		p.isSolid = true; p.isTransparent = false; p.name = "Gold Ore";
		setAllFaces(p, Texture::LAYER_GOLD_ORE);
	}

	// Diamond Ore
	{
		auto& p = m_table[static_cast<size_t>(BlockID::DiamondOre)];
		p.isSolid = true; p.isTransparent = false; p.name = "Diamond Ore";
		setAllFaces(p, Texture::LAYER_DIAMOND_ORE);
	}

	// Redstone Ore
	{
		auto& p = m_table[static_cast<size_t>(BlockID::RedstoneOre)];
		p.isSolid = true; p.isTransparent = false; p.name = "Redstone Ore";
		setAllFaces(p, Texture::LAYER_REDSTONE_ORE);
	}

	// Lapis Ore
	{
		auto& p = m_table[static_cast<size_t>(BlockID::LapisOre)];
		p.isSolid = true; p.isTransparent = false; p.name = "Lapis Ore";
		setAllFaces(p, Texture::LAYER_LAPIS_ORE);
	}

	// Emerald Ore
	{
		auto& p = m_table[static_cast<size_t>(BlockID::EmeraldOre)];
		p.isSolid = true; p.isTransparent = false; p.name = "Emerald Ore";
		setAllFaces(p, Texture::LAYER_EMERALD_ORE);
	}

	// Obsidian — dark purple-black uniform texture
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Obsidian)];
		p.isSolid = true; p.isTransparent = false; p.name = "Obsidian";
		setAllFaces(p, Texture::LAYER_OBSIDIAN);
	}

	// Grass shade variants 1-8 (top = grass variant, bottom = dirt, sides = grass-side variant)
	const int grassTopLayers[8] = {
		Texture::LAYER_GRASS_TOP_1, Texture::LAYER_GRASS_TOP_2, Texture::LAYER_GRASS_TOP_3, Texture::LAYER_GRASS_TOP_4,
		Texture::LAYER_GRASS_TOP_5, Texture::LAYER_GRASS_TOP_6, Texture::LAYER_GRASS_TOP_7, Texture::LAYER_GRASS_TOP_8
	};
	const int grassSideLayers[8] = {
		Texture::LAYER_GRASS_SIDE_1, Texture::LAYER_GRASS_SIDE_2, Texture::LAYER_GRASS_SIDE_3, Texture::LAYER_GRASS_SIDE_4,
		Texture::LAYER_GRASS_SIDE_5, Texture::LAYER_GRASS_SIDE_6, Texture::LAYER_GRASS_SIDE_7, Texture::LAYER_GRASS_SIDE_8
	};
	for (int i = 0; i < 8; ++i)
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Grass1) + i];
		p.isSolid = true; p.isTransparent = false; p.name = "Grass";
		setFaces(p, grassTopLayers[i], Texture::LAYER_DIRT, grassSideLayers[i]);
	}

	// Rock surface variants 1-8 (uniform per-variant texture on all faces)
	const int rockLayers[8] = {
		Texture::LAYER_ROCK_1, Texture::LAYER_ROCK_2, Texture::LAYER_ROCK_3, Texture::LAYER_ROCK_4,
		Texture::LAYER_ROCK_5, Texture::LAYER_ROCK_6, Texture::LAYER_ROCK_7, Texture::LAYER_ROCK_8
	};
	for (int i = 0; i < 8; ++i)
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Rock1) + i];
		p.isSolid = true; p.isTransparent = false; p.name = "Rock";
		setAllFaces(p, rockLayers[i]);
	}

	// Mushroom - solid decorative block
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Mushroom)];
		p.isSolid = true; p.isTransparent = false; p.name = "Mushroom";
		setAllFaces(p, Texture::LAYER_MUSHROOM);
	}

	// Torch - light source block (non-solid, transparent for placement)
	{
		auto& p = m_table[static_cast<size_t>(BlockID::Torch)];
		p.isSolid = false; p.isTransparent = true; p.name = "Torch";
		setAllFaces(p, Texture::LAYER_TORCH);
	}
}

const BlockRegistry& BlockRegistry::get()
{
	static BlockRegistry instance;
	return instance;
}

const BlockProperties& BlockRegistry::propertiesOf(BlockID id) const
{
	const size_t idx = static_cast<size_t>(id);
	if (idx < static_cast<size_t>(BlockID::COUNT))
		return m_table[idx];
	return m_table[0]; // fallback to Air
}

bool BlockRegistry::isSolid(BlockID id) const
{
	return propertiesOf(id).isSolid;
}

int BlockRegistry::texLayer(BlockID id, FaceDir face) const
{
	return propertiesOf(id).texLayer[static_cast<size_t>(face)];
}

} // namespace Voxel

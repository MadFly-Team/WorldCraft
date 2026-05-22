#pragma once

#include <cstdint>

namespace Voxel
{

// Unique identifier for every block type.
// Add new types before COUNT.
enum class BlockID : uint16_t
{
Air        = 0,
Stone      = 1,
Dirt       = 2,
Grass      = 3,
Sand       = 4,
Wood       = 5,  // Oak log
Leaf       = 6,  // Oak leaf

// --- Terrain & biome blocks ---
Bedrock    = 7,
Gravel     = 8,
SnowBlock  = 9,
Water      = 10,

// --- Ore blocks (stone base + coloured veins) ---
CoalOre    = 11,
IronOre    = 12,
GoldOre    = 13,
DiamondOre = 14,
RedstoneOre= 15,
LapisOre   = 16,
EmeraldOre = 17,

// --- Special ---
Obsidian   = 18,

// --- Grass shade variants (1 = base, 2-8 = progressively varied tones) ---
Grass1     = 19,
Grass2     = 20,
Grass3     = 21,
Grass4     = 22,
Grass5     = 23,
Grass6     = 24,
Grass7     = 25,
Grass8     = 26,

// --- Rock / stone surface variants ---
Rock1      = 27,
Rock2      = 28,
Rock3      = 29,
Rock4      = 30,
Rock5      = 31,
Rock6      = 32,
Rock7      = 33,
Rock8      = 34,

// --- Surface decorations (objects) ---
Mushroom   = 35,

COUNT
};

// The six axis-aligned faces of a cube block.
enum class FaceDir : uint8_t
{
PosX = 0, // Right
NegX = 1, // Left
PosY = 2, // Top
NegY = 3, // Bottom
PosZ = 4, // Front
NegZ = 5, // Back

COUNT
};

// World-space offsets for each FaceDir neighbour (indexed by FaceDir).
inline constexpr int FaceOffsetX[6] = {  1, -1,  0,  0,  0,  0 };
inline constexpr int FaceOffsetY[6] = {  0,  0,  1, -1,  0,  0 };
inline constexpr int FaceOffsetZ[6] = {  0,  0,  0,  0,  1, -1 };

} // namespace Voxel
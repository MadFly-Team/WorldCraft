#pragma once

#include <WorldCraft.h>
#include <cstdint>

namespace Texture
{

// ---------------------------------------------------------------------------
// Texture layer indices inside the GL_TEXTURE_2D_ARRAY.
// Each layer is a 16x16 RGBA image generated procedurally at startup.
// ---------------------------------------------------------------------------

// --- Original layers ---
inline constexpr int LAYER_STONE       = 0;
inline constexpr int LAYER_DIRT        = 1;
inline constexpr int LAYER_GRASS_TOP   = 2;
inline constexpr int LAYER_GRASS_SIDE  = 3;
inline constexpr int LAYER_SAND        = 4;
inline constexpr int LAYER_WOOD_TOP    = 5;
inline constexpr int LAYER_WOOD_SIDE   = 6;
inline constexpr int LAYER_LEAF        = 7;

// --- Terrain & biome ---
inline constexpr int LAYER_BEDROCK     = 8;
inline constexpr int LAYER_GRAVEL      = 9;
inline constexpr int LAYER_SNOW        = 10;
inline constexpr int LAYER_WATER       = 11;

// --- Ores ---
inline constexpr int LAYER_COAL_ORE    = 12;
inline constexpr int LAYER_IRON_ORE    = 13;
inline constexpr int LAYER_GOLD_ORE    = 14;
inline constexpr int LAYER_DIAMOND_ORE = 15;
inline constexpr int LAYER_REDSTONE_ORE= 16;
inline constexpr int LAYER_LAPIS_ORE   = 17;
inline constexpr int LAYER_EMERALD_ORE = 18;

// --- Special ---
inline constexpr int LAYER_OBSIDIAN    = 19;

// --- Grass shade variants (top layers) ---
inline constexpr int LAYER_GRASS_TOP_1 = 20;
inline constexpr int LAYER_GRASS_TOP_2 = 21;
inline constexpr int LAYER_GRASS_TOP_3 = 22;
inline constexpr int LAYER_GRASS_TOP_4 = 23;
inline constexpr int LAYER_GRASS_TOP_5 = 24;
inline constexpr int LAYER_GRASS_TOP_6 = 25;
inline constexpr int LAYER_GRASS_TOP_7 = 26;
inline constexpr int LAYER_GRASS_TOP_8 = 27;

// --- Grass shade variants (side layers) ---
inline constexpr int LAYER_GRASS_SIDE_1 = 28;
inline constexpr int LAYER_GRASS_SIDE_2 = 29;
inline constexpr int LAYER_GRASS_SIDE_3 = 30;
inline constexpr int LAYER_GRASS_SIDE_4 = 31;
inline constexpr int LAYER_GRASS_SIDE_5 = 32;
inline constexpr int LAYER_GRASS_SIDE_6 = 33;
inline constexpr int LAYER_GRASS_SIDE_7 = 34;
inline constexpr int LAYER_GRASS_SIDE_8 = 35;

// --- Rock / stone surface variants ---
inline constexpr int LAYER_ROCK_1      = 36;
inline constexpr int LAYER_ROCK_2      = 37;
inline constexpr int LAYER_ROCK_3      = 38;
inline constexpr int LAYER_ROCK_4      = 39;
inline constexpr int LAYER_ROCK_5      = 40;
inline constexpr int LAYER_ROCK_6      = 41;
inline constexpr int LAYER_ROCK_7      = 42;
inline constexpr int LAYER_ROCK_8      = 43;

// --- Surface decorations ---
inline constexpr int LAYER_MUSHROOM    = 44;

inline constexpr int TOTAL_LAYERS      = 45;

// Resolution of every texture layer (matches Minecraft classic).
inline constexpr int TEX_SIZE = 16;

// ---------------------------------------------------------------------------
// TextureArray — owns one GL_TEXTURE_2D_ARRAY object containing every block
// face texture generated procedurally.
// ---------------------------------------------------------------------------
class TextureArray
{
public:
TextureArray()  = default;
~TextureArray() = default;

// Generate all layers and upload to the GPU.
// Must be called after a valid OpenGL context exists.
void build();

// Bind to the given texture unit (default unit 0).
void bind(GLuint unit = 0) const;

// Free the GPU texture.
void destroy();

GLuint handle() const { return m_handle; }

// One generator per layer — each fills 'pixels' with TEX_SIZE*TEX_SIZE RGBA values.
// Public so free-function helpers in the .cpp can use the typedef.
using PixelBuf = uint8_t[TEX_SIZE * TEX_SIZE * 4];

// Shared pixel-write helper callable from free functions in the .cpp.
static void setPixelStatic(PixelBuf buf, int x, int y,
   uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
int i = (y * TEX_SIZE + x) * 4;
buf[i + 0] = r; buf[i + 1] = g;
buf[i + 2] = b; buf[i + 3] = a;
}

static void genStone    (PixelBuf pixels);
static void genDirt     (PixelBuf pixels);
static void genGrassTop (PixelBuf pixels);
static void genGrassSide(PixelBuf pixels);
static void genSand     (PixelBuf pixels);
static void genWoodTop      (PixelBuf pixels);
static void genWoodSide     (PixelBuf pixels);
static void genLeaf         (PixelBuf pixels);

// --- Terrain & biome ---
static void genBedrock      (PixelBuf pixels);
static void genGravel       (PixelBuf pixels);
static void genSnow         (PixelBuf pixels);
static void genWater        (PixelBuf pixels);

// --- Ores ---
static void genCoalOre      (PixelBuf pixels);
static void genIronOre      (PixelBuf pixels);
static void genGoldOre      (PixelBuf pixels);
static void genDiamondOre   (PixelBuf pixels);
static void genRedstoneOre  (PixelBuf pixels);
static void genLapisOre     (PixelBuf pixels);
static void genEmeraldOre   (PixelBuf pixels);

// --- Special ---
static void genObsidian     (PixelBuf pixels);

// --- Grass shade variants ---
static void genGrassTop1(PixelBuf pixels);
static void genGrassTop2(PixelBuf pixels);
static void genGrassTop3(PixelBuf pixels);
static void genGrassTop4(PixelBuf pixels);
static void genGrassTop5(PixelBuf pixels);
static void genGrassTop6(PixelBuf pixels);
static void genGrassTop7(PixelBuf pixels);
static void genGrassTop8(PixelBuf pixels);

static void genGrassSide1(PixelBuf pixels);
static void genGrassSide2(PixelBuf pixels);
static void genGrassSide3(PixelBuf pixels);
static void genGrassSide4(PixelBuf pixels);
static void genGrassSide5(PixelBuf pixels);
static void genGrassSide6(PixelBuf pixels);
static void genGrassSide7(PixelBuf pixels);
static void genGrassSide8(PixelBuf pixels);

// --- Rock variants ---
static void genRock1(PixelBuf pixels);
static void genRock2(PixelBuf pixels);
static void genRock3(PixelBuf pixels);
static void genRock4(PixelBuf pixels);
static void genRock5(PixelBuf pixels);
static void genRock6(PixelBuf pixels);
static void genRock7(PixelBuf pixels);
static void genRock8(PixelBuf pixels);

// --- Surface decorations ---
static void genMushroom(PixelBuf pixels);

private:
GLuint m_handle = 0;
};

} // namespace Texture
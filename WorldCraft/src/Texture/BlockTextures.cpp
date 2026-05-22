#include <Texture/BlockTextures.h>

// stb_perlin — torus-domain noise gives perfectly seamless tiling.
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#include <algorithm>
#include <cmath>

namespace Texture
{

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace
{

// Clamp a float to [0,255] and cast to uint8_t.
inline uint8_t clamp8(float v)
{
	return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
}

// Write one RGBA pixel into the flat buffer at (x, y).
inline void setPixel(uint8_t* buf, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	int i = (y * TEX_SIZE + x) * 4;
	buf[i + 0] = r;
	buf[i + 1] = g;
	buf[i + 2] = b;
	buf[i + 3] = a;
}

// Sample torus-domain Perlin noise so the texture tiles seamlessly.
// (x,y) are pixel coords 0..TEX_SIZE-1; freq controls the noise scale.
// Returns a value in roughly [-1, 1].
inline float torusNoise(int x, int y, float freq, float seed = 0.0f)
{
	// Map pixel coords to [0, 2*pi] then project onto a 4-D torus so all
	// four edges match — the standard seamless-tile trick.
	const float TWO_PI = 6.283185307f;
	const float u = static_cast<float>(x) / static_cast<float>(TEX_SIZE);
	const float v = static_cast<float>(y) / static_cast<float>(TEX_SIZE);

	const float nx = std::cos(u * TWO_PI) * freq;
	const float ny = std::sin(u * TWO_PI) * freq;
	const float nz = std::cos(v * TWO_PI) * freq + seed;
	const float nw = std::sin(v * TWO_PI) * freq;

	return stb_perlin_noise3(nx, ny + seed, nz + nw, 0, 0, 0);
}

// Convenience: map torusNoise result to [0,1].
inline float torusNoise01(int x, int y, float freq, float seed = 0.0f)
{
	return (torusNoise(x, y, freq, seed) + 1.0f) * 0.5f;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Generators
// ---------------------------------------------------------------------------

// Stone — cool grey with small high-frequency dark speckles.
void TextureArray::genStone(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 3.0f, 0.0f);
		float n2 = torusNoise01(x, y, 8.0f, 1.7f);

		// Base grey shifted by low-freq noise, darkened by high-freq speckles.
		float lum = 120.0f + n * 30.0f - n2 * 25.0f;
		uint8_t c = clamp8(lum);
		setPixel(pixels, x, y, c, c, c);
	}
}

// Dirt — warm mid-brown with patches and bright grain flecks.
void TextureArray::genDirt(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float patch = torusNoise01(x, y, 2.5f, 5.1f);
		float fleck = torusNoise01(x, y, 9.0f, 3.3f);

		float r = 120.0f + patch * 35.0f - fleck * 20.0f;
		float g =  82.0f + patch * 20.0f - fleck * 12.0f;
		float b =  45.0f + patch * 10.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Grass top — bright green with subtle high-freq variation.
void TextureArray::genGrassTop(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 4.0f, 11.0f);
		float n2 = torusNoise01(x, y, 9.0f,  2.2f);

		float r =  80.0f - n2 * 20.0f;
		float g = 140.0f + n  * 30.0f - n2 * 25.0f;
		float b =  50.0f - n  * 15.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Grass side — dirt body with a 2-pixel green strip at the top.
void TextureArray::genGrassSide(PixelBuf pixels)
{
	// First fill the whole face with the dirt pattern.
	genDirt(pixels);

	// Overwrite the bottom 2 rows with a green grass strip.
	// The grass band should be at Y=0,1 (bottom of texture) to appear at the top of the block.
	for (int y = 0; y < 2; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n = torusNoise01(x, y, 5.0f, 11.0f);
		float r =  75.0f - n * 10.0f;
		float g = 135.0f + n * 25.0f;
		float b =  45.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Sand — pale warm yellow with very low-frequency ripple noise.
void TextureArray::genSand(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 2.0f, 7.5f);
		float n2 = torusNoise01(x, y, 6.0f, 9.1f);

		float r = 215.0f + n  * 20.0f - n2 * 15.0f;
		float g = 195.0f + n  * 15.0f - n2 * 12.0f;
		float b = 135.0f + n  *  8.0f - n2 * 10.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Wood top — concentric ring pattern centred on the face, plus subtle grain.
void TextureArray::genWoodTop(PixelBuf pixels)
{
	const float cx = TEX_SIZE * 0.5f - 0.5f;
	const float cy = TEX_SIZE * 0.5f - 0.5f;

	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float dx   = static_cast<float>(x) - cx;
		float dy   = static_cast<float>(y) - cy;
		float dist = std::sqrt(dx * dx + dy * dy);

		// Ring pattern: bright peaks at even integers of dist.
		float ring = std::cos(dist * 1.8f) * 0.5f + 0.5f;
		float grain = torusNoise01(x, y, 6.0f, 14.0f) * 0.2f;

		float r = 110.0f + ring * 45.0f + grain * 20.0f;
		float g =  75.0f + ring * 30.0f + grain * 12.0f;
		float b =  40.0f + ring * 15.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Wood side — vertical brown grain lines, slightly varying in lightness.
void TextureArray::genWoodSide(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		// Grain varies only along X so vertical lines are continuous.
		float grain = torusNoise01(x, 0, 5.0f, 14.0f);
		// Add subtle vertical variation for depth.
		float var   = torusNoise01(x, y, 8.0f, 21.0f) * 0.3f;

		float r = 100.0f + grain * 55.0f + var * 15.0f;
		float g =  68.0f + grain * 35.0f + var * 10.0f;
		float b =  36.0f + grain * 18.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Leaf — dithered greens with random darker holes (alpha < 255 on sparse pixels).
void TextureArray::genLeaf(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 4.5f, 33.0f);
		float n2 = torusNoise01(x, y, 9.0f, 17.5f);

		// Sparse: some pixels are near-transparent gaps between leaves.
		uint8_t alpha = (n2 > 0.72f) ? 0 : 255;

		float r =  40.0f + n * 20.0f;
		float g = 105.0f + n * 45.0f - n2 * 30.0f;
		float b =  30.0f + n * 10.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b), alpha);
	}
}

// Bedrock — very dark grey with dense irregular fracturing.
void TextureArray::genBedrock(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 5.0f, 42.0f);
		float n2 = torusNoise01(x, y, 11.0f, 3.3f);
		float lum = 30.0f + n * 25.0f - n2 * 15.0f;
		uint8_t c = clamp8(lum);
		setPixel(pixels, x, y, c, c, c);
	}
}

// Gravel — light grey-brown, coarse pebble pattern.
void TextureArray::genGravel(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 6.0f, 55.0f);
		float n2 = torusNoise01(x, y, 12.0f, 8.8f);
		float lum = 130.0f + n * 30.0f - n2 * 20.0f;
		float r = lum;
		float g = lum - 8.0f;
		float b = lum - 15.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Snow — bright white with very subtle blue-grey variation.
void TextureArray::genSnow(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n = torusNoise01(x, y, 4.0f, 66.0f);
		float r = 235.0f + n * 20.0f;
		float g = 240.0f + n * 15.0f;
		float b = 250.0f + n *  5.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}

// Water — translucent animated-looking blue with wave ripples.
void TextureArray::genWater(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float wave = torusNoise01(x, y, 3.0f, 77.0f);
		float foam = torusNoise01(x, y, 8.0f, 13.1f);
		float r =  40.0f + wave * 20.0f;
		float g = 100.0f + wave * 40.0f + foam * 20.0f;
		float b = 200.0f + wave * 30.0f + foam * 15.0f;
		uint8_t a = 180;  // semi-transparent
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b), a);
	}
}

// Helper: stone base + coloured vein specks for ore blocks.
static void genOreLayer(TextureArray::PixelBuf pixels,
						float vR, float vG, float vB,
						float vFreq, float vSeed)
{
	// First fill with stone.
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 3.0f, 0.0f);
		float n2 = torusNoise01(x, y, 8.0f, 1.7f);
		float lum = 120.0f + n * 30.0f - n2 * 25.0f;
		uint8_t c = clamp8(lum);
		TextureArray::setPixelStatic(pixels, x, y, c, c, c);
	}
	// Overlay ore specks where high-freq noise exceeds threshold.
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float ore = torusNoise01(x, y, vFreq, vSeed);
		if (ore > 0.68f)
		{
			float bright = (ore - 0.68f) / 0.32f;
			uint8_t r = clamp8(vR * bright + (1.0f - bright) * 120.0f);
			uint8_t g = clamp8(vG * bright + (1.0f - bright) * 120.0f);
			uint8_t b = clamp8(vB * bright + (1.0f - bright) * 120.0f);
			TextureArray::setPixelStatic(pixels, x, y, r, g, b);
		}
	}
}

void TextureArray::genCoalOre(PixelBuf pixels)    { genOreLayer(pixels,  30,  30,  30, 10.0f, 21.0f); }
void TextureArray::genIronOre(PixelBuf pixels)    { genOreLayer(pixels, 205, 160, 130, 10.0f, 37.0f); }
void TextureArray::genGoldOre(PixelBuf pixels)    { genOreLayer(pixels, 255, 220,  30, 10.0f, 53.0f); }
void TextureArray::genDiamondOre(PixelBuf pixels) { genOreLayer(pixels,  50, 230, 230, 10.0f, 69.0f); }
void TextureArray::genRedstoneOre(PixelBuf pixels){ genOreLayer(pixels, 240,  30,  30, 11.0f, 85.0f); }
void TextureArray::genLapisOre(PixelBuf pixels)   { genOreLayer(pixels,  20,  70, 200, 10.0f,101.0f); }
void TextureArray::genEmeraldOre(PixelBuf pixels) { genOreLayer(pixels,  30, 220,  80, 12.0f,117.0f); }

// Obsidian — very dark purple-black with faint iridescent shimmer.
void TextureArray::genObsidian(PixelBuf pixels)
{
	for (int y = 0; y < TEX_SIZE; ++y)
	for (int x = 0; x < TEX_SIZE; ++x)
	{
		float n  = torusNoise01(x, y, 5.0f, 133.0f);
		float n2 = torusNoise01(x, y, 11.0f, 7.7f);
		float r = 15.0f + n * 15.0f;
		float g =  8.0f + n *  8.0f + n2 * 5.0f;
		float b = 22.0f + n * 20.0f + n2 * 10.0f;
		setPixel(pixels, x, y, clamp8(r), clamp8(g), clamp8(b));
	}
}


// ---------------------------------------------------------------------------
// Grass shade variants — 8 different tinted grass-top textures
// ---------------------------------------------------------------------------
namespace {

// Generate a grass-top tile with a given base hue offset applied to G channel.
static void genGrassTopVariant(uint8_t* pixels, float greenShift, float seed)
{
for (int y = 0; y < TEX_SIZE; ++y)
for (int x = 0; x < TEX_SIZE; ++x)
{
float n1 = torusNoise01(x, y, 3.0f, seed);
float n2 = torusNoise01(x, y, 8.0f, seed + 17.0f);
float r = clamp8(60.0f  + n1 * 20.0f);
float g = clamp8(greenShift + n1 * 30.0f + n2 * 15.0f);
float b = clamp8(20.0f  + n1 * 10.0f);
int i = (y * TEX_SIZE + x) * 4;
pixels[i+0] = r; pixels[i+1] = (uint8_t)g; pixels[i+2] = (uint8_t)b; pixels[i+3] = 255;
}
}

// Generate a grass-side tile matching the given top shade.
static void genGrassSideVariant(uint8_t* pixels, float greenShift, float seed)
{
for (int y = 0; y < TEX_SIZE; ++y)
for (int x = 0; x < TEX_SIZE; ++x)
{
	float n = torusNoise01(x, y, 4.0f, seed + 5.0f);
	uint8_t r, g, b;
	if (y < 2)
	{
		// Bottom strip (Y=0,1) — grass colour (appears at top of block).
		r = clamp8(55.0f  + n * 20.0f);
		g = clamp8(greenShift + n * 25.0f);
		b = clamp8(18.0f  + n * 10.0f);
	}
	else
	{
		// Body — dirt.
		r = clamp8(120.0f + n * 30.0f);
		g = clamp8( 80.0f + n * 20.0f);
		b = clamp8( 45.0f + n * 15.0f);
	}
	int i = (y * TEX_SIZE + x) * 4;
	pixels[i+0] = r; pixels[i+1] = g; pixels[i+2] = b; pixels[i+3] = 255;
}
}

// Generate a rock variant with a given grey bias and seed.
static void genRockVariant(uint8_t* pixels, float baseBright, float rTint, float gTint, float bTint, float seed)
{
for (int y = 0; y < TEX_SIZE; ++y)
for (int x = 0; x < TEX_SIZE; ++x)
{
float n1 = torusNoise01(x, y, 4.0f, seed);
float n2 = torusNoise01(x, y, 9.0f, seed + 31.0f);
float base = baseBright + n1 * 35.0f + n2 * 15.0f;
float r = clamp8(base + rTint);
float g = clamp8(base + gTint);
float b = clamp8(base + bTint);
int i = (y * TEX_SIZE + x) * 4;
pixels[i+0] = (uint8_t)r; pixels[i+1] = (uint8_t)g; pixels[i+2] = (uint8_t)b; pixels[i+3] = 255;
}
}

} // anonymous namespace (variant helpers)

// Grass top variants: 8 shades from bright/yellow-green to dark/blue-green.
void TextureArray::genGrassTop1(PixelBuf pixels){ genGrassTopVariant(pixels, 140.0f,  2.0f); }
void TextureArray::genGrassTop2(PixelBuf pixels){ genGrassTopVariant(pixels, 130.0f,  9.0f); }
void TextureArray::genGrassTop3(PixelBuf pixels){ genGrassTopVariant(pixels, 120.0f, 23.0f); }
void TextureArray::genGrassTop4(PixelBuf pixels){ genGrassTopVariant(pixels, 115.0f, 37.0f); }
void TextureArray::genGrassTop5(PixelBuf pixels){ genGrassTopVariant(pixels, 105.0f, 51.0f); }
void TextureArray::genGrassTop6(PixelBuf pixels){ genGrassTopVariant(pixels,  95.0f, 65.0f); }
void TextureArray::genGrassTop7(PixelBuf pixels){ genGrassTopVariant(pixels,  88.0f, 79.0f); }
void TextureArray::genGrassTop8(PixelBuf pixels){ genGrassTopVariant(pixels,  80.0f, 93.0f); }

// Grass side variants matching the eight top shades.
void TextureArray::genGrassSide1(PixelBuf pixels){ genGrassSideVariant(pixels, 130.0f,  2.0f); }
void TextureArray::genGrassSide2(PixelBuf pixels){ genGrassSideVariant(pixels, 120.0f,  9.0f); }
void TextureArray::genGrassSide3(PixelBuf pixels){ genGrassSideVariant(pixels, 112.0f, 23.0f); }
void TextureArray::genGrassSide4(PixelBuf pixels){ genGrassSideVariant(pixels, 105.0f, 37.0f); }
void TextureArray::genGrassSide5(PixelBuf pixels){ genGrassSideVariant(pixels,  97.0f, 51.0f); }
void TextureArray::genGrassSide6(PixelBuf pixels){ genGrassSideVariant(pixels,  88.0f, 65.0f); }
void TextureArray::genGrassSide7(PixelBuf pixels){ genGrassSideVariant(pixels,  80.0f, 79.0f); }
void TextureArray::genGrassSide8(PixelBuf pixels){ genGrassSideVariant(pixels,  73.0f, 93.0f); }

// Rock variants: 8 shades — grey, warm grey, cool blue-grey, greenish, brownish, dark, reddish, mossy.
void TextureArray::genRock1(PixelBuf pixels){ genRockVariant(pixels, 110.0f,  0.0f,  0.0f,  0.0f, 200.0f); } // neutral grey
void TextureArray::genRock2(PixelBuf pixels){ genRockVariant(pixels, 100.0f,  8.0f,  4.0f, -4.0f, 213.0f); } // warm grey
void TextureArray::genRock3(PixelBuf pixels){ genRockVariant(pixels, 105.0f, -4.0f,  0.0f,  8.0f, 227.0f); } // cool blue-grey
void TextureArray::genRock4(PixelBuf pixels){ genRockVariant(pixels,  95.0f, -3.0f,  5.0f, -3.0f, 241.0f); } // greenish
void TextureArray::genRock5(PixelBuf pixels){ genRockVariant(pixels,  90.0f, 12.0f,  6.0f, -6.0f, 255.0f); } // brownish
void TextureArray::genRock6(PixelBuf pixels){ genRockVariant(pixels,  70.0f,  0.0f,  0.0f,  0.0f, 270.0f); } // dark grey
void TextureArray::genRock7(PixelBuf pixels){ genRockVariant(pixels,  88.0f, 18.0f, -3.0f, -5.0f, 285.0f); } // reddish
void TextureArray::genRock8(PixelBuf pixels){ genRockVariant(pixels,  85.0f, -5.0f, 10.0f, -2.0f, 300.0f); } // mossy

// Mushroom cap — red/white spotted toadstool top.
void TextureArray::genMushroom(PixelBuf pixels)
{
for (int y = 0; y < TEX_SIZE; ++y)
for (int x = 0; x < TEX_SIZE; ++x)
{
float n1 = torusNoise01(x, y, 6.0f, 77.0f);
float n2 = torusNoise01(x, y, 12.0f, 44.0f);
// Bright white spots on a red base.
bool spot = (n1 > 0.72f && n2 > 0.5f);
uint8_t r = spot ? 240 : clamp8(180.0f + n1 * 40.0f);
uint8_t g = spot ? 240 : clamp8( 30.0f + n1 * 20.0f);
uint8_t b = spot ? 240 : clamp8( 20.0f + n1 * 15.0f);
int i = (y * TEX_SIZE + x) * 4;
pixels[i+0] = r; pixels[i+1] = g; pixels[i+2] = b; pixels[i+3] = 255;
}
}
// ---------------------------------------------------------------------------
// TextureArray — build / bind / destroy
// ---------------------------------------------------------------------------

void TextureArray::build()
{
	// Dispatch table: layer index -> generator function pointer.
	using GenFn = void(*)(PixelBuf);
	static const GenFn generators[TOTAL_LAYERS] = {
		genStone,        // 0
		genDirt,         // 1
		genGrassTop,     // 2
		genGrassSide,    // 3
		genSand,         // 4
		genWoodTop,      // 5
		genWoodSide,     // 6
		genLeaf,         // 7
		genBedrock,      // 8
		genGravel,       // 9
		genSnow,         // 10
		genWater,        // 11
		genCoalOre,      // 12
		genIronOre,      // 13
		genGoldOre,      // 14
		genDiamondOre,   // 15
		genRedstoneOre,  // 16
		genLapisOre,     // 17
		genEmeraldOre,   // 18
		genObsidian,     // 19
		genGrassTop1,    // 20
		genGrassTop2,    // 21
		genGrassTop3,    // 22
		genGrassTop4,    // 23
		genGrassTop5,    // 24
		genGrassTop6,    // 25
		genGrassTop7,    // 26
		genGrassTop8,    // 27
		genGrassSide1,   // 28
		genGrassSide2,   // 29
		genGrassSide3,   // 30
		genGrassSide4,   // 31
		genGrassSide5,   // 32
		genGrassSide6,   // 33
		genGrassSide7,   // 34
		genGrassSide8,   // 35
		genRock1,        // 36
		genRock2,        // 37
		genRock3,        // 38
		genRock4,        // 39
		genRock5,        // 40
		genRock6,        // 41
		genRock7,        // 42
		genRock8,        // 43
		genMushroom,     // 44
	};

	glGenTextures(1, &m_handle);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);

	// Allocate all layers at once (mipmap level 0 only for now).
	glTexImage3D(GL_TEXTURE_2D_ARRAY,
				 0,                  // mip level
				 GL_RGBA8,           // internal format
				 TEX_SIZE, TEX_SIZE, // width, height
				 TOTAL_LAYERS,       // depth = number of layers
				 0,                  // border
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 nullptr);           // no data yet

	// Generate each layer and upload.
	PixelBuf pixels;
	for (int layer = 0; layer < TOTAL_LAYERS; ++layer)
	{
		generators[layer](pixels);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
						0,                   // mip level
						0, 0, layer,         // x/y/z offset
						TEX_SIZE, TEX_SIZE,  // width, height
						1,                   // depth (1 layer)
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						pixels);
	}

	// Nearest-neighbour filtering preserves the pixelated Minecraft look.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Wrap mode ensures seamless tiling across adjacent faces.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void TextureArray::bind(GLuint unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
}

void TextureArray::destroy()
{
	if (m_handle)
	{
		glDeleteTextures(1, &m_handle);
		m_handle = 0;
	}
}

} // namespace Texture

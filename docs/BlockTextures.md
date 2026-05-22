# Block Texture Reference

This document describes the procedurally generated textures used in WorldCraft. All textures are generated at runtime using seamless torus-domain Perlin noise (64×64 pixels).

## Texture Generation

All block textures are created using `stb_perlin` noise functions with torus-domain wrapping to ensure perfect seamless tiling. This approach provides:
- Zero memory footprint for texture assets
- Infinite variation possibilities
- Perfect tilability with no seams
- Consistent art style across all blocks

---

## Natural Terrain Textures

### Stone
- **Pattern:** Cool grey base with small dark speckles
- **Color:** Grey (RGB ~120, 120, 120)
- **Noise Layers:** 
  - Low-frequency base variation (freq 3.0)
  - High-frequency dark speckles (freq 8.0)
- **Use:** Primary underground material, mountain surfaces

### Dirt
- **Pattern:** Warm brown with patches and grain flecks
- **Color:** Brown (RGB ~120, 82, 45)
- **Noise Layers:**
  - Medium-frequency patches (freq 2.5)
  - High-frequency bright flecks (freq 9.0)
- **Use:** Subsurface layer, exposed dirt

### Grass Top
- **Pattern:** Bright green with subtle high-frequency variation
- **Color:** Green (RGB ~80, 140, 50)
- **Noise Layers:**
  - Medium-frequency color variation (freq 4.0)
  - High-frequency texture detail (freq 9.0)
- **Use:** Top face of grass blocks
- **Variants:** 8 different shades for natural variation

### Grass Side
- **Pattern:** Dirt texture with 2-pixel green strip at top edge
- **Color:** Brown body with green (RGB ~75, 135, 45) strip
- **Special:** Composite texture combining dirt + grass band
- **Use:** Side faces of grass blocks
- **Variants:** 8 different grass colors matching top variants

### Sand
- **Pattern:** Pale warm yellow with ripple noise
- **Color:** Yellow-tan (RGB ~215, 195, 135)
- **Noise Layers:**
  - Low-frequency ripples (freq 2.0)
  - Medium-frequency texture (freq 6.0)
- **Use:** Desert biomes, beaches

### Gravel
- **Pattern:** Coarse grey pebble texture
- **Color:** Medium grey with variation
- **Noise Layers:**
  - Multiple frequencies for rocky appearance
- **Use:** Underground, riverbeds

### Snow
- **Pattern:** Pure white with subtle blue tint and sparkle variation
- **Color:** White (RGB ~240, 240, 245)
- **Noise Layers:**
  - Very high-frequency sparkle detail
- **Use:** Tundra biome, mountain peaks above Y=105

### Bedrock
- **Pattern:** Dark uniform texture with minimal variation
- **Color:** Dark grey-black (RGB ~35, 35, 35)
- **Noise Layers:**
  - Low-amplitude subtle texture
- **Use:** Indestructible bottom layer of world

---

## Vegetation Textures

### Wood Top
- **Pattern:** Concentric ring pattern (tree rings)
- **Color:** Warm brown (RGB ~130, 90, 50)
- **Algorithm:** Distance-from-center rings with noise distortion
- **Noise Layers:**
  - Radial distance for rings
  - Grain noise for organic variation
- **Use:** Top and bottom faces of log blocks

### Wood Side
- **Pattern:** Vertical grain lines
- **Color:** Brown (RGB ~130, 90, 50)
- **Noise Layers:**
  - Low-frequency for color variation
  - High-frequency vertical streaks for grain
- **Use:** Side faces of log blocks

### Leaf
- **Pattern:** Mottled green foliage
- **Color:** Forest green (RGB ~60, 130, 30)
- **Transparency:** Semi-transparent (renders in transparent pass)
- **Noise Layers:**
  - Multiple scales for organic leaf cluster look
- **Use:** Tree canopy

### Mushroom
- **Pattern:** Spotted cap texture
- **Color:** Red/brown with white spots (RGB varies)
- **Noise Layers:**
  - Base cap color
  - Spot pattern overlay
- **Use:** Decorative ground cover in forests

---

## Ore Textures

All ore textures follow the same pattern: **stone base with colored mineral veins**.

### Coal Ore
- **Vein Color:** Black (RGB ~20, 20, 20)
- **Pattern:** Dark coal chunks in stone
- **Rarity:** Common (freq 0.08)
- **Depth:** Y 5-128

### Iron Ore
- **Vein Color:** Tan-orange (RGB ~180, 140, 100)
- **Pattern:** Tan metallic veins in stone
- **Rarity:** Common (freq 0.06)
- **Depth:** Y 5-64

### Gold Ore
- **Vein Color:** Golden yellow (RGB ~220, 180, 50)
- **Pattern:** Bright gold veins in stone
- **Rarity:** Uncommon (freq 0.03)
- **Depth:** Y 5-32

### Redstone Ore
- **Vein Color:** Deep red (RGB ~180, 30, 30)
- **Pattern:** Red crystal veins in stone
- **Rarity:** Uncommon (freq 0.04)
- **Depth:** Y 5-16

### Lapis Lazuli Ore
- **Vein Color:** Deep blue (RGB ~30, 60, 180)
- **Pattern:** Blue crystal clusters in stone
- **Rarity:** Rare (freq 0.02)
- **Depth:** Y 5-32

### Diamond Ore
- **Vein Color:** Cyan-blue (RGB ~100, 220, 230)
- **Pattern:** Bright cyan crystals in stone
- **Rarity:** Very Rare (freq 0.01)
- **Depth:** Y 5-16

### Emerald Ore
- **Vein Color:** Bright green (RGB ~40, 200, 80)
- **Pattern:** Green crystal veins in stone
- **Rarity:** Very Rare (freq 0.008)
- **Depth:** Y 5-32
- **Special:** Only in mountain biomes

---

## Special Textures

### Water
- **Pattern:** Animated semi-transparent blue
- **Color:** Blue-cyan with transparency (RGBA ~30, 100, 180, 180)
- **Noise Layers:**
  - Animated surface ripples
  - Depth-based color variation
- **Special Features:**
  - Caustics lighting effect when underwater
  - Fog rendering for depth
  - No backface culling (visible from all angles)
- **Transparency:** Full transparent render pass

### Obsidian
- **Pattern:** Dark purple-black volcanic glass
- **Color:** Purple-black (RGB ~25, 15, 40)
- **Noise Layers:**
  - Subtle purple shimmer
  - Glass-like smooth variation
- **Use:** Rare special material

---

## Rock Variants (1-8)

Eight procedurally varied rock surface textures for visual diversity on mountains:

| Variant | Base Color | Pattern | Usage |
|---------|------------|---------|-------|
| Rock 1 | Cool grey | Smooth with small cracks | Default mountain surface |
| Rock 2 | Warm grey | Rough texture | Mountain variation |
| Rock 3 | Blue-grey | Weathered surface | High altitude |
| Rock 4 | Brown-grey | Sedimentary layers | Cliff faces |
| Rock 5 | Dark grey | Basalt-like | Volcanic regions |
| Rock 6 | Light grey | Limestone-like | Exposed rock |
| Rock 7 | Red-grey | Iron-rich stone | Desert mountains |
| Rock 8 | Green-grey | Mossy stone | Forest-edge mountains |

Each variant uses different noise seed values and color offsets to create distinct appearances while maintaining style consistency.

---

## Texture Array Layout

WorldCraft uses an OpenGL `GL_TEXTURE_2D_ARRAY` containing all block textures. The texture layers are indexed as follows:

```cpp
// Core textures
LAYER_STONE        = 0
LAYER_DIRT         = 1
LAYER_GRASS_TOP    = 2
LAYER_GRASS_SIDE   = 3
LAYER_SAND         = 4
LAYER_WOOD_TOP     = 5
LAYER_WOOD_SIDE    = 6
LAYER_LEAF         = 7
LAYER_BEDROCK      = 8
LAYER_GRAVEL       = 9
LAYER_SNOW         = 10
LAYER_WATER        = 11

// Ores (12-18)
LAYER_COAL_ORE     = 12
LAYER_IRON_ORE     = 13
LAYER_GOLD_ORE     = 14
LAYER_DIAMOND_ORE  = 15
LAYER_REDSTONE_ORE = 16
LAYER_LAPIS_ORE    = 17
LAYER_EMERALD_ORE  = 18

LAYER_OBSIDIAN     = 19

// Grass variants (20-27 top, 28-35 side)
LAYER_GRASS_TOP_1 through LAYER_GRASS_TOP_8    = 20-27
LAYER_GRASS_SIDE_1 through LAYER_GRASS_SIDE_8  = 28-35

// Rock variants (36-43)
LAYER_ROCK_1 through LAYER_ROCK_8              = 36-43

LAYER_MUSHROOM     = 44
```

Total texture layers: **45**

---

## Face Assignment

Blocks can have different textures on different faces:

### Uniform Blocks
Most blocks use the same texture on all 6 faces:
- Stone, Dirt, Sand, Ores, Water, etc.

### Multi-Face Blocks
Some blocks have face-specific textures:

**Grass Block:**
- Top (PosY): Grass top texture
- Bottom (NegY): Dirt texture
- Sides (4 faces): Grass side texture (dirt with green strip)

**Wood Block:**
- Top (PosY): Wood top texture (rings)
- Bottom (NegY): Wood top texture (rings)
- Sides (4 faces): Wood side texture (vertical grain)

**Snow Block:**
- Top (PosY): Snow texture
- Bottom (NegY): Dirt texture (hidden usually)
- Sides (4 faces): Snow texture

---

## Rendering Notes

### Texture Filtering
- **Min Filter:** `GL_NEAREST_MIPMAP_LINEAR`
- **Mag Filter:** `GL_NEAREST`
- **Result:** Crisp pixel-art appearance with smooth LOD transitions

### Mipmaps
- **Levels:** Automatically generated
- **Purpose:** LOD rendering performance and quality

### Transparency
Transparent blocks (Water, Leaf) are:
- Rendered in a separate draw pass
- Sorted back-to-front
- Rendered after all opaque geometry
- Support alpha blending

### Ambient Occlusion
All block faces receive baked ambient occlusion during meshing:
- 4 corner values per face
- Based on adjacent block occupancy
- Multiplied with texture color in shader
- Creates realistic shadow creases

---

## Future Texture Plans

Potential additions and improvements:

- [ ] Normal mapping for additional depth
- [ ] Specular/metallic maps for ores
- [ ] Animated textures (water flow, lava)
- [ ] Emissive textures (glowstone, redstone active)
- [ ] Biome-tinted grass/foliage
- [ ] Seasonal texture variants
- [ ] User-definable texture packs
- [ ] PBR (Physically Based Rendering) materials

---

*For implementation details, see `WorldCraft\src\Texture\BlockTextures.cpp`*

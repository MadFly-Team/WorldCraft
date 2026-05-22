# Texture Image Reference

Since WorldCraft uses **procedurally generated textures**, there are no static texture image files. All textures are created at runtime using Perlin noise algorithms.

## Capturing Texture Images

To create texture reference images for documentation:

### Method 1: Screenshot In-Game Blocks
1. Launch WorldCraft
2. Use Fly Camera mode to get close to blocks
3. Take screenshots of each block type
4. Crop to show clear block face textures

### Method 2: Extract from Texture Array (Code Required)
Add this debug function to capture texture layers to PNG files:

```cpp
// In BlockTextures.cpp - requires stb_image_write.h
void TextureArray::saveLayerToPNG(int layer, const char* filename)
{
	uint8_t pixels[TEX_SIZE * TEX_SIZE * 4];

	glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Extract specific layer and save
	// ... implementation details ...
}
```

### Method 3: Direct Generation
Call the texture generation functions directly and save the pixel buffers:

```cpp
uint8_t pixels[TEX_SIZE * TEX_SIZE * 4];
TextureArray::genStone(pixels);
stbi_write_png("docs/images/stone.png", TEX_SIZE, TEX_SIZE, 4, pixels, TEX_SIZE * 4);
```

## Recommended Image Set

For complete documentation, capture these block faces:

### Core Terrain (9 images)
- `stone.png` - Stone texture (uniform)
- `dirt.png` - Dirt texture (uniform)
- `grass_top.png` - Grass top face
- `grass_side.png` - Grass side face
- `sand.png` - Sand texture
- `gravel.png` - Gravel texture
- `snow.png` - Snow texture
- `bedrock.png` - Bedrock texture
- `obsidian.png` - Obsidian texture

### Vegetation (4 images)
- `wood_top.png` - Wood top (ring pattern)
- `wood_side.png` - Wood side (vertical grain)
- `leaf.png` - Leaf texture
- `mushroom.png` - Mushroom texture

### Ores (7 images)
- `coal_ore.png` - Coal ore
- `iron_ore.png` - Iron ore
- `gold_ore.png` - Gold ore
- `diamond_ore.png` - Diamond ore
- `redstone_ore.png` - Redstone ore
- `lapis_ore.png` - Lapis ore
- `emerald_ore.png` - Emerald ore

### Water (1 image)
- `water.png` - Water texture (semi-transparent)

### Variants (16 images - optional)
- `grass_top_1.png` through `grass_top_8.png`
- `rock_1.png` through `rock_8.png`

## Image Specifications

When capturing or generating texture images:

- **Resolution:** 64×64 pixels
- **Format:** PNG with alpha channel
- **Color Space:** sRGB
- **Naming:** Lowercase with underscores
- **Location:** `docs/images/textures/`

## Block Composition Examples

For blocks with multiple textures, create composite images showing all faces:

### Example: Grass Block Composite
```
+-------------+
|  grass_top  |  (Top face)
+-------------+
| grass_side  |  (Side faces ×4)
+-------------+
|    dirt     |  (Bottom face)
+-------------+
```

### Example: Wood Block Composite
```
+-------------+
|  wood_top   |  (Top face)
+-------------+
| wood_side   |  (Side faces ×4)
+-------------+
|  wood_top   |  (Bottom face)
+-------------+
```

## Creating Block Diagrams

For README.md table display, create 32×32 pixel preview icons:

1. Generate full 64×64 texture
2. Scale down to 32×32 with nearest-neighbor filtering
3. Add 1px border if needed for visibility
4. Save as `block_name_icon.png`

## Usage in README

Once images are created, update README.md table entries:

```markdown
| Block | Side Texture | Top Texture | Description |
|-------|--------------|-------------|-------------|
| **Grass** | ![Side](docs/images/textures/grass_side_icon.png) | ![Top](docs/images/textures/grass_top_icon.png) | Standard grass block |
```

## Automatic Image Generation Script

Create a Python/C++ utility to batch-generate all texture images:

```cpp
// TextureExporter.cpp (conceptual)
void exportAllTextures()
{
	TextureArray texArray;

	const char* names[] = {
		"stone", "dirt", "grass_top", "grass_side", 
		"sand", "wood_top", "wood_side", "leaf",
		// ... all texture names ...
	};

	for (int i = 0; i < numTextures; ++i)
	{
		uint8_t pixels[TEX_SIZE * TEX_SIZE * 4];
		texArray.generateLayer(i, pixels);

		char filename[256];
		sprintf(filename, "docs/images/textures/%s.png", names[i]);
		stbi_write_png(filename, TEX_SIZE, TEX_SIZE, 4, pixels, TEX_SIZE * 4);
	}
}
```

---

## Current Status

**Texture images are not yet generated.** To create them:

1. Implement texture export functionality
2. Run export utility to generate all PNG files
3. Create 32×32 icon versions
4. Update README.md with image links

**Alternative:** Use in-game screenshots of blocks in good lighting to showcase textures naturally.

---

*See [BlockTextures.md](BlockTextures.md) for detailed texture descriptions*

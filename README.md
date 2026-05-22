# WorldCraft

A high-performance voxel-based world exploration game built with modern C++20 and OpenGL, featuring procedurally generated terrain, dynamic cave systems, and multi-level LOD rendering.

---

## 🎮 Current State

**Version:** Pre-Alpha Development Build  
**Status:** Core systems functional, actively developed

### ✨ Implemented Features

#### World Generation
- **Procedural Terrain Generation** - Multi-biome world with realistic height variation
  - Plains, Deserts, Forests, Mountains, Tundra, and Ocean biomes
  - Seamless biome blending with smooth transitions
  - Configurable terrain amplitude, scale, and sea level
  - 8 grass variants and 8 rock surface variants for visual diversity

#### Cave Systems
- **Mountain Cave Entrances** - Rare, large cave openings on mountainsides
  - Visible from distance with debug beacon markers
  - Natural tube-like entrance tunnels
  - Configurable rarity and size parameters
- **Deep Cavern Networks** - Massive sealed underground chambers
  - Spawn between configurable depth ranges (default: Y 10-50)
  - Organic noise-based chamber formation
  - Natural ore and mineral deposits
- **Dynamic Cave Lighting** - Depth-based progressive darkening
  - Exponential light falloff below Y=60
  - Atmospheric exploration requiring light sources

#### Rendering System
- **5-Level LOD (Level of Detail) System**
  - LOD0 (full detail), LOD1 (1:2), LOD2 (1:3), LOD3 (1:4), LOD4 (1:6)
  - Interior-biased multi-point sampling for distant terrain
  - Smooth transitions between detail levels
  - Optimized chunk meshing with greedy mesh algorithm
- **Advanced Visual Effects**
  - Distance-based fog and atmospheric haze
  - Render-distance aware fog blending
  - Horizon color matching at dawn/dusk
  - Underwater rendering with caustics and fog
  - Ambient occlusion (AO) baking for realistic shadows
  - Gamma-corrected lighting (1/1.8 power curve)

#### Chunk System
- **Infinite World Streaming**
  - Dynamic chunk generation and loading
  - Smart chunk eviction based on distance
  - Configurable render distance (default: 20 chunks)
  - Memory-stable operation (~2.7GB typical usage)
  - Fast generation supporting up to 40+ chunk render distance

#### Camera System
- **Dual Camera Modes**
  - **Fly Camera** - Free-flight noclip mode for exploration
  - **Character Camera** - Walking mode with full physics
	- Gravity and jumping mechanics
	- AABB collision detection
	- Ground detection
	- Eye-height view offset
- **Toggle between modes** - Press `C` to switch

#### User Interface
- **Real-time HUD Display**
  - World coordinates (X, Y, Z)
  - Current camera mode indicator
  - DPI-aware ImGui overlay
- **Settings Dialog** (planned expansion)
  - World generation parameters
  - Cave system configuration
  - Visual effect toggles

---

## 🎨 Block Types

WorldCraft features **36 unique block types** with procedurally generated seamless textures. All textures are generated at runtime using torus-domain Perlin noise for perfect tiling.

### Natural Terrain Blocks

| Block | Type | Side Texture | Top Texture | Description |
|-------|------|--------------|-------------|-------------|
| **Grass** | Solid | Dirt with green strip | Grass top | Standard grass block with 8 color variants |
| **Dirt** | Solid | Uniform | Uniform | Warm mid-brown earth |
| **Stone** | Solid | Uniform | Uniform | Cool grey stone base |
| **Sand** | Solid | Uniform | Uniform | Pale yellow sand with ripple patterns |
| **Gravel** | Solid | Uniform | Uniform | Coarse grey pebble texture |
| **Bedrock** | Solid | Uniform | Uniform | Indestructible dark base layer |
| **Snow Block** | Solid | White | White | Pure white snow, found in tundra/mountains |

### Vegetation Blocks

| Block | Type | Side Texture | Top Texture | Description |
|-------|------|--------------|-------------|-------------|
| **Wood** | Solid | Vertical grain | Ring pattern | Oak log with natural wood texture |
| **Leaf** | Transparent | Uniform | Uniform | Green foliage, semi-transparent |
| **Mushroom** | Solid | Uniform | Uniform | Decorative fungus block |

### Ore Blocks

All ore blocks feature stone base with colored mineral veins.

| Block | Type | Side Texture | Top Texture | Rarity | Description |
|-------|------|--------------|-------------|--------|-------------|
| **Coal Ore** | Solid | Uniform | Uniform | Common | Black coal deposits |
| **Iron Ore** | Solid | Uniform | Uniform | Common | Tan-orange iron veins |
| **Gold Ore** | Solid | Uniform | Uniform | Uncommon | Golden yellow deposits |
| **Redstone Ore** | Solid | Uniform | Uniform | Uncommon | Red redstone veins |
| **Lapis Ore** | Solid | Uniform | Uniform | Rare | Deep blue lapis lazuli |
| **Diamond Ore** | Solid | Uniform | Uniform | Very Rare | Cyan diamond crystals |
| **Emerald Ore** | Solid | Uniform | Uniform | Very Rare | Green emerald deposits |

### Special Blocks

| Block | Type | Side Texture | Top Texture | Description |
|-------|------|--------------|-------------|-------------|
| **Water** | Transparent | Uniform | Uniform | Semi-transparent animated water |
| **Obsidian** | Solid | Uniform | Uniform | Dark purple-black volcanic glass |

### Variant Blocks

- **Grass Variants (1-8)** - Different shades of green grass tops and sides for natural variation
- **Rock Variants (1-8)** - Weathered stone surfaces with varying patterns for mountains

---

## 🎯 World Presets

WorldCraft includes several pre-configured world generation presets:

### Default
Balanced terrain with all biomes, moderate mountains, trees, and full cave systems.
- **Terrain Amplitude:** 80.0
- **Terrain Scale:** 100.0
- **Sea Level:** 64
- **Caves:** Mountain entrances + Deep caverns

### Flat
Nearly flat terrain for building and testing.
- **Terrain Amplitude:** 3.0
- **Terrain Scale:** 300.0
- **Sea Level:** 50
- **Caves:** Disabled

### Mountainous
Dramatic mountain ranges with tall peaks.
- **Terrain Amplitude:** 120.0
- **Terrain Scale:** 80.0
- **Caves:** Enabled

### Islands
Archipelago world with raised sea level.
- **Terrain Amplitude:** 60.0
- **Sea Level:** 80
- **Caves:** Enabled

### Cave World
Enhanced cave generation for underground exploration.
- **Mountain Cave Rarity:** Increased
- **Mountain Cave Size:** Larger (30.0)
- **Cavern Size:** Larger (45.0)
- **Cavern Depth:** Deeper range (5-55)

---

## 🛠️ Technical Specifications

### Architecture
- **Language:** C++20 / C++17
- **Build System:** CMake 4.3.1-msvc1 with Ninja generator (minimum CMake 3.27.0)
- **Graphics API:** OpenGL with modern shader pipeline
- **Toolchain:** MSVC (Visual Studio 2026)

### Core Systems
- **Chunk System:** 16×256×16 voxel chunks
- **Meshing:** Greedy mesh algorithm with face culling
- **LOD Meshing:** Multi-level detail generation with interior-biased sampling
- **Noise Generation:** STB Perlin with torus-domain seamless tiling
- **Collision:** AABB-based player collision with swept volume testing

### Performance
- **Target Frame Rate:** 60+ FPS
- **Memory Usage:** ~2.7GB typical (20 chunk render distance)
- **Render Distance:** Configurable 8-40+ chunks
- **Chunk Generation:** Real-time streaming, no loading screens

### Dependencies
- **SDL2** - Window management and input
- **OpenGL** - Rendering
- **ImGui** - UI and debug overlays
- **GLM** - Mathematics library
- **STB** - Image loading and Perlin noise
- **Assimp** - Asset loading (planned)

---

## 🎮 Controls

### Fly Camera Mode (Default)
- **W/A/S/D** - Move forward/left/back/right
- **Space** - Move up
- **Left Shift** - Move down
- **Mouse** - Look around
- **C** - Switch to Character Camera

### Character Camera Mode
- **W/A/S/D** - Walk forward/left/back/right
- **Space** - Jump
- **Mouse** - Look around
- **C** - Switch to Fly Camera

### UI
- **F1** - Toggle settings dialog (planned)
- **ESC** - Exit application

---

## 🏗️ Building from Source

### Prerequisites
- Visual Studio 2022 or later (2026 recommended)
- CMake 3.27.0 or later
- Ninja build system
- Windows 10/11 (currently Windows-only)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/MadFly-Team/WorldCraft
cd WorldCraft/Trunk

# Configure CMake
cmake -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build out/build/x64-Release

# Run
./out/build/x64-Release/WorldCraft/WorldCraft.exe
```

### Development Build
```bash
cmake -B out/build/x64-Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug
```

---

## 📚 Documentation

Additional documentation can be found in the `docs/` directory:

- **[Chunk Optimizations](docs/ChunkOptimizations.md)** - LOD system, memory management, and rendering optimizations
- **[Cave Generation](docs/CaveGeneration.md)** - Cave system architecture and generation algorithms

---

## 🚀 Planned Features

### Short Term
- [ ] Block breaking and placement mechanics
- [ ] Inventory system
- [ ] Torch placement for cave lighting
- [ ] Save/load world data
- [ ] Skybox with day/night cycle
- [ ] Dynamic sun position and lighting

### Medium Term
- [ ] Crafting system
- [ ] More biome types (jungle, swamp, badlands)
- [ ] Structure generation (villages, dungeons)
- [ ] Mob spawning and AI
- [ ] Sound effects and ambient audio
- [ ] Improved water physics

### Long Term
- [ ] Multiplayer support
- [ ] Mod API
- [ ] Advanced weather systems
- [ ] Shader-based lighting (dynamic shadows)
- [ ] Chunk LOD with geometry clipmaps
- [ ] Cross-platform support (Linux, macOS)

---

## 🤝 Contributing

This project is currently in early development. Contributions, suggestions, and feedback are welcome!

### Development Branch
Main development occurs on the `neil/develop` branch, please branch off this for any additions.

### Code Style
- Modern C++20 features preferred
- Follow existing naming conventions
- Comment complex algorithms
- Keep functions focused and modular

---

## 📜 License

*License information to be added*

---

## 🙏 Acknowledgments

### Third-Party Libraries
- **SDL2** - Cross-platform window and input management
- **ImGui** - Immediate mode GUI library
- **STB** - Single-file public domain libraries
- **GLM** - OpenGL Mathematics library
- **Assimp** - Asset import library

### Inspiration
WorldCraft draws inspiration from voxel games like Minecraft while exploring modern C++ techniques and rendering optimizations.

---

## 📞 Contact

**Repository:** [https://github.com/MadFly-Team/WorldCraft](https://github.com/MadFly-Team/WorldCraft)  
**Branch:** neil/develop

---

*Last Updated: May 2026*  
*Built with ❤️ using C++20 and OpenGL*

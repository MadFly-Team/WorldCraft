# WorldCraft Documentation Index

Welcome to the WorldCraft documentation! This index provides quick access to all documentation resources.

## 📖 Core Documentation

### [README.md](../README.md)
**Main project overview and getting started guide**
- Current feature set and game state
- Complete block type reference table
- World generation presets
- Technical specifications
- Building instructions
- Controls reference
- Planned features roadmap

---

## 🔧 Technical Documentation

### [ChunkOptimizations.md](ChunkOptimizations.md)
**Chunk system performance and rendering optimizations**
- LOD (Level of Detail) system architecture
- Multi-level mesh generation strategies
- Memory management techniques
- Chunk streaming and eviction
- Performance optimization history
- Visual effects (fog, haze, lighting)
- Camera system implementation

### [CaveGeneration.md](CaveGeneration.md)
**Cave system generation algorithms and design**
- Mountain cave entrance generation
- Deep cavern network algorithms
- Noise-based cave formation
- Ore and mineral placement
- Cave lighting system
- Water interaction with caves

### [BlockTextures.md](BlockTextures.md)
**Complete texture reference and generation details**
- Procedural texture generation algorithms
- All 36+ block types with descriptions
- Texture array layout and indexing
- Face assignment rules
- Noise generation parameters
- Color specifications for each block type
- Ore distribution and rarity

---

## 🎨 Asset Documentation

### [images/TextureImageGuide.md](images/TextureImageGuide.md)
**Guide for capturing and creating texture reference images**
- Methods to extract procedural textures
- Image specifications and formats
- Block diagram creation guidelines
- Texture export utility concepts

### [images/LogoPlaceholder.md](images/LogoPlaceholder.md)
**Logo specifications and creation guidelines**
- Recommended logo dimensions and style
- Color palette suggestions
- ASCII art alternatives

---

## 🏗️ Project Structure

```
WorldCraft/
├── WorldCraft/          # Main game source code
│   ├── inc/             # Header files
│   │   ├── Chunk/       # Chunk system
│   │   ├── Meshing/     # Mesh generation
│   │   ├── Renderer/    # Rendering and camera
│   │   ├── Texture/     # Texture generation
│   │   ├── UI/          # User interface
│   │   ├── Voxel/       # Block types and registry
│   │   └── WorldGen/    # World generation
│   └── src/             # Implementation files
│       └── (mirrors inc/ structure)
│
├── docs/                # Documentation
│   ├── README.md        # This file
│   ├── ChunkOptimizations.md
│   ├── CaveGeneration.md
│   ├── BlockTextures.md
│   └── images/          # Image assets and guides
│
├── third_party/         # External dependencies
│   ├── SDL2/
│   ├── glm/
│   ├── imgui/
│   ├── stb/
│   └── ...
│
├── CMakeLists.txt       # Main CMake configuration
└── README.md            # Project root README
```

---

## 🚀 Quick Start

### For Players
1. Read [README.md](../README.md) - Main documentation
2. Check **Controls** section for gameplay
3. Review **World Presets** for world types

### For Developers
1. Read [README.md](../README.md) - Build instructions
2. Review [ChunkOptimizations.md](ChunkOptimizations.md) - Architecture
3. Check [BlockTextures.md](BlockTextures.md) - Asset system
4. Explore source code in `WorldCraft/src/`

### For Contributors
1. Read [README.md](../README.md) - Contributing guidelines
2. Review technical docs for architecture understanding
3. Check **Planned Features** for contribution ideas
4. Follow code style conventions

---

## 📚 Documentation Topics

### World Generation
- **Biome System:** Multi-biome terrain with seamless blending
- **Terrain Generation:** Perlin noise-based height maps
- **Cave Systems:** Mountain entrances and deep caverns
- **Ore Distribution:** Depth-based ore spawning
- **Tree Placement:** Biome-specific vegetation

See: [README.md](../README.md), [CaveGeneration.md](CaveGeneration.md)

### Rendering System
- **LOD Meshing:** 5-level detail system
- **Chunk Streaming:** Infinite world support
- **Lighting:** AO baking and cave darkness
- **Visual Effects:** Fog, haze, underwater rendering
- **Camera Modes:** Fly and character cameras

See: [ChunkOptimizations.md](ChunkOptimizations.md)

### Block System
- **Block Types:** 36+ unique blocks
- **Texture Generation:** Procedural seamless textures
- **Face Assignment:** Multi-texture block support
- **Transparency:** Water and foliage rendering
- **Collision:** AABB-based solid block detection

See: [BlockTextures.md](BlockTextures.md)

### Performance
- **Memory Management:** ~2.7GB typical usage
- **Chunk Eviction:** Distance-based unloading
- **Mesh Optimization:** Greedy meshing algorithm
- **GPU Upload:** Efficient vertex buffer management
- **LOD Selection:** Distance-based detail switching

See: [ChunkOptimizations.md](ChunkOptimizations.md)

---

## 🔍 Finding Information

### By Feature
- **Caves:** See [CaveGeneration.md](CaveGeneration.md)
- **Blocks:** See [BlockTextures.md](BlockTextures.md)
- **LOD/Performance:** See [ChunkOptimizations.md](ChunkOptimizations.md)
- **Building/Setup:** See [README.md](../README.md)

### By File Type
- **Headers (`.h`):** `WorldCraft/inc/` - Interface definitions
- **Implementation (`.cpp`):** `WorldCraft/src/` - Core logic
- **Shaders (inline):** `WorldCraft/src/WorldCraft.cpp` - GLSL code
- **Build Config:** `CMakeLists.txt` - CMake setup

### By System
| System | Header Path | Implementation Path |
|--------|-------------|---------------------|
| Chunks | `WorldCraft/inc/Chunk/` | `WorldCraft/src/Chunk/` |
| Meshing | `WorldCraft/inc/Meshing/` | `WorldCraft/src/Meshing/` |
| Rendering | `WorldCraft/inc/Renderer/` | `WorldCraft/src/Renderer/` |
| World Gen | `WorldCraft/inc/WorldGen/` | `WorldCraft/src/WorldGen/` |
| Textures | `WorldCraft/inc/Texture/` | `WorldCraft/src/Texture/` |
| Blocks | `WorldCraft/inc/Voxel/` | `WorldCraft/src/Voxel/` |
| UI | `WorldCraft/inc/UI/` | `WorldCraft/src/UI/` |

---

## 📝 Documentation Status

| Document | Status | Last Updated |
|----------|--------|--------------|
| README.md | ✅ Complete | May 2026 |
| ChunkOptimizations.md | ✅ Complete | May 2026 |
| CaveGeneration.md | ✅ Complete | May 2026 |
| BlockTextures.md | ✅ Complete | May 2026 |
| TextureImageGuide.md | 📝 Reference Only | May 2026 |
| LogoPlaceholder.md | 📝 Placeholder | May 2026 |

**Legend:**
- ✅ Complete and up-to-date
- 📝 Reference/guide document
- 🚧 Work in progress
- ⏳ Planned

---

## 🤝 Contributing to Documentation

Documentation improvements are always welcome!

### Guidelines
- Keep technical accuracy paramount
- Use clear, concise language
- Include code examples where relevant
- Add diagrams for complex systems
- Update index when adding new docs
- Follow existing markdown formatting

### Adding New Documentation
1. Create markdown file in `docs/`
2. Add entry to this index
3. Link from README.md if appropriate
4. Update **Documentation Status** table

---

## 📞 Support

- **Repository:** [https://github.com/MadFly-Team/WorldCraft](https://github.com/MadFly-Team/WorldCraft)
- **Branch:** neil/setup
- **Issues:** Use GitHub Issues for bug reports
- **Discussions:** Use GitHub Discussions for questions

---

*This documentation reflects WorldCraft as of May 2026*  
*For the latest code, always refer to the source files*

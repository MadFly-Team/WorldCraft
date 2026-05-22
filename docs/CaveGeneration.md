# Cave Generation System

This document explains the cave generation algorithm and how to tune it.

## The Problem We Solved

The previous cave system created massive voids that hollowed out the entire underground. Additionally:
1. Caves generated underwater causing water interaction issues
2. LOD1/LOD2 chunks showed internal faces (culling bug)
3. Caves were too narrow and too frequent

## The Solution: Land-Only Worm Caves

We now use a "worm cave" algorithm that only generates caves on land above sea level.

### How It Works

```
1. Check if position is above sea level - 10 blocks (land only)
2. Sample two 3D Perlin noise values (n1, n2) at the same position
3. Calculate distance from "center line": tunnelDist = sqrt(n1² + n2²)
4. If tunnelDist < caveWidth, carve out the block
5. Apply vertical limiter to prevent caves too far above ground
```

### Why This Works

- **Land-only generation**: No underwater caves = no water issues
- **Winding tunnels**: The noise creates a 3D path that winds through the terrain
- **Predictable size**: Single distance threshold controls tunnel width
- **Wider tunnels**: Increased width multiplier (0.35 vs 0.25)
- **Fewer caves**: Reduced frequency and larger scale
- **No massive voids**: Creates distinct tunnel structures, not giant caverns

## Key Parameters

### Cave Frequency (default: 0.7)
Controls how often cave tunnels appear:
- **Lower (0.5)**: Very rare, special cave systems
- **Higher (1.5)**: More frequent cave networks
- **Formula**: `scale = 0.015 / frequency` (reduced from 0.02)

### Cave Threshold (default: 0.5)
Controls tunnel width:
- **Lower (0.3)**: Very wide, spacious tunnels (Cave World preset)
- **Higher (0.7)**: Narrow, tight passages
- **Formula**: `caveWidth = 0.35 * (1.0 - threshold)` (increased from 0.25)

### Effective Widths

| Threshold | Cave Width | Tunnel Size |
|-----------|------------|-------------|
| 0.3       | 0.245      | Very wide   |
| 0.4       | 0.210      | Wide        |
| 0.5       | 0.175      | Medium-wide |
| 0.6       | 0.140      | Medium      |
| 0.7       | 0.105      | Narrow      |

## Vertical Behavior

### Deep Underground (y < sea level - 10)
- No caves
- Prevents underwater cave systems
- Avoids water/cave interaction issues

### Mid-depth (sea level - 10 to sea level + 5)
- Full cave generation
- Most cave systems exist here
- Safe from water

### Near Surface (sea level + 5 to + 35)
- Gradual fade-out
- Tunnels become rarer and narrower
- Allows occasional surface openings on hillsides

### Far Above Sea Level (> +35)
- No caves
- Prevents mountain/hill undermining
- Maintains terrain integrity

## LOD Culling Fix

### Problem
LOD1 and LOD2 meshes were showing internal faces between different solid blocks, causing visual artifacts underwater and in caves.

### Solution
Updated LOD mesher to properly cull internal faces:
- **Opaque blocks**: Only show faces to Air or transparent blocks
- **Transparent blocks**: Show faces to different block types
- **Result**: Clean rendering with proper face culling at all LOD levels

## Preset Configurations

### Default World
```cpp
caveFrequency = 0.7f;   // Moderate density, land only
caveThreshold = 0.5f;   // Wide tunnels
```

### Cave World
```cpp
caveFrequency = 2.0f;   // High density
caveThreshold = 0.4f;   // Very wide tunnels
```

### Flat World
```cpp
caveFrequency = 0.0f;   // Disabled
```

## Technical Details

### Land-Only Generation
```cpp
const float seaLevel = m_settings.seaLevel;
if (wy < seaLevel - 10.0f) return false;  // No underwater caves
```

### Noise Scale
- Base scale: `0.015 / frequency` (larger caves than before)
- Y-axis: 0.5× scale for more horizontal tunnels
- Results in fewer, larger cave systems

### Distance Formula
We use Euclidean distance: `sqrt(n1² + n2²)`
- Values near zero → tunnel center
- Values far from zero → solid rock
- Wider threshold (0.35 vs 0.25) creates larger tunnels

### Surface Opening Formula
```cpp
depthFactor = 1.0 - ((wy - (seaLevel + 5)) / 30.0)
adjustedWidth = caveWidth * depthFactor * 0.5
```

Surface openings only occur on hillsides and mountains above sea level.

## Benefits of Land-Only Caves

### Performance
- ✅ LOD chunks now cull internal faces properly
- ✅ Fewer vertices in underground/underwater scenes
- ✅ No need to render flooded cave interiors

### Gameplay
- ✅ No water flooding into caves
- ✅ Caves stay dry and explorable
- ✅ Predictable cave locations (on land)
- ✅ No underwater surprises

### Visual Quality
- ✅ Clean underwater rendering (no visible internal faces)
- ✅ Proper occlusion in all LOD levels
- ✅ Better performance = more detail budget elsewhere

## Tuning Guidelines

### Too Many Caves?
- **Increase threshold** (0.5 → 0.6): Narrower tunnels
- **Decrease frequency** (0.7 → 0.5): Fewer tunnels

### Too Few Caves?
- **Decrease threshold** (0.5 → 0.4): Wider tunnels
- **Increase frequency** (0.7 → 1.0): More tunnels

### Caves Too Wide?
- **Increase threshold** (narrows tunnels)

### Caves Too Narrow?
- **Decrease threshold** (0.5 → 0.3): Much wider tunnels

### Want Underwater Caves?
- Remove the `if (wy < seaLevel - 10.0f)` check
- **Warning**: Caves will flood with water (by design)

## Comparison: Old vs New

| Aspect | Old System | New System |
|--------|-----------|------------|
| Algorithm | Any depth worm | Land-only worm |
| Water Issues | Frequent | None |
| LOD Culling | Broken | Fixed |
| Tunnel Width | Narrow (0.25) | Wide (0.35) |
| Frequency | Too high | Balanced |
| Result | Too many small caves | Fewer, larger systems |
| Underwater | Messy | Clean |

## Testing

Generate a new world (F11 → Generate New World) to see the changes. Old worlds will keep their old cave system until regenerated.

### What You Should See
- ✅ Caves only on land (above sea level)
- ✅ Wider, more spacious tunnels
- ✅ Fewer overall cave systems
- ✅ Clean underwater rendering (no internal faces)
- ✅ Proper LOD culling in all situations
- ✅ Occasional surface cave entrances on hillsides
- ✅ No massive voids or hollowed-out terrain

### What You Should NOT See
- ❌ Underwater cave systems
- ❌ Internal faces showing in LOD chunks
- ❌ Water flooding into caves
- ❌ Caves everywhere
- ❌ Swiss cheese terrain
- ❌ Ocean floor with caves underneath

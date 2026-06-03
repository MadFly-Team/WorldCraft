# WorldCraft Realistic Block Physics System

## Overview
A comprehensive mass-based physics simulation system for realistic block behavior, including falling blocks, impact energy transfer, chain reactions, water displacement, and throw mechanics. **All settings can be adjusted on-the-fly via the in-game Settings menu.**

## Quick Controls

- **Remove blocks**: Left-click in Removal mode → triggers physics checks on surrounding blocks
- **Throw block**: Press **T** key while targeting a block → throws it in camera direction with 2x mass velocity
- **Physics settings**: Press **Escape** → Settings → "Physics Settings" section

## Architecture

### Core Components

#### 1. BlockMassRegistry (`World/BlockMassRegistry.h/cpp`)
Central registry for block material properties:
- **Mass values**: 50 (water) to 1000 (obsidian)
- **Hardness**: Resistance to destruction (0.0-1.0)
- **Friction**: Energy loss on deflection
- **Density**: For water displacement calculations
- **Flags**: isFluid, isDestructible

Key features:
- Static initialization of all block types
- Dampening calculation based on material interaction
- Destruction threshold checking (2x block mass)

#### 2. FallingBlock (`World/FallingBlock.h/cpp`)
Entity representing a detached, falling block:
- Tracks position, velocity, direction
- Mass accumulation: 10% per block fallen (up to 3x base mass)
- Initial 50% mass boost when fall begins
- Energy calculation: kinetic (0.5*m*v²) + potential (m*g*h)
- Deflection logic with energy dampening (30% per deflection)
- Safety limit: MAX_DEFLECTIONS = 8

#### 3. BlockPhysics (`World/BlockPhysics.h/cpp`)
Main simulation manager:
- Updates all falling blocks each frame
- Collision detection with terrain
- Impact energy transfer to struck blocks
- Chain reaction propagation (6-neighbor scanning)
- Destruction vs trigger-fall decision logic
- Directional deflection (135° anticlockwise rotation)
- Water displacement integration

Key algorithms:
- **Collision**: Checks block below, applies impact, destroys or deflects
- **Propagation**: Scans 6 neighbors, applies directional bias (dot product)
- **Deflection**: 135° rotation in XZ plane, fallback to reverse, then settle

#### 4. WaterPhysics (`World/WaterPhysics.h/cpp`)
Specialized water behavior:
- Ripple system (expanding radius 5 blocks/sec, decay 2/sec)
- Displacement with spherical propagation
- Temporary air pocket creation (>200 energy impacts)
- Water level detection with 5-second cache
- Underwater checking for falling blocks

#### 5. PhysicsSettings (`World/PhysicsSettings.h`)
Runtime-configurable settings:
- **enabled**: Master enable/disable switch
- **gravityMultiplier**: Scale gravity (0.1 to 5.0)
- **massAccumulationRate**: Mass gain per block fallen (0.0 to 0.5)
- **maxMassMultiplier**: Maximum mass cap (1.0 to 10.0)
- **energyDampeningFactor**: Global energy loss multiplier (0.5 to 2.0)
- **destructionThreshold**: Multiplier for block destruction (1.0 to 10.0)
- **maxChainReactionDepth**: Maximum propagation steps (1 to 20)
- **enableWaterPhysics**: Enable/disable water effects

### Integration Points

#### WorldCraft.cpp
- **Initialization**: blockPhysics created after world, before game loop
- **Update loop**: Called every frame (after itemManager, before time progression)
- **World regeneration**: Reset and recreate on new world
- **Block removal**: Triggers physics checks in 3x3x5 area (up to 5 blocks above + surrounding)
- **Block placement**: Triggers physics checks on 5 positions (above + diagonals)
- **Throw mechanic**: T key throws targeted block with initial velocity
- **Settings integration**: Physics settings changes applied immediately via callback

#### SettingsDialog.cpp
- **Physics Settings UI**: Collapsible section with sliders for all parameters
- **Live updates**: Changes apply immediately during gameplay
- **Reset button**: Restores default physics values
- **Tooltips**: Helpful descriptions for each setting

## Physics Behavior

### Mass and Energy
```
Base Mass: 50 (water) → 1000 (obsidian)
Falling Mass = Base * (1 + 0.5 + 0.1 * blocks_fallen) [capped at 3x base]
Energy = 0.5 * mass * velocity² + mass * gravity * blocks_fallen
```

### Impact Resolution
1. **On collision**:
   - Calculate transferred energy (with dampening)
   - Check destruction threshold (2x struck block mass)
   - Destroy or deflect based on threshold

2. **Dampening factors**:
   - Base: struck block hardness
   - Friction: average of both materials
   - Mass ratio: heavier blocks dampen more
   - Combined: clamped to 5-95% retention

3. **Chain reactions**:
   - Energy propagates to 6 neighbors
   - Directional bias (0.5 base + 0.5 * dot product)
   - Minimum energy threshold: 10.0
   - Natural termination via energy decay

### Deflection Behavior
When blocked:
1. Try 135° anticlockwise rotation in XZ plane
2. Check if path is clear
3. Fallback to reverse direction
4. If both blocked, settle with zero velocity
5. Each deflection: lose 30% velocity and energy
6. Max 8 deflections before forced removal

### Water Behavior
- **Impact**: Creates ripples, displaces water spherically
- **Displacement**: Strong impacts (>200 energy) create temporary air pockets
- **Underwater blocks**: Sink and disappear when settled
- **Ripples**: Visual effect, max radius 8 blocks

### Throw Mechanic
- **Activation**: Press **T** key while targeting a block
- **Velocity**: Initial velocity = 2x block mass × 0.1 (configurable multiplier)
- **Direction**: Thrown in camera forward direction
- **Physics**: Full mass accumulation and impact energy applies
- **Restrictions**: Cannot throw Air, Bedrock, or Water blocks

## In-Game Settings

Access via **ESC → Settings → Physics Settings**:

1. **Enable Physics** - Master on/off switch
2. **Gravity Multiplier** (0.1 - 5.0) - Control fall speed
3. **Mass Gain Rate** (0.0 - 0.5) - How much mass accumulates when falling
4. **Max Mass Multiplier** (1.0 - 10.0) - Cap on accumulated mass
5. **Energy Dampening** (0.5 - 2.0) - Control bounciness (higher = less bouncy)
6. **Destruction Threshold** (1.0 - 10.0) - How much energy needed to destroy blocks
7. **Max Chain Depth** (1 - 20) - How far chain reactions propagate
8. **Enable Water Physics** - Toggle water displacement effects

All settings apply **immediately** during gameplay - no restart required!

## Usage

### Triggering Falls
```cpp
// Check if block should fall
if (blockPhysics->shouldBlockFall(blockPos)) {
	blockPhysics->triggerBlockFall(blockPos);
}
```

### Per-Frame Update
```cpp
// In main game loop
blockPhysics->update(deltaTime);
```

### Checking Status
```cpp
size_t activeCount = blockPhysics->getActiveFallingBlockCount();
```

## Configuration Constants

### BlockPhysics
- `COLLISION_THRESHOLD`: 0.5 (distance to detect collision)
- `MAX_IMPACT_PROPAGATION`: 5 (max chain reaction depth)

### FallingBlock
- `GRAVITY`: 9.8
- `MASS_ACCUMULATION_RATE`: 0.10 (10% per block)
- `MAX_MASS_MULTIPLIER`: 3.0 (up to 3x base)
- `INITIAL_MASS_BOOST`: 0.50 (50% boost on fall start)
- `REST_ENERGY_THRESHOLD`: 50.0 (below this, block settles)
- `MAX_DEFLECTIONS`: 8 (safety limit)

### WaterPhysics
- `RIPPLE_SPEED`: 5.0 blocks/sec
- `RIPPLE_DECAY`: 2.0 strength loss/sec
- `MIN_RIPPLE_STRENGTH`: 0.1
- `MAX_RIPPLE_RADIUS`: 8 blocks
- `DISPLACEMENT_THRESHOLD`: 50.0 (min energy)

## Future Enhancements

### Potential Improvements
1. **Performance**: Spatial partitioning for falling blocks
2. **Water**: Full fluid simulation with level equilibrium
3. **Erosion**: Gradual material degradation under stress
4. **Sound**: Impact sound effects based on energy
5. **Particles**: Dust/debris on destruction, splash on water impact
6. **Persistence**: Save/load falling block state
7. **Multiplayer**: Network synchronization of physics events

### Tuning Parameters
All constants can be adjusted to change behavior:
- Increase mass values for heavier feel
- Adjust dampening for bouncier/stickier collisions
- Change deflection angle for different ricochet patterns
- Modify energy thresholds for more/less destruction

## Implementation Notes

### Build System
- Files automatically included via CMake `GLOB_RECURSE`
- No manual CMakeLists.txt changes needed for new physics files

### Dependencies
- ChunkWorld: Block queries and mutations
- BlockRegistry: Block type information
- GLM: Vector math and transformations

### Namespace
All physics components live in `WorldPhysics` namespace

## Testing Recommendations

1. **Basic falling**: Remove block below another, verify fall
2. **Chain reactions**: Drop heavy block (rock) onto lighter materials
3. **Water impact**: Drop blocks into water, check displacement
4. **Deflection**: Create narrow passages, observe ricochet
5. **Mass accumulation**: Drop from great height, observe destruction radius
6. **Underwater settling**: Drop blocks into deep water, verify disappearance

---

*System designed and implemented based on user requirements for realistic mass-based block physics with energy transfer, chain reactions, and water interaction.*

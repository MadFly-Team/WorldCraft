# Physics System Updates

## Issues Fixed

### 1. Blocks Not Falling When Cleared Around
**Problem**: Rocks weren't falling when all surrounding blocks were removed.

**Solution**: Expanded the fall-trigger area to check a 3x3x5 zone (3x3 horizontally, up to 5 blocks above) when a block is removed. This ensures tall columns and surrounding blocks all get checked properly.

**Code Changes**:
- `WorldCraft/src/WorldCraft.cpp` - Block removal now checks wider area with nested loops

## New Features

### 2. Runtime Physics Settings (On-The-Fly)
**Feature**: All physics parameters can now be adjusted during gameplay via the Settings menu.

**Access**: Press ESC → Settings → "Physics Settings"

**Available Settings**:
- Enable Physics (master switch)
- Gravity Multiplier (0.1 - 5.0)
- Mass Gain Rate (0.0 - 0.5)
- Max Mass Multiplier (1.0 - 10.0)
- Energy Dampening (0.5 - 2.0)
- Destruction Threshold (1.0 - 10.0)
- Max Chain Depth (1 - 20)
- Enable Water Physics

**Implementation**:
- `WorldCraft/inc/World/PhysicsSettings.h` - New settings structure
- `WorldCraft/inc/World/BlockPhysics.h` - Added settings member + accessors
- `WorldCraft/inc/UI/SettingsDialog.h` - Added physics settings UI
- `WorldCraft/src/UI/SettingsDialog.cpp` - Implemented `renderPhysicsSettings()`
- `WorldCraft/src/WorldCraft.cpp` - Connected callback to apply settings immediately

### 3. Throw Block Mechanic
**Feature**: Throw the targeted block in the camera direction with velocity based on 2x its mass.

**Controls**: Press **T** key while targeting a block

**Behavior**:
- Removes block from terrain
- Creates FallingBlock entity with initial velocity
- Velocity = 2x block mass × 0.1 (configurable multiplier)
- Direction = camera forward vector
- Full physics simulation applies (mass accumulation, impacts, destruction)

**Restrictions**:
- Cannot throw Air blocks
- Cannot throw Bedrock (Y=0 protection)
- Cannot throw Water/fluid blocks

**Implementation**:
- `WorldCraft/inc/World/FallingBlock.h` - Added `setVelocity()` method
- `WorldCraft/inc/World/BlockPhysics.h` - Added `throwBlock()` method
- `WorldCraft/src/World/BlockPhysics.cpp` - Implemented throw with velocity calculation
- `WorldCraft/src/WorldCraft.cpp` - T key handler in main game loop

## Files Modified

### Created
- `WorldCraft/inc/World/PhysicsSettings.h`

### Modified
- `WorldCraft/inc/World/BlockPhysics.h`
- `WorldCraft/src/World/BlockPhysics.cpp`
- `WorldCraft/inc/World/FallingBlock.h`
- `WorldCraft/inc/UI/SettingsDialog.h`
- `WorldCraft/src/UI/SettingsDialog.cpp`
- `WorldCraft/src/WorldCraft.cpp`
- `WorldCraft/PHYSICS_SYSTEM.md`

## Build Status
✅ **Build Successful** - All changes compile without errors

## Testing Recommendations

1. **Fall trigger fix**:
   - Build a tall pillar of blocks
   - Remove the bottom block
   - Verify entire column falls

2. **Physics settings**:
   - Open Settings → Physics Settings
   - Try different gravity values (0.5, 2.0)
   - Observe blocks fall faster/slower
   - Adjust destruction threshold, watch impact behavior change

3. **Throw mechanic**:
   - Target a rock block
   - Press T
   - Watch it fly in camera direction
   - Try throwing light blocks (grass) vs heavy blocks (rock)
   - Notice velocity difference based on mass

4. **Water interaction**:
   - Throw rocks into water
   - Observe displacement and ripples
   - Toggle "Enable Water Physics" on/off to compare

## User Experience Improvements

- **Immediate feedback**: All settings apply instantly, no restart needed
- **Visual tooltips**: Hover over each setting for explanation
- **Reset button**: Restore defaults with one click
- **Intuitive controls**: T key for throw (easy to reach)
- **Better physics triggers**: More reliable block falling when terrain changes

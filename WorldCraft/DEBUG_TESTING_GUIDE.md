# Debug Testing Guide

## IMPORTANT DISCOVERY

**The issue was camera mode!** Building (placing/removing blocks) only works in **Character mode**, but the game starts in **FreeFly mode**.

## Key Changes Made

1. **Throw now works in ALL camera modes** (FreeFly, Character, Chase)
2. **Building restricted to Character mode only** (by design - like creative mode)
3. **Added camera mode indicators** so you know which mode you're in
4. **Added startup help text** explaining the controls

## Controls

- **C key**: Toggle between FreeFly and Character modes
- **B key**: Toggle between Placement and Removal build modes
- **G key**: Throw the highlighted block (works in ALL modes now!)
- **Left mouse**: Place/remove blocks (CHARACTER MODE ONLY)

## What You'll See at Startup

```
=== WorldCraft Controls ===
[Camera] Starting in FREEFLY mode
[Controls] Press C to toggle between FreeFly and Character modes
[Controls] Building (place/remove) only works in CHARACTER mode
[Controls] Throw (G key) works in ALL modes
[Controls] Build mode (B key): toggle between Placement and Removal
=========================
```

## Testing Steps

### 1. Test Throw Mechanic (Works in FreeFly mode)

1. **Launch the game** - you'll start in FreeFly mode
2. **Look at the floating rock** in your screenshot
3. **Press G** - it should throw!
4. **Watch console for**:
   ```
   [Throw] G key pressed, attempting to throw block
   [Throw] Target block at (X, Y, Z)
   [Throw] Throwing block with direction (...)
   [Throw] Block thrown successfully
   ```

### 2. Test Rock Falling (Need to switch to Character mode)

1. **Press C key** to switch to Character mode
   - Console will show: `[Camera] Switched to CHARACTER mode (building enabled)`
2. **Look at a block near the floating rock**
3. **Left click to remove it** (make sure you're in Removal mode - check red text on screen)
4. **Watch console for**:
   ```
   [BlockRemoval] Checking for falling blocks around removed block at (X, Y, Z)
   [Physics] Block at (...) should fall - below is 0
   [BlockRemoval] Triggered N blocks to fall
   ```

### 3. Test Block Placement

1. **Press B key** until you see "MODE: PLACEMENT" on screen
2. **Make sure you're in Character mode** (press C if needed)
3. **Look at a block and left click** to place adjacent to it
4. The block should place

## Debug Output

### Camera Mode Changes:
```
[Camera] Switched to CHARACTER mode (building enabled)
[Camera] Switched to FREEFLY mode (building disabled, throw enabled)
```

### Throw Mechanic:
```
[Throw] G key pressed, attempting to throw block
[Throw] Target block at (X, Y, Z)
[Throw] Throwing block with direction (X, Y, Z)
[Throw] Block thrown successfully
```

### Physics:
```
[Physics] Physics disabled, block won't fall  (if physics is off)
[Physics] Block at (X, Y, Z) should fall - below is [BlockID]
```

### Block Removal:
```
[BlockRemoval] Checking for falling blocks around removed block at (X, Y, Z)
[BlockRemoval] Triggered N blocks to fall
```

## Expected Behavior

✅ **FreeFly mode (default):**
- Can fly around freely
- Can throw blocks with G key
- CANNOT place/remove blocks

✅ **Character mode (press C):**
- Character physics (gravity, collision)
- Can throw blocks with G key
- CAN place/remove blocks with left mouse
- Rock physics triggers when you remove support blocks

## Summary

The problem was that you were in **FreeFly mode** where building is disabled. Now:
- Press **C** to switch to Character mode for building
- Press **G** to throw blocks (works in any mode now!)
- All physics features work once you're in Character mode

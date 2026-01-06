# Items System Overview

## Introduction

The items system implements a Mario Kart-style power-up system for Kart Mania. Players collect item boxes during races to receive random items that can be used offensively or defensively. The system supports both single-player and multiplayer modes with different item probabilities and behaviors.

---

## Quick Facts

- **Location:** `source/gameplay/items/`
- **Files:** 13 files (5 headers, 8 implementation files)
- **Max Active Items:** 32 simultaneous track items
- **Max Item Boxes:** 6 on Scorching Sands (current map)
- **Supported Maps:** Scorching Sands (others TODO)
- **Multiplayer:** Full network synchronization support

---

## Item Types

### Power-Ups (Player Inventory)

| Item | Type | Effect | Duration |
|------|------|--------|----------|
| **Banana** | Hazard | Drop behind; spins and slows cars on hit | 30 seconds or until hit |
| **Oil Slick** | Hazard | Drop behind; slows cars for 64 pixels | 10 seconds |
| **Bomb** | Hazard | Explodes after delay; knockback in radius | 2 seconds |
| **Green Shell** | Projectile | Straight-line projectile; bounces off walls | Until collision or 20 seconds |
| **Red Shell** | Homing Projectile | Follows racing line; locks onto nearby cars | Until collision or 20 seconds |
| **Missile** | Homing Projectile | Targets 1st place; fastest homing speed | Until collision or 20 seconds |
| **Mushroom** | Self-Effect | Confusion (swapped controls) | 3.5 seconds |
| **Speed Boost** | Self-Effect | 2x max speed increase | 2.5 seconds |

### Special Items

| Item | Type | Effect |
|------|------|--------|
| **Item Box** | Pickup | Gives random item based on rank |
| **ITEM_NONE** | Empty | No item in inventory |

---

## Key Features

### 1. **Rank-Based Probabilities**
Items are distributed based on race position:
- **Leaders** (1st-3rd): Get defensive items (bananas, oil, self-buffs)
- **Mid-pack** (4th-6th): Balanced mix of items
- **Back** (7th-8th): Get powerful offensive items (missiles, red shells)

Different probability tables for single-player vs multiplayer modes.

### 2. **Homing Navigation**
Red shells and missiles use a waypoint-based navigation system:
- Follow the racing line using 119 waypoints
- Lock onto cars within detection range
- Smooth turning with configurable turn rate
- Complete ~1 lap before hitting shooter (immunity system)

### 3. **Multiplayer Synchronization**
All item actions are broadcast to peers:
- Projectile firing (position, angle, speed)
- Hazard placement
- Item box pickups
- Ensures consistent game state across all players

### 4. **Performance Optimizations**
- Collision detection only for on-screen items
- Efficient slot-based item pool (no dynamic allocation)
- Stable OAM sprite mapping
- Distance-based culling for rendering

---

## Architecture

The items system is organized into specialized modules:

```
items/
├── items_types.h          → Type definitions (Item, TrackItem, etc.)
├── items_constants.h      → Game balance constants
├── items_internal.h       → Internal shared state
├── items_api.h           → Public API interface
├── item_navigation.h/c   → Waypoint navigation for homing
│
├── items_state.c         → Lifecycle and state management
├── items_effects.c       → Player status effects
├── items_render.c        → Sprite rendering
├── items_debug.c         → Debug/testing functions
├── items_inventory.c     → Item usage and selection
├── items_spawning.c      → Projectile/hazard spawning
└── items_update.c        → Update loop and collisions
```

---

## Usage Flow

### Initialization
```c
// At gameplay start
Items_LoadGraphics();        // Load sprite graphics into VRAM
Items_Init(ScorchingSands);  // Initialize for specific map
```

### Game Loop
```c
// Every frame (60 FPS)
Items_Update();                              // Update projectiles, timers, respawns
Items_CheckCollisions(cars, carCount, scrollX, scrollY);  // Check item interactions
Items_UpdatePlayerEffects(player, effects); // Update status effect timers
Items_Render(scrollX, scrollY);             // Draw items on screen
```

### Player Interaction
```c
// When player hits item box
if (checkItemBoxPickup(&player, &box)) {
    player.item = Items_GetRandomItem(player.rank);
    PlayBoxSFX();
}

// When player uses item
if (buttonPressed(KEY_L)) {
    Items_UsePlayerItem(&player, fireForward: false);
}
```

### Cleanup
```c
// When leaving gameplay
Items_FreeGraphics();  // Free VRAM
```

---

## Single Player vs Multiplayer

### Single Player Mode
- **Items:** Defensive only (banana, oil, mushroom, speed boost)
- **No offensive projectiles** (shells, missiles, bombs disabled)
- **AI behavior:** TODO:checkout AI_BOTS branch
- **Immunity:** Lap-based for red shells

### Multiplayer Mode
- **Items:** Full item set including offensive projectiles
- **Network sync:** All item actions broadcast to peers
- **Immunity:** Time + distance based for red shells
- **Shooter protection:** Can't hit yourself immediately after firing

---

## Performance Considerations

### Memory Usage
- **Track Items:** 32 slots × ~60 bytes = ~1.9 KB
- **Item Boxes:** 8 slots × ~16 bytes = ~128 bytes
- **Graphics:** ~7 sprite allocations in VRAM
- **Total:** Minimal memory footprint

### Frame Budget
- **Update loop:** ~0.5ms typical (30 FPS safe)
- **Collision checks:** Culled to on-screen items only
- **Rendering:** O(active items), typically <10 sprites

### Network Bandwidth
- **Item placement:** ~12 bytes per action
- **Item box pickup:** 4 bytes per pickup
- **Typical usage:** <100 bytes/second per player

---

## Common Patterns

### Creating a New Item Type
1. Add enum value to `Item` in [items_types.h](../source/gameplay/items/items_types.h)
2. Add probability weights to [items_constants.h](../source/gameplay/items/items_constants.h)
3. Add sprite graphics and palette
4. Implement behavior in `Items_UsePlayerItem()` in [items_inventory.c](../source/gameplay/items/items_inventory.c)
5. Add collision handling in [items_update.c](../source/gameplay/items/items_update.c)
6. Update random selection logic in `Items_GetRandomItem()`

### Adding Item Boxes to a New Map
1. Define spawn locations as `Vec2` array
2. Add case to `initItemBoxSpawns()` in [items_state.c](../source/gameplay/items/items_state.c)
3. Set `itemBoxCount` to number of spawn points
4. Test collision detection and respawn behavior

### Creating Waypoints for a New Map
1. Record racing line positions during test drives
2. Create `Waypoint` array with positions and next indices
3. Add case to `getWaypointsForMap()` in [item_navigation.c](../source/gameplay/items/item_navigation.c)
4. Set waypoint count constant
5. Test red shell and missile navigation

---

## Related Documentation

- **[Items API Reference](items_api.md)** - Detailed function documentation
- **[Items Architecture](items_architecture.md)** - Internal design details
- **[Multiplayer System](multiplayer.md)** - Network synchronization
- **[Sound System](sound.md)** - Box pickup sound effects

---

## Future Enhancements

### Planned Features
- [ ] Dynamic item box spawning based on race progress
- [ ] Item boxes for additional maps (Alpin Rush, Neon Circuit)
- [ ] Triple shell/banana defensive formations
- [ ] Lightning bolt (shrinks all opponents)
- [ ] Star (invincibility + speed boost)

### Performance Improvements
- [ ] Spatial hashing for collision detection
- [ ] Predictive projectile simulation for network lag
- [ ] Item pooling with active/inactive lists

---

## Debugging

### Debug Functions
```c
// Get all item box spawns
int count;
const ItemBoxSpawn* boxes = Items_GetBoxSpawns(&count);

// Get all active track items
const TrackItem* items = Items_GetActiveItems(&count);

// Manually deactivate an item box
Items_DeactivateBox(boxIndex);
```

### Common Issues
- **Items not spawning:** Check map initialization in `Items_Init()`
- **Collisions not working:** Verify scroll offsets are correct
- **Projectiles not homing:** Ensure waypoints exist for current map
- **Multiplayer desync:** Check network message handling in `Items_Update()`

---

## Authors
- **Bahey Shalash**
- **Hugo Svolgaard**

**Version:** 1.0
**Last Updated:** 06.01.2026

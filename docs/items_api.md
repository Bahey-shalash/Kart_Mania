# Items System API Reference

## Overview

This document provides detailed documentation for all public functions in the items system API. For architectural details and usage patterns, see [Items Overview](items_overview.md).

**Header File:** `source/gameplay/items/items_api.h`

---

## Table of Contents

1. [Lifecycle Management](#lifecycle-management)
2. [Item Spawning](#item-spawning)
3. [Player Items and Effects](#player-items-and-effects)
4. [Rendering](#rendering)
5. [Debug and Testing](#debug-and-testing)

---

## Lifecycle Management

### Items_Init
```c
void Items_Init(Map map);
```

**Description:** Initializes the items system for a specific map. Sets up item box spawn locations and clears all active items.

**Parameters:**
- `map` - The map to initialize item boxes for (e.g., `ScorchingSands`)

**Behavior:**
1. Clears all active track items
2. Loads item box spawn locations for the specified map
3. Initializes player effects to zero
4. Graphics pointers are set during `Items_LoadGraphics()`

**When to call:** Once at the start of gameplay, after map selection.

**Example:**
```c
Items_LoadGraphics();
Items_Init(ScorchingSands);
```

**See:** [items_state.c:46-54](../source/gameplay/items/items_state.c#L46-L54)

---

### Items_Reset
```c
void Items_Reset(void);
```

**Description:** Resets the items system to its initial state without changing the map. Clears all active items, reactivates all item boxes, and resets player effects.

**Behavior:**
1. Clears all active track items
2. Reactivates all item boxes (sets `active = true`, `respawnTimer = 0`)
3. Resets player effects (confusion, speed boost, oil slow)

**When to call:** Between races on the same map, or when restarting a race.

**Example:**
```c
// Restart race
Items_Reset();
```

**See:** [items_state.c:56-68](../source/gameplay/items/items_state.c#L56-L68)

---

### Items_Update
```c
void Items_Update(void);
```

**Description:** Updates all active items each frame. Handles projectile movement, homing behavior, lifetime tracking, item box respawning, and multiplayer synchronization.

**Behavior:**
1. Receives and processes multiplayer item placements
2. Updates projectile positions and angles
3. Updates homing behavior for red shells and missiles
4. Decrements lifetime timers and despawns expired items
5. Updates immunity timers (time-based and lap-based)
6. Updates item box respawn timers

**When to call:** Every frame in the main gameplay loop.

**Frame Budget:** ~0.3-0.5ms typical

**Example:**
```c
while (gameActive) {
    Items_Update();
    // ... other updates
    swiWaitForVBlank();
}
```

**See:** [items_update.c:38-141](../source/gameplay/items/items_update.c#L38-L141)

---

### Items_CheckCollisions
```c
void Items_CheckCollisions(Car* cars, int carCount, int scrollX, int scrollY);
```

**Description:** Checks for collisions between items and cars. Processes item box pickups, projectile hits, and hazard interactions.

**Parameters:**
- `cars` - Array of cars to check collisions against
- `carCount` - Number of cars in the array
- `scrollX` - Horizontal camera scroll offset for culling
- `scrollY` - Vertical camera scroll offset for culling

**Behavior:**
1. Checks item box pickups (gives random item to player)
2. Checks projectile collisions (shells, missiles)
3. Checks hazard collisions (bananas, oil, bombs)
4. Only processes items near the visible screen (culling optimization)

**When to call:** Every frame, after `Items_Update()`

**Example:**
```c
Items_CheckCollisions(race->cars, race->carCount, camera.scrollX, camera.scrollY);
```

**See:** [items_update.c:143-147](../source/gameplay/items/items_update.c#L143-L147)

---

## Item Spawning

### Items_FireProjectile
```c
void Items_FireProjectile(Item type, Vec2 pos, int angle512, Q16_8 speed, int targetCarIndex);
```

**Description:** Fires a projectile item (shell or missile) from a position. Broadcasts to multiplayer peers and initializes homing behavior for red shells and missiles.

**Parameters:**
- `type` - Type of projectile (`ITEM_GREEN_SHELL`, `ITEM_RED_SHELL`, `ITEM_MISSILE`)
- `pos` - Starting position (Q16.8 fixed-point)
- `angle512` - Launch angle (0-511, where 512 = 360°)
- `speed` - Projectile speed (Q16.8 fixed-point)
- `targetCarIndex` - Target car for homing (-1 for none)

**Behavior:**
1. Finds inactive item slot in pool
2. Initializes position, speed, angle
3. Sets up homing navigation (waypoints, immunity)
4. Assigns sprite graphics
5. Broadcasts to multiplayer if enabled

**Lifetime:** Projectiles despawn on collision or after timeout

**Example:**
```c
// Fire green shell forward
Vec2 spawnPos = Vec2_Add(player.position, forward_offset);
Q16_8 shellSpeed = FixedMul(player.maxSpeed, GREEN_SHELL_SPEED_MULT);
Items_FireProjectile(ITEM_GREEN_SHELL, spawnPos, player.angle512, shellSpeed, -1);
```

**See:** [items_spawning.c:84-89](../source/gameplay/items/items_spawning.c#L84-L89)

---

### Items_PlaceHazard
```c
void Items_PlaceHazard(Item type, Vec2 pos);
```

**Description:** Places a stationary hazard item on the track (banana, oil slick, or bomb).

**Parameters:**
- `type` - Type of hazard (`ITEM_OIL`, `ITEM_BOMB`, `ITEM_BANANA`)
- `pos` - Position to place the hazard (Q16.8 fixed-point)

**Behavior:**
1. Finds inactive item slot
2. Sets position and type
3. Sets lifetime based on item type:
   - Oil: 10 seconds
   - Bomb: 2 seconds (then explodes)
   - Banana: 30 seconds (or until hit)
4. Assigns sprite graphics
5. Broadcasts to multiplayer if enabled

**Example:**
```c
// Drop banana behind player
Vec2 backward = Vec2_FromAngle((player.angle512 + ANGLE_HALF) & ANGLE_MASK);
Vec2 dropPos = Vec2_Add(player.position, Vec2_Scale(backward, IntToFixed(16)));
Items_PlaceHazard(ITEM_BANANA, dropPos);
```

**See:** [items_spawning.c:132-134](../source/gameplay/items/items_spawning.c#L132-L134)

---

## Player Items and Effects

### Items_UsePlayerItem
```c
void Items_UsePlayerItem(Car* player, bool fireForward);
```

**Description:** Uses the item currently held by the player. Applies item effects based on the item type (drop hazard, fire projectile, apply boost, etc.).

**Parameters:**
- `player` - The player car using the item
- `fireForward` - `true` to fire forward, `false` to fire backward (for shells only)

**Behavior:**
Clears player's inventory and executes item-specific logic:

| Item | Action |
|------|--------|
| Banana/Oil/Bomb | Drops hazard behind player |
| Green Shell | Fires straight in specified direction |
| Red Shell | Fires with homing, follows racing line |
| Missile | Targets 1st place car |
| Mushroom | Applies confusion effect |
| Speed Boost | Applies speed multiplier |

**Example:**
```c
// Fire item forward when L button pressed
if (keysDown() & KEY_L) {
    Items_UsePlayerItem(&player, fireForward: true);
}

// Drop item backward when R button pressed
if (keysDown() & KEY_R) {
    Items_UsePlayerItem(&player, fireForward: false);
}
```

**See:** [items_inventory.c:14-112](../source/gameplay/items/items_inventory.c#L14-L112)

---

### Items_GetRandomItem
```c
Item Items_GetRandomItem(int playerRank);
```

**Description:** Selects a random item based on player rank and game mode. Items are weighted by probability tables (different for single player vs multiplayer).

**Parameters:**
- `playerRank` - Current race position (1 = 1st place, 2 = 2nd, etc.)

**Returns:**
Random item type based on probability distribution

**Behavior:**
1. Clamps rank to 1-8 range
2. Selects probability table (single-player or multiplayer)
3. Generates random number
4. Returns item based on weighted probabilities

**Probability Distribution (Single Player - Defensive Only):**
| Rank | Banana | Oil | Mushroom | Speed Boost |
|------|--------|-----|----------|-------------|
| 1st  | 35%    | 35% | 15%      | 15%         |
| 8th+ | 10%    | 10% | 25%      | 55%         |

**Probability Distribution (Multiplayer - Full Set):**
| Rank | Banana | Oil | Bomb | Green | Red | Missile | Mushroom | Boost |
|------|--------|-----|------|-------|-----|---------|----------|-------|
| 1st  | 17%    | 18% | 5%   | 15%   | 10% | 0%      | 15%      | 20%   |
| 8th+ | 7%     | 7%  | 5%   | 17%   | 17% | 5%      | 17%      | 25%   |

**Example:**
```c
// Player picks up item box
if (hitItemBox) {
    player.item = Items_GetRandomItem(player.rank);
}
```

**See:** [items_inventory.c:114-167](../source/gameplay/items/items_inventory.c#L114-L167)

---

### Items_UpdatePlayerEffects
```c
void Items_UpdatePlayerEffects(Car* player, PlayerItemEffects* effects);
```

**Description:** Updates all active player status effects each frame. Handles timers for confusion, speed boosts, and distance-based oil slows.

**Parameters:**
- `player` - The player car to update effects for
- `effects` - The player's current item effects state

**Behavior:**
1. **Confusion:** Decrements timer, deactivates when expired
2. **Speed Boost:** Decrements timer, restores original max speed when expired
3. **Oil Slow:** Checks distance traveled, deactivates after 64 pixels

**Effect Durations:**
- Confusion: 3.5 seconds
- Speed Boost: 2.5 seconds
- Oil Slow: 64 pixels traveled

**When to call:** Every frame in the main gameplay loop

**Example:**
```c
PlayerItemEffects* effects = Items_GetPlayerEffects();
Items_UpdatePlayerEffects(&player, effects);
```

**See:** [items_effects.c:23-43](../source/gameplay/items/items_effects.c#L23-L43)

---

### Items_GetPlayerEffects
```c
PlayerItemEffects* Items_GetPlayerEffects(void);
```

**Description:** Returns a pointer to the global player effects state.

**Returns:**
Pointer to `PlayerItemEffects` structure

**Usage:** Get current status effects to check state or pass to other functions

**Example:**
```c
PlayerItemEffects* effects = Items_GetPlayerEffects();
if (effects->confusionActive) {
    // Swap control inputs
    handleConfusedControls();
}
```

**See:** [items_effects.c:45-47](../source/gameplay/items/items_effects.c#L45-L47)

---

### Items_ApplyConfusion
```c
void Items_ApplyConfusion(PlayerItemEffects* effects);
```

**Description:** Applies confusion effect to the player (swapped controls from mushroom).

**Parameters:**
- `effects` - The player's item effects state

**Behavior:**
- Sets `confusionActive = true`
- Sets `confusionTimer = MUSHROOM_CONFUSION_DURATION` (3.5 seconds)

**Example:**
```c
// Mushroom item used
Items_ApplyConfusion(effects);
```

**See:** [items_effects.c:49-52](../source/gameplay/items/items_effects.c#L49-L52)

---

### Items_ApplySpeedBoost
```c
void Items_ApplySpeedBoost(Car* player, PlayerItemEffects* effects);
```

**Description:** Applies speed boost effect to the player. Temporarily increases max speed by 2x.

**Parameters:**
- `player` - The player car to apply boost to
- `effects` - The player's item effects state

**Behavior:**
1. Stores original max speed (if not already boosting)
2. Multiplies max speed by `SPEED_BOOST_MULT` (2x)
3. Sets boost timer to 2.5 seconds
4. Multiple boosts extend duration, not stack speed

**Example:**
```c
// Speed boost item used
Items_ApplySpeedBoost(&player, effects);
```

**See:** [items_effects.c:54-61](../source/gameplay/items/items_effects.c#L54-L61)

---

### Items_ApplyOilSlow
```c
void Items_ApplyOilSlow(Car* player, PlayerItemEffects* effects);
```

**Description:** Applies oil slow effect to the player. Reduces speed and tracks distance traveled for duration.

**Parameters:**
- `player` - The player car to slow down
- `effects` - The player's item effects state

**Behavior:**
1. Instantly reduces speed by dividing by `OIL_SPEED_DIVISOR`
2. Marks oil slow as active
3. Records starting position
4. Effect lasts until player travels 64 pixels

**Example:**
```c
// Player hits oil slick
Items_ApplyOilSlow(&player, effects);
```

**See:** [items_effects.c:63-70](../source/gameplay/items/items_effects.c#L63-L70)

---

## Rendering

### Items_Render
```c
void Items_Render(int scrollX, int scrollY);
```

**Description:** Renders all visible items (boxes and track items) to the screen using OAM sprites.

**Parameters:**
- `scrollX` - Horizontal camera scroll offset
- `scrollY` - Vertical camera scroll offset

**Behavior:**
1. **Item Boxes:** Renders active boxes at their spawn positions
2. **Track Items:** Renders projectiles and hazards
3. **Rotation:** Applies affine transformations to shells and missiles
4. **Culling:** Hides offscreen items to save OAM slots
5. **Stable Mapping:** Each item slot maps to a fixed OAM sprite

**OAM Allocation:**
- Slots 1-8: Item boxes
- Slots 9-40: Track items

**When to call:** Every frame during gameplay rendering

**Example:**
```c
Items_Render(camera.scrollX, camera.scrollY);
oamUpdate(&oamMain);
```

**See:** [items_render.c:27-137](../source/gameplay/items/items_render.c#L27-L137)

---

### Items_LoadGraphics
```c
void Items_LoadGraphics(void);
```

**Description:** Loads all item sprite graphics into VRAM. Should be called once during gameplay initialization.

**Behavior:**
1. Allocates sprite graphics for all item types
2. Copies tile data from ROM to VRAM
3. Loads palettes to separate palette slots
4. Updates item box spawn graphics pointers

**Graphics Loaded:**
- Item box (8×8, 16-color)
- Banana (16×16, 16-color)
- Bomb (16×16, 16-color)
- Green shell (16×16, 16-color)
- Red shell (16×16, 16-color)
- Missile (16×32, 16-color)
- Oil slick (32×32, 16-color)

**When to call:** Once before calling `Items_Init()`

**Example:**
```c
// Gameplay initialization
Items_LoadGraphics();
Items_Init(currentMap);
```

**See:** [items_render.c:139-175](../source/gameplay/items/items_render.c#L139-L175)

---

### Items_FreeGraphics
```c
void Items_FreeGraphics(void);
```

**Description:** Frees all item sprite graphics from VRAM. Should be called when leaving gameplay to free resources.

**Behavior:**
- Deallocates all sprite graphics using `oamFreeGfx()`
- Sets all graphics pointers to NULL

**When to call:** When exiting gameplay or switching game states

**Example:**
```c
// Cleanup when leaving gameplay
Items_FreeGraphics();
```

**See:** [items_render.c:177-206](../source/gameplay/items/items_render.c#L177-L206)

---

## Debug and Testing

### Items_GetBoxSpawns
```c
const ItemBoxSpawn* Items_GetBoxSpawns(int* count);
```

**Description:** Returns a pointer to the item box spawn array for debugging.

**Parameters:**
- `count` - Output parameter for number of item boxes

**Returns:**
Pointer to item box spawn array

**Usage:** Debug visualization, testing item box positions

**Example:**
```c
int count;
const ItemBoxSpawn* boxes = Items_GetBoxSpawns(&count);
for (int i = 0; i < count; i++) {
    printf("Box %d at (%d, %d)\n", i,
           FixedToInt(boxes[i].position.x),
           FixedToInt(boxes[i].position.y));
}
```

**See:** [items_debug.c:19-22](../source/gameplay/items/items_debug.c#L19-L22)

---

### Items_GetActiveItems
```c
const TrackItem* Items_GetActiveItems(int* count);
```

**Description:** Returns a pointer to the active items array for debugging.

**Parameters:**
- `count` - Output parameter for number of active items

**Returns:**
Pointer to active items array

**Usage:** Debug visualization, testing collision detection

**Example:**
```c
int count;
const TrackItem* items = Items_GetActiveItems(&count);
printf("Active items: %d\n", count);
```

**See:** [items_debug.c:24-32](../source/gameplay/items/items_debug.c#L24-L32)

---

### Items_DeactivateBox
```c
void Items_DeactivateBox(int boxIndex);
```

**Description:** Manually deactivates an item box and starts its respawn timer. Used for multiplayer synchronization.

**Parameters:**
- `boxIndex` - Index of the item box to deactivate (0 to `itemBoxCount-1`)

**Behavior:**
- Sets box `active = false`
- Sets `respawnTimer = ITEM_BOX_RESPAWN_TICKS` (3 seconds)

**Usage:** Multiplayer sync when remote player picks up a box

**Example:**
```c
// Received item box pickup from network
int boxIndex = Multiplayer_ReceiveItemBoxPickup();
if (boxIndex >= 0) {
    Items_DeactivateBox(boxIndex);
}
```

**See:** [items_update.c:149-156](../source/gameplay/items/items_update.c#L149-L156)

---

## Related Documentation

- **[Items Overview](items_overview.md)** - High-level system overview
- **[Items Architecture](items_architecture.md)** - Internal design details

---

## Authors
- **Bahey Shalash**
- **Hugo Svolgaard**

**Version:** 1.0
**Last Updated:** 06.01.2026


---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)
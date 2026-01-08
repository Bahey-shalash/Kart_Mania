/**
 * File: items_api.h
 * -----------------
 * Description: Public API interface for the items system. Provides functions
 *              for item lifecycle management, spawning, player interaction,
 *              rendering, and debugging. This is the primary interface for
 *              external code to interact with the items system.
 *
 * Item Behaviors:
 *   ITEM_NONE       - Car has no item in inventory
 *   ITEM_BOX        - Gives random item when collected (probabilities by rank)
 *   ITEM_OIL        - Dropped behind; slows cars that run over it; despawns after 10s
 *   ITEM_BOMB       - Dropped behind; explodes after delay; hits all cars in radius
 *   ITEM_BANANA     - Dropped behind; slows and spins cars on hit; despawns on hit
 *   ITEM_GREEN_SHELL - Projectile fired in facing direction; despawns on wall/car hit
 *   ITEM_RED_SHELL  - Homing projectile targeting car ahead; despawns on collision
 *   ITEM_MISSILE    - Targets 1st place directly; despawns on hit
 *   ITEM_MUSHROOM   - Applies confusion (swapped controls) for several seconds
 *   ITEM_SPEEDBOOST - Temporary speed increase for several seconds
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef ITEMS_API_H
#define ITEMS_API_H

#include "items_types.h"
#include "items_constants.h"

// Forward declaration to avoid circular include with Car.h
typedef struct Car Car;

//=============================================================================
// Lifecycle Management
//=============================================================================

/**
 * Function: Items_Init
 * --------------------
 * Initializes the items system for a specific map. Sets up item box spawn
 * locations and clears all active items.
 *
 * Parameters:
 *   map - The map to initialize item boxes for
 */
void Items_Init(Map map);

/**
 * Function: Items_Reset
 * ---------------------
 * Resets the items system to its initial state. Clears all active items,
 * reactivates all item boxes, and resets player effects.
 */
void Items_Reset(void);

/**
 * Function: Items_Update
 * ----------------------
 * Updates all active items each frame. Handles projectile movement, homing
 * behavior, lifetime tracking, item box respawning, and multiplayer
 * synchronization.
 */
void Items_Update(void);

/**
 * Function: Items_CheckCollisions
 * --------------------------------
 * Checks for collisions between items and cars. Processes item box pickups,
 * projectile hits, and hazard interactions.
 *
 * Parameters:
 *   cars    - Array of cars to check collisions against
 *   carCount - Number of cars in the array
 *   scrollX - Horizontal scroll offset for culling
 *   scrollY - Vertical scroll offset for culling
 */
void Items_CheckCollisions(Car* cars, int carCount, int scrollX, int scrollY);

//=============================================================================
// Item Spawning
//=============================================================================

/**
 * Function: Items_FireProjectile
 * -------------------------------
 * Fires a projectile item (shell or missile) from a position.
 *
 * Parameters:
 *   type           - Type of projectile (GREEN_SHELL, RED_SHELL, MISSILE)
 *   pos            - Starting position
 *   angle512       - Launch angle (512-based angle system)
 *   speed          - Projectile speed (Q16.8 fixed-point)
 *   targetCarIndex - Target car for homing (-1 for none)
 */
void Items_FireProjectile(Item type, const Vec2* pos, int angle512, Q16_8 speed,
                          int targetCarIndex);

/**
 * Function: Items_PlaceHazard
 * ---------------------------
 * Places a stationary hazard item on the track.
 *
 * Parameters:
 *   type - Type of hazard (OIL, BOMB, BANANA)
 *   pos  - Position to place the hazard
 */
void Items_PlaceHazard(Item type, const Vec2* pos);

//=============================================================================
// Player Items and Effects
//=============================================================================

/**
 * Function: Items_UsePlayerItem
 * ------------------------------
 * Uses the item currently held by the player. Applies item effects based
 * on the item type (drop hazard, fire projectile, apply boost, etc.).
 *
 * Parameters:
 *   player      - The player car using the item
 *   fireForward - true to fire forward, false to fire backward (for shells)
 */
void Items_UsePlayerItem(Car* player, bool fireForward);

/**
 * Function: Items_GetRandomItem
 * ------------------------------
 * Selects a random item based on player rank and game mode. Items are
 * weighted by probability tables (different for single player vs multiplayer).
 *
 * Parameters:
 *   playerRank - Current race position (1st, 2nd, 3rd, etc.)
 *
 * Returns:
 *   Random item type based on probability distribution
 */
Item Items_GetRandomItem(int playerRank);

/**
 * Function: Items_UpdatePlayerEffects
 * ------------------------------------
 * Updates all active player status effects each frame. Handles timers for
 * confusion, speed boosts, and distance-based oil slows.
 *
 * Parameters:
 *   player  - The player car to update effects for
 *   effects - The player's current item effects state
 */
void Items_UpdatePlayerEffects(Car* player, PlayerItemEffects* effects);

/**
 * Function: Items_GetPlayerEffects
 * ---------------------------------
 * Returns a pointer to the global player effects state.
 *
 * Returns:
 *   Pointer to PlayerItemEffects structure
 */
PlayerItemEffects* Items_GetPlayerEffects(void);

/**
 * Function: Items_ApplyConfusion
 * -------------------------------
 * Applies confusion effect to the player (swapped controls from mushroom).
 *
 * Parameters:
 *   effects - The player's item effects state
 */
void Items_ApplyConfusion(PlayerItemEffects* effects);

/**
 * Function: Items_ApplySpeedBoost
 * --------------------------------
 * Applies speed boost effect to the player. Temporarily increases max speed.
 *
 * Parameters:
 *   player  - The player car to apply boost to
 *   effects - The player's item effects state
 */
void Items_ApplySpeedBoost(Car* player, PlayerItemEffects* effects);

/**
 * Function: Items_ApplyOilSlow
 * ----------------------------
 * Applies oil slow effect to the player. Reduces speed and tracks distance
 * traveled for duration.
 *
 * Parameters:
 *   player  - The player car to slow down
 *   effects - The player's item effects state
 */
void Items_ApplyOilSlow(Car* player, PlayerItemEffects* effects);

//=============================================================================
// Rendering
//=============================================================================

/**
 * Function: Items_Render
 * -----------------------
 * Renders all visible items (boxes and track items) to the screen.
 *
 * Parameters:
 *   scrollX - Horizontal scroll offset for camera
 *   scrollY - Vertical scroll offset for camera
 */
void Items_Render(int scrollX, int scrollY);

/**
 * Function: Items_LoadGraphics
 * ----------------------------
 * Loads all item sprite graphics into VRAM. Should be called once during
 * gameplay initialization.
 */
void Items_LoadGraphics(void);

/**
 * Function: Items_FreeGraphics
 * ----------------------------
 * Frees all item sprite graphics from VRAM. Should be called when leaving
 * gameplay to free resources.
 */
void Items_FreeGraphics(void);

//=============================================================================
// Debug and Testing API
//=============================================================================

/**
 * Function: Items_GetBoxSpawns
 * ----------------------------
 * Returns a pointer to the item box spawn array for debugging.
 *
 * Parameters:
 *   count - Output parameter for number of item boxes
 *
 * Returns:
 *   Pointer to item box spawn array
 */
const ItemBoxSpawn* Items_GetBoxSpawns(int* count);

/**
 * Function: Items_GetActiveItems
 * -------------------------------
 * Returns a pointer to the active items array for debugging.
 *
 * Parameters:
 *   count - Output parameter for number of active items
 *
 * Returns:
 *   Pointer to active items array
 */
const TrackItem* Items_GetActiveItems(int* count);

/**
 * Function: Items_DeactivateBox
 * ------------------------------
 * Manually deactivates an item box and starts its respawn timer.
 * Used for multiplayer synchronization.
 *
 * Parameters:
 *   boxIndex - Index of the item box to deactivate
 */
void Items_DeactivateBox(int boxIndex);

/**
 * Function: Items_SpawnBoxes
 * --------------------------
 * Declared but currently unused in the implementation.
 * Reserved for future dynamic item box spawning.
 */
void Items_SpawnBoxes(void);

#endif  // ITEMS_API_H

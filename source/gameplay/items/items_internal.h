/**
 * File: items_internal.h
 * ----------------------
 * Description: Internal shared state and helper functions for the items system.
 *              This file defines module-level state variables and internal
 *              functions that are shared across multiple .c files but not
 *              exposed to the public API.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef ITEMS_INTERNAL_H
#define ITEMS_INTERNAL_H

#include "items_types.h"
#include "items_constants.h"

//=============================================================================
// Shared Module State
//=============================================================================
extern TrackItem activeItems[MAX_TRACK_ITEMS];
extern ItemBoxSpawn itemBoxSpawns[MAX_ITEM_BOX_SPAWNS];
extern int itemBoxCount;
extern PlayerItemEffects playerEffects;

//=============================================================================
// Graphics Pointers
//=============================================================================
extern u16* itemBoxGfx;
extern u16* bananaGfx;
extern u16* bombGfx;
extern u16* greenShellGfx;
extern u16* redShellGfx;
extern u16* missileGfx;
extern u16* oilSlickGfx;

//=============================================================================
// Internal Helper Functions
//=============================================================================

/**
 * Function: fireProjectileInternal
 * ---------------------------------
 * Internal version of Items_FireProjectile with additional control over
 * network broadcasting and shooter tracking.
 *
 * Parameters:
 *   type            - Type of projectile to fire
 *   pos             - Starting position
 *   angle512        - Launch angle (512-based angle system)
 *   speed           - Projectile speed (Q16.8 fixed-point)
 *   targetCarIndex  - Target car for homing projectiles (-1 for none)
 *   sendNetwork     - Whether to broadcast to multiplayer peers
 *   shooterCarIndex - Index of car that fired the projectile
 */
void fireProjectileInternal(Item type, const Vec2* pos, int angle512, Q16_8 speed,
                            int targetCarIndex, bool sendNetwork,
                            int shooterCarIndex);

/**
 * Function: placeHazardInternal
 * ------------------------------
 * Internal version of Items_PlaceHazard with additional control over
 * network broadcasting.
 *
 * Parameters:
 *   type        - Type of hazard to place (banana, oil, bomb)
 *   pos         - Position to place the hazard
 *   sendNetwork - Whether to broadcast to multiplayer peers
 */
void placeHazardInternal(Item type, const Vec2* pos, bool sendNetwork);

#endif  // ITEMS_INTERNAL_H

/**
 * File: items_types.h
 * -------------------
 * Description: Type definitions for the items system. Defines all item types,
 *              probabilities, track item state, item box spawns, and player
 *              status effects for the kart racing game's power-up system.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef ITEMS_TYPES_H
#define ITEMS_TYPES_H

#include <nds.h>
#include <stdbool.h>

#include "../../math/fixedmath.h"
#include "../../core/game_types.h"

//=============================================================================
// Item Types
//=============================================================================

/**
 * Enum: Item
 * ----------
 * Defines all available item types in the game. Each item has unique
 * behavior when picked up or collided with during gameplay.
 */
typedef enum {
    ITEM_NONE = 0,
    ITEM_BOX,
    ITEM_OIL,
    ITEM_BOMB,
    ITEM_BANANA,
    ITEM_GREEN_SHELL,
    ITEM_RED_SHELL,
    ITEM_MISSILE,
    ITEM_MUSHROOM,
    ITEM_SPEEDBOOST
} Item;

/**
 * Struct: ItemProbability
 * -----------------------
 * Probability distribution for item drops based on player rank.
 * All values represent relative weights (not percentages).
 */
typedef struct {
    int banana;
    int oil;
    int bomb;
    int greenShell;
    int redShell;
    int missile;
    int mushroom;
    int speedBoost;
} ItemProbability;

//=============================================================================
// Track Items
//=============================================================================

/**
 * Struct: TrackItem
 * -----------------
 * Represents an active item on the track (projectile or hazard).
 * Includes position, movement, collision data, and special behavior flags.
 */
typedef struct {
    Item type;
    Vec2 position;
    Vec2 startPosition;  // For oil slick distance tracking
    Q16_8 speed;
    int angle512;
    int hitbox_width;
    int hitbox_height;
    int lifetime_ticks;
    int targetCarIndex;  // For homing missiles/red shells (-1 = none)
    bool active;
    u16* gfx;  // Sprite graphics pointer

    int currentWaypoint;    // Which waypoint we're heading toward
    int waypointsVisited;   // Counter to prevent infinite loops
    bool usePathFollowing;  // true = follow waypoints, false = direct homing

    // Shooter immunity (for homing projectiles only)
    int shooterCarIndex;  // Who fired this projectile (-1 = no shooter)
    int immunityTimer;    // Frames of immunity remaining

    // Lap-based immunity (single player only) red shell hits you
    int startingWaypoint;  // Waypoint where projectile spawned
    bool hasCompletedLap;  // True after completing full lap
} TrackItem;

//=============================================================================
// Item Boxes
//=============================================================================

/**
 * Struct: ItemBoxSpawn
 * --------------------
 * Represents a fixed item box spawn location on the track. Item boxes
 * give random items when collected and respawn after a delay.
 */
typedef struct {
    Vec2 position;
    bool active;       // Is box available for pickup?
    int respawnTimer;  // Ticks until respawn
    u16* gfx;          // Sprite graphics pointer
} ItemBoxSpawn;

//=============================================================================
// Player Effects
//=============================================================================

/**
 * Struct: PlayerItemEffects
 * -------------------------
 * Tracks temporary status effects applied to the player from items.
 * Effects include confusion, speed boosts, and oil slows.
 */
typedef struct {
    bool confusionActive;  // Mushroom confusion (swapped controls)
    int confusionTimer;
    bool speedBoostActive;
    int speedBoostTimer;
    Q16_8 originalMaxSpeed;  // Store original before boost
    bool oilSlowActive;      // Currently sliding on oil
    Vec2 oilSlowStart;       // Position where oil slow started
} PlayerItemEffects;

#endif  // ITEMS_TYPES_H

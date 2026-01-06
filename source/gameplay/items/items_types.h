#ifndef ITEMS_TYPES_H
#define ITEMS_TYPES_H

#include <nds.h>
#include <stdbool.h>

#include "../../math/fixedmath.h"
#include "../../core/game_types.h"

// Track item kind
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

// Track item state
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

// Item box spawn location
typedef struct {
    Vec2 position;
    bool active;       // Is box available for pickup?
    int respawnTimer;  // Ticks until respawn
    u16* gfx;          // Sprite graphics pointer
} ItemBoxSpawn;

// Player status effects
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

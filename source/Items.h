#ifndef ITEMS_H
#define ITEMS_H

/*
 * Item Behaviors:
 *
 * ITEM_NONE: Car has no item in inventory
 *
 * ITEM_BOX: Spawns on track every ~3 seconds at fixed locations. When hit,
 *           car receives random item (probabilities affected by race position).
 *           If car already has item, box is wasted.
 *
 * ITEM_OIL: Placed behind car. Slows down cars that run over it.
 *           Despawns after 10 seconds.
 *
 * ITEM_BOMB: Placed behind car. Explodes after delay, damaging all cars
 *            within radius. Despawns after explosion.
 *
 * ITEM_BANANA: Placed behind car. Slows down car on hit. Despawns on hit.
 *
 * ITEM_GREEN_SHELL: Projectile fired in car's facing direction.
 *                   Despawns on wall or car collision.
 *
 * ITEM_RED_SHELL: Homing projectile that targets car in front. Cannot miss.
 *                 Despawns on collision.
 *
 * ITEM_MISSILE: Targets 1st place car directly. Despawns on hit.
 *
 * ITEM_MUSHROOM: Inverts left/right controls for 5 seconds (confusion effect).
 *
 * ITEM_SPEEDBOOST: Temporary speed increase for several seconds.
 */

#include <nds.h>
#include <stdbool.h>

#include "game_types.h"
#include "timer.h"
#include "vect2.h"

// Forward declaration to avoid circular include with Car.h
typedef struct Car Car;

//=============================================================================
// Constants
//=============================================================================

// Pool sizes
#define MAX_TRACK_ITEMS 32
#define MAX_ITEM_BOX_SPAWNS 8

// OAM sprite allocation
#define ITEM_BOX_OAM_START 1
#define TRACK_ITEM_OAM_START 9

// Durations (all dependent on RACE_TICK_FREQ for easy tuning)
#define SPEED_BOOST_DURATION (int)(2.5f * RACE_TICK_FREQ)         // 2.5 seconds
#define MUSHROOM_CONFUSION_DURATION (int)(3.5f * RACE_TICK_FREQ)  // 3.5 seconds
#define OIL_LIFETIME_TICKS (10 * RACE_TICK_FREQ)                  // 10 seconds
#define ITEM_BOX_RESPAWN_TICKS (3 * RACE_TICK_FREQ)               // 3 seconds
#define OIL_SLOW_DISTANCE IntToFixed(64)  // 64 pixels of slowdown

// Speed multipliers (relative to car max speed)
#define GREEN_SHELL_SPEED_MULT FixedMul(IntToFixed(15), IntToFixed(1)) / 10  // 1.5x
#define RED_SHELL_SPEED_MULT FixedMul(IntToFixed(15), IntToFixed(1)) / 10    // 1.5x
#define MISSILE_SPEED_MULT FixedMul(IntToFixed(17), IntToFixed(1)) / 10      // 1.7x
#define SPEED_BOOST_MULT IntToFixed(2)  // 2x max speed

// Effect values
#define BOMB_EXPLOSION_RADIUS IntToFixed(50)  // 50 pixels
#define BOMB_KNOCKBACK_IMPULSE IntToFixed(8)  // Impulse strength

// Hitbox sizes (in pixels)
#define ITEM_BOX_HITBOX 8
#define OIL_SLICK_HITBOX 32
#define BOMB_HITBOX 16
#define SHELL_HITBOX 16
#define BANANA_HITBOX 16
#define MISSILE_HITBOX_W 16
#define MISSILE_HITBOX_H 32

//=============================================================================
// Item Probability Tables (by race position)
//=============================================================================
// Format: [position][item_type] = probability (0-100)
// Positions: 0=1st, 1=2nd, ..., 7=8th+

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

// Probability distributions by rank
static const ItemProbability ITEM_PROBABILITIES[8] = {
    // 1st place - Defensive items
    {30, 30, 10, 10, 0, 0, 10, 10},  // Banana, oil, bomb dominant

    // 2nd place - Mostly defensive
    {25, 25, 15, 15, 5, 0, 10, 5},

    // 3rd place - Balanced
    {20, 20, 10, 20, 15, 0, 10, 5},

    // 4th place - Slightly offensive
    {15, 15, 10, 20, 20, 5, 10, 5},

    // 5th place - Offensive
    {10, 10, 10, 15, 25, 10, 10, 10},

    // 6th place - More offensive
    {5, 5, 5, 15, 30, 15, 15, 10},

    // 7th place - Very offensive
    {5, 5, 5, 10, 25, 20, 15, 15},

    // 8th+ place - Maximum offense
    {5, 5, 5, 10, 20, 25, 15, 15}};

//=============================================================================
// Data Structures
//=============================================================================

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

//=============================================================================
// Public API - Lifecycle
//=============================================================================

/**
 * Initialize item system for the given map
 * - Loads item box spawn locations
 * - Clears all active track items
 * - Initializes sprite graphics
 */
void Items_Init(Map map);

/**
 * Reset item system (for race restart)
 * - Clears all active items
 * - Respawns all item boxes
 */
void Items_Reset(void);

/**
 * Update all items (called from Race_Tick at 60Hz)
 * - Updates projectile positions
 * - Handles homing behavior
 * - Checks expiration timers
 * - Updates item box respawn timers
 */
void Items_Update(void);

/**
 * Check collisions between cars and items
 * - Item box pickups
 * - Hazard hits (banana, oil, bomb)
 * - Projectile hits (shells, missile)
 */
void Items_CheckCollisions(Car* cars, int carCount);

/**
 * Spawn/respawn item boxes
 * Called from Items_Update when respawn timers expire
 */
void Items_SpawnBoxes(void);

/**
 * Use/deploy the player's current item
 * Called when player presses KEY_R
 *
 * @param player - The player's car
 * @param fireForward - true = forward, false = backward (from KEY_UP/DOWN)
 */
void Items_UsePlayerItem(Car* player, bool fireForward);

/**
 * Get random item weighted by player's race position
 *
 * @param playerRank - 1=1st place, 2=2nd, etc.
 * @return Random item based on probability table
 */
Item Items_GetRandomItem(int playerRank);

//=============================================================================
// Public API - Item Spawning
//=============================================================================

/**
 * Fire a projectile (green/red shell, missile)
 *
 * @param type - ITEM_GREEN_SHELL, ITEM_RED_SHELL, or ITEM_MISSILE
 * @param pos - Starting position
 * @param angle512 - Firing direction
 * @param speed - Projectile speed
 * @param targetCarIndex - For homing (red shell/missile), -1 for none
 */
void Items_FireProjectile(Item type, Vec2 pos, int angle512, Q16_8 speed,
                          int targetCarIndex);

/**
 * Place a hazard on the track (banana, bomb, oil)
 *
 * @param type - ITEM_BANANA, ITEM_BOMB, or ITEM_OIL
 * @param pos - Position to place hazard
 */
void Items_PlaceHazard(Item type, Vec2 pos);

//=============================================================================
// Public API - Player Effects
//=============================================================================

/**
 * Update player status effects (confusion, speed boost, oil slow)
 * Called from Race_Tick for the player car
 *
 * @param player - The player's car
 * @param effects - Player's effect state
 */
void Items_UpdatePlayerEffects(Car* player, PlayerItemEffects* effects);

/**
 * Get player's effect state
 * @return Pointer to player effects (read-only)
 */
const PlayerItemEffects* Items_GetPlayerEffects(void);

/**
 * Apply confusion effect (mushroom)
 * @param effects - Player's effect state
 */
void Items_ApplyConfusion(PlayerItemEffects* effects);

/**
 * Apply speed boost effect
 * @param player - The player's car
 * @param effects - Player's effect state
 */
void Items_ApplySpeedBoost(Car* player, PlayerItemEffects* effects);

/**
 * Apply oil slick slowdown
 * @param player - The player's car
 * @param effects - Player's effect state
 */
void Items_ApplyOilSlow(Car* player, PlayerItemEffects* effects);

//=============================================================================
// Public API - Rendering (called from Gameplay_OnVBlank)
//=============================================================================

/**
 * Render all active items and item boxes
 * Updates OAM with sprite positions/rotations
 *
 * @param scrollX - Camera scroll X
 * @param scrollY - Camera scroll Y
 */
void Items_Render(int scrollX, int scrollY);

/**
 * Load item sprite graphics into VRAM
 * Called from configureItems() in gameplay.c
 */
void Items_LoadGraphics(void);

//=============================================================================
// Debug/Testing API (optional - can remove for production)
//=============================================================================

/**
 * Get current item box states (for debugging)
 */
const ItemBoxSpawn* Items_GetBoxSpawns(int* count);

/**
 * Get active track items (for debugging)
 */
const TrackItem* Items_GetActiveItems(int* count);

#endif  // ITEMS_H

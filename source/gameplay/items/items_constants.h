#ifndef ITEMS_CONSTANTS_H
#define ITEMS_CONSTANTS_H

#include "../../core/timer.h"
#include "../../core/game_constants.h"
#include "../../math/fixedmath.h"

#include "items_types.h"

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
#define GREEN_SHELL_SPEED_MULT FixedDiv(IntToFixed(3), IntToFixed(2))  // 1.5x
#define RED_SHELL_SPEED_MULT FixedDiv(IntToFixed(6), IntToFixed(5))    // 1.2x
#define MISSILE_SPEED_MULT FixedDiv(IntToFixed(17), IntToFixed(10))    // 1.7x
#define SPEED_BOOST_MULT \
    IntToFixed(2)  // x2 //FixedDiv(IntToFixed(3), IntToFixed(2))  // 1.5x max speed
// Note: Applying a second speed boost while one is active will reset
// the timer to full duration but maintain the original maxSpeed reference,
// so multiple boosts extend duration rather than multiply speed further.

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

// Probability distributions by rank - Single Player (only banana/oil/mushroom/speed)
static const ItemProbability ITEM_PROBABILITIES_SP[8] = {
    {35, 35, 0, 0, 0, 0, 15, 15},  // 1st
    {30, 30, 0, 0, 0, 0, 20, 20},  // 2nd
    {25, 25, 0, 0, 0, 0, 20, 30},  // 3rd
    {20, 20, 0, 0, 0, 0, 20, 40},  // 4th
    {18, 18, 0, 0, 0, 0, 19, 45},  // 5th
    {15, 15, 0, 0, 0, 0, 20, 50},  // 6th
    {12, 12, 0, 0, 0, 0, 21, 55},  // 7th
    {10, 10, 0, 0, 0, 0, 25, 55}   // 8th+
};

// Probability distributions by rank - Multiplayer (full set)
static const ItemProbability ITEM_PROBABILITIES_MP[8] = {
    {17, 18, 5, 15, 10, 0, 15, 20},  // 1st
    {17, 17, 5, 16, 12, 0, 13, 20},  // 2nd
    {15, 15, 5, 15, 15, 0, 12, 23},  // 3rd
    {13, 13, 5, 17, 17, 0, 10, 25},  // 4th
    {12, 12, 5, 18, 18, 0, 10, 25},  // 5th
    {10, 10, 5, 18, 18, 0, 14, 25},  // 6th
    {8, 8, 5, 18, 18, 0, 18, 25},    // 7th
    {7, 7, 5, 17, 17, 5, 17, 25}     // 8th+
};

#endif  // ITEMS_CONSTANTS_H

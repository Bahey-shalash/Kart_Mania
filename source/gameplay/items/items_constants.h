/**
 * File: items_constants.h
 * -----------------------
 * Description: Constants and configuration values for the items system.
 *              Defines pool sizes, durations, speed multipliers, effect values,
 *              hitbox sizes, and probability distributions for item drops.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef ITEMS_CONSTANTS_H
#define ITEMS_CONSTANTS_H

#include "../../core/timer.h"
#include "../../core/game_constants.h"
#include "../../math/fixedmath.h"

#include "items_types.h"

//=============================================================================
// Pool Sizes and OAM Allocation
//=============================================================================
#define MAX_TRACK_ITEMS 32
#define MAX_ITEM_BOX_SPAWNS 8

#define ITEM_BOX_OAM_START 1
#define TRACK_ITEM_OAM_START 9

//=============================================================================
// Durations
//=============================================================================
// All durations are dependent on RACE_TICK_FREQ for easy tuning
#define SPEED_BOOST_DURATION (int)(2.5f * RACE_TICK_FREQ)         // 2.5 seconds
#define MUSHROOM_CONFUSION_DURATION (int)(3.5f * RACE_TICK_FREQ)  // 3.5 seconds
#define OIL_LIFETIME_TICKS (10 * RACE_TICK_FREQ)                  // 10 seconds
#define ITEM_BOX_RESPAWN_TICKS (3 * RACE_TICK_FREQ)               // 3 seconds
#define OIL_SLOW_DISTANCE IntToFixed(64)  // 64 pixels of slowdown

//=============================================================================
// Speed Multipliers
//=============================================================================
// All multipliers are relative to car max speed
#define GREEN_SHELL_SPEED_MULT FixedDiv(IntToFixed(3), IntToFixed(2))  // 1.5x
#define RED_SHELL_SPEED_MULT FixedDiv(IntToFixed(6), IntToFixed(5))    // 1.2x
#define MISSILE_SPEED_MULT FixedDiv(IntToFixed(17), IntToFixed(10))    // 1.7x
#define SPEED_BOOST_MULT IntToFixed(2)                                 // 2x max speed

// Note: Applying a second speed boost while one is active will reset
// the timer to full duration but maintain the original maxSpeed reference,
// so multiple boosts extend duration rather than multiply speed further.

//=============================================================================
// Effect Values
//=============================================================================
#define BOMB_EXPLOSION_RADIUS IntToFixed(50)  // 50 pixels
#define BOMB_KNOCKBACK_IMPULSE IntToFixed(8)  // Impulse strength

//=============================================================================
// Hitbox Sizes
//=============================================================================
#define ITEM_BOX_HITBOX 8
#define OIL_SLICK_HITBOX 32
#define BOMB_HITBOX 16
#define SHELL_HITBOX 16
#define BANANA_HITBOX 16
#define MISSILE_HITBOX_W 16
#define MISSILE_HITBOX_H 32

//=============================================================================
// Item Probability Distributions
//=============================================================================

/**
 * ITEM_PROBABILITIES_SP - Single Player Mode
 * -------------------------------------------
 * Balanced for solo play with AI opponents. Only includes defensive items
 * (banana, oil, mushroom, speed boost). No offensive projectiles.
 */
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

/**
 * ITEM_PROBABILITIES_MP - Multiplayer Mode
 * -----------------------------------------
 * Balanced for competitive multiplayer. Includes full item set with
 * offensive projectiles (shells, bombs, missiles) for player vs player combat.
 */
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

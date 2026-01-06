/**
 * File: game_types.h
 * ------------------
 * Description: Core game-wide type definitions. Contains enums and structs used
 *              across multiple modules. UI-specific types are defined locally
 *              in their respective implementation files.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <nds.h>
#include <stdbool.h>
#include "../math/fixedmath.h"

//=============================================================================
// GAME STATE MACHINE
//=============================================================================

/**
 * Main game state machine states.
 *
 * State flow:
 *   HOME_PAGE → {MAPSELECTION, MULTIPLAYER_LOBBY, SETTINGS}
 *   MAPSELECTION → GAMEPLAY
 *   MULTIPLAYER_LOBBY → GAMEPLAY
 *   GAMEPLAY → {PLAYAGAIN, HOME_PAGE}
 *   PLAYAGAIN → {HOME_PAGE, GAMEPLAY}
 *   SETTINGS → HOME_PAGE
 *   REINIT_HOME → HOME_PAGE (special: forces full reinit after WiFi failure)
 */
typedef enum {
    HOME_PAGE,           // Main menu
    MAPSELECTION,        // Track selection screen (single player)
    MULTIPLAYER_LOBBY,   // Multiplayer waiting room
    GAMEPLAY,            // Active racing
    PLAYAGAIN,           // Post-race options (retry/home)
    SETTINGS,            // Settings menu (WiFi, audio toggles)
    REINIT_HOME          // Forces home page reinit after WiFi failure
} GameState;

//=============================================================================
// MAP DEFINITIONS
//=============================================================================

/**
 * Available race tracks.
 * NONEMAP is a sentinel value for "no map selected".
 */
typedef enum {
    NONEMAP,          // No map selected (sentinel value)
    ScorchingSands,   // Desert track (Lap count: 2)
    AlpinRush,        // Mountain track (Lap count: 10)
    NeonCircuit       // City track (Lap count: 10)
} Map;

//=============================================================================
// QUADRANT SYSTEM
//=============================================================================

/**
 * Quadrant IDs for 3×3 map grid.
 *
 * The 1024×1024 world is divided into 9 quadrants of 256×256 pixels each:
 *
 *   TL | TC | TR
 *   ---+----+---
 *   ML | MC | MR
 *   ---+----+---
 *   BL | BC | BR
 *
 * Used for dynamic map loading to conserve VRAM.
 */
typedef enum {
    QUAD_TL = 0,  // Top-Left
    QUAD_TC = 1,  // Top-Center
    QUAD_TR = 2,  // Top-Right
    QUAD_ML = 3,  // Middle-Left
    QUAD_MC = 4,  // Middle-Center
    QUAD_MR = 5,  // Middle-Right
    QUAD_BL = 6,  // Bottom-Left
    QUAD_BC = 7,  // Bottom-Center
    QUAD_BR = 8   // Bottom-Right
} QuadrantID;

#endif  // GAME_TYPES_H

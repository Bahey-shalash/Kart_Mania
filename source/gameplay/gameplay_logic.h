/**
 * File: gameplay_logic.h
 * ----------------------
 * Description: Core racing logic and physics simulation. Manages race state,
 *              car updates, countdown system, checkpoint progression, finish
 *              line detection, and multiplayer synchronization. Separate from
 *              graphics (gameplay.c handles rendering).
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef GAMEPLAY_LOGIC_H
#define GAMEPLAY_LOGIC_H

#include <stdbool.h>

#include "../core/game_types.h"
#include "Car.h"
#include "items/items_api.h"
#include "../core/game_types.h"
#include "wall_collision.h"

//=============================================================================
// PUBLIC CONSTANTS
//=============================================================================

// Note: MAX_CARS, MAX_CHECKPOINTS, QUAD_OFFSET, FINISH_DELAY_FRAMES moved to game_constants.h

//=============================================================================
// PUBLIC TYPES
//=============================================================================

/**
 * Game mode for race initialization.
 */
typedef enum {
    SinglePlayer,  // Solo racing against time
    MultiPlayer    // Networked racing (up to 8 players)
} GameMode;

/**
 * Countdown sequence states before race starts.
 */
typedef enum {
    COUNTDOWN_3 = 0,       // Display "3"
    COUNTDOWN_2,           // Display "2"
    COUNTDOWN_1,           // Display "1"
    COUNTDOWN_GO,          // Display "GO!"
    COUNTDOWN_FINISHED     // Race started, countdown done
} CountdownState;

/**
 * Axis-aligned bounding box for checkpoint detection.
 */
typedef struct {
    Vec2 topLeft;
    Vec2 bottomRight;
} CheckpointBox;

/**
 * Complete race state including all cars, lap tracking, and finish status.
 */
typedef struct {
    bool raceStarted;          // Race has been initialized
    bool raceFinished;         // Race completed (all laps done)

    GameMode gameMode;         // SinglePlayer or MultiPlayer
    Map currentMap;            // Current track being raced

    int carCount;              // Number of cars in race (1 for single, up to 8 for multi)
    int playerIndex;           // Index of local player (0 for single, varies for multi)
    Car cars[MAX_CARS];        // All car states

    int totalLaps;             // Laps required to complete race

    int checkpointCount;       // Number of checkpoints (currently unused)
    CheckpointBox checkpoints[MAX_CHECKPOINTS];

    int finishDelayTimer;      // Frames to wait before showing end screen (5 seconds)
    int finalTimeMin;          // Total race time (minutes)
    int finalTimeSec;          // Total race time (seconds, 0-59)
    int finalTimeMsec;         // Total race time (milliseconds, 0-999)
} RaceState;

//=============================================================================
// PUBLIC API - Lifecycle
//=============================================================================

/**
 * Function: Race_Init
 * -------------------
 * Initializes race state for a new race.
 *
 * Sets up:
 *   - Car spawns (single or multiplayer positioning)
 *   - Lap count (map-specific or 5 laps for multiplayer)
 *   - Countdown sequence (3, 2, 1, GO!)
 *   - Items system
 *   - Pause interrupt
 *
 * Parameters:
 *   map  - Track to race on (ScorchingSands, AlpinRush, NeonCircuit)
 *   mode - SinglePlayer or MultiPlayer
 */
void Race_Init(Map map, GameMode mode);

/**
 * Function: Race_Tick
 * -------------------
 * Main race logic update (called every frame at 60 Hz).
 *
 * Handles:
 *   - Player input (steering, acceleration, item usage)
 *   - Car physics updates
 *   - Terrain effects (sand slowdown)
 *   - Wall collision
 *   - Checkpoint progression
 *   - Item collisions and effects
 *   - Multiplayer network sync (every 4 frames)
 */
void Race_Tick(void);

/**
 * Function: Race_Reset
 * --------------------
 * Resets race to initial state (restart same race).
 *
 * Resets:
 *   - Car positions to spawn
 *   - Timer state
 *   - Countdown sequence
 *   - Items on track
 */
void Race_Reset(void);

/**
 * Function: Race_Stop
 * -------------------
 * Stops current race and cleans up resources.
 *
 * Cleans up:
 *   - Pause interrupt
 *   - Race timer
 *   - Sets raceStarted = false
 */
void Race_Stop(void);

/**
 * Function: Race_MarkAsCompleted
 * -------------------------------
 * Marks race as completed and stores final time.
 *
 * Parameters:
 *   min  - Final minutes
 *   sec  - Final seconds (0-59)
 *   msec - Final milliseconds (0-999)
 */
void Race_MarkAsCompleted(int min, int sec, int msec);

//=============================================================================
// PUBLIC API - Countdown System
//=============================================================================

/**
 * Updates countdown sequence (3 → 2 → 1 → GO → FINISHED).
 * Called by Gameplay_OnVBlank() during countdown phase.
 */
void Race_UpdateCountdown(void);

/**
 * Returns true if countdown is still active (not COUNTDOWN_FINISHED).
 */
bool Race_IsCountdownActive(void);

/**
 * Returns current countdown state (COUNTDOWN_3/2/1/GO/FINISHED).
 */
CountdownState Race_GetCountdownState(void);

/**
 * Network sync during countdown (multiplayer only).
 * Shares spawn positions every 4 frames.
 */
void Race_CountdownTick(void);

//=============================================================================
// PUBLIC API - State Queries
//=============================================================================

/**
 * Gets read-only pointer to player's car (local player).
 */
const Car* Race_GetPlayerCar(void);

/**
 * Gets read-only pointer to complete race state.
 */
const RaceState* Race_GetState(void);

/**
 * Returns true if race is started and not finished.
 */
bool Race_IsActive(void);

/**
 * Returns true if race is finished (all laps completed).
 */
bool Race_IsCompleted(void);

/**
 * Gets final race time (only valid after Race_MarkAsCompleted()).
 */
void Race_GetFinalTime(int* min, int* sec, int* msec);

/**
 * Gets total lap count for current map.
 */
int Race_GetLapCount(void);

/**
 * Checks if player car crossed finish line (called by Gameplay_OnVBlank()).
 */
bool Race_CheckFinishLineCross(const Car* car);

//=============================================================================
// PUBLIC API - Graphics Integration
//=============================================================================

/**
 * Sets sprite graphics pointer for a specific car.
 */
void Race_SetCarGfx(int index, u16* gfx);

/**
 * Updates currently loaded track quadrant (for terrain detection).
 */
void Race_SetLoadedQuadrant(QuadrantID quad);

//=============================================================================
// PUBLIC API - Pause System
//=============================================================================

/**
 * Initializes START key interrupt for pause functionality.
 */
void Race_InitPauseInterrupt(void);

/**
 * ISR for pause toggle (START key pressed).
 */
void Race_PauseISR(void);

/**
 * Updates pause debounce timer (prevents button bounce).
 */
void Race_UpdatePauseDebounce(void);

/**
 * Cleans up pause interrupt when exiting race.
 */
void Race_CleanupPauseInterrupt(void);

#endif  // GAMEPLAY_LOGIC_H

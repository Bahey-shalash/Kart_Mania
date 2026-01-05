/**
 * gameplay_logic.h
 * Core race logic, physics updates, and state management
 */

#ifndef GAMEPLAY_LOGIC_H
#define GAMEPLAY_LOGIC_H

#include <stdbool.h>

#include "../core/game_types.h"
#include "../gameplay/wall_collision.h"
#include "../gameplay/Car.h"

//=============================================================================
// Constants
//=============================================================================

#define MAX_CARS 8
#define MAX_CHECKPOINTS 16

//=============================================================================
// Types
//=============================================================================

typedef enum { SinglePlayer, MultiPlayer } GameMode;

typedef enum {
    COUNTDOWN_3 = 0,
    COUNTDOWN_2,
    COUNTDOWN_1,
    COUNTDOWN_GO,
    COUNTDOWN_FINISHED
} CountdownState;

typedef struct {
    Vec2 topLeft;
    Vec2 bottomRight;
} CheckpointBox;

typedef struct {
    bool raceStarted;
    bool raceFinished;
    GameMode gameMode;
    Map currentMap;
    
    int carCount;
    int playerIndex;
    Car cars[MAX_CARS];
    
    int totalLaps;
    int checkpointCount;
    CheckpointBox checkpoints[MAX_CHECKPOINTS];
    
    int finishDelayTimer;
    int finalTimeMin;
    int finalTimeSec;
    int finalTimeMsec;
} RaceState;

//=============================================================================
// Public API - Lifecycle
//=============================================================================

/**
 * Initialize a new race
 * @param map The map to race on
 * @param mode Single-player or multiplayer
 */
void Race_Init(Map map, GameMode mode);

/**
 * Update race logic for one frame (60 FPS)
 * Handles physics, input, collisions, and items
 */
void Race_Tick(void);

/**
 * Update countdown only (network sync during countdown)
 * Used in multiplayer to sync spawn positions
 */
void Race_CountdownTick(void);

/**
 * Reset the race to initial state
 * Resets cars, timers, and items
 */
void Race_Reset(void);

/**
 * Stop the race and cleanup resources
 * Disables timers and interrupts
 */
void Race_Stop(void);

/**
 * Mark race as completed with final time
 * @param min Final minutes
 * @param sec Final seconds
 * @param msec Final milliseconds
 */
void Race_MarkAsCompleted(int min, int sec, int msec);

//=============================================================================
// Public API - State Queries
//=============================================================================

/**
 * Get read-only access to race state
 * @return Pointer to current race state
 */
const RaceState* Race_GetState(void);

/**
 * Get read-only access to player's car
 * @return Pointer to player's car
 */
const Car* Race_GetPlayerCar(void);

/**
 * Check if race is currently active (started but not finished)
 * @return True if race is running
 */
bool Race_IsActive(void);

/**
 * Check if race has completed
 * @return True if race is finished
 */
bool Race_IsCompleted(void);

/**
 * Get total number of laps for current map
 * @return Number of laps
 */
int Race_GetLapCount(void);

/**
 * Check if car has crossed finish line this frame
 * @param car The car to check
 * @return True if finish line was crossed
 */
bool Race_CheckFinishLineCross(const Car* car);

/**
 * Get final race time
 * @param min Output: minutes
 * @param sec Output: seconds
 * @param msec Output: milliseconds
 */
void Race_GetFinalTime(int* min, int* sec, int* msec);

//=============================================================================
// Public API - Countdown System
//=============================================================================

/**
 * Update countdown state
 * Called each frame during countdown
 */
void Race_UpdateCountdown(void);

/**
 * Check if countdown is still active
 * @return True if countdown has not finished
 */
bool Race_IsCountdownActive(void);

/**
 * Get current countdown state
 * @return Current countdown phase
 */
CountdownState Race_GetCountdownState(void);

//=============================================================================
// Public API - Configuration
//=============================================================================

/**
 * Set graphics pointer for a car sprite
 * @param index Car index
 * @param gfx Pointer to sprite graphics
 */
void Race_SetCarGfx(int index, u16* gfx);

/**
 * Update which quadrant is currently loaded
 * @param quad The loaded quadrant ID
 */
void Race_SetLoadedQuadrant(QuadrantID quad);

//=============================================================================
// Public API - Pause System
//=============================================================================

/**
 * Initialize pause interrupt (START button)
 * Sets up IRQ handler for pause functionality
 */
void Race_InitPauseInterrupt(void);

/**
 * Cleanup pause interrupt on race exit
 * Disables IRQ and resets pause state
 */
void Race_CleanupPauseInterrupt(void);

/**
 * Update pause debounce counter
 * Called each frame to prevent button bounce
 */
void Race_UpdatePauseDebounce(void);

/**
 * Pause interrupt service routine
 * Toggles pause state and timer
 */
void Race_PauseISR(void);

#endif  // GAMEPLAY_LOGIC_H
/**
 * gameplay.h
 * Main gameplay screen with race logic, rendering, and UI
 */

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "../core/game_types.h"

//=============================================================================
// Public API - Lifecycle
//=============================================================================

/**
 * Initialize the gameplay screen
 * Sets up graphics, backgrounds, sprites, and race state
 */
void Gameplay_Initialize(void);

/**
 * Update gameplay screen logic
 * Handles input, race progression, and state transitions
 * @return The next GameState
 */
GameState Gameplay_Update(void);

/**
 * VBlank callback for rendering gameplay elements
 * Updates sprites, scrolling, and display elements
 */
void Gameplay_OnVBlank(void);

/**
 * Cleanup gameplay resources
 * Frees sprites and allocated memory
 */
void Gameplay_Cleanup(void);

//=============================================================================
// Public API - Timer Access
//=============================================================================

/**
 * Get current lap minutes
 * @return Minutes elapsed in current lap
 */
int Gameplay_GetRaceMin(void);

/**
 * Get current lap seconds
 * @return Seconds elapsed in current lap
 */
int Gameplay_GetRaceSec(void);

/**
 * Get current lap milliseconds
 * @return Milliseconds elapsed in current lap
 */
int Gameplay_GetRaceMsec(void);

/**
 * Get current lap number
 * @return Current lap (1-indexed)
 */
int Gameplay_GetCurrentLap(void);

/**
 * Increment race timer
 * Updates both lap and total race timers
 */
void Gameplay_IncrementTimer(void);

//=============================================================================
// Public API - Sub Screen Display
//=============================================================================

/**
 * Update lap display on sub screen
 * @param currentLap Current lap number
 * @param totalLaps Total number of laps
 */
void Gameplay_UpdateLapDisplay(int currentLap, int totalLaps);

/**
 * Update chronometer display on sub screen
 * @param min Minutes to display
 * @param sec Seconds to display
 * @param msec Milliseconds to display
 */
void Gameplay_UpdateChronoDisplay(int min, int sec, int msec);

/**
 * Change background color of sub screen display
 * @param color RGB color value
 */
void Gameplay_ChangeDisplayColor(uint16 color);

#endif  // GAMEPLAY_H
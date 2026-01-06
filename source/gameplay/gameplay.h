/**
 * File: gameplay.h
 * ----------------
 * Description: Main gameplay screen interface for racing. Handles graphics
 *              initialization, VBlank rendering updates, timer management,
 *              and sub-screen display updates (lap counter, chrono, item
 *              display). Coordinates with gameplay_logic for race state.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "../core/game_types.h"

//=============================================================================
// PUBLIC CONSTANTS
//=============================================================================

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define MAP_SIZE 1024
#define QUADRANT_SIZE 512
#define QUAD_OFFSET 256
#define MAX_SCROLL_X (MAP_SIZE - SCREEN_WIDTH)
#define MAX_SCROLL_Y (MAP_SIZE - SCREEN_HEIGHT)

//=============================================================================
// PUBLIC API - Lifecycle
//=============================================================================

/**
 * Function: Gameplay_Initialize
 * -----------------------------
 * Initializes all graphics, sprites, and race state for gameplay screen.
 *
 * Sets up:
 *   - Main screen background (race track quadrants)
 *   - Sub screen display (timer, lap counter, item display)
 *   - Kart sprites and OAM
 *   - Race state via Race_Init()
 *   - Camera scroll position
 *
 * Must be called before Gameplay_Update() or Gameplay_OnVBlank().
 */
void Gameplay_Initialize(void);

/**
 * Function: Gameplay_Update
 * -------------------------
 * Updates gameplay logic and handles input (non-rendering updates).
 *
 * Handles:
 *   - SELECT key to exit to home
 *   - Best time saving when race finishes
 *   - Finish display timer countdown
 *   - State transitions (GAMEPLAY → PLAYAGAIN/HOME_PAGE)
 *
 * Returns:
 *   GAMEPLAY    - Continue racing
 *   PLAYAGAIN   - Race finished (single player)
 *   HOME_PAGE   - User pressed SELECT or race finished (multiplayer)
 */
GameState Gameplay_Update(void);

/**
 * Function: Gameplay_OnVBlank
 * ---------------------------
 * VBlank interrupt handler for all rendering updates (60 Hz).
 *
 * Renders:
 *   - Camera scroll (follows player kart)
 *   - Quadrant loading and switching
 *   - Countdown display (3, 2, 1, GO!)
 *   - Kart sprites (single or multiplayer)
 *   - Items on track
 *   - Final time display (2.5 seconds after finish)
 *   - Sub-screen updates (timer, lap, item)
 *
 * Called automatically by VBlank interrupt.
 */
void Gameplay_OnVBlank(void);

/**
 * Function: Gameplay_Cleanup
 * --------------------------
 * Cleans up graphics resources when exiting gameplay screen.
 *
 * Frees:
 *   - Kart sprite graphics
 *   - Item sprite graphics
 *   - Sub-screen item display sprite
 */
void Gameplay_Cleanup(void);

//=============================================================================
// PUBLIC API - Timer Access
//=============================================================================

/**
 * Gets current lap minutes (resets each lap).
 */
int Gameplay_GetRaceMin(void);

/**
 * Gets current lap seconds (0-59, resets each lap).
 */
int Gameplay_GetRaceSec(void);

/**
 * Gets current lap milliseconds (0-999, resets each lap).
 */
int Gameplay_GetRaceMsec(void);

/**
 * Gets current lap number (1-based).
 */
int Gameplay_GetCurrentLap(void);

/**
 * Increments race timers (both lap and total time).
 * Called by timer interrupt at 1000 Hz.
 */
void Gameplay_IncrementTimer(void);

//=============================================================================
// PUBLIC API - Sub-Screen Display
//=============================================================================

/**
 * Updates lap display on sub-screen (e.g., "3:5" for lap 3 of 5).
 */
void Gameplay_UpdateLapDisplay(int currentLap, int totalLaps);

/**
 * Updates chronometer display on sub-screen (MM:SS.mmm format).
 */
void Gameplay_UpdateChronoDisplay(int min, int sec, int msec);

/**
 * Changes sub-screen background color (BLACK, DARK_GREEN for new record).
 */
void Gameplay_ChangeDisplayColor(uint16 c);

/**
 * Renders a single digit (0-9) or separator (:, .) to tilemap.
 * Used internally by chrono/lap display functions.
 */
void Gameplay_PrintDigit(u16* map, int number, int x, int y);

#endif  // GAMEPLAY_H
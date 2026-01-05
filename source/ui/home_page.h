/**
 * File: home_page.h
 * -----------------
 * Description: Home page screen interface. Main menu with Singleplayer,
 *              Multiplayer, and Settings options. Features animated kart
 *              sprite on top screen and interactive menu with selection
 *              highlighting on bottom screen.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 */

#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include "../core/game_types.h"

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * Function: HomePage_Initialize
 * -----------------------------
 * Initializes the Home Page screen with dual-screen graphics and animated
 * kart sprite. Sets up top screen artwork with sprite layer and bottom
 * screen interactive menu with selection highlights.
 *
 * Graphics setup:
 *   - Main screen: Bitmap mode with home artwork + sprite layer for animated kart
 *   - Sub screen: Dual-layer with menu graphics and selection highlights
 *   - Initializes VBlank timer for sprite animation
 */
void HomePage_Initialize(void);

/**
 * Function: HomePage_Update
 * -------------------------
 * Updates the Home Page screen state. Handles input and menu selection.
 *
 * Controls:
 *   - D-Pad Up/Down: Navigate menu items
 *   - Touch: Direct selection by touching menu items
 *   - A Button: Confirm selection
 *
 * Returns:
 *   MAPSELECTION     - User selected Singleplayer
 *   MULTIPLAYER_LOBBY - User selected Multiplayer (if WiFi enabled and connection succeeded)
 *   REINIT_HOME      - Multiplayer connection failed, reinitialize home
 *   SETTINGS         - User selected Settings
 *   HOME_PAGE        - Stay on screen (no selection or WiFi disabled for multiplayer)
 *
 * Side effects:
 *   - Initializes multiplayer connection when Multiplayer selected
 *   - Sets multiplayer mode flag in context
 *   - Plays audio feedback for selections
 */
GameState HomePage_Update(void);

/**
 * Function: HomePage_OnVBlank
 * ---------------------------
 * VBlank callback for Home Page screen. Animates the kart sprite moving
 * across the top screen.
 *
 * Called: From timerISRVblank() when currentGameState == HOME_PAGE or REINIT_HOME
 */
void HomePage_OnVBlank(void);

/**
 * Function: HomePage_Cleanup
 * --------------------------
 * Cleans up Home Page resources. Frees sprite graphics memory allocated
 * for the animated kart.
 *
 * Called: From StateMachine_Cleanup() when leaving HOME_PAGE state
 */
void HomePage_Cleanup(void);

#endif  // HOME_PAGE_H
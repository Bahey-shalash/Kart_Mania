/**
 * File: play_again.h
 * ------------------
 * Description: Post-race screen interface. Presents YES/NO choice to restart
 *              the race or return to home menu. Handles multiplayer cleanup
 *              when exiting and provides button highlighting with D-pad and
 *              touch input support.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#ifndef PLAY_AGAIN_H
#define PLAY_AGAIN_H

#include "../core/game_types.h"

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * Function: PlayAgain_Initialize
 * ------------------------------
 * Initializes the Play Again screen with graphics and default selection.
 * Sets up dual-layer display with menu graphics and selection highlights.
 *
 * Default state: YES button selected
 */
void PlayAgain_Initialize(void);

/**
 * Function: PlayAgain_Update
 * --------------------------
 * Updates the Play Again screen state. Handles input and selection logic.
 *
 * Returns:
 *   GAMEPLAY  - User selected YES (restart race)
 *   HOME_PAGE - User selected NO or pressed SELECT (return home)
 *   PLAYAGAIN - Stay on screen (no selection made)
 *
 * Side effects:
 *   - Stops race timers when exiting to home
 *   - Cleans up multiplayer connection when exiting to home
 */
GameState PlayAgain_Update(void);

/**
 * Function: PlayAgain_OnVBlank
 * ----------------------------
 * VBlank callback for Play Again screen. Currently unused but reserved
 * for future animations.
 */
void PlayAgain_OnVBlank(void);

#endif  // PLAY_AGAIN_H

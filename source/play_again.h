#ifndef PLAY_AGAIN_H
#define PLAY_AGAIN_H

#include <nds.h>
#include "game_types.h"

//=============================================================================
// Play Again Button Types
//=============================================================================
typedef enum {
    PA_BTN_YES = 0,
    PA_BTN_NO = 1,
    PA_BTN_COUNT = 2,
    PA_BTN_NONE = -1
} PlayAgainButton;

//=============================================================================
// Public API
//=============================================================================

/**
 * Initialize the Play Again screen (like Map_Selection_initialize)
 */
void PlayAgain_Initialize(void);

/**
 * Update Play Again screen logic (call every frame, like Map_selection_update)
 * @return The next GameState (GAMEPLAY to restart, HOME_PAGE to exit, PLAYAGAIN to stay)
 */
GameState PlayAgain_Update(void);

/**
 * VBlank update for Play Again screen (call during VBlank, like Map_selection_OnVBlank)
 */
void PlayAgain_OnVBlank(void);

#endif // PLAY_AGAIN_H
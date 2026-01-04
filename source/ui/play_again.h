/**
 * play_again.h
 */

#ifndef PLAY_AGAIN_H
#define PLAY_AGAIN_H

#include <nds.h>
#include "../core/game_types.h"

//=============================================================================
// Public API
//=============================================================================

/**
 * Initialize the Play Again screen
 */
void PlayAgain_Initialize(void);

/**
 * Update Play Again screen logic 
 * @return The next GameState
 */
GameState PlayAgain_Update(void);

#endif // PLAY_AGAIN_H
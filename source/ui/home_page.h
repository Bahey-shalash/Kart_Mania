/**
 * home_page.h
 */

#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include "../core/game_types.h"

//=============================================================================
// Public API
//=============================================================================

/**
 * Initialize the Home Page screen
 */
void HomePage_Initialize(void);

/**
 * Update Home Page screen logic
 * @return The next GameState
 */
GameState HomePage_Update(void);

/**
 * VBlank callback for animating home screen sprite
 */
void HomePage_OnVBlank(void);

/**
 * Cleanup Home Page resources
 */
void HomePage_Cleanup(void);

#endif  // HOME_PAGE_H

/**
 * settings.h
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "../core/game_types.h"

//=============================================================================
// Public API
//=============================================================================

/**
 * Initialize the Settings screen
 */
void Settings_Initialize(void);

/**
 * Update Settings screen logic
 * @return The next GameState
 */
GameState Settings_Update(void);

#endif  // SETTINGS_H

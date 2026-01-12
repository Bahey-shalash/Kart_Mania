/**
 * File: settings.h
 * ----------------
 * Description: Settings screen interface. Provides toggle controls for WiFi,
 *              music, and sound effects with visual ON/OFF indicators. Supports
 *              saving/loading settings to persistent storage and factory reset
 *              functionality via START+SELECT combo.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "../core/game_types.h"

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * Function: Settings_Initialize
 * -----------------------------
 * Initializes the Settings screen with dual-screen graphics and current
 * setting states. Loads top screen artwork and bottom screen interactive
 * controls with toggle indicators.
 *
 * Graphics setup:
 *   - Main screen: Bitmap mode with settings artwork
 *   - Sub screen: Dual-layer with menu graphics and toggle/selection layers
 */
void Settings_Initialize(void);

/**
 * Function: Settings_Update
 * -------------------------
 * Updates the Settings screen state. Handles input and toggle logic.
 *
 * Controls:
 *   - D-Pad: Navigate between settings (Up/Down for vertical, Left/Right for bottom
 * row)
 *   - Touch: Direct selection by touching setting labels or toggle pills
 *   - A Button: Toggle selected setting or activate button (Save/Back/Home)
 *   - START+SELECT+A on Save: Factory reset all settings to defaults
 *
 * Returns:
 *   HOME_PAGE - User pressed Back or Home button
 *   SETTINGS  - Stay on screen (no navigation action)
 *
 * Side effects:
 *   - Immediately applies setting changes (music starts/stops, sound mutes/unmutes)
 *   - Updates persistent storage when Save is pressed
 *   - Resets to defaults and refreshes UI when factory reset triggered
 */
GameState Settings_Update(void);

#endif  // SETTINGS_H

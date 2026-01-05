/**
 * File: context.h
 * --------------
 * Description: Global game context management. Provides a singleton pattern
 *              for accessing shared game state including user settings, current
 *              game state, selected map, and multiplayer mode. This serves as
 *              the single source of truth for global application state.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdbool.h>

#include "game_types.h"

/**
 * Struct: GameContext
 * -------------------
 * Central data structure holding all global game state.
 *
 * Fields:
 *   userSettings.wifiEnabled   - Whether WiFi/multiplayer is enabled
 *   userSettings.musicEnabled  - Whether background music is playing
 *   userSettings.soundFxEnabled - Whether sound effects are enabled
 *   currentGameState           - Current screen/state (HOME_PAGE, GAMEPLAY, etc.)
 *   SelectedMap                - Currently selected map for gameplay
 *   isMultiplayerMode          - True if in multiplayer, false for singleplayer
 */
typedef struct {
    struct {
        bool wifiEnabled;
        bool musicEnabled;
        bool soundFxEnabled;
    } userSettings;

    GameState currentGameState;
    Map SelectedMap;
    bool isMultiplayerMode;
} GameContext;

//=============================================================================
// CONTEXT ACCESS
//=============================================================================

/**
 * Function: GameContext_Get
 * -------------------------
 * Returns a pointer to the global game context singleton.
 *
 * Returns: Pointer to the GameContext instance
 */
GameContext* GameContext_Get(void);

/**
 * Function: GameContext_InitDefaults
 * ----------------------------------
 * Initializes the game context with default values. Should be called once
 * at application startup.
 *
 * Default values:
 *   - All settings enabled (wifi, music, sound fx)
 *   - Game state: HOME_PAGE
 *   - Map: NONEMAP
 *   - Multiplayer mode: false
 */
void GameContext_InitDefaults(void);

//=============================================================================
// SETTINGS MANAGEMENT
//=============================================================================

/**
 * Function: GameContext_SetMusicEnabled
 * -------------------------------------
 * Enables or disables background music setting and immediately applies the change.
 *
 * Parameters:
 *   enabled - true to enable music, false to disable
 *
 * Side effects: Calls MusicSetEnabled() to start/stop music playback
 */
void GameContext_SetMusicEnabled(bool enabled);

/**
 * Function: GameContext_SetSoundFxEnabled
 * ---------------------------------------
 * Enables or disables sound effects setting and immediately applies the change.
 *
 * Parameters:
 *   enabled - true to enable sound fx, false to disable
 *
 * Side effects: Calls SOUNDFX_ON() or SOUNDFX_OFF() to control volume
 */
void GameContext_SetSoundFxEnabled(bool enabled);

/**
 * Function: GameContext_SetWifiEnabled
 * ------------------------------------
 * Enables or disables WiFi/multiplayer setting.
 *
 * Parameters:
 *   enabled - true to enable WiFi, false to disable
 */
void GameContext_SetWifiEnabled(bool enabled);

//=============================================================================
// GAME STATE MANAGEMENT
//=============================================================================

/**
 * Function: GameContext_SetMap
 * ----------------------------
 * Sets the currently selected map for gameplay.
 *
 * Parameters:
 *   SelectedMap - Map enum value (ScorchingSands, AlpinRush, NeonCircuit)
 */
void GameContext_SetMap(Map SelectedMap);

/**
 * Function: GameContext_GetMap
 * ----------------------------
 * Gets the currently selected map.
 *
 * Returns: Current map selection
 */
Map GameContext_GetMap(void);

/**
 * Function: GameContext_SetMultiplayerMode
 * ----------------------------------------
 * Sets whether the game is in multiplayer mode.
 *
 * Parameters:
 *   isMultiplayer - true for multiplayer, false for singleplayer
 */
void GameContext_SetMultiplayerMode(bool isMultiplayer);

/**
 * Function: GameContext_IsMultiplayerMode
 * ---------------------------------------
 * Checks if the game is currently in multiplayer mode.
 *
 * Returns: true if multiplayer mode, false if singleplayer
 */
bool GameContext_IsMultiplayerMode(void);

#endif

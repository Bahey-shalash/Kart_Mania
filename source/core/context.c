/**
 * File: context.c
 * --------------
 * Description: Implementation of the global game context singleton. Manages
 *              application-wide state and provides controlled access to settings
 *              and game state with automatic side effects (e.g., changing music
 *              settings immediately starts/stops playback).
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */
#include "context.h"

#include "../audio/sound.h"

static GameContext gGameContext;  // Singleton instance

GameContext* GameContext_Get(void) {
    return &gGameContext;
}
// TODO: Return const pointer to prevent direct modification via GameContext_Get()

void GameContext_InitDefaults(void) {
    gGameContext.userSettings.wifiEnabled = true;
    gGameContext.userSettings.musicEnabled = true;
    gGameContext.userSettings.soundFxEnabled = true;

    gGameContext.currentGameState = HOME_PAGE;
    gGameContext.SelectedMap = NONEMAP;
    gGameContext.isMultiplayerMode = false;
}

//=============================================================================
// SETTINGS MANAGEMENT
//=============================================================================

void GameContext_SetMusicEnabled(bool enabled) {
    gGameContext.userSettings.musicEnabled = enabled;
    MusicSetEnabled(enabled);  // Immediately apply music change
}

void GameContext_SetSoundFxEnabled(bool enabled) {
    gGameContext.userSettings.soundFxEnabled = enabled;
    if (enabled)
        SOUNDFX_ON();
    else
        SOUNDFX_OFF();
}

void GameContext_SetWifiEnabled(bool enabled) {
    gGameContext.userSettings.wifiEnabled = enabled;
    // Note: WiFi stack is initialized once at startup and kept alive throughout
    // the program lifetime. This setting only controls the user preference, not
    // the actual WiFi hardware state (see documentation for more info)
}

//=============================================================================
// GAME STATE MANAGEMENT
//=============================================================================

void GameContext_SetMap(Map SelectedMap) {
    gGameContext.SelectedMap = SelectedMap;
}
Map GameContext_GetMap(void) {
    return gGameContext.SelectedMap;
}

void GameContext_SetMultiplayerMode(bool isMultiplayer) {
    gGameContext.isMultiplayerMode = isMultiplayer;
}

bool GameContext_IsMultiplayerMode(void) {
    return gGameContext.isMultiplayerMode;
}

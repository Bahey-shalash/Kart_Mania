// context.h
#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdbool.h>

#include "game_types.h"

typedef struct {
    struct {
        bool wifiEnabled;
        bool musicEnabled;
        bool soundFxEnabled;
    } userSettings;

    GameState currentGameState;
    Map SelectedMap;
} GameContext;

// Single source of truth access
GameContext* GameContext_Get(void);
void GameContext_InitDefaults(void);

void GameContext_SetMusicEnabled(bool enabled);
void GameContext_SetSoundFxEnabled(bool enabled);
void GameContext_SetWifiEnabled(bool enabled);
void GameContext_SetMap(Map SelectedMap);
Map GameContext_GetMap(void);

#endif
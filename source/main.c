/*
 * Kart Mania - Main Source File
 */
#include <nds.h>

#include "context.h"
#include "game_types.h"
#include "gameplay.h"
#include "gameplay_logic.h"
#include "graphics.h"
#include "home_page.h"
#include "map_selection.h"
#include "multiplayer.h"
#include "multiplayer_lobby.h"
#include "settings.h"
#include "sound.h"
#include "storage.h"

//=============================================================================
// PROTOTYPES
//=============================================================================

static GameState update_state(GameState state);
static void init_state(GameState state);
static void cleanup_state(GameState state);

//=============================================================================
// MAIN
//=============================================================================

int main(void) {
    // Initialize storage first (includes fatInitDefault)
    bool storageAvailable = Storage_Init();

    // Initialize context with hardcoded defaults
    GameContext_InitDefaults();
    GameContext* ctx = GameContext_Get();

    // If storage available, load saved settings (overwrites defaults)
    if (storageAvailable) {
        Storage_LoadSettings();
    }

    initSoundLibrary();
    LoadALLSoundFX();
    loadMUSIC();

    // enables Music because default sound effect is true
    GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);
    init_state(ctx->currentGameState);

    while (true) {
        GameState nextState = update_state(ctx->currentGameState);

        if (nextState != ctx->currentGameState) {
            cleanup_state(ctx->currentGameState);
            ctx->currentGameState = nextState;
            video_nuke();
            init_state(nextState);
        }

        swiWaitForVBlank();
    }

    UnloadALLSoundFX();

    return 0;
}

//=============================================================================
// IMPLEMENTATION
//=============================================================================

static GameState update_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
            return HomePage_update();
        case SETTINGS:
            return Settings_update();
        case MAPSELECTION:
            return Map_selection_update();
        case MULTIPLAYER_LOBBY:  // NEW
            return MultiplayerLobby_Update();
        case GAMEPLAY:
            return Gameplay_update();
    }
    return state;
}

static void init_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
            HomePage_initialize();
            break;
        case MAPSELECTION:
            Map_Selection_initialize();
            break;
        case MULTIPLAYER_LOBBY:
            MultiplayerLobby_Init();
            break;
        case GAMEPLAY:
            Graphical_Gameplay_initialize();
            break;
        case SETTINGS:
            Settings_initialize();
            break;
    }
}

static void cleanup_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
            break;
        case MAPSELECTION:
            break;
        case MULTIPLAYER_LOBBY:
            // Nothing to cleanup here (Multiplayer_Cleanup called elsewhere if needed)
            break;
        case GAMEPLAY:
            Race_Stop();
            if (GameContext_IsMultiplayerMode()) {
                Multiplayer_Cleanup();
                GameContext_SetMultiplayerMode(false);
            }
            break;
        case SETTINGS:
            break;
    }
}

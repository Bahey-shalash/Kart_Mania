/*
 * Kart Mania - Main Source File
 */
#include <nds.h>
#include <dswifi9.h>
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
#include "play_again.h"
// BAHEY------
//=============================================================================
// PROTOTYPES
//=============================================================================
static GameState update_state(GameState state);
static void init_state(GameState state);
static void cleanup_state(GameState state, GameState nextState);

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

    // sound Fx

    if (!ctx->userSettings.soundFxEnabled) {
        SOUNDFX_OFF();
    }

    // Initialize WiFi stack ONCE at program start (critical for reconnection)
    // DO NOT call Wifi_InitDefault() again later - just connect/disconnect
    Wifi_InitDefault(false);

    init_state(ctx->currentGameState);

    while (true) {
        // Update DSWifi state (critical for multiplayer)
        Wifi_Update();

        GameState nextState = update_state(ctx->currentGameState);

        if (nextState != ctx->currentGameState) {
            cleanup_state(ctx->currentGameState, nextState);
            ctx->currentGameState = nextState;
            // Note: Multiplayer_NukeConnectivity() is called by HomePage_initialize()
            // No need to call it here - let each state handle its own init
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
        case REINIT_HOME:  // NEW: Treat same as HOME_PAGE
            return HomePage_update();
        case SETTINGS:
            return Settings_update();
        case MAPSELECTION:
            return Map_selection_update();
        case MULTIPLAYER_LOBBY:
            return MultiplayerLobby_Update();
        case GAMEPLAY:
            return Gameplay_update();
        case PLAYAGAIN:
            return PlayAgain_Update();
        default:
            return state;
    }
}

static void init_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
        case REINIT_HOME:  // NEW: Both initialize the same way
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
        case PLAYAGAIN:
            PlayAgain_Initialize();
            break;
    }
}

static void cleanup_state(GameState state, GameState nextState) {
    switch (state) {
        case HOME_PAGE:
        case REINIT_HOME:  // NEW: Both cleanup the same way
            HomePage_Cleanup();
            break;
        case MAPSELECTION:
            break;
        case MULTIPLAYER_LOBBY:
            // Keep connection alive only when heading into gameplay
            if (nextState != GAMEPLAY && GameContext_IsMultiplayerMode()) {
                Multiplayer_Cleanup();
                GameContext_SetMultiplayerMode(false);
            }
            break;
        case GAMEPLAY:
            RaceTick_TimerStop();  // Stop physics/race timers
            Gameplay_Cleanup();
            Race_Stop();
            // Only cleanup multiplayer if we were in multiplayer mode
            if (GameContext_IsMultiplayerMode()) {
                Multiplayer_Cleanup();
                GameContext_SetMultiplayerMode(false);
            }
            break;
        case SETTINGS:
            break;
        case PLAYAGAIN:
            // IMPORTANT: Don't cleanup multiplayer here either!
            // The player might choose "YES" to play again, and we need
            // to keep the WiFi connection alive
            break;
    }
}

/*
 * Kart Mania - Main Source File
 */
#include <nds.h>

#include "context.h"
#include "game.h"
#include "game_types.h"
#include "graphics.h"
#include "home_page.h"
#include "settings.h"
#include "sound.h"

//=============================================================================
// PROTOTYPES
//=============================================================================

GameState update_state(GameState state);
void init_state(GameState state);
//=============================================================================
// MAIN
//=============================================================================

int main(void) {
    GameContext_InitDefaults();
    GameContext* ctx = GameContext_Get();

    initSoundLibrary();
    LoadALLSoundFX();
    loadMUSIC();

    // enables sound effects because default sound effect is true
    
    // enables Music because default sound effect is true
    GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);
    init_state(ctx->currentGameState);

    while (true) {
        GameState nextState = update_state(ctx->currentGameState);

        if (nextState != ctx->currentGameState) {
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

GameState update_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
            return HomePage_update();
        case SETTINGS:
            return Settings_update();
        case SINGLEPLAYER:
            return Singleplayer_update();
        case MULTIPLAYER:
            return HomePage_update();  // placeholder
    }
    return state;
}

void init_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
            HomePage_initialize();
            break;
        case SINGLEPLAYER:
            Singleplayer_initialize();
            break;
        case MULTIPLAYER:
            HomePage_initialize();
            break;
        case SETTINGS:
            Settings_initialize();
            break;
    }
}

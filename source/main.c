/*
 * Kart Mania - Main Source File
 */
#include <nds.h>

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
GameState currentState_GLOBAL = HOME_PAGE;

int main(void) {
    initSoundLibrary();
    SOUNDFX_ON();
    LoadALLSoundFX();
    loadMUSIC();
    // the load settings from memory part should be above this //will not feed true
    //  constatly in the future
    MusicSetEnabled(true);
    init_state(currentState_GLOBAL);

    while (true) {
        GameState nextState = update_state(currentState_GLOBAL);

        if (nextState != currentState_GLOBAL) {
            currentState_GLOBAL = nextState;
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

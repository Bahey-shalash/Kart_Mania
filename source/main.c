/*
 * Kart Mania - Main Source File
 */
#include <nds.h>

#include "game_types.h"
#include "graphics.h"
#include "home_page.h"
#include "settings.h"

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
    init_state(currentState_GLOBAL);

    while (true) {
        GameState nextState = update_state(currentState_GLOBAL);

        if (nextState != currentState_GLOBAL) {
            video_nuke();
            init_state(nextState);
            currentState_GLOBAL = nextState;
        }

        swiWaitForVBlank();
    }

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
            return HomePage_update();  // placeholder
        case MULTIPLAYER:
            return HomePage_update();  // placeholder
    }
    return state;
}

void init_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
        case SINGLEPLAYER:
        case MULTIPLAYER:
            HomePage_initialize();
            break;
        case SETTINGS:
            Settings_initialize();
            break;
    }
}
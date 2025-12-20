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

int main(void) {
    GameState currentState = HOME_PAGE;

    init_state(currentState);

    while (true) {
        GameState nextState = update_state(currentState);

        if (nextState != currentState) {
            video_nuke();
            init_state(nextState);
            currentState = nextState;
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
            move_homeKart();
            return HomePage_update();
        case SETTINGS:
            return Settings_update();
        case SINGLEPLAYER:
            move_homeKart();
            return HomePage_update();  // placeholder
        case MULTIPLAYER:
            move_homeKart();
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
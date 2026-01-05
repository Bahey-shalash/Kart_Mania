/**
 * File: state_machine.c
 * ---------------------
 * Description: Implementation of the game state machine. Routes state updates,
 *              initialization, and cleanup to state-specific handlers.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#include "state_machine.h"

#include "../gameplay/gameplay.h"
#include "../gameplay/gameplay_logic.h"
#include "../graphics/graphics.h"
#include "../network/multiplayer.h"
#include "../ui/home_page.h"
#include "../ui/map_selection.h"
#include "../ui/multiplayer_lobby.h"
#include "../ui/play_again.h"
#include "../ui/settings.h"
#include "context.h"
#include "timer.h"

//=============================================================================
// STATE UPDATE DISPATCH
//=============================================================================

GameState StateMachine_Update(GameState state) {
    switch (state) {
        case HOME_PAGE:
        case REINIT_HOME:  // REINIT_HOME treated same as HOME_PAGE
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
            return state;  // Unknown state, stay in current state
    }
}

//=============================================================================
// STATE INITIALIZATION
//=============================================================================

void StateMachine_Init(GameState state) {
    switch (state) {
        case HOME_PAGE:
        case REINIT_HOME:  // Both initialize the same way
            HomePage_initialize();
            break;

        case MAPSELECTION:
            Map_Selection_initialize();
            break;

        case MULTIPLAYER_LOBBY:
            MultiplayerLobby_Init();
            break;

        case GAMEPLAY:
            Graphical_Gameplay_initialize();  // Sets up graphics, starts race timers
            break;

        case SETTINGS:
            Settings_initialize();
            break;

        case PLAYAGAIN:
            PlayAgain_Initialize();
            break;
    }
}

//=============================================================================
// STATE CLEANUP
//=============================================================================

void StateMachine_Cleanup(GameState state, GameState nextState) {
    switch (state) {
        case HOME_PAGE:
        case REINIT_HOME:  // Both cleanup the same way
            HomePage_Cleanup();
            break;

        case MAPSELECTION:
            // No cleanup needed for map selection
            break;

        case MULTIPLAYER_LOBBY:
            // Keep WiFi connection alive when heading into gameplay
            // Only disconnect if leaving lobby without starting a race
            if (nextState != GAMEPLAY && GameContext_IsMultiplayerMode()) {
                Multiplayer_Cleanup();
                GameContext_SetMultiplayerMode(false);
            }
            break;

        case GAMEPLAY:
            RaceTick_TimerStop();  // Stop TIMER0 (physics) and TIMER1 (chronometer)
            Gameplay_Cleanup();    // Clean up gameplay graphics/resources
            Race_Stop();           // Stop race logic

            // Only cleanup multiplayer if we were in multiplayer mode
            if (GameContext_IsMultiplayerMode()) {
                Multiplayer_Cleanup();
                GameContext_SetMultiplayerMode(false);
            }
            break;

        case SETTINGS:
            // No cleanup needed (settings auto-save on change)
            break;

        case PLAYAGAIN:
            // IMPORTANT: Don't cleanup multiplayer here!
            break;
    }
}

// TODO: think about sound cleanup

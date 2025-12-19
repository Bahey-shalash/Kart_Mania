/*
 * Kart Mania - Main Source File
 */
#include <nds.h>
#include <stdio.h>

#include "game_types.h"
#include "home_page.h"
#include "settings.h"

int main(void)
{
    enum GameState currentState = HOME_PAGE;

    // Initialize home page
    HomePage_initialize();

    // Main game loop
    while (true)
    {
        enum GameState nextState = currentState;

        switch (currentState)
        {
        case HOME_PAGE:
            nextState = HomePage_update();
            move_homeKart();
            break;

        case SETTINGS:
            Settings_update();
            break;
        }

        if (nextState != currentState)
        {
            // Cleanup old state
            switch (currentState)
            {
            case HOME_PAGE:
                HomePage_cleanup();
                break;
            case SETTINGS:
                Settings_cleanup();
                break;
            }

            // Init new state
            switch (nextState)
            {
            case HOME_PAGE:
                HomePage_initialize();
                break;
            case SETTINGS:
                Settings_initialize();
                break;
            }

            currentState = nextState;
        }

        swiWaitForVBlank();
    }

    return 0;
}
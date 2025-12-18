/*
 * Kart Mania - Main Source File
 */
#include <nds.h>
#include <stdio.h>

#include "game_types.h"
#include "home_page.h"

int main(void) {
    enum GameState currentState = HOME_PAGE;

    // Initialize home page
    HomePage_initialize();

    // Main game loop
    while (true) {
        // Read input
        scanKeys();
        // Handle current game state
        if (currentState == HOME_PAGE) {
            HomePage_handleInput();
            HomePage_updateMenu();
            move_homeKart();
        }

        // Wait for vertical blank
        swiWaitForVBlank();
    }

    return 0;
}
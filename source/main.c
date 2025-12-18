/*
 * Kart Mania - Main Source File
 */

#include <nds.h>
#include <stdio.h>

#include "game_types.h"
#include "home_page.h"

int main(void) {
    enum GameState currentState = HOME_PAGE;
    //---sub screen graphics setup---
    HomePage_initialize();
    //------------------------------

    while (true) {
        // Touchscreen status (struct)
        scanKeys();
        touchPosition touch;
        touchRead(&touch);
        if (currentState == HOME_PAGE) {
            // Handle home page interactions here
            touchscreen_controlls_home_page(&touch);
            move_homeKart();
        }

        swiWaitForVBlank();
    }
}

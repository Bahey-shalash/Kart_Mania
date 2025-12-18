#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <nds.h>

#include "game_types.h"
void HomePage_initialize();
void configureGraphics_Sub_home_page();
void configBG2_Sub_homepage();

/*
 * Handle touchscreen controls on the home page.
 * single player button area: Top-Left: (33, 23) ; Top-Right: (223, 23);Bottom-Left:
 * (33, 61); Bottom-Right: (223, 61)
 * multiplayer button area: Top-Left: (33, 77); Top-Right: (223, 77); Bottom-Left:
 * (33, 115);Bottom-Right: (223, 115)
 * settings button area: Top-Left: Top-Left: (33, 131), Top-Right: (223,131),
 * Bottom-Left: (33, 169); Bottom-Right: (223, 169)
 */
void touchscreen_controlls_home_page(touchPosition* touch);

void configureGraphics_MAIN_home_page();
void configBG_Main_homepage();

void configurekartSpritehome();
void move_homeKart();

#endif  // HOME_PAGE_H
#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <nds.h>
#include <stdbool.h>

#include "game_types.h"

//----------Layout constants (SUB screen)----------

#define HOME_MENU_X 32
#define HOME_MENU_WIDTH 192
#define HOME_MENU_HEIGHT 40
#define HOME_MENU_SPACING 54
#define HOME_MENU_Y_START 24

// Macro to define menu item hit boxes
#define MENU_COUNT 3

/**
 * Generate a MenuItemHitBox for a vertically stacked menu item.
 * Uses fixed X/width/height; Y position is derived from the item index.
 *
 * @param i Zero-based menu item index (top item = 0)
 */
#define MENU_ITEM_ROW(i)                                                        \
    {HOME_MENU_X, HOME_MENU_Y_START + (i) * HOME_MENU_SPACING, HOME_MENU_WIDTH, \
     HOME_MENU_HEIGHT}

void configureGraphics_MAIN_home_page();
void configBG_Main_homepage();

void configurekartSpritehome();
void move_homeKart();

//----------Initialization & Cleanup----------

/**
 * Initialize the home page - sets up graphics, background, and button sprites
 * Call this once when entering the home page state
 */

void HomePage_initialize(void);

void HomePage_cleanup(void);

//----------Configuration Functions----------

/**
 * Configure the SUB screen display mode and VRAM banks
 * Sets up MODE_5_2D for bitmap background and allocates VRAM for sprites
 * Called internally by HomePage_initialize()
 */
void configGraphics_Sub(void);

/**
 * Configure and load the background bitmap image
 * Sets up BG2 in extended rotoscale mode and copies bitmap data to VRAM
 * Called internally by HomePage_initialize()
 */
void configBackground_Sub(void);

//----------Input Handling----------
enum GameState HomePage_update(void);

/**
 * Handle D-pad input for menu navigation
 * Detects UP/DOWN for selection and A button for activation
 * Updates selectedButton index and triggers button actions
 * Called internally by HomePage_handleInput()
 */
void handleDPadInput(void);

/**
 * Handle touchscreen input for button presses
 * Reads touch position and determines which button (if any) is being touched
 * Updates selectedButton and sets pressed state
 * Called internally by HomePage_handleInput()
 */
void handleTouchInput(void);

void setButtonOverlay(int buttonIndex, bool show);

#endif  // HOME_PAGE_H

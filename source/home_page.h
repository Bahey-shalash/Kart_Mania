#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <nds.h>
#include <stdbool.h>

#include "game_types.h"

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
void HomePage_update(void);

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

#endif // HOME_PAGE_H

#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <nds.h>
#include <stdbool.h>

// MenuButton struct - represents a single button sprite on screen
typedef struct
{
    int id;       // Sprite OAM index (0, 1, 2, etc.)
    int x;        // X position on screen
    int y;        // Y position on screen
    bool pressed; // Whether button is currently pressed
} MenuButton;

//----------Initialization & Cleanup----------

/**
 * Initialize the home page - sets up graphics, background, and button sprites
 * Call this once when entering the home page state
 */
void HomePage_initialize(void);

/**
 * Clean up home page resources - frees sprite graphics and clears OAM
 * Call this when transitioning away from home page
 */
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

/**
 * Configure sprites - initialize OAM, allocate graphics, load tiles and palettes
 * Loads all button sprite data (3 frames per button) into VRAM
 * Called internally by HomePage_initialize()
 */
void configSprites_Sub(void);

//----------Input Handling----------

/**
 * Handle all input (touch and D-pad) for the home page
 * Resets pressed states, then processes D-pad and touch input
 * Call this every frame before HomePage_updateMenu()
 */
void HomePage_handleInput(void);

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

//----------Rendering----------

/**
 * Update and render all button sprites to screen
 * Updates each button sprite based on selection/pressed state and updates OAM
 * Call this every frame in your main loop after HomePage_handleInput()
 */
void HomePage_updateMenu(void);

/**
 * Update a single button sprite's appearance based on state
 * @param btn - Pointer to MenuButton to update
 * @param isSelected - Whether this button is currently selected (highlighted)
 * @param isPressed - Whether this button is currently pressed
 * Selects the appropriate frame (normal/highlighted/pressed) and updates OAM entry
 * Called internally by HomePage_updateMenu() for each button
 */
void updateButtonSprite(MenuButton *btn, bool isSelected, bool isPressed);

#endif // HOME_PAGE_H
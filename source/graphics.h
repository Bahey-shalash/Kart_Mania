#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <nds.h>

/**
 * Generate a MenuItemHitBox for a vertically stacked menu item.
 * Uses fixed X/width/height; Y position is derived from the item index.
 *
 * @param i Zero-based menu item index (top item = 0)
 */
#define MENU_ITEM_ROW(i)                                                        \
    {HOME_MENU_X, HOME_MENU_Y_START + (i) * HOME_MENU_SPACING, HOME_MENU_WIDTH, \
     HOME_MENU_HEIGHT}

void video_nuke(void);

#endif  // GRAPHICS_H
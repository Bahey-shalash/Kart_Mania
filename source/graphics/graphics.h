/**
 * File: graphics.h
 * ----------------
 * Description: Graphics utilities for safe screen transitions. Provides
 *              video_nuke(), which resets displays, OAM allocators, palettes,
 *              VRAM banks, and BG registers to a known clean state before
 *              initializing the next screen.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

/**
 * Function: video_nuke
 * --------------------
 * Fully clears DS display state: turns off both screens, wipes sprites,
 * palettes, VRAM banks, and resets BG control/offset/affine registers.
 * Call during state transitions prior to reconfiguring graphics.
 */
void video_nuke(void);

#endif  // GRAPHICS_H

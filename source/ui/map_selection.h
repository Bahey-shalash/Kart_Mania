/**
 * File: map_selection.h
 * ----------------------
 * Description: Map selection screen for Kart Mania. Displays three racing maps
 *              (Scorching Sands, Alpine Rush, Neon Circuit) with thumbnail previews
 *              and allows player to choose which track to race on. Features animated
 *              cloud background and dual-layer transparency for selection
 * highlighting.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 *
 * Screen Layout:
 *   Main Screen (Top):
 *     - BG0 (priority 1): Base layer with combined map thumbnail graphics (back)
 *     - BG1 (priority 0): Scrolling clouds at 0.5 px/frame (front)
 *     - Cloud animation creates atmospheric effect over the map selection
 *
 *   Sub Screen (Bottom):
 *     - BG0 (priority 0): Menu UI with transparent areas (front layer)
 *     - BG1 (priority 1): Selection tiles positioned under transparent areas (back
 * layer)
 *     - BG0 has transparent regions where selection highlights should appear
 *     - BG1 tiles underneath show through these transparent areas
 *     - When selected, BG1 tile color changes from BLACK to WHITE
 *
 * Selection Highlighting Technique:
 *   The menu graphics (BG0) have transparent areas built in. Solid-color tiles on BG1
 *   are positioned exactly under these transparent regions. By changing the tile color
 *   from BLACK (invisible) to WHITE (visible), we create selection highlights without
 *   needing to modify the front layer graphics.
 *
 * Selection Tiles:
 *   - 4 separate solid-color tiles (one per button) at palette indices 240-243
 *   - Each tile is 64 bytes (8x8 pixels), all pixels set to the same palette index
 *   - Palette 240: MAP1 (Scorching Sands) selection tile
 *   - Palette 241: MAP2 (Alpine Rush) selection tile
 *   - Palette 242: MAP3 (Neon Circuit) selection tile
 *   - Palette 243: HOME button selection tile
 *
 * Input Methods:
 *   - D-Pad: Navigate between maps and home button (with wraparound)
 *   - Touch: Tap map thumbnail or home button to select
 *   - A button: Confirm selection and proceed to chosen map/state
 *
 * Current Implementation Status:
 *   - MAP1 (Scorching Sands): Fully implemented, leads to GAMEPLAY state
 *   - MAP2 (Alpine Rush): Placeholder, returns to HOME_PAGE
 *   - MAP3 (Neon Circuit): Placeholder, returns to HOME_PAGE
 *   - HOME button: Returns to HOME_PAGE
 */

#ifndef MAPSELECTION_H
#define MAPSELECTION_H
#include "../core/game_types.h"

/**
 * Initialize map selection screen.
 * - Configures graphics for both main and sub screens
 * - Sets up BG0 and BG1 layers on both screens
 * - Loads map thumbnails, cloud graphics, and selection tiles
 * - Positions selection tiles under transparent areas of menu UI
 * - Resets selection state to BTN_NONE
 * - Initializes cloud scrolling offset to 0
 *
 * Must be called when entering MAPSELECTION state.
 */
void MapSelection_Initialize(void);

/**
 * Update map selection screen (call every frame).
 * - Processes D-pad and touch input for navigation
 * - Updates selection highlighting when selection changes
 * - Handles A button or touch release to confirm selection
 * - Returns next game state based on button pressed
 *
 * Returns:
 *   - GAMEPLAY if MAP1 (Scorching Sands) selected
 *   - HOME_PAGE if MAP2, MAP3, or HOME selected
 *   - MAPSELECTION to stay on this screen
 */
GameState MapSelection_Update(void);

/**
 * VBlank handler for map selection screen.
 * - Animates cloud scrolling on main screen (BG1)
 * - Scrolls clouds at 0.5 pixels per frame (30 px/sec at 60 FPS)
 * - Uses subpixel counter to achieve fractional scroll speed
 * - Updates REG_BG1HOFS hardware register for smooth animation
 *
 * Must be called from IRQ_VBLANK interrupt handler.
 */
void MapSelection_OnVBlank(void);

#endif  // MAPSELECTION_H
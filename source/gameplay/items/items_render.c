/**
 * File: items_render.c
 * --------------------
 * Description: Rendering system for items. Handles sprite allocation, graphics
 *              loading, and OAM management for item boxes and track items.
 *              Supports rotation for projectiles and visibility culling.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "items_internal.h"
#include "items_api.h"

#include "data/items/banana.h"
#include "data/items/bomb.h"
#include "data/items/green_shell.h"
#include "data/items/item_box.h"
#include "data/items/missile.h"
#include "data/items/oil_slick.h"
#include "data/items/red_shell.h"

//=============================================================================
// Rendering
//=============================================================================
void Items_Render(int scrollX, int scrollY) {
    // =========================================================================
    // ITEM BOXES
    // =========================================================================
    for (int i = 0; i < itemBoxCount; i++) {
        int oamSlot = ITEM_BOX_OAM_START + i;

        if (!itemBoxSpawns[i].active) {
            // Hide inactive item boxes
            oamSet(&oamMain, oamSlot, 0, 192, OBJPRIORITY_2, 1, SpriteSize_8x8,
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, 1, true, false,
                   false, false, false);
            continue;
        }

        // Calculate screen position (centered on hitbox)
        int screenX =
            FixedToInt(itemBoxSpawns[i].position.x) - scrollX - (ITEM_BOX_HITBOX / 2);
        int screenY =
            FixedToInt(itemBoxSpawns[i].position.y) - scrollY - (ITEM_BOX_HITBOX / 2);

        // Render if on-screen, otherwise hide
        if (screenX >= -16 && screenX < 256 && screenY >= -16 && screenY < 192) {
            // Priority 2 so item boxes appear at same layer as track items but below karts
            oamSet(&oamMain, oamSlot, screenX, screenY, OBJPRIORITY_2, 1, SpriteSize_8x8,
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, 1, false, false,
                   false, false, false);
        } else {
            // Hide offscreen boxes
            oamSet(&oamMain, oamSlot, 0, 192, OBJPRIORITY_2, 1, SpriteSize_8x8,
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, 1, true, false,
                   false, false, false);
        }
    }

    // =========================================================================
    // TRACK ITEMS (Bananas, Shells, etc.)
    // =========================================================================

    // Clear/hide all track item OAM slots
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        int oamSlot = TRACK_ITEM_OAM_START + i;
        oamSet(&oamMain, oamSlot, 0, 192, OBJPRIORITY_2, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, NULL, -1, true, false, false, false, false);
    }

    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];
        int oamSlot = TRACK_ITEM_OAM_START + i;  // STABLE MAPPING

        // Calculate screen position
        int screenX =
            FixedToInt(item->position.x) - scrollX - (item->hitbox_width / 2);
        int screenY =
            FixedToInt(item->position.y) - scrollY - (item->hitbox_height / 2);

        // Skip if offscreen (slot already hidden in step 1)
        if (screenX < -32 || screenX > 256 || screenY < -32 || screenY > 192)
            continue;

        // Determine sprite size and palette based on item type
        SpriteSize spriteSize;
        int paletteNum;

        switch (item->type) {
            case ITEM_MISSILE:
                spriteSize = SpriteSize_16x32;
                paletteNum = 6;
                break;
            case ITEM_OIL:
                spriteSize = SpriteSize_32x32;
                paletteNum = 7;
                break;
            case ITEM_BANANA:
                spriteSize = SpriteSize_16x16;
                paletteNum = 2;
                break;
            case ITEM_BOMB:
                spriteSize = SpriteSize_16x16;
                paletteNum = 3;
                break;
            case ITEM_GREEN_SHELL:
                spriteSize = SpriteSize_16x16;
                paletteNum = 4;
                break;
            case ITEM_RED_SHELL:
                spriteSize = SpriteSize_16x16;
                paletteNum = 5;
                break;
            default:
                spriteSize = SpriteSize_16x16;
                paletteNum = 0;
                break;
        }

        // Determine if sprite needs rotation (projectiles)
        bool useRotation =
            (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL ||
             item->type == ITEM_MISSILE);

        if (useRotation) {
            // Use separate affine matrix slots for each rotating item
            // DS has 32 affine matrices, we'll use slots 1-4 rotating
            int affineSlot = 1 + (i % 4);
            int rotation = -(item->angle512 << 6);  // Convert to DS angle

            oamRotateScale(&oamMain, affineSlot, rotation, (1 << 8), (1 << 8));
            // Priority 2 so items appear below karts (priority 0) but above background (priority 1)
            oamSet(&oamMain, oamSlot, screenX, screenY, OBJPRIORITY_2, paletteNum, spriteSize,
                   SpriteColorFormat_16Color, item->gfx, affineSlot, false, false,
                   false, false, false);
        } else {
            // No rotation - static sprites (oil, bananas, bombs)
            // Priority 2 so items appear below karts (priority 0) but above background (priority 1)
            oamSet(&oamMain, oamSlot, screenX, screenY, OBJPRIORITY_2, paletteNum, spriteSize,
                   SpriteColorFormat_16Color, item->gfx, -1, false, false, false,
                   false, false);
        }
    }
}

void Items_LoadGraphics(void) {
    // Allocate sprite graphics - NOTE: 16-color format!
    itemBoxGfx = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_16Color);
    bananaGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    bombGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    greenShellGfx =
        oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    redShellGfx =
        oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    missileGfx = oamAllocateGfx(&oamMain, SpriteSize_16x32, SpriteColorFormat_16Color);
    oilSlickGfx =
        oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);

    // Copy tile data (note: for 16-color, still divide by 2)
    dmaCopy(item_boxTiles, itemBoxGfx, item_boxTilesLen);
    dmaCopy(bananaTiles, bananaGfx, bananaTilesLen);
    dmaCopy(bombTiles, bombGfx, bombTilesLen);
    dmaCopy(green_shellTiles, greenShellGfx, green_shellTilesLen);
    dmaCopy(red_shellTiles, redShellGfx, red_shellTilesLen);
    dmaCopy(missileTiles, missileGfx, missileTilesLen);
    dmaCopy(oil_slickTiles, oilSlickGfx, oil_slickTilesLen);

    // Copy palettes to separate palette slots (like the example)
    // Start at palette slot 1 (slot 0 is for the kart)
    dmaCopy(item_boxPal, &SPRITE_PALETTE[16], item_boxPalLen);
    dmaCopy(bananaPal, &SPRITE_PALETTE[32], bananaPalLen);
    dmaCopy(bombPal, &SPRITE_PALETTE[48], bombPalLen);
    dmaCopy(green_shellPal, &SPRITE_PALETTE[64], green_shellPalLen);
    dmaCopy(red_shellPal, &SPRITE_PALETTE[80], red_shellPalLen);
    dmaCopy(missilePal, &SPRITE_PALETTE[96], missilePalLen);
    dmaCopy(oil_slickPal, &SPRITE_PALETTE[112], oil_slickPalLen);

    // Update item box spawns with the newly allocated graphics pointer
    for (int i = 0; i < itemBoxCount; i++) {
        itemBoxSpawns[i].gfx = itemBoxGfx;
    }
}

void Items_FreeGraphics(void) {
    if (itemBoxGfx) {
        oamFreeGfx(&oamMain, itemBoxGfx);
        itemBoxGfx = NULL;
    }
    if (bananaGfx) {
        oamFreeGfx(&oamMain, bananaGfx);
        bananaGfx = NULL;
    }
    if (bombGfx) {
        oamFreeGfx(&oamMain, bombGfx);
        bombGfx = NULL;
    }
    if (greenShellGfx) {
        oamFreeGfx(&oamMain, greenShellGfx);
        greenShellGfx = NULL;
    }
    if (redShellGfx) {
        oamFreeGfx(&oamMain, redShellGfx);
        redShellGfx = NULL;
    }
    if (missileGfx) {
        oamFreeGfx(&oamMain, missileGfx);
        missileGfx = NULL;
    }
    if (oilSlickGfx) {
        oamFreeGfx(&oamMain, oilSlickGfx);
        oilSlickGfx = NULL;
    }
}

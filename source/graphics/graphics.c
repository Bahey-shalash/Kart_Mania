/**
 * File: graphics.c
 * ----------------
 * Description: Graphics utility implementations. Provides video_nuke(), a
 *              defensive reset that wipes displays, sprites, palettes, VRAM
 *              banks, and BG registers so the next screen can start from a
 *              clean state.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#include "graphics.h"

#include <nds.h>
#include <string.h>

#include "../core/game_constants.h"

void video_nuke(void) {
    // 1) Turn off both displays (prevents seeing garbage during transition)
    REG_DISPCNT = 0;
    REG_DISPCNT_SUB = 0;

    // 2) Kill sprites
    oamClear(&oamMain, 0, 128);
    oamClear(&oamSub, 0, 128);

    // Reset OAM allocators to prevent memory leaks
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    oamInit(&oamSub, SpriteMapping_1D_32, false);

    // 3) Clear palettes
    memset(BG_PALETTE, 0, PALETTE_SIZE);
    memset(SPRITE_PALETTE, 0, PALETTE_SIZE);
    memset(BG_PALETTE_SUB, 0, PALETTE_SIZE);
    memset(SPRITE_PALETTE_SUB, 0, PALETTE_SIZE);

    // 4) Make VRAM banks CPU-visible in a known mapping, then clear them
    // Adjust to the banks you actually use.
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_SUB_SPRITE;

    memset((void*)VRAM_A, 0, VRAM_BANK_SIZE);
    memset((void*)VRAM_B, 0, VRAM_BANK_SIZE);
    memset((void*)VRAM_C, 0, VRAM_BANK_SIZE);
    memset((void*)VRAM_D, 0, VRAM_BANK_SIZE);

    // 5) Reset BG control regs (optional but clean)
    for (int i = 0; i < 4; i++) {
        BGCTRL[i] = 0;
        BGCTRL_SUB[i] = 0;
    }

    // 6) Reset common transforms/offsets ( avoids “why is it scrolled?” bugs)
    REG_BG0HOFS = REG_BG0VOFS = 0;
    REG_BG1HOFS = REG_BG1VOFS = 0;
    REG_BG2HOFS = REG_BG2VOFS = 0;
    REG_BG3HOFS = REG_BG3VOFS = 0;

    REG_BG0HOFS_SUB = REG_BG0VOFS_SUB = 0;
    REG_BG1HOFS_SUB = REG_BG1VOFS_SUB = 0;
    REG_BG2HOFS_SUB = REG_BG2VOFS_SUB = 0;
    REG_BG3HOFS_SUB = REG_BG3VOFS_SUB = 0;

    // 7) Affine identity (main)
    REG_BG2PA = REG_BG2PD = 256;
    REG_BG2PB = REG_BG2PC = 0;
    REG_BG3PA = REG_BG3PD = 256;
    REG_BG3PB = REG_BG3PC = 0;

    // Affine identity (sub)
    REG_BG2PA_SUB = REG_BG2PD_SUB = 256;
    REG_BG2PB_SUB = REG_BG2PC_SUB = 0;
    REG_BG3PA_SUB = REG_BG3PD_SUB = 256;
    REG_BG3PB_SUB = REG_BG3PC_SUB = 0;
}

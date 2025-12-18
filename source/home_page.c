#include "home_page.h"
#include <stdio.h>
#include "ds_menu.h"
#include "game.h"
#include "settings.h"
#include "button_single_player.h"
#include "button_multi_player.h"
#include "button_settings.h"

//----------Sprite indices for Home Menu----------
#define SPRITE_SINGLE_PLAYER 0
#define SPRITE_MULTIPLAYER 1
#define SPRITE_SETTINGS 2

//----------Button positions----------------------
#define BTN_SINGLE_Y 42
#define BTN_MULTI_Y 96
#define BTN_SETTINGS_Y 150
#define BTN_X 128

//----Sprite Graphics pointers (3 frames each: normal/hovering/pressed)----
u16 *single_player_gfx[3];
u16 *multi_player_gfx[3];
u16 *settings_gfx[3];

MenuButton buttons[3];
int selectedButton = 0;

//----------Configuration Functions----------

void configGraphics_Sub(void)
{
    REG_DISPCNT_SUB = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_SUB_SPRITE;
}

void configBackground_Sub(void)
{
    BGCTRL_SUB[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    swiCopy(ds_menuBitmap, BG_BMP_RAM_SUB(0), ds_menuBitmapLen / 2);
    swiCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen / 2);

    // Affine matrix for background
    REG_BG2PA_SUB = 256;
    REG_BG2PC_SUB = 0;
    REG_BG2PB_SUB = 0;
    REG_BG2PD_SUB = 256;
}

void configSprites_Sub(void)
{
    oamInit(&oamSub, SpriteMapping_1D_128, false);

    // Allocate graphics for all button frames
    for (int i = 0; i < 3; i++)
    {
        single_player_gfx[i] = oamAllocateGfx(&oamSub, SpriteSize_64x32, SpriteColorFormat_256Color);
        multi_player_gfx[i] = oamAllocateGfx(&oamSub, SpriteSize_64x32, SpriteColorFormat_256Color);
        settings_gfx[i] = oamAllocateGfx(&oamSub, SpriteSize_64x32, SpriteColorFormat_256Color);
    }

    // Load palette (shared between all buttons)
    swiCopy(button_single_playerPal, SPRITE_PALETTE_SUB, button_single_playerPalLen / 2);

    // Load tile data for each button (each frame is 2048 bytes)
    u8 *sp_tiles = (u8 *)button_single_playerTiles;
    u8 *mp_tiles = (u8 *)button_multi_playerTiles;
    u8 *set_tiles = (u8 *)button_settingsTiles;

    for (int i = 0; i < 3; i++)
    {
        swiCopy(sp_tiles + (i * 2048), single_player_gfx[i], 2048 / 2);
        swiCopy(mp_tiles + (i * 2048), multi_player_gfx[i], 2048 / 2);
        swiCopy(set_tiles + (i * 2048), settings_gfx[i], 2048 / 2);
    }

    // Initialize button data
    buttons[0].id = SPRITE_SINGLE_PLAYER;
    buttons[0].x = BTN_X;
    buttons[0].y = BTN_SINGLE_Y;
    buttons[0].pressed = false;

    buttons[1].id = SPRITE_MULTIPLAYER;
    buttons[1].x = BTN_X;
    buttons[1].y = BTN_MULTI_Y;
    buttons[1].pressed = false;

    buttons[2].id = SPRITE_SETTINGS;
    buttons[2].x = BTN_X;
    buttons[2].y = BTN_SETTINGS_Y;
    buttons[2].pressed = false;
}

//----------Initialization & Cleanup----------

void HomePage_initialize(void)
{
    configGraphics_Sub();
    configBackground_Sub();
    configSprites_Sub();
}

void HomePage_cleanup(void)
{
    // Free all allocated sprite graphics
    for (int i = 0; i < 3; i++)
    {
        oamFreeGfx(&oamSub, single_player_gfx[i]);
        oamFreeGfx(&oamSub, multi_player_gfx[i]);
        oamFreeGfx(&oamSub, settings_gfx[i]);
    }

    // Clear all OAM entries
    oamClear(&oamSub, 0, 128);
    oamUpdate(&oamSub);
}

//----------Rendering----------

void updateButtonSprite(MenuButton *btn, bool isSelected, bool isPressed)
{
    // Determine frame based on state
    int frame = isPressed ? 2 : (isSelected ? 1 : 0);

    // Select graphics pointer based on button ID
    u16 *gfx_ptr;
    switch (btn->id)
    {
    case SPRITE_SINGLE_PLAYER:
        gfx_ptr = single_player_gfx[frame];
        break;
    case SPRITE_MULTIPLAYER:
        gfx_ptr = multi_player_gfx[frame];
        break;
    case SPRITE_SETTINGS:
        gfx_ptr = settings_gfx[frame];
        break;
    default:
        return;
    }

    // Update sprite in OAM
    oamSet(&oamSub,
           btn->id,
           btn->x - 32, // Center X
           btn->y - 16, // Center Y
           0,           // Priority
           0,           // Palette
           SpriteSize_64x32,
           SpriteColorFormat_256Color,
           gfx_ptr,
           -1, false, false, false, false, false);
}

void HomePage_updateMenu(void)
{
    // Update all button sprites
    for (int i = 0; i < 3; i++)
    {
        updateButtonSprite(&buttons[i], i == selectedButton, buttons[i].pressed);
    }

    // Update OAM hardware
    oamUpdate(&oamSub);
}

//----------Input Handling----------

void handleDPadInput(void)
{
    int keys = keysDown();

    // Navigate with UP/DOWN
    if (keys & KEY_UP)
    {
        selectedButton = (selectedButton - 1 + 3) % 3;
    }
    if (keys & KEY_DOWN)
    {
        selectedButton = (selectedButton + 1) % 3;
    }

    // Activate with A button
    if (keys & KEY_A)
    {
        buttons[selectedButton].pressed = true;

        switch (selectedButton)
        {
        case 0:
            single_player_pressed();
            break;
        case 1:
            multiplayer_pressed();
            break;
        case 2:
            settings_pressed();
            break;
        }
    }
}

void handleTouchInput(void)
{
    int keys_Held = keysHeld();
    touchPosition touch;
    touchRead(&touch);

    // Only process if actually touching
    if (!(keys_Held & KEY_TOUCH))
        return;

    // Check if touch is within horizontal bounds
    if (touch.px < 33 || touch.px > 223)
        return;

    // Check which button is touched
    if (touch.py >= 23 && touch.py <= 61)
    {
        // Single Player button
        selectedButton = 0;
        buttons[0].pressed = true;
    }
    else if (touch.py >= 77 && touch.py <= 115)
    {
        // Multiplayer button
        selectedButton = 1;
        buttons[1].pressed = true;
    }
    else if (touch.py >= 131 && touch.py <= 169)
    {
        // Settings button
        selectedButton = 2;
        buttons[2].pressed = true;
    }
}

void HomePage_handleInput(void)
{
    // Reset all pressed states
    for (int i = 0; i < 3; i++)
    {
        buttons[i].pressed = false;
    }

    // Handle both input types
    handleDPadInput();
    handleTouchInput();
}
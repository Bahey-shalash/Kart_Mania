#ifndef GAME_TYPES_H
#define GAME_TYPES_H
#include <nds.h>
#include <stdbool.h>

#include "vect2.h"

typedef enum { HOME_PAGE, SETTINGS, MAPSELECTION, GAMEPLAY } GameState;

typedef struct {
    u16* gfx;
    int x;
    int y;
    int id;
} HomeKartSprite;

typedef struct {
    int x, y, width, height;
} MenuItemHitBox;

typedef enum {
    HOME_BTN_NONE = -1,
    HOME_BTN_SINGLEPLAYER = 0,
    HOME_BTN_MULTIPLAYER = 1,
    HOME_BTN_SETTINGS = 2,
    HOME_BTN_COUNT
} HomeButtonSelected;

typedef enum {
    SETTINGS_BTN_NONE = -1,
    SETTINGS_BTN_WIFI = 0,
    SETTINGS_BTN_MUSIC = 1,
    SETTINGS_BTN_SOUND_FX = 2,
    SETTINGS_BTN_SAVE = 3,
    SETTINGS_BTN_BACK = 4,
    SETTINGS_BTN_HOME = 5,
    SETTINGS_BTN_COUNT
} SettingsButtonSelected;

typedef enum {
    SP_BTN_NONE = -1,
    SP_BTN_MAP1 = 0,  // Scorching Sands
    SP_BTN_MAP2 = 1,  // Alpine Rush
    SP_BTN_MAP3 = 2,  // Neon Circuit
    SP_BTN_HOME = 3,  // Home button
    SP_BTN_COUNT
} MapSelectionButton;

typedef enum {
    TILE_RED = 3,
    TILE_GREEN = 4,
    TILE_SEL_WIFI = 5,
    TILE_SEL_MUSIC = 6,
    TILE_SEL_SOUNDFX = 7,
    TILE_SEL_SAVE = 8,
    TILE_SEL_BACK = 9,
    TILE_SEL_HOME = 10
} SettingsTileIndex;

typedef enum { NONEMAP, ScorchingSands, AlpinRush, NeonCircuit } Map;

typedef enum {
    TILE_SEL_MAP1 = 0,
    TILE_SEL_MAP2 = 1,
    TILE_SEL_MAP3 = 2,
    TILE_SEL_SP_HOME = 3
} Map_sel_TileIndex;

typedef enum {
    QUAD_TL = 0,  // Top-Left
    QUAD_TC = 1,  // Top-Center
    QUAD_TR = 2,  // Top-Right
    QUAD_ML = 3,  // Middle-Left
    QUAD_MC = 4,  // Middle-Center
    QUAD_MR = 5,  // Middle-Right
    QUAD_BL = 6,  // Bottom-Left
    QUAD_BC = 7,  // Bottom-Center
    QUAD_BR = 8   // Bottom-Right
} QuadrantID;
#endif  // GAME_TYPES_H
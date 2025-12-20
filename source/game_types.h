#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <nds.h>
#include <stdbool.h>

typedef enum{
    HOME_PAGE,
    SETTINGS,
    SINGLEPLAYER,
    MULTIPLAYER
}GameState ;  // will add more states later

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
    TOGGLE_OFF = 0,
    TOGGLE_ON = 1
} ToggleState;

#endif  // GAME_TYPES_H
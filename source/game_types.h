#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <nds.h>
#include <stdbool.h>

enum GameState {
    HOME_PAGE,
    SETTINGS,
    SINGLEPLAYER,
    MULTIPLAYER
};  // will add more states later

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

#endif  // GAME_TYPES_H
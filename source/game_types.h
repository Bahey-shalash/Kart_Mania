#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdint.h>

enum GameState { HOME_PAGE, SETTINGS };  // will add more states later

typedef struct {
    u16* gfx;
    int x;
    int y;
    int id;
} HomeKartSprite;

enum HomeButtonselected { NONE_button, SINGLE_PLAYER_button, MULTIPLAYER_button, SETTINGS_button };

#endif  // GaME_TYPES_H
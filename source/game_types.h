#ifndef GaME_TYPES_H
#define GaME_TYPES_H

enum GameState { HOME_PAGE, SETTINGS };  // will add more states later

typedef struct {
    u16* gfx;
    int x;
    int y;
    int id;
} HomeKartSprite;

#endif  // GaME_TYPES_H
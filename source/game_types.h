#ifndef GaME_TYPES_H
#define GaME_TYPES_H

enum GameState { HOME_PAGE, SETTINGS };  // will add more states later

typedef struct {
    u16* gfx;
    int x;
    int y;
    int id;
} HomeKartSprite;

typedef struct {
    int id;        // Sprite OAM index (0, 1, 2, etc.)
    int x;         // X position on screen
    int y;         // Y position on screen
    bool pressed;  // Whether button is currently pressed
} MenuButton;
// MenuButton struct - represents a single button sprite on screen

#endif  // GaME_TYPES_H
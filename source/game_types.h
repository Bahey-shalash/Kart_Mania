#ifndef GaME_TYPES_H
#define GaME_TYPES_H

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
    int id;        // Sprite OAM index (0, 1, 2, etc.)
    int x;         // X position on screen
    int y;         // Y position on screen
    bool pressed;  // Whether button is currently pressed
} MenuButton;
// MenuButton struct - represents a single button sprite on screen

typedef struct {
    int x, y, width, height;
} MenuItemHitBox;

enum HomeButtonselected {
    NONE_button = -1,
    SINGLE_PLAYER_button = 0,
    MULTIPLAYER_button = 1,
    SETTINGS_button = 2
};

#endif  // GaME_TYPES_H
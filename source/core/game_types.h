// HUGO------

#ifndef GAME_TYPES_H
#define GAME_TYPES_H
#include <nds.h>
#include <stdbool.h>
#include "../math/fixedmath2d.h"
typedef enum {
    HOME_PAGE,
    MAPSELECTION,
    MULTIPLAYER_LOBBY,
    GAMEPLAY,
    PLAYAGAIN,
    SETTINGS,
    REINIT_HOME  // NEW: Forces home page reinit after WiFi failure
} GameState;

typedef enum { NONEMAP, ScorchingSands, AlpinRush, NeonCircuit } Map;

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
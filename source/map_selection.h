#ifndef MAPSELECTION_H
#define MAPSELECTION_H
#include "game_types.h"
// BAHEY------
void Map_Selection_initialize(void);
GameState Map_selection_update(void);
void Map_selection_OnVBlank(void);

#endif  // MAPSELECTION_H
#ifndef SINGLEPLAYER_H
#define SINGLEPLAYER_H
#include "game_types.h"

void Singleplayer_initialize(void);
GameState Singleplayer_update(void);
void Singleplayer_OnVBlank(void);

#endif  // SINGLEPLAYER_H
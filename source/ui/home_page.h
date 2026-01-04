#ifndef HOME_PAGE_H
#define HOME_PAGE_H
// BAHEY------
#include "../core/game_types.h"

void HomePage_initialize(void);
GameState HomePage_update(void);
void HomePage_OnVBlank(void);
void HomePage_Cleanup(void);

#endif
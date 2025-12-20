#ifndef GAME_H
#define GAME_H
#include "game_types.h"

void Singleplayer_initialize(void);
GameState Singleplayer_update(void);
void configureGraphics_MAIN_Singleplayer(void);
void configBG_Main_Singleplayer(void);
void configureGraphics_Sub_Singleplayer(void);
void configBG_Sub_Singleplayer(void);
void handleDPadInputSingleplayer(void);
void handleTouchInputSingleplayer(void);

#endif // GAME_H
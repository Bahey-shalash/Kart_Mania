#ifndef SETTINGS_H
#define SETTINGS_H

#include <nds.h>
#include <stdbool.h>

#include "game_types.h"

void Settings_initialize(void);
GameState Settings_update(void);

//=============================================================================
// INTERNAL (exposed for testing if needed)
//=============================================================================

void configBG_Main_Settings(void);
void configureGraphics_MAIN_Settings(void);
void configGraphics_Sub_SETTINGS(void);
void configBackground_Sub_SETTINGS(void);


#endif

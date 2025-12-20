#ifndef SETTINGS_H
#define SETTINGS_H

#include <nds.h>
#include <stdbool.h>

#include "game_types.h"

#define BG_SCROLL_MAX 320
#define BG_SCROLL_STEP 8

void Settings_initialize(void);
GameState Settings_update(void);
void Settings_cleanup(void);
void Settings_configGraphics_Sub(void);
void Settings_configBackground_Sub(void);

#endif

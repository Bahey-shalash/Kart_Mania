#ifndef SETTINGS_H
#define SETTINGS_H

#include <nds.h>
#include <stdbool.h>

#include "game_types.h"

void Settings_initialize(void);
void Settings_update(void);
void Settings_cleanup(void);
void setCursorOverlay(int settingIndex, bool show);
void Settings_configGraphics_Sub(void);
void Settings_configBackground_Sub(void);

#endif

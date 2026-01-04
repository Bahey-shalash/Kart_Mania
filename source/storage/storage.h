/**
 * storage.h
 * Game settings persistence (save/load)
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>

//=============================================================================
// Constants
//=============================================================================

#define STORAGE_DIR "/kart-mania"
#define SETTINGS_FILE "/kart-mania/settings.txt"
#define DEFAULT_SETTINGS_FILE "/kart-mania/default_settings.txt"

//=============================================================================
// Public API
//=============================================================================

/** Initialize storage (FAT filesystem, create directory and files if needed) */
bool Storage_Init(void);

/** Load settings from file into GameContext */
bool Storage_LoadSettings(void);

/** Save current GameContext settings to file */
bool Storage_SaveSettings(void);

/** Reset settings to default values */
bool Storage_ResetToDefaults(void);

#endif  // STORAGE_H
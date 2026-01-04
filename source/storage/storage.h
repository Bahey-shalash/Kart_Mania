#ifndef STORAGE_H
#define STORAGE_H
// HUGO------
/*
 *Storage never triggers side effects. It only mutates GameContext data.
 */

#include <stdbool.h>

#define STORAGE_DIR "/kart-mania"
#define SETTINGS_FILE "/kart-mania/settings.txt"
#define DEFAULT_SETTINGS_FILE "/kart-mania/default_settings.txt"

// Initialize storage - call fatInitDefault(), create directory and files if needed
// Returns true if storage is available, false if SD card missing/failed
bool Storage_Init(void);

// Load settings from settings.txt into GameContext
// Returns true on success, false on failure (will use defaults)
bool Storage_LoadSettings(void);

// Save current GameContext settings to settings.txt
// Returns true on success
bool Storage_SaveSettings(void);

// Reset settings.txt to default values (for later: START+SELECT+SAVE)
// Returns true on success
bool Storage_ResetToDefaults(void);

#endif  // STORAGE_H
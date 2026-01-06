/**
 * File: storage.h
 * ---------------
 * Description: Persistent storage interface for game settings on SD card.
 *              Manages user preferences (WiFi, music, sound effects) using
 *              FAT filesystem. Provides initialization, load, save, and
 *              factory reset operations. Does not trigger side effects -
 *              only mutates GameContext data.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>

//=============================================================================
// PUBLIC CONSTANTS
//=============================================================================

// Storage directory path on SD card
#define STORAGE_DIR "/kart-mania"

// User settings file path (modified by user)
#define SETTINGS_FILE "/kart-mania/settings.txt"

// Default settings file path (reference copy)
#define DEFAULT_SETTINGS_FILE "/kart-mania/default_settings.txt"

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * Function: Storage_Init
 * ----------------------
 * Initializes FAT filesystem and creates storage directory structure.
 *
 * Creates the following if they don't exist:
 *   - /kart-mania directory
 *   - /kart-mania/default_settings.txt (reference defaults)
 *   - /kart-mania/settings.txt (user settings, initialized from defaults)
 *   - /kart-mania/best_times.txt (personal best lap times)
 *
 * Returns:
 *   true  - Storage initialized successfully, SD card accessible
 *   false - Initialization failed (SD card missing or filesystem error)
 */
bool Storage_Init(void);

/**
 * Function: Storage_LoadSettings
 * ------------------------------
 * Loads user settings from settings.txt into GameContext.
 *
 * Reads settings file and updates GameContext fields:
 *   - userSettings.wifiEnabled
 *   - userSettings.musicEnabled
 *   - userSettings.soundFxEnabled
 *
 * Does NOT trigger side effects (no audio start/stop, no WiFi init).
 * Caller is responsible for applying loaded settings.
 *
 * Returns:
 *   true  - Settings loaded successfully
 *   false - File not found or read error (caller should use defaults)
 */
bool Storage_LoadSettings(void);

/**
 * Function: Storage_SaveSettings
 * ------------------------------
 * Saves current GameContext settings to settings.txt.
 *
 * Writes current values to persistent storage:
 *   - wifi=0/1
 *   - music=0/1
 *   - soundfx=0/1
 *
 * Returns:
 *   true  - Settings saved successfully
 *   false - Write error (SD card removed or full)
 */
bool Storage_SaveSettings(void);

/**
 * Function: Storage_ResetToDefaults
 * ----------------------------------
 * Resets settings.txt to factory defaults and reloads into GameContext.
 *
 * Default values:
 *   - WiFi: Enabled (1)
 *   - Music: Enabled (1)
 *   - Sound FX: Enabled (1)
 *
 * Triggered by: START+SELECT+A on Settings screen Save button
 *
 * Returns:
 *   true  - Reset successful, defaults loaded into context
 *   false - Write or reload error
 */
bool Storage_ResetToDefaults(void);

#endif  // STORAGE_H
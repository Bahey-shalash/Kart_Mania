/**
 * File: storage.c
 * ---------------
 * Description: Implementation of persistent storage for game settings on SD card.
 *              Uses FAT filesystem to read/write user preferences. Implements
 *              directory creation, file I/O, and default settings management.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "storage.h"

#include <dirent.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include "../core/context.h"
#include "storage_pb.h"

//=============================================================================
// PRIVATE HELPER FUNCTIONS
//=============================================================================

/**
 * Checks if a directory exists on the filesystem.
 *
 * Parameters:
 *   path - Directory path to check
 *
 * Returns: true if directory exists and is accessible, false otherwise
 */
static bool Storage_DirectoryExists(const char* path) {
    DIR* dir = opendir(path);
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
}

/**
 * Checks if a file exists on the filesystem.
 *
 * Parameters:
 *   path - File path to check
 *
 * Returns: true if file exists and is accessible, false otherwise
 */
static bool Storage_FileExists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

/**
 * Writes factory default settings to a file.
 *
 * Default settings:
 *   - wifi=1 (WiFi enabled)
 *   - music=1 (Music enabled)
 *   - soundfx=1 (Sound effects enabled)
 *
 * Parameters:
 *   path - File path to write defaults to
 *
 * Returns: true on success, false if file cannot be created/written
 */
static bool Storage_WriteDefaultsToFile(const char* path) {
    FILE* file = fopen(path, "w+");
    if (file == NULL)
        return false;

    fprintf(file, "wifi=1\n");
    fprintf(file, "music=1\n");
    fprintf(file, "soundfx=1\n");
    fclose(file);
    return true;
}

//=============================================================================
// PUBLIC API
//=============================================================================

bool Storage_Init(void) {
    // Initialize FAT filesystem
    if (!fatInitDefault())
        return false;

    // Create /kart-mania directory if it doesn't exist
    if (!Storage_DirectoryExists(STORAGE_DIR)) {
        mkdir(STORAGE_DIR, 0777);
    }

    // Create default_settings.txt if it doesn't exist
    if (!Storage_FileExists(DEFAULT_SETTINGS_FILE)) {
        if (!Storage_WriteDefaultsToFile(DEFAULT_SETTINGS_FILE))
            return false;
    }

    // Create settings.txt if it doesn't exist (copy defaults)
    if (!Storage_FileExists(SETTINGS_FILE)) {
        if (!Storage_WriteDefaultsToFile(SETTINGS_FILE))
            return false;
    }
    // Initialize personal best times
    if (!StoragePB_Init()) {
        return false;
    }
    return true;
}

bool Storage_LoadSettings(void) {
    FILE* file = fopen(SETTINGS_FILE, "r");
    if (file == NULL)
        return false;

    GameContext* ctx = GameContext_Get();

    int wifi = 1, music = 1, soundfx = 1;
    char line[32];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "wifi=", 5) == 0) {
            wifi = (line[5] == '1') ? 1 : 0;
        } else if (strncmp(line, "music=", 6) == 0) {
            music = (line[6] == '1') ? 1 : 0;
        } else if (strncmp(line, "soundfx=", 8) == 0) {
            soundfx = (line[8] == '1') ? 1 : 0;
        }
    }

    fclose(file);

    // Apply to context (don't trigger side effects yet - main.c will do that)
    ctx->userSettings.wifiEnabled = wifi;
    ctx->userSettings.musicEnabled = music;
    ctx->userSettings.soundFxEnabled = soundfx;

    return true;
}

bool Storage_SaveSettings(void) {
    GameContext* ctx = GameContext_Get();

    FILE* file = fopen(SETTINGS_FILE, "w+");
    if (file == NULL)
        return false;

    fprintf(file, "wifi=%d\n", ctx->userSettings.wifiEnabled ? 1 : 0);
    fprintf(file, "music=%d\n", ctx->userSettings.musicEnabled ? 1 : 0);
    fprintf(file, "soundfx=%d\n", ctx->userSettings.soundFxEnabled ? 1 : 0);

    fclose(file);
    return true;
}

bool Storage_ResetToDefaults(void) {
    // Write factory defaults to settings.txt
    if (!Storage_WriteDefaultsToFile(SETTINGS_FILE))
        return false;

    // Reload defaults into context
    return Storage_LoadSettings();
}

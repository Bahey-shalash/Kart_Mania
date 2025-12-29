#include "storage.h"

#include <dirent.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include "context.h"
#include "storage_pb.h"
// Check if a directory exists
static bool directoryExists(const char* path) {
    DIR* dir = opendir(path);
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
}

// Check if a file exists
static bool fileExists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

// Write default settings to a file
static bool writeDefaultsToFile(const char* path) {
    FILE* file = fopen(path, "w+");
    if (file == NULL)
        return false;

    fprintf(file, "wifi=1\n");
    fprintf(file, "music=1\n");
    fprintf(file, "soundfx=1\n");
    fclose(file);
    return true;
}

bool Storage_Init(void) {
    // Initialize FAT filesystem
    if (!fatInitDefault())
        return false;

    // Create /kart-mania directory if it doesn't exist
    if (!directoryExists(STORAGE_DIR)) {
        mkdir(STORAGE_DIR, 0777);
    }

    // Create default_settings.txt if it doesn't exist
    if (!fileExists(DEFAULT_SETTINGS_FILE)) {
        if (!writeDefaultsToFile(DEFAULT_SETTINGS_FILE))
            return false;
    }

    // Create settings.txt if it doesn't exist (copy defaults)
    if (!fileExists(SETTINGS_FILE)) {
        if (!writeDefaultsToFile(SETTINGS_FILE))
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
    // For now, just write hardcoded defaults to settings.txt
    // Later could copy from default_settings.txt
    if (!writeDefaultsToFile(SETTINGS_FILE))
        return false;

    // Reload into context
    return Storage_LoadSettings();
}
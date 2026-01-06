# Storage Module

## Overview

The storage module provides persistent storage for game settings and personal best lap times on SD card using the FAT filesystem. It manages user preferences (WiFi, music, sound effects) and racing records across game sessions. The module follows a strict separation of concerns: storage functions only mutate GameContext data without triggering side effects.

**Key Features:**
- **FAT Filesystem Integration**: Uses libnds FAT library for SD card access
- **User Settings**: WiFi, music, and sound FX toggles persisted across sessions
- **Personal Best Times**: Per-map lap time records with automatic record detection
- **Factory Reset**: Restore default settings with START+SELECT+A combo
- **No Side Effects**: Storage operations only update data structures, callers apply changes
- **Directory Structure**: Organized `/kart-mania/` directory with separate files

## Architecture

### File System Structure

```
SD Card Root
└── kart-mania/
    ├── settings.txt           - User settings (modified by user)
    ├── default_settings.txt   - Factory defaults (reference copy)
    └── best_times.txt         - Personal best lap times per map
```

### File Formats

**settings.txt and default_settings.txt:**
```
wifi=1
music=1
soundfx=1
```
- Values: `0` (disabled) or `1` (enabled)
- Line-based key=value format
- All three keys always present

**best_times.txt:**
```
ScorchingSands=01:23.456
AlpinRush=02:15.789
NeonCircuit=03:45.123
```
- Format: `MapName=MM:SS.mmm`
- One line per map
- Only maps with recorded times appear in file
- File starts empty, grows as records are set

### Module Organization

The storage system is split into two sub-modules:

| Module | Files | Responsibility |
|--------|-------|----------------|
| **Settings Storage** | storage.h/c | User preferences (WiFi, music, sound FX) |
| **Personal Bests** | storage_pb.h/c | Lap time records per map |

Both modules share the same `/kart-mania/` directory.

### Initialization Flow

```
main.c calls Storage_Init()
    ↓
1. Initialize FAT filesystem (fatInitDefault)
2. Create /kart-mania/ directory if missing
3. Create default_settings.txt if missing (write defaults)
4. Create settings.txt if missing (copy defaults)
5. Call StoragePB_Init()
    ↓
   5a. Create best_times.txt if missing (empty file)
    ↓
6. Return success/failure
```

## Public API - Settings Storage

### Storage_Init

```c
bool Storage_Init(void);
```

**Location:** [storage.c:92](../source/storage/storage.c#L92)

Initializes FAT filesystem and creates storage directory structure.

**Creates the following if they don't exist:**
- `/kart-mania` directory
- `/kart-mania/default_settings.txt` (factory defaults)
- `/kart-mania/settings.txt` (user settings)
- `/kart-mania/best_times.txt` (personal bests)

**Returns:**
- `true` - Storage initialized successfully, SD card accessible
- `false` - Initialization failed (SD card missing, filesystem error, or file creation failed)

**Example Usage:**
```c
// In main.c during game startup
if (!Storage_Init()) {
    // SD card unavailable, run without persistent storage
    iprintf("SD card not detected\n");
    iprintf("Settings will not be saved\n");
    // Continue with defaults
}
```

### Storage_LoadSettings

```c
bool Storage_LoadSettings(void);
```

**Location:** [storage.c:120](../source/storage/storage.c#L120)

Loads user settings from settings.txt into GameContext.

**Updates GameContext fields:**
- `userSettings.wifiEnabled` - WiFi toggle
- `userSettings.musicEnabled` - Music toggle
- `userSettings.soundFxEnabled` - Sound effects toggle

**Does NOT trigger side effects:**
- No audio start/stop
- No WiFi initialization
- Only mutates GameContext data

**Returns:**
- `true` - Settings loaded successfully
- `false` - File not found or read error (caller should use defaults)

**Example Usage:**
```c
// Load settings and apply them
if (Storage_LoadSettings()) {
    GameContext* ctx = GameContext_Get();

    // Apply WiFi setting
    if (ctx->userSettings.wifiEnabled) {
        WiFi_Init();
    }

    // Apply audio settings
    if (ctx->userSettings.musicEnabled) {
        Sound_PlayMusic();
    }

    if (!ctx->userSettings.soundFxEnabled) {
        Sound_DisableEffects();
    }
} else {
    // Use defaults (all enabled)
    iprintf("Using default settings\n");
}
```

### Storage_SaveSettings

```c
bool Storage_SaveSettings(void);
```

**Location:** [storage.c:149](../source/storage/storage.c#L149)

Saves current GameContext settings to settings.txt.

**Writes current values:**
- `wifi=0/1` - From `userSettings.wifiEnabled`
- `music=0/1` - From `userSettings.musicEnabled`
- `soundfx=0/1` - From `userSettings.soundFxEnabled`

**Returns:**
- `true` - Settings saved successfully
- `false` - Write error (SD card removed, full, or write-protected)

**Example Usage:**
```c
// After user changes settings
GameContext* ctx = GameContext_Get();

// User toggled music off
ctx->userSettings.musicEnabled = false;
Sound_StopMusic();

// Save to persistent storage
if (!Storage_SaveSettings()) {
    iprintf("Failed to save settings\n");
    iprintf("SD card may be removed\n");
}
```

### Storage_ResetToDefaults

```c
bool Storage_ResetToDefaults(void);
```

**Location:** [storage.c:165](../source/storage/storage.c#L165)

Resets settings.txt to factory defaults and reloads into GameContext.

**Default values:**
- WiFi: Enabled (1)
- Music: Enabled (1)
- Sound FX: Enabled (1)

**Triggered by:** START+SELECT+A on Settings screen Save button

**Returns:**
- `true` - Reset successful, defaults loaded into context
- `false` - Write or reload error

**Example Usage:**
```c
// In Settings screen when START+SELECT+A pressed on Save button
if (keysHeld() & KEY_START && keysHeld() & KEY_SELECT && keysDown() & KEY_A) {
    if (Storage_ResetToDefaults()) {
        iprintf("Settings reset to defaults\n");

        // Apply restored defaults
        GameContext* ctx = GameContext_Get();
        WiFi_Init();
        Sound_PlayMusic();
        Sound_EnableEffects();
    } else {
        iprintf("Reset failed\n");
    }
}
```

## Public API - Personal Bests Storage

### StoragePB_Init

```c
bool StoragePB_Init(void);
```

**Location:** [storage_pb.c:89](../source/storage/storage_pb.c#L89)

Initializes best times file if it doesn't exist.

**Creates:** `/kart-mania/best_times.txt` (empty file)

**Returns:**
- `true` - File initialized or already exists
- `false` - File creation failed (SD card error)

**Note:** Called automatically by Storage_Init(), not usually called directly.

### StoragePB_LoadBestTime

```c
bool StoragePB_LoadBestTime(Map map, int* min, int* sec, int* msec);
```

**Location:** [storage_pb.c:100](../source/storage/storage_pb.c#L100)

Loads best lap time for a specific map from file.

**Parameters:**
- `map` - Map enum (ScorchingSands, AlpinRush, NeonCircuit)
- `min` - Output: Minutes component
- `sec` - Output: Seconds component (0-59)
- `msec` - Output: Milliseconds component (0-999)

**Returns:**
- `true` - Record found, output parameters populated
- `false` - No record exists for this map, output parameters unchanged

**Example Usage:**
```c
// Display personal best for selected map
Map selected_map = ScorchingSands;
int best_min, best_sec, best_msec;

if (StoragePB_LoadBestTime(selected_map, &best_min, &best_sec, &best_msec)) {
    iprintf("Personal Best:\n");
    iprintf("%02d:%02d.%03d\n", best_min, best_sec, best_msec);
} else {
    iprintf("No record yet!\n");
}
```

### StoragePB_SaveBestTime

```c
bool StoragePB_SaveBestTime(Map map, int min, int sec, int msec);
```

**Location:** [storage_pb.c:128](../source/storage/storage_pb.c#L128)

Saves best lap time for a specific map (only if it's a new record).

**Compares new time against existing record. Only writes if:**
1. No previous record exists, OR
2. New time is faster than existing record

**Parameters:**
- `map` - Map enum to save time for
- `min` - Minutes component
- `sec` - Seconds component (0-59)
- `msec` - Milliseconds component (0-999)

**Returns:**
- `true` - New record! Time was saved (faster or first time)
- `false` - Not a record, existing time is faster (nothing saved)

**Example Usage:**
```c
// After race finishes
Map completed_map = ScorchingSands;
int race_min = 1;
int race_sec = 20;
int race_msec = 543;

if (StoragePB_SaveBestTime(completed_map, race_min, race_sec, race_msec)) {
    // New record!
    iprintf("NEW RECORD!\n");
    iprintf("%02d:%02d.%03d\n", race_min, race_sec, race_msec);
    Sound_PlayNewRecordFanfare();
} else {
    iprintf("Race complete\n");
    // Show existing record for comparison
    int best_min, best_sec, best_msec;
    StoragePB_LoadBestTime(completed_map, &best_min, &best_sec, &best_msec);
    iprintf("Best: %02d:%02d.%03d\n", best_min, best_sec, best_msec);
}
```

## Private Implementation - Settings

### Storage_DirectoryExists

```c
static bool Storage_DirectoryExists(const char* path);
```

**Location:** [storage.c:37](../source/storage/storage.c#L37)

Checks if a directory exists on the filesystem.

**Implementation:**
```c
DIR* dir = opendir(path);
if (dir) {
    closedir(dir);
    return true;
}
return false;
```

### Storage_FileExists

```c
static bool Storage_FileExists(const char* path);
```

**Location:** [storage.c:54](../source/storage/storage.c#L54)

Checks if a file exists on the filesystem.

**Implementation:**
```c
FILE* file = fopen(path, "r");
if (file) {
    fclose(file);
    return true;
}
return false;
```

### Storage_WriteDefaultsToFile

```c
static bool Storage_WriteDefaultsToFile(const char* path);
```

**Location:** [storage.c:76](../source/storage/storage.c#L76)

Writes factory default settings to a file.

**Default Settings:**
```c
fprintf(file, "wifi=1\n");
fprintf(file, "music=1\n");
fprintf(file, "soundfx=1\n");
```

All settings enabled by default.

## Private Implementation - Personal Bests

### StoragePB_MapToString

```c
static const char* StoragePB_MapToString(Map map);
```

**Location:** [storage_pb.c:31](../source/storage/storage_pb.c#L31)

Converts map enum to string representation for file storage.

**Mapping:**
```c
ScorchingSands → "ScorchingSands"
AlpinRush      → "AlpinRush"
NeonCircuit    → "NeonCircuit"
default        → "Unknown"
```

### StoragePB_IsTimeFaster

```c
static bool StoragePB_IsTimeFaster(int min1, int sec1, int msec1,
                                    int min2, int sec2, int msec2);
```

**Location:** [storage_pb.c:55](../source/storage/storage_pb.c#L55)

Compares two lap times to determine which is faster.

**Algorithm:**
```c
1. Compare minutes: if min1 < min2, time1 is faster
2. If minutes equal, compare seconds
3. If seconds equal, compare milliseconds
4. Return true if time1 < time2
```

**Example:**
```c
// Is 01:23.456 faster than 01:25.123?
bool result = StoragePB_IsTimeFaster(1, 23, 456, 1, 25, 123);
// result = true (23 seconds < 25 seconds)
```

### StoragePB_FileExists

```c
static bool StoragePB_FileExists(const char* path);
```

**Location:** [storage_pb.c:76](../source/storage/storage_pb.c#L76)

Checks if a file exists (same implementation as Storage_FileExists).

## Usage Patterns

### Complete Initialization Sequence

```c
// main.c startup
int main(void) {
    // Initialize storage
    bool has_storage = Storage_Init();

    if (!has_storage) {
        iprintf("SD card not found\n");
        iprintf("Running with defaults\n");
    }

    // Load saved settings
    if (has_storage && Storage_LoadSettings()) {
        // Apply loaded settings
        GameContext* ctx = GameContext_Get();

        if (ctx->userSettings.wifiEnabled) {
            WiFi_Init();
        }
        if (ctx->userSettings.musicEnabled) {
            Sound_PlayMusic();
        }
        if (!ctx->userSettings.soundFxEnabled) {
            Sound_DisableEffects();
        }
    } else {
        // Use defaults (all enabled)
        WiFi_Init();
        Sound_PlayMusic();
    }

    // ... rest of game initialization ...
}
```

### Settings Screen Save Flow

```c
// In Settings_OnSavePressed() function
GameContext* ctx = GameContext_Get();

// Check for factory reset combo: START+SELECT+A
if (keysHeld() & KEY_START && keysHeld() & KEY_SELECT && keysDown() & KEY_A) {
    // Factory reset
    if (Storage_ResetToDefaults()) {
        iprintf("Settings reset!\n");

        // Apply defaults
        WiFi_Init();
        Sound_PlayMusic();
        Sound_EnableEffects();

        return STATE_SETTINGS;  // Stay on settings screen
    }
} else {
    // Normal save
    if (Storage_SaveSettings()) {
        iprintf("Settings saved!\n");
    } else {
        iprintf("Save failed\n");
    }

    return STATE_HOME;  // Return to home screen
}
```

### Race Completion with Record Check

```c
// After race timer stops
void OnRaceComplete(Map completed_map, int final_min, int final_sec, int final_msec) {
    // Check if it's a new record
    bool is_record = StoragePB_SaveBestTime(completed_map, final_min, final_sec,
                                              final_msec);

    if (is_record) {
        // NEW RECORD!
        DisplayNewRecordScreen(final_min, final_sec, final_msec);
        Sound_PlayNewRecordFanfare();
    } else {
        // Not a record, show comparison
        int best_min, best_sec, best_msec;
        StoragePB_LoadBestTime(completed_map, &best_min, &best_sec, &best_msec);

        DisplayRaceResultsScreen(final_min, final_sec, final_msec,
                                  best_min, best_sec, best_msec);
    }
}
```

### Map Selection Screen Display

```c
// Display personal best for currently selected map
void MapSelection_DrawBestTime(Map selected_map) {
    int best_min, best_sec, best_msec;

    if (StoragePB_LoadBestTime(selected_map, &best_min, &best_sec, &best_msec)) {
        iprintf("\nPersonal Best:\n");
        iprintf("%02d:%02d.%03d\n", best_min, best_sec, best_msec);
    } else {
        iprintf("\nNo record yet\n");
        iprintf("Set your first time!\n");
    }
}
```

### Handling SD Card Removal

```c
// Check if save succeeded
if (!Storage_SaveSettings()) {
    // Save failed
    iprintf("ERROR\n");
    iprintf("SD card may be removed\n");
    iprintf("Settings not saved\n");

    // Game continues running with current settings
    // (no crash, graceful degradation)
}
```

## Design Notes

### No Side Effects Policy

**Why storage functions don't trigger side effects:**

1. **Separation of Concerns**: Storage layer only handles persistence, not application logic
2. **Caller Control**: Caller decides when/how to apply loaded settings
3. **Testing**: Easier to test storage without audio/WiFi dependencies
4. **Flexibility**: Settings can be loaded without immediately applying them

**Example:**
```c
// Load settings without applying them
Storage_LoadSettings();  // Only updates GameContext

// Caller decides what to apply
GameContext* ctx = GameContext_Get();
if (ctx->userSettings.wifiEnabled && user_wants_wifi) {
    WiFi_Init();  // Explicit side effect
}
```

### Factory Reset Security

**START+SELECT+A combo prevents accidental resets:**
- Requires three buttons pressed simultaneously
- Must be on Save button
- Unlikely to trigger accidentally
- Provides immediate visual feedback

### Personal Best File Format

**Why text format instead of binary:**

1. **Human Readable**: Easy to debug, inspect with text editor
2. **Small Size**: Only 3 maps, minimal overhead
3. **Flexibility**: Easy to add/remove entries
4. **Portability**: Works across different DS flashcarts
5. **Corruption Recovery**: Partial corruption doesn't invalidate entire file

**Trade-offs:**
- Slightly slower parsing (negligible for 3 entries)
- Larger file size (negligible: ~80 bytes total)

### In-Memory Record Update

**StoragePB_SaveBestTime() updates file in-place:**

```
1. Read all existing records into memory (10 lines max)
2. Update or add new record in memory
3. Write entire file back
```

**Why not append-only:**
- Need to update existing records (better times)
- File stays small (no duplicates)
- Fast enough for 3 maps

**Memory usage:** 640 bytes (10 lines × 64 bytes per line)

### Error Handling Strategy

**All functions return bool for success/failure:**

```c
if (!Storage_Init()) {
    // Graceful degradation: use defaults, no persistence
}

if (!Storage_SaveSettings()) {
    // Inform user, continue running
}
```

**No exceptions or error codes:**
- Simple success/failure model
- Caller decides how to handle failure
- Game never crashes due to storage errors

## Performance & Integration

### Performance Characteristics

**Settings Operations:**
- `Storage_Init()`: ~50ms (filesystem init + 3 file checks/creates)
- `Storage_LoadSettings()`: ~5ms (single file read, 3 lines)
- `Storage_SaveSettings()`: ~10ms (file write, 3 lines)
- `Storage_ResetToDefaults()`: ~15ms (write + reload)

**Personal Best Operations:**
- `StoragePB_Init()`: ~5ms (single file check/create)
- `StoragePB_LoadBestTime()`: ~5ms (scan up to 10 lines)
- `StoragePB_SaveBestTime()`: ~15ms (read all, modify, write all)

**Typical Usage:**
- Init: Once at startup
- Load: Once at startup, once after reset
- Save: ~2 times per session (settings changes)
- PB Save: ~3 times per session (race completions)

### Dependencies

**Required Libraries:**
- `fat.h` - FAT filesystem (libnds)
- `stdio.h` - File I/O (fopen, fprintf, fgets, etc.)
- `string.h` - String operations (strcmp, strncmp, strncpy, snprintf)
- `dirent.h` - Directory operations (opendir, closedir)
- `sys/stat.h` - File/directory creation (mkdir)

**Internal Dependencies:**
- `context.h` - GameContext access for settings
- `game_types.h` - Map enum for personal bests
- `storage_pb.h` - Personal bests sub-module

### Integration Points

**Called by:**
- `main.c` - Storage_Init(), Storage_LoadSettings() at startup
- `settings.c` - Storage_SaveSettings(), Storage_ResetToDefaults() when saving
- `gameplay.c` / race completion - StoragePB_SaveBestTime() after race
- `map_selection.c` - StoragePB_LoadBestTime() to display records

**State Dependencies:**
- Requires SD card inserted and accessible
- GameContext must be initialized before Load/Save
- FAT filesystem must be initialized before any operations

### Testing Considerations

**Test Cases:**
1. **No SD Card**: Init fails gracefully, game runs with defaults
2. **Empty SD Card**: Creates directory and files on first run
3. **Existing Files**: Loads and preserves existing data
4. **Corrupted settings.txt**: Defaults to factory settings
5. **Corrupted best_times.txt**: Returns "no record" for affected maps
6. **SD Card Removed During Save**: Returns false, no crash
7. **Factory Reset**: Restores all defaults, writes to file
8. **New Record**: Saves only if faster than existing
9. **Slower Time**: Returns false, doesn't overwrite record
10. **Multiple Maps**: Each map has independent record

**Manual Testing:**
- Insert/remove SD card during gameplay
- Manually edit files with invalid data
- Test factory reset combo on settings screen
- Race multiple times, verify record updates correctly
- Check file contents with text editor

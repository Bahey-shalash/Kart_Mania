# Game Context System

## Overview

The context system provides a global singleton that manages all application-wide game state. It serves as the single source of truth for user settings, current game state, and gameplay configuration.

The context is implemented as a singleton pattern in [context.c](../source/core/context.c) and [context.h](../source/core/context.h), accessible throughout the codebase via `GameContext_Get()`.

**Key Features:**
- Singleton pattern for centralized state management
- Automatic side effects when changing settings (e.g., music starts/stops immediately)
- Type-safe access through dedicated getter/setter functions
- Initialized once at startup with saved values or defaults

## Architecture

### GameContext Structure

The `GameContext` struct ([context.h:19-25](../source/core/context.h#L19-L25)) contains:

```c
typedef struct {
    UserSettings userSettings;     // WiFi, music, sound FX toggles
    GameState currentGameState;    // Current screen (HOME_PAGE, GAMEPLAY, etc.)
    Map SelectedMap;               // Selected map for gameplay
    bool isMultiplayerMode;        // Singleplayer vs multiplayer mode
} GameContext;
```

#### UserSettings

User preferences that persist across state transitions:
- **wifiEnabled**: WiFi/multiplayer preference (see wifi.md for implementation details)
- **musicEnabled**: Background music toggle
- **soundFxEnabled**: Sound effects toggle

#### Game State

- **currentGameState**: Current screen/mode from `GameState` enum (HOME_PAGE, MAPSELECTION, GAMEPLAY, PLAYAGAIN)
- **SelectedMap**: Map chosen for racing from `Map` enum (MAP1, MAP2, MAP3, etc.)
- **isMultiplayerMode**: Whether the current session is multiplayer or singleplayer

## Context Access

### GameContext_Get

**Signature:** `GameContext* GameContext_Get(void)`
**Defined in:** [context.c:19-21](../source/core/context.c#L19-L21)

Returns a pointer to the singleton `GameContext` instance. This is the primary way to access global game state.

```c
const GameContext* ctx = GameContext_Get();
if (ctx->currentGameState == GAMEPLAY) {
    // Do gameplay-specific logic
}
```

**Note:** Currently returns a non-const pointer. See [context.c:22](../source/core/context.c#L22) TODO about making it const to prevent direct modification.

### GameContext_InitDefaults

**Signature:** `void GameContext_InitDefaults(void)`
**Defined in:** [context.c:24-32](../source/core/context.c#L24-L32)

Initializes the global context with default values. Called once during game startup in `main()`.

**Default Values:**
- WiFi enabled: `true`
- Music enabled: `true`
- Sound effects enabled: `true`
- Game state: `HOME_PAGE`
- Selected map: `NONEMAP`
- Multiplayer mode: `false`

## Settings Management

All settings setters have **immediate side effects** - they don't just update the context, they also apply the change to the relevant subsystem.

### GameContext_SetMusicEnabled

**Signature:** `void GameContext_SetMusicEnabled(bool enabled)`
**Defined in:** [context.c:38-41](../source/core/context.c#L38-L41)

Enables or disables background music.

**Side Effect:** Calls `MusicSetEnabled()` to immediately start/stop music playback.

```c
GameContext_SetMusicEnabled(true);   // Music starts playing
GameContext_SetMusicEnabled(false);  // Music stops
```

### GameContext_SetSoundFxEnabled

**Signature:** `void GameContext_SetSoundFxEnabled(bool enabled)`
**Defined in:** [context.c:43-49](../source/core/context.c#L43-L49)

Enables or disables sound effects.

**Side Effect:** Calls `SOUNDFX_ON()` or `SOUNDFX_OFF()` to immediately enable/disable sound effect playback.

```c
GameContext_SetSoundFxEnabled(true);   // Sound effects enabled
GameContext_SetSoundFxEnabled(false);  // Sound effects muted
```

### GameContext_SetWifiEnabled

**Signature:** `void GameContext_SetWifiEnabled(bool enabled)`
**Defined in:** [context.c:51-56](../source/core/context.c#L51-L56)

Stores the user's WiFi preference.

**Important:** This only sets the user preference. The WiFi stack is initialized once at startup and kept alive throughout the program lifetime. See wifi.md for details on WiFi architecture and reconnection handling.

```c
GameContext_SetWifiEnabled(false);  // User disabled WiFi in settings
```

## Game State Management

### GameContext_SetMap / GameContext_GetMap

**Signatures:**
- `void GameContext_SetMap(Map SelectedMap)` - [context.c:62-64](../source/core/context.c#L62-L64)
- `Map GameContext_GetMap(void)` - [context.c:65-67](../source/core/context.c#L65-L67)

Sets or retrieves the currently selected map for gameplay.

```c
GameContext_SetMap(MAP1);
Map currentMap = GameContext_GetMap();
```

### GameContext_SetMultiplayerMode / GameContext_IsMultiplayerMode

**Signatures:**
- `void GameContext_SetMultiplayerMode(bool isMultiplayer)` - [context.c:69-71](../source/core/context.c#L69-L71)
- `bool GameContext_IsMultiplayerMode(void)` - [context.c:73-75](../source/core/context.c#L73-L75)

Sets or checks whether the current session is multiplayer or singleplayer.

```c
GameContext_SetMultiplayerMode(true);
if (GameContext_IsMultiplayerMode()) {
    // Initialize multiplayer networking
}
```

## Usage Patterns

### Checking Current State

```c
const GameContext* ctx = GameContext_Get();
switch (ctx->currentGameState) {
    case HOME_PAGE:
        // Render home screen
        break;
    case GAMEPLAY:
        // Run game loop
        break;
    default:
        break;
}
```

### Applying User Settings

```c
// From settings screen
void ApplySettings(bool music, bool sfx, bool wifi) {
    GameContext_SetMusicEnabled(music);    // Music starts/stops immediately
    GameContext_SetSoundFxEnabled(sfx);    // SFX enabled/disabled immediately
    GameContext_SetWifiEnabled(wifi);      // Preference stored
}
```

### Configuring Gameplay Session

```c
// Before starting a race
void StartRace(Map selectedMap, bool isMultiplayer) {
    GameContext_SetMap(selectedMap);
    GameContext_SetMultiplayerMode(isMultiplayer);

    GameContext* ctx = GameContext_Get();
    ctx->currentGameState = GAMEPLAY;  // Transition to gameplay state
}
```

### Reading Settings in Subsystems

```c
// From gameplay code
void Gameplay_Update(void) {
    GameContext* ctx = GameContext_Get();

    if (ctx->isMultiplayerMode) {
        UpdateMultiplayerState();
    } else {
        UpdateSingleplayerState();
    }
}
```

## Design Notes

- **Singleton Pattern**: Only one `GameContext` instance exists globally ([context.c:17](../source/core/context.c#L17))
- **No Dynamic Allocation**: Static storage for predictable memory usage on Nintendo DS
- **Side Effects**: Settings changes are applied immediately rather than requiring manual sync
- **State Transitions**: `currentGameState` is typically modified directly via the context pointer
- **TODO**: Future improvement would make `GameContext_Get()` return a const pointer to prevent bypassing setter side effects

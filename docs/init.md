# Initialization System

## Overview

The initialization system handles all one-time startup setup for Kart Mania. It initializes subsystems in a specific order to ensure dependencies are satisfied before use.

The implementation is in [init.c](../source/core/init.c) and [init.h](../source/core/init.h), called once from `main()` before entering the game loop (see [main.md](main.md)).

**Key Responsibilities:**
- Storage system initialization (FAT filesystem)
- Game context initialization (defaults + saved settings)
- Audio system initialization (MaxMod + soundbank)
- WiFi stack initialization (critical one-time setup)
- Initial game state initialization (HOME_PAGE)

## Architecture

### Initialization Flow

The `InitGame()` function ([init.c:85-98](../source/core/init.c#L85-L98)) orchestrates five initialization steps in strict order:

```c
void InitGame(void) {
    // 1. Initialize storage and load settings
    init_storage_and_context();

    // 2. Initialize audio system
    init_audio_system();

    // 3. Initialize WiFi stack (CRITICAL - only once!)
    init_wifi_stack();

    // 4. Initialize starting game state (HOME_PAGE)
    GameContext* ctx = GameContext_Get();
    StateMachine_Init(ctx->currentGameState);
}
```

### Why This Order Matters

1. **Storage First** - Need filesystem before loading saved settings
2. **Context Second** - Other systems depend on context (e.g., audio uses music/SFX settings)
3. **Audio Third** - Can now apply settings from context
4. **WiFi Fourth** - Independent of other systems, must be initialized once
5. **State Last** - All subsystems ready, safe to display first screen

## Storage and Context Initialization

### init_storage_and_context

**Function:** `static void init_storage_and_context(void)`
**Defined in:** [init.c:32-43](../source/core/init.c#L32-L43)
**Called by:** [InitGame()](../source/core/init.c#L86)

Initializes the FAT filesystem and game context, then loads saved settings if storage is available.

**Implementation:**
```c
static void init_storage_and_context(void) {
    // Initialize FAT filesystem first (required for loading saved settings)
    bool storageAvailable = Storage_Init();

    // Initialize context with hardcoded defaults (WiFi on, music on, etc.)
    GameContext_InitDefaults();

    // If SD card available, load saved settings to overwrite defaults
    if (storageAvailable) {
        Storage_LoadSettings();
    }
}
```

**Flow:**

1. **Storage_Init()** - Initializes FAT filesystem via `fatInitDefault()`
   - Returns `true` if SD card detected and mounted
   - Returns `false` if no SD card or mount failed
   - See storage.md for implementation details

2. **GameContext_InitDefaults()** - Sets hardcoded default values ([context.c:24-32](../source/core/context.c#L24-L32))
   - WiFi enabled: `true`
   - Music enabled: `true`
   - Sound effects enabled: `true`
   - Game state: `HOME_PAGE`
   - See [context.md](context.md) for all defaults

3. **Storage_LoadSettings()** (conditional) - Loads saved preferences
   - Only runs if storage available
   - Overwrites defaults with saved values
   - Graceful fallback: if file doesn't exist, keeps defaults
   - See storage.md for file format

**Why check storage availability:**
- Nintendo DS can run without an SD card inserted
- Game must work even if storage fails
- Hardcoded defaults ensure playable experience

## Audio System Initialization

### init_audio_system

**Function:** `static void init_audio_system(void)`
**Defined in:** [init.c:51-65](../source/core/init.c#L51-L65)
**Called by:** [InitGame()](../source/core/init.c#L90)

Initializes the MaxMod audio library and applies user audio settings from the context.

**Implementation:**
```c
static void init_audio_system(void) {
    const GameContext* ctx = GameContext_Get();

    initSoundLibrary();  // Initialize MaxMod with soundbank
    LoadALLSoundFX();    // Load all sound effects into memory
    loadMUSIC();         // Load background music module

    // Apply music setting (starts/stops playback based on saved preference)
    GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);

    // Apply sound effects setting (mute if disabled in settings)
    if (!ctx->userSettings.soundFxEnabled) {
        SOUNDFX_OFF();
    }
}
```

**Flow:**

1. **Get Context** - Retrieve saved/default audio preferences

2. **initSoundLibrary()** - Initialize MaxMod library
   - Calls `mmInitDefaultMem()` with soundbank binary
   - Sets up MaxMod's internal state for audio playback
   - See [sound.md](sound.md) for MaxMod details

3. **LoadALLSoundFX()** - Load all sound effects
   - Loads SFX_CLICK, SFX_DING, SFX_BOX into memory
   - Uses `mmLoadEffect()` for each sound
   - See [sound.md - Load/Unload Pattern](sound.md#loadunload-pattern)

4. **loadMUSIC()** - Load background music
   - Loads MOD_TROPICAL music module
   - Uses `mmLoad()` to prepare music for playback
   - See [sound.md - Music Functions](sound.md#music-functions)

5. **Apply Music Setting** - Start/stop music based on preference
   - If `musicEnabled` is `true`: starts music playback
   - If `musicEnabled` is `false`: keeps music stopped
   - Has immediate side effect (see [context.md](context.md))

6. **Apply SFX Setting** - Mute sound effects if disabled
   - Sets volume to 0 if sound effects disabled
   - Sounds still loaded, just muted
   - See [sound.md - Volume Control](sound.md#volume-control)

**Why load all sounds upfront:**
- Simplifies initial state (HOME_PAGE) setup
- Sounds will be cleaned up per-screen later
- Nintendo DS has ~4MB RAM, plenty for startup
- Individual screens unload unused sounds (see [sound.md](sound.md))

## WiFi Stack Initialization

### init_wifi_stack

**Function:** `static void init_wifi_stack(void)`
**Defined in:** [init.c:77-79](../source/core/init.c#L77-L79)
**Called by:** [InitGame()](../source/core/init.c#L93)

Initializes the Nintendo DS WiFi stack ONCE at program startup. This is the most critical initialization step for multiplayer.

**Implementation:**
```c
static void init_wifi_stack(void) {
    Wifi_InitDefault(false);
}
```

**CRITICAL WARNINGS:**

⚠️ **ONE-TIME ONLY**
- WiFi stack can ONLY be initialized once per program execution
- Re-calling `Wifi_InitDefault()` causes connection corruption
- DO NOT call anywhere else in the codebase

⚠️ **NEVER DISABLE**
- Stack stays alive for entire program lifetime
- User's "WiFi enabled" setting only controls preferences, not stack lifecycle
- Must call `Wifi_Update()` every frame even if WiFi "disabled"

⚠️ **MULTIPLAYER RECONNECTION**
- One-time initialization enables reconnection between matches
- Players can disconnect, return to home, reconnect to new lobby
- WiFi stack must stay alive throughout for this to work

**Parameter:** `false`
- `false` = don't use firmware WiFi settings (use default AP)
- `true` = use firmware settings (not used in Kart Mania)

**See:** wifi.md for complete WiFi architecture and reconnection handling

## Initial State Initialization

### State Machine Init Call

**Function:** `StateMachine_Init(GameState state)`
**Called at:** [init.c:97](../source/core/init.c#L97)
**Parameter:** `ctx->currentGameState` (defaults to HOME_PAGE)

Initializes graphics, timers, and resources for the starting game state.

**What it does for HOME_PAGE:**
- Sets up background graphics for main menu
- Loads kart sprite animations
- Initializes VBlank timer for menu animations
- Configures touch input handling

**See:** [state_machine.md - State Initialization](state_machine.md#state-initialization) for all state init details

**Why last:**
- All subsystems (storage, context, audio, WiFi) are ready
- State can safely load graphics, play sounds, etc.
- No risk of using uninitialized subsystems

## Initialization Summary

| Step | Function | Purpose | Depends On |
|------|----------|---------|------------|
| 1 | `init_storage_and_context()` | FAT + context + settings | None |
| 2 | `init_audio_system()` | MaxMod + sounds + music | Context |
| 3 | `init_wifi_stack()` | WiFi firmware | None |
| 4 | `StateMachine_Init()` | HOME_PAGE graphics/timers | All above |

## Usage Patterns

### Standard Startup

Called once from `main()` before entering game loop:

```c
int main(void) {
    // Perform one-time initialization
    InitGame();

    GameContext* ctx = GameContext_Get();

    // Enter game loop
    while (true) {
        // ...
    }
}
```

**See:** [main.md](main.md) for complete main loop documentation

### Initialization with No SD Card

If no SD card is inserted, the game still initializes successfully:

```c
// Storage_Init() returns false
bool storageAvailable = Storage_Init();  // false

// Context gets defaults instead of saved values
GameContext_InitDefaults();  // WiFi on, music on, SFX on

// Skip Storage_LoadSettings() - keeps defaults
if (storageAvailable) {  // false, skipped
    Storage_LoadSettings();
}

// Continue with audio initialization...
```

Game runs with default settings (all features enabled).

### Initialization with Saved Settings

If SD card has saved settings, they override defaults:

```c
// Storage_Init() returns true
bool storageAvailable = Storage_Init();  // true

// Context gets defaults first
GameContext_InitDefaults();  // WiFi on, music on, SFX on

// Load saved settings (overwrites defaults)
if (storageAvailable) {  // true, executes
    Storage_LoadSettings();  // e.g., music off
}

// Audio initialization applies saved preference
GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);  // false
// Music does NOT start playing
```

Game runs with user's saved preferences.

## Design Notes

- **Ordered Dependencies**: Each step depends on previous steps completing
- **Graceful Degradation**: Game works even if storage unavailable
- **One-Time WiFi**: WiFi stack initialized once, never re-initialized
- **Settings Priority**: Saved settings override defaults when available
- **Private Functions**: All helpers are static - only `InitGame()` is public

## Cross-References

- **Main Loop:** [main.md](main.md) - Calls InitGame() at startup
- **State Machine:** [state_machine.md](state_machine.md) - Initial state setup
- **Context System:** [context.md](context.md) - Default values and settings
- **Audio System:** [sound.md](sound.md) - MaxMod initialization and soundbank
- **WiFi Architecture:** wifi.md - One-time initialization and reconnection
- **Storage System:** storage.md - FAT filesystem and settings persistence

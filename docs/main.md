# Main Game Loop

## Overview

The main game loop is the central orchestrator for Kart Mania, responsible for coordinating all major subsystems. It performs one-time initialization, then runs an infinite loop that updates WiFi, processes the current game state, handles state transitions, and synchronizes to the display.

The implementation is intentionally minimal in [main.c](../source/core/main.c) to maintain clean architecture - all initialization logic lives in [init.c](../source/core/init.c) (see [init.md](init.md)) and state management lives in [state_machine.c](../source/core/state_machine.c) (see [state_machine.md](state_machine.md)).

**Key Responsibilities:**
- One-time initialization via `InitGame()`
- Continuous WiFi stack pumping (critical for multiplayer)
- State machine update dispatch
- State transition handling
- 60Hz frame synchronization

## Architecture

### Main Function Structure

The `main()` function (see `source/core/main.c`) has two distinct phases:

**1. Initialization Phase (Once)**
```c
InitGame();  // See init.md for details
GameContext* ctx = GameContext_Get();
```

**2. Game Loop (Infinite)**
```c
while (true) {
    Wifi_Update();
    GameState nextState = StateMachine_Update(ctx->currentGameState);

    if (nextState != ctx->currentGameState) {
        // Handle state transition
    }

    swiWaitForVBlank();
}
```

## Initialization Phase

### InitGame Call

**Function:** `InitGame()`
**Defined in:** `source/core/init.c`
**Called at:** `main.c`

Performs all one-time initialization before entering the game loop. See [init.md](init.md) for complete details on the initialization sequence.

**What it initializes:**
1. Storage system (FAT filesystem for saved settings)
2. Game context (global state singleton)
3. Audio system (MaxMod library, sound effects, music)
4. WiFi stack (critical one-time initialization)
5. Initial game state (HOME_PAGE)

### Context Access

**Function:** `GameContext_Get()`
**Called at:** `main.c`

Retrieves the singleton context pointer used throughout the game loop to check and update the current game state. See [context.md](context.md) for details on the context system.

## Game Loop

The infinite loop runs at 60Hz, synchronized to the Nintendo DS vertical blank interrupt.

### WiFi Update (Critical)

**Function:** `Wifi_Update()`
**Called at:** `main.c`
**Frequency:** Every frame (60 times per second)

**Purpose:** Pumps the DSWifi state machine to keep WiFi connections alive.

**Why every frame:**
- The Nintendo DS WiFi firmware requires continuous updates
- Skipping frames causes connection drops and timeouts
- Must run even when WiFi is "disabled" in settings because the stack is initialized once and kept alive
- See wifi.md for details on WiFi architecture

**CRITICAL:** This call cannot be conditional or paused. Even in singleplayer mode or with WiFi "disabled" in settings, `Wifi_Update()` must run every frame to prevent WiFi stack corruption.

### State Machine Update

**Function:** `StateMachine_Update(GameState state)`
**Called at:** `main.c`
**Returns:** Next GameState to transition to

Dispatches to the current state's update function (e.g., `HomePage_Update()`, `Gameplay_Update()`, etc.). Each state processes input, updates logic, and returns either the same state (no transition) or a different state (transition requested).

**See:** [state_machine.md](state_machine.md) for complete state machine documentation

### State Transition Handling

**Condition:** `if (nextState != ctx->currentGameState)`

When a state transition is detected, the loop performs a carefully ordered cleanup and initialization sequence.

#### Transition Steps

**1. Cleanup Old State**
```c
StateMachine_Cleanup(ctx->currentGameState, nextState);
```
**Line:** `main.c`

Cleans up resources for the state being exited. The cleanup function knows which state is being entered (`nextState`) so it can conditionally preserve resources like WiFi connections when transitioning between multiplayer screens.

**Examples:**
- GAMEPLAY cleanup stops race timers and unloads gameplay graphics
- MULTIPLAYER_LOBBY cleanup preserves WiFi when entering GAMEPLAY
- HOME_PAGE cleanup unloads menu sprites

**See:** [state_machine.md - State Cleanup](state_machine.md#state-cleanup) for per-state cleanup behavior

**2. Update Context State**
```c
ctx->currentGameState = nextState;
```
**Line:** `main.c`

Updates the global context to reflect the new state. This is used by VBlank ISR routing (see [timer.md](timer.md)) and throughout the codebase to check the current screen.

**3. Clear Video Memory**
```c
video_nuke();
```
**Line:** `main.c`

Clears all video memory (VRAM) to prevent graphical artifacts from the previous state bleeding into the new state. This ensures each state starts with a clean slate.

**What it clears:**
- Sprite data
- Background tiles
- Palette data
- OAM (Object Attribute Memory)

**4. Initialize New State**
```c
StateMachine_Init(nextState);
```
**Line:** `main.c`

Initializes graphics, timers, and resources for the new state.

**Examples:**
- HOME_PAGE init sets up menu backgrounds and kart sprites
- GAMEPLAY init loads map graphics and starts race timers
- SETTINGS init loads settings UI and current preference values

**See:** [state_machine.md - State Initialization](state_machine.md#state-initialization) for per-state init behavior

### VBlank Synchronization

**Function:** `swiWaitForVBlank()`
**Called at:** `main.c`
**Frequency:** 60Hz

Blocks execution until the next vertical blank interrupt, ensuring the game loop runs at exactly 60 frames per second.

**Why synchronize:**
- Prevents screen tearing (updating graphics mid-draw)
- Locks game speed to 60 FPS for consistent gameplay
- Coordinates with VBlank ISR (see [timer.md](timer.md))
- Matches Nintendo DS hardware refresh rate

**What happens during VBlank:**
- The `timerISRVblank()` ISR runs automatically (see [timer.md](timer.md))
- Safe window to update sprites and backgrounds without visual artifacts
- Race tick timers continue running independently

## Frame Execution Flow

Here's what happens in a single frame (1/60th of a second):

```
Frame N:
├─ Wifi_Update()                    ← Pump WiFi firmware
├─ StateMachine_Update()            ← Run current state logic
│  └─ (e.g., HomePage_Update())
│     ├─ Process touch input
│     ├─ Update menu animations
│     └─ Return next state
├─ State transition? (if changed)
│  ├─ StateMachine_Cleanup()        ← Clean up old state
│  ├─ ctx->currentGameState = new  ← Update context
│  ├─ video_nuke()                  ← Clear VRAM
│  └─ StateMachine_Init()           ← Initialize new state
└─ swiWaitForVBlank()               ← Wait for 60Hz sync

[VBlank occurs - timerISRVblank() runs in background]

Frame N+1:
└─ Loop repeats...
```

## Timing Relationship

The main loop and VBlank ISR work together but run independently:

**Main Loop (Foreground):**
- Runs at 60 FPS (blocked by `swiWaitForVBlank()`)
- Handles game logic and state transitions
- Processes WiFi and input

**VBlank ISR (Background):**
- Triggered by hardware at 60Hz (independent of main loop)
- Updates sprite positions and displays
- Routes to state-specific OnVBlank handlers

**See:** [timer.md](timer.md) for complete VBlank ISR documentation

## State Transition Guarantees

The centralized transition handling in `main()` provides important guarantees:

**1. Cleanup Always Runs First**
- Resources are freed before new state allocates
- Prevents memory leaks from abandoned sprites or timers

**2. Video Memory is Always Cleared**
- No graphical artifacts between states
- Each state starts with clean VRAM

**3. Context is Updated Once**
- `currentGameState` changes atomically
- VBlank ISR always sees consistent state

**4. Conditional Cleanup Works**
- Cleanup function knows the destination state
- WiFi connections can be preserved for multiplayer flow

## Usage Patterns

### Requesting a State Transition

States request transitions by returning a different `GameState` from their update function:

```c
// In HomePage_Update()
GameState HomePage_Update(void) {
    // Process input...

    if (singleplayerButtonPressed) {
        return MAPSELECTION;  // Request transition to map selection
    }

    if (settingsButtonPressed) {
        return SETTINGS;  // Request transition to settings
    }

    return HOME_PAGE;  // Stay on home page
}
```

The main loop automatically handles the cleanup → update context → clear VRAM → init sequence.

### Preserving Resources Across Transitions

Some resources (like WiFi connections) need to stay alive across related transitions. The cleanup function receives `nextState` to make conditional decisions:

```c
// In StateMachine_Cleanup()
case MULTIPLAYER_LOBBY:
    // Keep WiFi alive when entering GAMEPLAY
    if (nextState != GAMEPLAY && GameContext_IsMultiplayerMode()) {
        Multiplayer_Cleanup();  // Only cleanup if not going to gameplay
    }
    break;
```

**See:** [state_machine.md](state_machine.md) for all conditional cleanup logic

### Reading Current State

Any code can check the current state via the context:

```c
const GameContext* ctx = GameContext_Get();
if (ctx->currentGameState == GAMEPLAY) {
    // Gameplay-specific code
}
```

## Design Notes

- **Minimal main()**: Under 50 lines - all complexity delegated to subsystems
- **Single Responsibility**: Main loop only orchestrates, doesn't implement logic
- **Predictable Timing**: Always runs at 60 FPS via VBlank sync
- **Clean Transitions**: Centralized handling prevents state machine bugs
- **WiFi Reliability**: Continuous `Wifi_Update()` prevents connection drops

## Cross-References

- **Initialization:** [init.md](init.md) - One-time startup sequence
- **State Machine:** [state_machine.md](state_machine.md) - Update/Init/Cleanup functions
- **Timing System:** [timer.md](timer.md) - VBlank ISR and race timers
- **Context System:** [context.md](context.md) - Global state singleton
- **WiFi Architecture:** wifi.md - Why Wifi_Update() must run continuously


---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)

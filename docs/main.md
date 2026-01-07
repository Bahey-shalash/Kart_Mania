# Main Loop (Housekeeping Task)

## Overview

**IMPORTANT:** The main loop is NOT the game loop. Kart Mania uses an **interrupt-driven architecture** where hardware timers drive gameplay. The main loop is a lightweight housekeeping task that maintains WiFi and handles state transitions.

The implementation is intentionally minimal in [main.c](../source/core/main.c) - all initialization logic lives in [init.c](../source/core/init.c) (see [init.md](init.md)) and state management lives in [state_machine.c](../source/core/state_machine.c) (see [state_machine.md](state_machine.md)).

**Key Responsibilities:**
- One-time initialization via `InitGame()`
- Baseline WiFi heartbeat at 60Hz (WiFi operations pump faster when needed)
- State transition handling (cleanup, context update, VRAM clear, init)
- VBlank synchronization (blocks ~16.67ms per frame)

**What the main loop does NOT do:**
- Gameplay logic (handled by TIMER0 ISR calling `Race_Tick()`)
- Sprite updates (handled by VBlank ISR)
- Race timing (handled by TIMER1 ISR)

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

## Housekeeping Loop

The `while(true)` loop has no inherent frequency - it runs as fast as the code executes, then **blocks on `swiWaitForVBlank()`** which limits it to 60Hz. The actual "loop" behavior is just sleeping between VBlanks.

### WiFi Heartbeat

**Function:** `Wifi_Update()`
**Called at:** `main.c:29`
**Frequency:** At least 60Hz (this is the baseline heartbeat)

**Purpose:** Provides continuous WiFi firmware servicing.

**Why the main loop calls it:**
- Guarantees WiFi stack receives updates even when no blocking operations are running
- The Nintendo DS WiFi firmware requires regular servicing
- Must run even when WiFi is "disabled" in settings because the stack is initialized once at startup
- See [wifi.md](wifi.md) for details on WiFi architecture

**CRITICAL:** This is just the **baseline heartbeat**. WiFi operations (scanning, connecting, cleanup) pump `Wifi_Update()` much more frequently in tight loops. The main loop ensures a minimum 60Hz rate between those operations.

**Note:** During blocking WiFi operations (e.g., access point scanning, connection establishment), `Wifi_Update()` is pumped at much higher frequencies (tight loops) to ensure responsive WiFi stack communication.

### State Machine Update

**Function:** `StateMachine_Update(GameState state)`
**Called at:** `main.c:32`
**Returns:** Next GameState to transition to

Dispatches to the current state's update function (e.g., `HomePage_Update()`, `Gameplay_Update()`, etc.).

**IMPORTANT:** During GAMEPLAY, `Gameplay_Update()` returns immediately - all gameplay logic runs in the TIMER0 ISR (`Race_Tick()`). State update functions primarily check for state transition conditions, not game logic.

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
**Called at:** `main.c:43`
**Frequency:** 60Hz (hardware-enforced)

Blocks execution until the next vertical blank interrupt. This is what gives the main loop its 60Hz frequency - without this call, the `while(true)` would spin millions of times per second.

**Why this matters:**
- Main loop spends ~16.67ms blocked here (most of its time)
- During this blocking period, hardware timers continue firing independently
- TIMER0 ISR continues calling `Race_Tick()` for gameplay
- TIMER1 ISR continues incrementing race chronometer
- VBlank ISR continues updating sprites
- This is just a sleep mechanism, not a game driver

**What happens during VBlank:**
- The `timerISRVblank()` ISR runs automatically (see [timer.md](timer.md))
- Hardware timers (TIMER0, TIMER1) continue firing independently
- Gameplay continues in interrupt handlers while main loop sleeps

## Execution Flow

Here's what happens in a single main loop iteration (synchronized to VBlank at 60Hz):

```
Main Loop Iteration (60Hz):
├─ Wifi_Update()                    ← WiFi heartbeat (~microseconds)
├─ StateMachine_Update()            ← Check for state changes (~microseconds)
│  └─ (e.g., Gameplay_Update())    ← Returns immediately during gameplay
│     └─ Return GAMEPLAY           ← No transition, stay in gameplay
├─ State transition? (rarely)
│  ├─ StateMachine_Cleanup()        ← Clean up old state
│  ├─ ctx->currentGameState = new  ← Update context
│  ├─ video_nuke()                  ← Clear VRAM
│  └─ StateMachine_Init()           ← Initialize new state
└─ swiWaitForVBlank()               ← BLOCK HERE for ~16.67ms

[While main loop is blocked, interrupts continue firing independently:]
  - VBlank ISR fires (60Hz) → Sprite updates
  - TIMER0 ISR fires (60Hz) → Race_Tick() → Physics, collisions, input
  - TIMER1 ISR fires (1000Hz) → Race chronometer increments

Main Loop Iteration N+1:
└─ Loop repeats...
```

## Interrupt-Driven Architecture

**CRITICAL UNDERSTANDING:** The game is driven by hardware interrupts, not the main loop.

**Hardware Interrupts (Precise Frequencies):**
- **VBlank ISR** (60Hz): Sprite updates, display refresh - fires at hardware VBlank signal
- **TIMER0 ISR** (60Hz): Calls `Race_Tick()` - physics, collisions, input handling
- **TIMER1 ISR** (1000Hz): Race chronometer - millisecond precision timing
- These run **independently** of the main loop

**Main Loop (VBlank-Synchronized, 60Hz):**
- **NOT a game loop** - just housekeeping
- Provides baseline WiFi heartbeat
- Handles state transitions when requested
- Spends most time blocked on `swiWaitForVBlank()`
- Gameplay continues in ISRs while main loop sleeps

**See:** [timer.md](timer.md) for complete ISR documentation

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

- **Interrupt-Driven**: Gameplay runs in TIMER0 ISR, not main loop
- **Minimal main()**: Under 50 lines - just WiFi heartbeat and state transitions
- **Single Responsibility**: Main loop is housekeeping, not a game loop
- **VBlank Synchronization**: Main loop blocked 16.67ms per iteration
- **WiFi Baseline**: Main loop ensures 60Hz minimum (operations pump faster)
- **Clean Transitions**: Centralized cleanup/init handling prevents bugs

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

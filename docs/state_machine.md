# State Machine System

## Overview

The state machine system manages the game's screens and their lifecycles. It handles state transitions, resource initialization, and cleanup for all game states (HOME_PAGE, SETTINGS, GAMEPLAY, etc.).

The implementation is in [state_machine.c](../source/core/state_machine.c) and [state_machine.h](../source/core/state_machine.h), called from the main game loop (see [main.md](main.md)).

**Key Responsibilities:**
- Dispatch state updates to screen-specific logic
- Initialize graphics/timers/resources when entering states
- Clean up resources when exiting states
- Manage WiFi connection lifecycle across state transitions
- Route to state-specific handlers for all game screens

## Architecture

### Game States

The `GameState` enum ([game_types.h:9-17](../source/core/game_types.h#L9-L17)) defines all possible screens:

```c
typedef enum {
    HOME_PAGE,           // Main menu
    MAPSELECTION,        // Map selection screen
    MULTIPLAYER_LOBBY,   // Multiplayer lobby/waiting room
    GAMEPLAY,            // Active race
    PLAYAGAIN,           // Post-race results/retry screen
    SETTINGS,            // Settings menu
    REINIT_HOME          // Forces home page reinit after WiFi failure
} GameState;
```

### State Machine Functions

The state machine exposes three public functions:

1. **StateMachine_Update()** - Run current state's logic, return next state
2. **StateMachine_Init()** - Initialize a state's resources
3. **StateMachine_Cleanup()** - Clean up a state's resources

## State Update Dispatch

### StateMachine_Update

**Signature:** `GameState StateMachine_Update(GameState state)`
**Defined in:** [state_machine.c:30-54](../source/core/state_machine.c#L30-L54)
**Called from:** [main.c:31](../source/core/main.c#L31)

Dispatches to the current state's update function. Each state processes input, updates logic, and returns the next state.

**Dispatch Table:**

| State | Handler | Returns |
|-------|---------|---------|
| HOME_PAGE | `HomePage_update()` ([state_machine.c:34](../source/core/state_machine.c#L34)) | MAPSELECTION, MULTIPLAYER_LOBBY, SETTINGS, or HOME_PAGE |
| REINIT_HOME | `HomePage_update()` ([state_machine.c:34](../source/core/state_machine.c#L34)) | Same as HOME_PAGE |
| SETTINGS | `Settings_update()` ([state_machine.c:37](../source/core/state_machine.c#L37)) | HOME_PAGE or SETTINGS |
| MAPSELECTION | `Map_selection_update()` ([state_machine.c:40](../source/core/state_machine.c#L40)) | GAMEPLAY, HOME_PAGE, or MAPSELECTION |
| MULTIPLAYER_LOBBY | `MultiplayerLobby_Update()` ([state_machine.c:43](../source/core/state_machine.c#L43)) | GAMEPLAY, HOME_PAGE, or MULTIPLAYER_LOBBY |
| GAMEPLAY | `Gameplay_update()` ([state_machine.c:46](../source/core/state_machine.c#L46)) | PLAYAGAIN or GAMEPLAY |
| PLAYAGAIN | `PlayAgain_Update()` ([state_machine.c:49](../source/core/state_machine.c#L49)) | GAMEPLAY, HOME_PAGE, or PLAYAGAIN |
| Unknown | No handler ([state_machine.c:52](../source/core/state_machine.c#L52)) | Same state (stay) |

**REINIT_HOME Special Case:**
- Treated identically to HOME_PAGE for update and init
- Used to force full home page reinitialization after WiFi failures
- Allows home page to reset after connection errors

**See:** [main.md - State Machine Update](main.md#state-machine-update) for how this integrates with the game loop

## State Initialization

### StateMachine_Init

**Signature:** `void StateMachine_Init(GameState state)`
**Defined in:** [state_machine.c:60-87](../source/core/state_machine.c#L60-L87)
**Called from:** [main.c:38](../source/core/main.c#L38) (transitions) and [init.c:96](../source/core/init.c#L96) (startup)

Initializes graphics, timers, and resources for a state being entered.

**Initialization Table:**

| State | Handler | Initializes |
|-------|---------|-------------|
| HOME_PAGE | `HomePage_initialize()` ([state_machine.c:64](../source/core/state_machine.c#L64)) | Main menu backgrounds, kart sprites, VBlank timer |
| REINIT_HOME | `HomePage_initialize()` ([state_machine.c:64](../source/core/state_machine.c#L64)) | Same as HOME_PAGE (full reinit) |
| MAPSELECTION | `Map_Selection_initialize()` ([state_machine.c:68](../source/core/state_machine.c#L68)) | Map preview graphics, cloud animations, VBlank timer |
| MULTIPLAYER_LOBBY | `MultiplayerLobby_Init()` ([state_machine.c:72](../source/core/state_machine.c#L72)) | Lobby UI, player list, WiFi connection status |
| GAMEPLAY | `Graphical_Gameplay_initialize()` ([state_machine.c:76](../source/core/state_machine.c#L76)) | Map graphics, kart sprites, race timers (TIMER0/TIMER1) |
| SETTINGS | `Settings_initialize()` ([state_machine.c:80](../source/core/state_machine.c#L80)) | Settings UI, toggle states from context |
| PLAYAGAIN | `PlayAgain_Initialize()` ([state_machine.c:84](../source/core/state_machine.c#L84)) | Results screen, final time display |

**Per-State Details:**

#### HOME_PAGE / REINIT_HOME
- Loads main menu background graphics
- Initializes animated kart sprites
- Sets up VBlank timer for animations (see [timer.md](timer.md))
- Loads click sound effect (see [sound.md](sound.md))

#### MAPSELECTION
- Loads map preview backgrounds
- Initializes cloud animation sprites
- Sets up VBlank timer for cloud movement
- Loads click sound effect

#### MULTIPLAYER_LOBBY
- Initializes lobby UI graphics
- Displays player connection status
- Shows "waiting for players" messages
- Keeps WiFi connection alive from previous screens

#### GAMEPLAY
- Loads selected map graphics (from context)
- Initializes kart sprites for all players
- Starts race timers: TIMER0 (physics at 60Hz), TIMER1 (chronometer at 1000Hz)
- Sets up VBlank timer for sprite updates
- Loads box sound effect for item pickups
- Starts background music if enabled

**See:** [timer.md](timer.md) for timer initialization details

#### SETTINGS
- Loads settings screen graphics
- Displays current toggle states from context
- Loads click and ding sound effects

#### PLAYAGAIN
- Displays race results
- Shows final race time
- Displays "Play Again?" / "Home" options
- Preserves WiFi connection for multiplayer rematch

## State Cleanup

### StateMachine_Cleanup

**Signature:** `void StateMachine_Cleanup(GameState state, GameState nextState)`
**Defined in:** [state_machine.c:93-133](../source/core/state_machine.c#L93-L133)
**Called from:** [main.c:35](../source/core/main.c#L35)

Cleans up resources for the state being exited. The cleanup function receives both the current state and the destination state (`nextState`) to make conditional cleanup decisions.

**Why nextState parameter:**
- Allows preserving resources across related transitions
- WiFi connections can stay alive when moving between multiplayer screens
- Prevents unnecessary disconnect/reconnect cycles

**Cleanup Table:**

| State | Handler | Cleans Up | Conditional Logic |
|-------|---------|-----------|-------------------|
| HOME_PAGE | `HomePage_Cleanup()` ([state_machine.c:97](../source/core/state_machine.c#L97)) | Menu sprites, click SFX | None |
| REINIT_HOME | `HomePage_Cleanup()` ([state_machine.c:97](../source/core/state_machine.c#L97)) | Same as HOME_PAGE | None |
| MAPSELECTION | No cleanup ([state_machine.c:101](../source/core/state_machine.c#L101)) | Nothing | None |
| MULTIPLAYER_LOBBY | Conditional ([state_machine.c:107-110](../source/core/state_machine.c#L107-L110)) | WiFi connection (conditional) | Only if NOT going to GAMEPLAY |
| GAMEPLAY | `Gameplay_Cleanup()` ([state_machine.c:114-122](../source/core/state_machine.c#L114-L122)) | Race timers, graphics, WiFi (conditional) | WiFi cleanup only if in multiplayer |
| SETTINGS | No cleanup ([state_machine.c:126](../source/core/state_machine.c#L126)) | Nothing (auto-save) | None |
| PLAYAGAIN | No cleanup ([state_machine.c:130](../source/core/state_machine.c#L130)) | Nothing | Preserves WiFi for rematch |

**Per-State Cleanup Details:**

#### HOME_PAGE / REINIT_HOME
**Cleanup:**
- Unloads menu sprite graphics
- Unloads click sound effect (see [sound.md](sound.md))

**No conditional logic** - always cleans up fully

#### MAPSELECTION
**No cleanup needed:**
- Map selection graphics automatically overwritten by next state
- VBlank timer reconfigured by next state
- Sound effects cleaned up by next state

#### MULTIPLAYER_LOBBY
**Conditional WiFi cleanup:**
```c
if (nextState != GAMEPLAY && GameContext_IsMultiplayerMode()) {
    Multiplayer_Cleanup();
    GameContext_SetMultiplayerMode(false);
}
```

**Logic:**
- If going to GAMEPLAY: Keep WiFi alive for the race
- If going anywhere else (e.g., HOME_PAGE): Disconnect WiFi

**Why:** Players transitioning from lobby to race need WiFi to stay connected. But if they back out to home, disconnect to free resources.

#### GAMEPLAY
**Always:**
1. **Stop Race Timers** ([state_machine.c:114](../source/core/state_machine.c#L114))
   - `RaceTick_TimerStop()` - Stops TIMER0 (physics) and TIMER1 (chronometer)
   - See [timer.md - RaceTick_TimerStop](timer.md#racetick_timerstop)

2. **Clean Up Graphics** ([state_machine.c:115](../source/core/state_machine.c#L115))
   - `Gameplay_Cleanup()` - Unloads map graphics, kart sprites

3. **Stop Race Logic** ([state_machine.c:116](../source/core/state_machine.c#L116))
   - `Race_Stop()` - Halts race state machine

**Conditional WiFi cleanup:**
```c
if (GameContext_IsMultiplayerMode()) {
    Multiplayer_Cleanup();
    GameContext_SetMultiplayerMode(false);
}
```

**Logic:**
- Only cleanup WiFi if race was multiplayer
- Singleplayer races skip WiFi cleanup (no connection to clean)

#### SETTINGS
**No cleanup needed:**
- Settings auto-save on change via context setters
- UI graphics overwritten by next state

#### PLAYAGAIN
**No cleanup:**
- Intentionally preserves ALL resources
- WiFi connection stays alive for potential rematch
- Players can choose "Play Again" to restart race with same lobby

**Critical:** If cleanup ran here, multiplayer players couldn't rematch without reconnecting.

## State Transition Flow

When a state transition occurs, the sequence is:

```
1. State returns different GameState from update function
   ↓
2. Main loop detects change (see main.md)
   ↓
3. StateMachine_Cleanup(oldState, newState)
   - Conditional cleanup based on destination
   ↓
4. Context state updated: ctx->currentGameState = newState
   ↓
5. video_nuke() - Clear VRAM
   ↓
6. StateMachine_Init(newState)
   - Initialize new state resources
   ↓
7. Game loop continues with new state
```

**See:** [main.md - State Transition Handling](main.md#state-transition-handling) for complete details

## WiFi Connection Lifecycle

The state machine carefully manages WiFi connections to enable multiplayer reconnection:

**Connection Timeline:**

```
[STARTUP]
  ↓ InitGame() - WiFi stack initialized ONCE
  ↓
[HOME_PAGE]
  ↓ User clicks "Multiplayer"
  ↓
[MULTIPLAYER_LOBBY]
  ↓ WiFi connects to lobby
  ↓ cleanup: WiFi PRESERVED (going to GAMEPLAY)
  ↓
[GAMEPLAY]
  ↓ Race completes
  ↓ cleanup: WiFi DISCONNECTED (going to PLAYAGAIN)
  ↓
[PLAYAGAIN]
  ↓ User clicks "Play Again"
  ↓ cleanup: WiFi PRESERVED (rematch flow)
  ↓
[MULTIPLAYER_LOBBY]
  ↓ Reconnects to new lobby (WiFi stack still alive!)
  ...
```

**Key Insight:** WiFi stack initialization is one-time (see [init.md](init.md)), but connections are managed per-transition by the cleanup logic.

**See:** wifi.md for complete WiFi architecture

## Usage Patterns

### Requesting State Transitions

States request transitions by returning a different `GameState`:

```c
GameState HomePage_update(void) {
    // Handle input
    if (touchInSingleplayerButton()) {
        return MAPSELECTION;  // Transition to map selection
    }

    if (touchInSettingsButton()) {
        return SETTINGS;  // Transition to settings
    }

    return HOME_PAGE;  // Stay on home page
}
```

Main loop automatically handles cleanup/init sequence (see [main.md](main.md)).

### Preserving Resources Across Transitions

Use the `nextState` parameter to conditionally preserve resources:

```c
void StateMachine_Cleanup(GameState state, GameState nextState) {
    switch (state) {
        case MULTIPLAYER_LOBBY:
            // Only cleanup WiFi if NOT going to race
            if (nextState != GAMEPLAY && GameContext_IsMultiplayerMode()) {
                Multiplayer_Cleanup();
            }
            break;
    }
}
```

### Full State Reinit (REINIT_HOME)

`REINIT_HOME` forces complete home page reinitialization:

```c
// In multiplayer code after WiFi failure
return REINIT_HOME;  // Force full home page reinit
```

Treated identically to HOME_PAGE but signals that a full reset is needed.

## State Transition Graph

```
                    ┌─────────────┐
           ┌────────┤  HOME_PAGE  │◄────────┐
           │        └─────────────┘         │
           │          │ │       │            │
           │          │ │       │            │
    [Settings]   [Single] [Multi]       [Back]
           │          │       │              │
           ↓          ↓       ↓              │
     ┌──────────┐  ┌────────────┐  ┌────────────────┐
     │ SETTINGS │  │MAPSELECTION│  │MULTIPLAYER_LOBBY│
     └──────────┘  └────────────┘  └────────────────┘
                          │                │
                     [Start Race]     [Start Race]
                          │                │
                          └───────┬────────┘
                                  ↓
                           ┌──────────┐
                           │ GAMEPLAY │
                           └──────────┘
                                  │
                           [Race Complete]
                                  ↓
                            ┌──────────┐
                            │PLAYAGAIN │
                            └──────────┘
                             │        │
                       [Play Again] [Home]
                             │        │
                             │        └───────────────┐
                      ┌──────┘                        │
                      ↓                               ↓
         ┌────────────────────┐               ┌─────────────┐
         │MULTIPLAYER_LOBBY OR│               │  HOME_PAGE  │
         │   MAPSELECTION     │               └─────────────┘
         └────────────────────┘
```

## Design Notes

- **Centralized Control**: Main loop handles all transitions, states just return next state
- **Conditional Cleanup**: Cleanup knows destination state for smart resource management
- **WiFi Lifecycle**: Carefully preserved across multiplayer flow for reconnection
- **REINIT_HOME**: Special state for forcing full home page reset after errors
- **Minimal Coupling**: States don't know about each other, just return enum values
- **TODO**: Sound cleanup strategy needs review ([state_machine.c:135](../source/core/state_machine.c#L135))

## Cross-References

- **Main Loop:** [main.md](main.md) - Calls state machine functions in game loop
- **Initialization:** [init.md](init.md) - Calls StateMachine_Init() at startup
- **Timer System:** [timer.md](timer.md) - Race timers managed by state machine
- **Context System:** [context.md](context.md) - State stored in global context
- **WiFi Architecture:** wifi.md - WiFi connection lifecycle across states
- **Audio System:** [sound.md](sound.md) - Sound loading/unloading per state

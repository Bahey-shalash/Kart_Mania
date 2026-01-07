# Timer System

## Overview

The timer system manages all interrupt service routines (ISRs) for the game, providing two distinct timer subsystems:

1. **VBlank Timer** - 60Hz graphics updates for all animated screens
2. **Race Tick Timers** - Hardware timers for physics (TIMER0) and chronometer (TIMER1) during gameplay

The timer system is implemented in [timer.c](../source/core/timer.c) and [timer.h](../source/core/timer.h), using the Nintendo DS hardware interrupt system to achieve precise timing for graphics synchronization and gameplay logic.

## Architecture

### Dual Timer System

The game uses two independent timing mechanisms:

**VBlank ISR (60Hz):**
- Triggered by hardware vertical blank interrupt
- Active on all animated screens (HOME_PAGE, MAPSELECTION, GAMEPLAY, PLAYAGAIN)
- Routes to state-specific handlers for sprite updates and display refreshes
- Runs continuously at 60Hz synchronized with screen refresh

**Hardware Timers (Gameplay Only):**
- **TIMER0**: Physics tick at `RACE_TICK_FREQ` Hz (default 60Hz) - calls `Race_Tick()` for movement, collisions, item logic
- **TIMER1**: Chronometer at 1000Hz (1ms precision) - calls `Gameplay_IncrementTimer()` for race time tracking
- Only active during GAMEPLAY state
- Can be paused/resumed for game pause functionality

### RACE_TICK_FREQ Configuration

**Constant:** `RACE_TICK_FREQ`
**Defined in:** [game_constants.h:210-214](../source/core/game_constants.h#L210-L214) (moved from `timer.h`; see note at [timer.h:21](../source/core/timer.h#L21))
**Default Value:** 60 Hz

Controls how often physics updates occur during gameplay. This constant is tunable:
- **60 Hz (default)**: Matches VBlank for synchronized physics/graphics, good battery life
- **Higher values (e.g., 120 Hz)**: Smoother gameplay at the cost of increased battery consumption

```c
#define RACE_TICK_FREQ 60  // Can be increased for smoother physics
```

## VBlank Timer System

### initTimer

**Signature:** `void initTimer(void)`
**Defined in:** [timer.c:34-42](../source/core/timer.c#L34-L42)

Initializes the VBlank interrupt for the current game state. Sets up `timerISRVblank()` to be called at 60Hz for screens that require animation.

**Active States:**
- HOME_PAGE
- MAPSELECTION
- GAMEPLAY
- PLAYAGAIN

**Current wiring:** `initTimer()` is invoked from `HomePage_Initialize()` so VBlank IRQ setup occurs when entering the home screen and stays enabled as the state machine transitions through other animated screens. If you add new animated states or want per-state reconfiguration, call `initTimer()` from their init paths as well.

```c
void initTimer(void) {
    GameContext* ctx = GameContext_Get();
    GameState state = ctx->currentGameState;
    if (state == HOME_PAGE || state == MAPSELECTION || state == GAMEPLAY ||
        state == PLAYAGAIN) {
        irqSet(IRQ_VBLANK, &timerISRVblank);
        irqEnable(IRQ_VBLANK);
    }
}
```

### timerISRVblank

**Signature:** `void timerISRVblank(void)`
**Defined in:** [timer.c:44-79](../source/core/timer.c#L44-L79)

VBlank interrupt service routine called at 60Hz by the hardware. Routes to state-specific OnVBlank handlers for display refreshes and updates pause button debouncing every frame.

**State-Specific Behavior:**

| State | Handler | Purpose |
|-------|---------|---------|
| HOME_PAGE | `HomePage_OnVBlank()` ([timer.c:50](../source/core/timer.c#L50)) | Animate kart sprites |
| MAPSELECTION | `MapSelection_OnVBlank()` ([timer.c:54](../source/core/timer.c#L54)) | Animate clouds and map previews |
| PLAYAGAIN | `PlayAgain_OnVBlank()` ([timer.c:58](../source/core/timer.c#L58)) | Update UI elements |
| GAMEPLAY | `Gameplay_OnVBlank()` + lap/time display ([timer.c:62-73](../source/core/timer.c#L62-L73)) | Sprite updates, countdown, chronometer display |

**Gameplay-Specific Logic:**
- During countdown: Calls `Race_CountdownTick()` for network-synchronized countdown (no movement)
- During active race: Updates lap display and chronometer display every frame
- After race completion: Displays final time

## Race Tick Timer System

Hardware timers (TIMER0 and TIMER1) used exclusively during GAMEPLAY state for physics and race time tracking.

### RaceTick_TimerInit

**Signature:** `void RaceTick_TimerInit(void)`
**Defined in:** [timer.c:84-96](../source/core/timer.c#L84-L96)

Initializes both hardware timers for gameplay:

**TIMER0 - Physics Tick:**
- Frequency: `RACE_TICK_FREQ` Hz (60Hz default)
- ISR: `RaceTick_ISR()` ([timer.c:121-123](../source/core/timer.c#L121-L123))
- Calls `Race_Tick()` for game logic updates (movement, collisions, items)

**TIMER1 - Chronometer:**
- Frequency: 1000 Hz (1ms precision)
- ISR: `ChronoTick_ISR()` ([timer.c:125-127](../source/core/timer.c#L125-L127))
- Calls `Gameplay_IncrementTimer()` for race time tracking

**Called:** When the countdown finishes, from `Race_CountdownTick()` in
`gameplay_logic.c` (starts after showing “GO”). It is not invoked when entering
GAMEPLAY; the countdown gate starts the timers.

```c
void RaceTick_TimerInit(void) {
    // TIMER0: Physics tick at RACE_TICK_FREQ Hz (60Hz default)
    TIMER_DATA(0) = TIMER_FREQ_1024(RACE_TICK_FREQ);
    TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1024 | TIMER_IRQ_REQ;
    irqSet(IRQ_TIMER0, RaceTick_ISR);
    irqEnable(IRQ_TIMER0);

    // TIMER1: Chronometer tick at 1000Hz (1ms precision for race time)
    TIMER_DATA(1) = TIMER_FREQ_1024(1000);
    TIMER1_CR = TIMER_ENABLE | TIMER_DIV_1024 | TIMER_IRQ_REQ;
    irqSet(IRQ_TIMER1, ChronoTick_ISR);
    irqEnable(IRQ_TIMER1);
}
```

### RaceTick_TimerStop

**Signature:** `void RaceTick_TimerStop(void)`
**Defined in:** [timer.c:98-104](../source/core/timer.c#L98-L104)

Stops and disables both race timers. Clears pending interrupts to prevent stray ISR calls.

**Called:** When exiting gameplay or transitioning to non-racing states

```c
void RaceTick_TimerStop(void) {
    // Disable and clear both timers
    irqDisable(IRQ_TIMER0);
    irqClear(IRQ_TIMER0);
    irqDisable(IRQ_TIMER1);
    irqClear(IRQ_TIMER1);
}
```

### RaceTick_TimerPause

**Signature:** `void RaceTick_TimerPause(void)`
**Defined in:** [timer.c:106-110](../source/core/timer.c#L106-L110)

Temporarily disables both race timers without clearing their state. Used for pause functionality where timers can be resumed later.

**Called:** When the game is paused during gameplay

```c
void RaceTick_TimerPause(void) {
    // Pause both timers (state preserved for resume)
    irqDisable(IRQ_TIMER0);
    irqDisable(IRQ_TIMER1);
}
```

### RaceTick_TimerEnable

**Signature:** `void RaceTick_TimerEnable(void)`
**Defined in:** [timer.c:112-116](../source/core/timer.c#L112-L116)

Re-enables both race timers after being paused. Resumes physics ticks and chronometer updates from their previous state.

**Called:** When unpausing the game to resume gameplay

```c
void RaceTick_TimerEnable(void) {
    // Resume both timers from paused state
    irqEnable(IRQ_TIMER0);
    irqEnable(IRQ_TIMER1);
}
```

## Private ISRs

### RaceTick_ISR

**Signature:** `static void RaceTick_ISR(void)`
**Defined in:** [timer.c:121-123](../source/core/timer.c#L121-L123)

Physics update interrupt handler called at `RACE_TICK_FREQ` Hz by TIMER0.

**Purpose:** Calls `Race_Tick()` to update game physics (movement, collisions, item logic)

### ChronoTick_ISR

**Signature:** `static void ChronoTick_ISR(void)`
**Defined in:** [timer.c:125-127](../source/core/timer.c#L125-L127)

Chronometer interrupt handler called at 1000 Hz (1ms precision) by TIMER1.

**Purpose:** Calls `Gameplay_IncrementTimer()` to increment race time by 1ms

## Usage Patterns

### Initializing VBlank Timer

```c
// In init_state() when entering an animated screen
void init_state(GameState state) {
    // ... setup graphics, sprites, etc ...

    initTimer();  // Sets up VBlank ISR for 60Hz updates
}
```

### Starting Gameplay Timers

```c
// At the start of a race
void StartRace(void) {
    RaceTick_TimerInit();  // Start both TIMER0 (physics) and TIMER1 (chronometer)

    // Physics and chronometer now running at 60Hz and 1000Hz respectively
}
```

### Implementing Pause Functionality

```c
// When user presses pause button
void PauseGame(void) {
    RaceTick_TimerPause();  // Stop physics and chronometer (state preserved)

    // Display pause menu...
}

void ResumeGame(void) {
    RaceTick_TimerEnable();  // Resume physics and chronometer from paused state
}
```

### Ending Gameplay

```c
// When race completes or player exits gameplay
void EndRace(void) {
    RaceTick_TimerStop();  // Stop and clear both timers

    // Transition to PLAYAGAIN or HOME_PAGE state...
}
```

### VBlank ISR Flow Example

```c
// Automatically called by hardware at 60Hz
void timerISRVblank(void) {
    GameContext* ctx = GameContext_Get();
    Race_UpdatePauseDebounce();  // Update pause button every frame

    switch (ctx->currentGameState) {
        case GAMEPLAY:
            if (Race_IsCountdownActive()) {
                Race_CountdownTick();  // Network-synchronized countdown
            }
            Gameplay_OnVBlank();  // Update sprites

            // Update displays during active racing
            if (!Race_IsCountdownActive() && !Race_IsCompleted()) {
                Gameplay_UpdateChronoDisplay(...);  // Update race time display
                Gameplay_UpdateLapDisplay(...);     // Update lap counter display
            }
            break;
        // ... other states ...
    }
}
```

## Performance Tuning

### Adjusting Physics Rate

To make the game run smoother at the cost of battery life:

1. Edit [game_constants.h:210-214](../source/core/game_constants.h#L210-L214):
   ```c
   #define RACE_TICK_FREQ 120  // Doubled from 60Hz
   ```

2. Rebuild the project

3. Physics updates will now run at 120Hz while VBlank remains at 60Hz

**Trade-offs:**
- **Higher RACE_TICK_FREQ**: Smoother physics, more responsive controls, faster battery drain
- **Lower RACE_TICK_FREQ**: Jerkier physics, better battery life (not recommended below 30Hz)

## Design Notes

- **Separation of Concerns**: VBlank for graphics, hardware timers for gameplay logic
- **State-Specific ISRs**: VBlank routes to different handlers based on game state
- **Pause/Resume**: Hardware timers can be paused without losing state
- **Synchronized Updates**: Default 60Hz physics matches 60Hz VBlank for smooth gameplay
- **Millisecond Precision**: 1000Hz chronometer provides accurate race time tracking
- **Nintendo DS Hardware**: Uses IRQ_VBLANK, IRQ_TIMER0, and IRQ_TIMER1 interrupts


---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)

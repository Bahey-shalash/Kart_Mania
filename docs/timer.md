# Timer System (Interrupt-Driven Architecture)

## Overview

**CRITICAL:** Kart Mania uses an **interrupt-driven architecture**. The game is NOT driven by the main loop - it's driven by hardware timer interrupts with precise frequencies. The timer system manages all ISRs that actually run the game.

The timer system provides two distinct timer subsystems:

1. **VBlank ISR** - 60Hz hardware interrupt for sprite updates and display refresh
2. **Race Tick ISRs** - Hardware timers for physics (TIMER0) and chronometer (TIMER1) during gameplay

The timer system is implemented in [timer.c](../source/core/timer.c) and [timer.h](../source/core/timer.h), using the Nintendo DS hardware interrupt system to achieve precise timing independent of the main loop.

## Architecture

### Interrupt-Driven Gameplay

**IMPORTANT DISTINCTION:** The main loop ([main.c](../source/core/main.c)) is NOT the game loop. It's a housekeeping task that handles WiFi and state transitions. The actual game runs in these hardware interrupts:

**VBlank ISR (60Hz Hardware Interrupt):**
- Triggered by Nintendo DS vertical blank signal (independent of main loop)
- Active on all animated screens (HOME_PAGE, MAPSELECTION, GAMEPLAY, PLAYAGAIN)
- Routes to state-specific handlers for sprite updates and display refreshes
- Fires at exactly 60Hz regardless of main loop state

**Hardware Timers (Gameplay Only - Independent of Main Loop):**
- **TIMER0**: Physics tick at `RACE_TICK_FREQ` Hz (default 60Hz) - calls `Race_Tick()` for movement, collisions, item logic
- **TIMER1**: Chronometer at 1000Hz (1ms precision) - calls `Gameplay_IncrementTimer()` for race time tracking
- Only active during GAMEPLAY state
- Can be paused/resumed for game pause functionality
- Fire independently - main loop can be blocked and these continue running

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

## Race Tick Timer System (THE ACTUAL GAME LOOP)

**CRITICAL:** This is where the game actually runs. Not in the main loop - in these hardware timer ISRs.

Hardware timers (TIMER0 and TIMER1) used exclusively during GAMEPLAY state for physics and race time tracking. These fire independently of the main loop and continue running even while the main loop is blocked on `swiWaitForVBlank()`.

### RaceTick_TimerInit

**Signature:** `void RaceTick_TimerInit(void)`
**Defined in:** [timer.c:84-96](../source/core/timer.c#L84-L96)

Initializes both hardware timers for gameplay:

**TIMER0 - Physics Tick (THE REAL GAME LOOP):**
- Frequency: `RACE_TICK_FREQ` Hz (60Hz default)
- ISR: `RaceTick_ISR()` ([timer.c:121-123](../source/core/timer.c#L121-L123))
- **Calls `Race_Tick()` - THIS IS WHERE ALL GAMEPLAY HAPPENS**
  - Input handling
  - Physics updates
  - Collision detection
  - Item logic
  - AI updates
- Runs independently of main loop

**TIMER1 - Chronometer:**
- Frequency: 1000 Hz (1ms precision)
- ISR: `ChronoTick_ISR()` ([timer.c:125-127](../source/core/timer.c#L125-L127))
- Calls `Gameplay_IncrementTimer()` for race time tracking
- Runs independently of main loop

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

## Private ISRs (Where the Game Actually Runs)

### RaceTick_ISR (THE ACTUAL GAME LOOP)

**Signature:** `static void RaceTick_ISR(void)`
**Defined in:** [timer.c:121-123](../source/core/timer.c#L121-L123)

**CRITICAL:** This is the actual game loop. Hardware triggers this ISR at `RACE_TICK_FREQ` Hz (60Hz default), completely independent of the main loop.

**What it does:**
```c
static void RaceTick_ISR(void) {
    Race_Tick();  // ALL gameplay happens here
}
```

**Inside `Race_Tick()` ([gameplay_logic.c:326](../source/gameplay/gameplay_logic.c#L326)):**
- Reads controller input
- Updates car physics (acceleration, steering, braking)
- Checks wall/terrain collisions
- Updates item effects
- Processes checkpoint progression
- Handles network synchronization
- **This is the ENTIRE game logic, running in an interrupt**

**Frequency:** Hardware-enforced 60Hz (or `RACE_TICK_FREQ` if configured differently)

### ChronoTick_ISR

**Signature:** `static void ChronoTick_ISR(void)`
**Defined in:** [timer.c:125-127](../source/core/timer.c#L125-L127)

Chronometer interrupt handler called at 1000 Hz (1ms precision) by TIMER1.

**Purpose:** Calls `Gameplay_IncrementTimer()` to increment race time by 1ms

**Frequency:** Hardware-enforced 1000Hz (runs independently of main loop and TIMER0)

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

Since gameplay runs in the TIMER0 ISR (not the main loop), you can independently adjust physics frequency:

1. Edit [game_constants.h:210-214](../source/core/game_constants.h#L210-L214):
   ```c
   #define RACE_TICK_FREQ 120  // Doubled from 60Hz
   ```

2. Rebuild the project

3. **TIMER0 ISR will now fire at 120Hz** (calling `Race_Tick()` 120 times per second)
4. VBlank ISR remains at 60Hz (sprite updates)
5. Main loop remains at 60Hz (WiFi heartbeat)

**Why this works:** Each system runs independently:
- **TIMER0** (physics) → 120Hz
- **VBlank** (sprites) → 60Hz (hardware-enforced)
- **Main loop** (WiFi) → 60Hz (VBlank-synchronized)

**Trade-offs:**
- **Higher RACE_TICK_FREQ**: Smoother physics, more responsive controls, faster battery drain
- **Lower RACE_TICK_FREQ**: Jerkier physics, better battery life (not recommended below 30Hz)

## Design Notes

- **Interrupt-driven**: Gameplay in TIMER0 ISR, graphics in VBlank ISR, independent of main loop
- **Hardware precision**: Exact frequencies regardless of main loop state
- **State-specific routing**: VBlank dispatches to different handlers per game state
- **Tunable physics**: RACE_TICK_FREQ can be adjusted independently of display refresh
- **Pause/resume support**: Timers disable/enable without losing state


---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)

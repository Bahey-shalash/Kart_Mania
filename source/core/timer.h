/**
 * File: timer.h
 * --------------
 * Description: Timer and interrupt service routine (ISR) management for the game.
 *              Provides two timer systems: VBlank ISR for 60Hz graphics updates,
 *              and hardware timers for physics ticks (RACE_TICK_FREQ=60Hz) and race
 *              chronometer (1000Hz) during gameplay.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#ifndef TIMER_H
#define TIMER_H

//=============================================================================
// Configuration
//=============================================================================

// Note: RACE_TICK_FREQ moved to game_constants.h

//=============================================================================
// VBlank Timer (60Hz graphics)
//=============================================================================

/**
 * Function: initTimer
 * -------------------
 * Initializes the VBlank interrupt for the current game state. Sets up
 * timerISRVblank() to be called at 60Hz for graphics updates on screens
 * that require animation (HOME_PAGE, MAPSELECTION, GAMEPLAY, PLAYAGAIN).
 *
 * Called: Once per state transition in init_state()
 */
void initTimer(void);

/**
 * Function: timerISRVblank
 * ------------------------
 * VBlank interrupt service routine called at 60Hz. Routes to state-specific
 * OnVBlank handlers display refreshes.
 *
 * State-specific behavior:
 *   HOME_PAGE    - HomePage_OnVBlank() for animated kart sprites
 *   MAPSELECTION - MapSelection_OnVBlank() for cloud animations
 *   PLAYAGAIN    - PlayAgain_OnVBlank() for UI updates
 *   GAMEPLAY     - Gameplay_OnVBlank() + lap/time display updates
 *
 * Called: Automatically by hardware at 60Hz when VBlank IRQ is enabled
 */
void timerISRVblank(void);

//=============================================================================
// Race Tick Timer (physics updates)
//=============================================================================

/**
 * Function: RaceTick_TimerInit
 * ----------------------------
 * Initializes hardware timers for gameplay. Sets up two timers:
 *   - TIMER0: RACE_TICK_FREQ Hz physics tick (calls Race_Tick() for game tick updates)
 *   - TIMER1: 1000Hz chronometer (calls Gameplay_IncrementTimer() for race time)
 *
 * Called: At the start of gameplay when entering GAMEPLAY state
 */
void RaceTick_TimerInit(void);

/**
 * Function: RaceTick_TimerStop
 * ----------------------------
 * Stops and disables both race timers (TIMER0 and TIMER1). Clears pending
 * interrupts to prevent stray ISR calls.
 *
 * Called: When exiting gameplay or transitioning to non-racing states
 */
void RaceTick_TimerStop(void);

/**
 * Function: RaceTick_TimerPause
 * -----------------------------
 * Temporarily disables both race timers without clearing their state.
 * Used for pause functionality where timers can be resumed later.
 *
 * Called: When the game is paused during gameplay
 */
void RaceTick_TimerPause(void);

/**
 * Function: RaceTick_TimerEnable
 * ------------------------------
 * Re-enables both race timers after being paused. Resumes physics ticks
 * and chronometer updates from their previous state.
 *
 * Called: When unpausing the game to resume gameplay
 */
void RaceTick_TimerEnable(void);

#endif  // TIMER_H

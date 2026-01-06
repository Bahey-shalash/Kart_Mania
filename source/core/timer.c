/**
 * File: timer.c
 * --------------
 * Description: Implementation of timer ISRs for graphics updates and gameplay ticks.
 *              VBlank ISR runs at 60Hz for all animated screens, while hardware
 *              timers (TIMER0/TIMER1) handle physics and chronometer during races.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#include "timer.h"

#include <nds.h>

#include "../gameplay/gameplay.h"
#include "../gameplay/gameplay_logic.h"
#include "../ui/home_page.h"
#include "../ui/map_selection.h"
#include "../ui/play_again.h"
#include "context.h"
#include "game_types.h"

//=============================================================================
// Private Prototypes
//=============================================================================
static void RaceTick_ISR(void);
static void ChronoTick_ISR(void);

//=============================================================================
// VBlank ISR - 60Hz Graphics Updates
//=============================================================================
void initTimer(void) {
    GameContext* ctx = GameContext_Get();
    GameState state = ctx->currentGameState;
    if (state == HOME_PAGE || state == MAPSELECTION || state == GAMEPLAY ||
        state == PLAYAGAIN) {
        irqSet(IRQ_VBLANK, &timerISRVblank);
        irqEnable(IRQ_VBLANK);
    }
}

void timerISRVblank(void) {
    GameContext* ctx = GameContext_Get();
    Race_UpdatePauseDebounce();  // Update pause button state every frame

    switch (ctx->currentGameState) {
        case HOME_PAGE:
            HomePage_OnVBlank();  // Animate kart sprites
            break;

        case MAPSELECTION:
            MapSelection_OnVBlank();  // Animate clouds and map previews
            break;

        case PLAYAGAIN:
            PlayAgain_OnVBlank();  // Update UI elements
            break;

        case GAMEPLAY:
            if (Race_IsCountdownActive()) {
                Race_CountdownTick();  // Countdown timer (network sync, no movement)
            }
            Gameplay_OnVBlank();  // Sprite updates and final time display

            // Update lap/time displays only during active racing
            if (!Race_IsCountdownActive() && !Race_IsCompleted()) {
                Gameplay_UpdateChronoDisplay(Gameplay_GetRaceMin(), Gameplay_GetRaceSec(),
                                              Gameplay_GetRaceMsec());
                const RaceState* state = Race_GetState();
                Gameplay_UpdateLapDisplay(Gameplay_GetCurrentLap(), state->totalLaps);
            }
            break;

        default:
            break;
    }
}

//=============================================================================
// Race Tick Timers
//=============================================================================
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

void RaceTick_TimerStop(void) {
    // Disable and clear both timers
    irqDisable(IRQ_TIMER0);
    irqClear(IRQ_TIMER0);
    irqDisable(IRQ_TIMER1);
    irqClear(IRQ_TIMER1);
}

void RaceTick_TimerPause(void) {
    // Pause both timers (state preserved for resume)
    irqDisable(IRQ_TIMER0);
    irqDisable(IRQ_TIMER1);
}

void RaceTick_TimerEnable(void) {
    // Resume both timers from paused state
    irqEnable(IRQ_TIMER0);
    irqEnable(IRQ_TIMER1);
}

//=============================================================================
// Private ISRs
//=============================================================================
static void RaceTick_ISR(void) {
    Race_Tick();  // Physics update: movement, collisions, item logic
}

static void ChronoTick_ISR(void) {
    Gameplay_IncrementTimer();  // Increment race time by 1ms
}

#include "timer.h"
#include "context.h"
#include "game_types.h"
#include "gameplay.h"
#include "gameplay_logic.h"
#include "home_page.h"
#include "map_selection.h"

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
    
    if (state == HOME_PAGE || state == MAPSELECTION || state == GAMEPLAY) {
        irqSet(IRQ_VBLANK, &timerISRVblank);
        irqEnable(IRQ_VBLANK);
    }
}

void timerISRVblank(void) {
    GameContext* ctx = GameContext_Get();
    
    switch (ctx->currentGameState) {
        case HOME_PAGE:
            HomePage_OnVBlank();
            break;
            
        case MAPSELECTION:
            Map_selection_OnVBlank();
            break;
            
        case GAMEPLAY: {
            Gameplay_OnVBlank();
            
            // Update sub-screen displays
            updateChronoDisp_Sub(
                Gameplay_GetRaceMin(), 
                Gameplay_GetRaceSec(), 
                Gameplay_GetRaceMsec()
            );
            
            const RaceState* state = Race_GetState();
            updateLapDisp_Sub(Gameplay_GetCurrentLap(), state->totalLaps);
            break;
        }
            
        default:
            break;
    }
}

//=============================================================================
// Race Tick Timers
//=============================================================================
void RaceTick_TimerInit(void) {
    // TIMER0: Physics at RACE_TICK_FREQ Hz
    TIMER_DATA(0) = TIMER_FREQ_1024(RACE_TICK_FREQ);
    TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1024 | TIMER_IRQ_REQ;
    irqSet(IRQ_TIMER0, RaceTick_ISR);
    irqEnable(IRQ_TIMER0);
    
    // TIMER1: Chronometer at 1000 Hz (1ms intervals)
    TIMER_DATA(1) = TIMER_FREQ_1024(1000);
    TIMER1_CR = TIMER_ENABLE | TIMER_DIV_1024 | TIMER_IRQ_REQ;
    irqSet(IRQ_TIMER1, ChronoTick_ISR);
    irqEnable(IRQ_TIMER1);
}

void RaceTick_TimerStop(void) {
    irqDisable(IRQ_TIMER0);
    irqClear(IRQ_TIMER0);
    irqDisable(IRQ_TIMER1);
    irqClear(IRQ_TIMER1);
}

void RaceTick_TimerPause(void) {
    irqDisable(IRQ_TIMER0);
    irqDisable(IRQ_TIMER1);
}

void RaceTick_TimerEnable(void) {
    irqEnable(IRQ_TIMER0);
    irqEnable(IRQ_TIMER1);
}

//=============================================================================
// Private ISRs
//=============================================================================
static void RaceTick_ISR(void) {
    Race_Tick();
}

static void ChronoTick_ISR(void) {
    Gameplay_IncrementTimer();
}
#include "timer.h"

#include "context.h"
#include "game_types.h"
#include "gameplay.h"
#include "gameplay_logic.h"
#include "home_page.h"
#include "map_selection.h"

//=============================================================================
// VBlank ISR - 60Hz graphics updates ONLY
//=============================================================================
void initTimer(void) {
    GameContext* ctx = GameContext_Get();

    if (ctx->currentGameState == HOME_PAGE || ctx->currentGameState == MAPSELECTION ||
        ctx->currentGameState == GAMEPLAY) {
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
        case GAMEPLAY:
            // Graphics only - reads current car state, doesn't modify it
            Gameplay_OnVBlank();
            break;
        default:
            break;
    }
}

//=============================================================================
// Race Tick Timer - physics at RACE_TICK_FREQ Hz using TIMER0
//=============================================================================
static void RaceTick_ISR(void);

void RaceTick_TimerInit(void) {
    // TIMER0 with 1024 divider
    // TIMER_FREQ_1024(freq) calculates the reload value for desired frequency
    TIMER_DATA(0) = TIMER_FREQ_1024(RACE_TICK_FREQ);
    TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1024 | TIMER_IRQ_REQ;

    irqSet(IRQ_TIMER0, RaceTick_ISR);
    irqEnable(IRQ_TIMER0);
}

void RaceTick_TimerStop(void) {
    irqDisable(IRQ_TIMER0);
    irqClear(IRQ_TIMER0);
}
void RaceTick_TimerPause(void) {
    irqDisable(IRQ_TIMER0);
}

void RaceTick_TimerEnable(void) {
    irqEnable(IRQ_TIMER0);
}


static void RaceTick_ISR(void) {
    Race_Tick();
}
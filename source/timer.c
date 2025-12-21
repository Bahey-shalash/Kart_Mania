// timer.c
#include "timer.h"

#include "context.h"
#include "game_types.h"
#include "home_page.h"

void initTimer(void) {
    // only start timer on HOME
    GameContext* ctx = GameContext_Get();
    if (ctx->currentGameState == HOME_PAGE) {
        irqSet(IRQ_VBLANK, &timerISRVblank);
        irqEnable(IRQ_VBLANK);
    } else {
        return; // for future dev just return for
    }
}

void timerISRVblank(void) {
    GameContext* ctx = GameContext_Get();
    if (ctx->currentGameState == HOME_PAGE) {
        move_homeKart();
    }
}
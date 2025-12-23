#include "timer.h"

#include "context.h"
#include "game_types.h"
#include "home_page.h"
#include "map_selection.h"

void initTimer(void) {
    // only start timer on HOME
    GameContext* ctx = GameContext_Get();
    if (ctx->currentGameState == HOME_PAGE || ctx->currentGameState == MAPSELECTION) {
        irqSet(IRQ_VBLANK, &timerISRVblank);
        irqEnable(IRQ_VBLANK);
    } else {
        return;  // for future dev just return for now
    }
}

void timerISRVblank(void) {
    GameContext* ctx = GameContext_Get();
    if (ctx->currentGameState == HOME_PAGE) {
        HomePage_OnVBlank();
    }
    if (ctx->currentGameState == MAPSELECTION) {
        Map_selection_OnVBlank();
    }
}
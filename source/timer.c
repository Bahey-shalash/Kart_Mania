// timer.c
#include "timer.h"

#include "game_types.h"
#include "home_page.h"  // for move_homeKart()

extern GameState currentState_GLOBAL;

/* void stopTimer0(void) {
    irqDisable(IRQ_TIMER0);  // stop its IRQ line
} */

void initTimer(void) {
    // only start timer on HOME
    if (currentState_GLOBAL == HOME_PAGE) {
        irqSet(IRQ_VBLANK, &timerISRVblank);
        irqEnable(IRQ_VBLANK);
    } else {
        return;
    }
}

void timerISRVblank(void) {
    if (currentState_GLOBAL == HOME_PAGE) {
        move_homeKart();
    }
}
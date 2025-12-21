#ifndef TIMER_H
#define TIMER_H

#include <nds.h>


void initTimer(void);
void timerISRVblank(void);
void stopTimer(void);
#endif  // TIMER_H
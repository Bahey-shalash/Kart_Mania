#ifndef TIMER_H
#define TIMER_H

#include <nds.h>

//=============================================================================
// VBlank Timer (graphics at 60Hz)
//=============================================================================
void initTimer(void);
void timerISRVblank(void);

//=============================================================================
// Race Tick Timer (physics - independent rate)
//=============================================================================
// Adjust this to tune physics responsiveness vs CPU usage:
//   120 = very smooth, higher CPU
//    60 = matches display, good default
#define RACE_TICK_FREQ 60

void RaceTick_TimerInit(void);
void RaceTick_TimerStop(void);

void RaceTick_TimerPause(void);
void RaceTick_TimerEnable(void);

#endif  // TIMER_H
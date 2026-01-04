#ifndef TIMER_H
#define TIMER_H
// BAHEY------
//=============================================================================
// Configuration
//=============================================================================
#define RACE_TICK_FREQ 60  // Physics update rate (Hz)

//=============================================================================
// VBlank Timer (60Hz graphics)
//=============================================================================
void initTimer(void);
void timerISRVblank(void);

//=============================================================================
// Race Tick Timer (physics updates)
//=============================================================================
void RaceTick_TimerInit(void);
void RaceTick_TimerStop(void);
void RaceTick_TimerPause(void);
void RaceTick_TimerEnable(void);

#endif  // TIMER_H

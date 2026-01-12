/**
 * File: state_machine.h
 * ---------------------
 * Description: Game state machine interface. Manages state transitions,
 * initialization, and cleanup for all game states (HOME_PAGE, GAMEPLAY, SETTINGS,
 * etc.).
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "game_types.h"

/**
 * Function: StateMachine_Update
 * -----------------------------
 * Dispatches to the current state's update function. Each state's update
 * function handles input, logic, and returns the next state to transition to.
 *
 * Parameters:
 *   state - Current GameState
 *
 * Returns: Next GameState (may be same as current if no transition)
 */
GameState StateMachine_Update(GameState state);

/**
 * Function: StateMachine_Init
 * ---------------------------
 * Initializes graphics, timers, and resources for a new state. Called when
 * transitioning to a new state.
 *
 * Parameters:
 *   state - GameState to initialize
 */
void StateMachine_Init(GameState state);

/**
 * Function: StateMachine_Cleanup
 * -------------------------------
 * Cleans up resources for the state being exited. Carefully manages multiplayer
 * connection lifecycle - keeps WiFi alive when transitioning to gameplay or when
 * player might restart a race.
 *
 * Parameters:
 *   state - GameState being exited
 *   nextState - GameState being entered (used for conditional cleanup)
 */
void StateMachine_Cleanup(GameState state, GameState nextState);

#endif  // STATE_MACHINE_H

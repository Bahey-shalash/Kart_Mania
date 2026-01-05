/**
 * File: main.c
 * ------------
 * Description: Main entry point for Kart Mania. Contains only the main game loop
 *              that handles state transitions and frame synchronization.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#include <nds.h>
#include <dswifi9.h>

#include "../graphics/graphics.h"
#include "context.h"
#include "init.h"
#include "state_machine.h"

int main(void) {
    // Perform one-time initialization of all subsystems
    InitGame();

    GameContext* ctx = GameContext_Get();

    // Main game loop
    while (true) {
        // Update DSWifi state machine every frame (critical for multiplayer)
        Wifi_Update();

        // Run current state's update logic (returns next state)
        GameState nextState = StateMachine_Update(ctx->currentGameState);

        // Handle state transitions
        if (nextState != ctx->currentGameState) {
            StateMachine_Cleanup(ctx->currentGameState, nextState);
            ctx->currentGameState = nextState;
            video_nuke();
            StateMachine_Init(nextState);
        }

        // Wait for vertical blank (60Hz synchronization)
        swiWaitForVBlank();
    }

    return 0;
}

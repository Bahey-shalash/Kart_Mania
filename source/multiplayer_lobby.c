#include "multiplayer_lobby.h"

#include <nds.h>
#include <stdio.h>

#include "context.h"
#include "game_types.h"
#include "multiplayer.h"
#include "WiFi_minilib.h"

//=============================================================================
// Lobby State
//=============================================================================

static int countdownTimer = 0;
static bool countdownActive = false;

//=============================================================================
// Public API - Lobby Initialization
//=============================================================================

void MultiplayerLobby_Init(void) {
    // Initialize console on sub-screen
    consoleDemoInit();
    consoleClear();

    printf("\x1b[2J");  // Clear screen
    printf("=== MULTIPLAYER LOBBY ===\n\n");
    printf("Connecting...\n");

    // Join the lobby (broadcasts presence to other players)
    Multiplayer_JoinLobby();

    // Set default map for multiplayer (ScorchingSands for now)
    // TODO: Add map selection for multiplayer later
    GameContext_SetMap(ScorchingSands);

    countdownTimer = 0;
    countdownActive = false;
}

//=============================================================================
// Public API - Lobby Update (call every frame)
//=============================================================================

GameState MultiplayerLobby_Update(void) {
    scanKeys();
    int keys = keysDown();

    // Toggle ready state (disabled once countdown starts)
    if (keys & KEY_SELECT && !countdownActive) {
        int myID = Multiplayer_GetMyPlayerID();
        bool currentReady = Multiplayer_IsPlayerReady(myID);
        Multiplayer_SetReady(!currentReady);  // Toggle ready state
    }

    // Allow cancel at any time (even during countdown/timeout)
    if (keys & KEY_B) {
        Multiplayer_Cleanup();
        GameContext_SetMultiplayerMode(false);
        return HOME_PAGE;
    }

    //=========================================================================
    // Update lobby state (receive packets, check timeouts)
    //=========================================================================
    bool allReady = Multiplayer_UpdateLobby();

    //=========================================================================
    // Display lobby status on console
    //=========================================================================
    consoleClear();
    printf("=== MULTIPLAYER LOBBY ===\n\n");

    int myID = Multiplayer_GetMyPlayerID();
    int connectedCount = 0;
    int readyCount = 0;

    // Display all connected players
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            connectedCount++;
            bool ready = Multiplayer_IsPlayerReady(i);
            if (ready)
                readyCount++;

            printf("Player %d: %s%s\n", i + 1, ready ? "[READY]   " : "[WAITING] ",
                   (i == myID) ? "(YOU)" : "");
        }
    }

    printf("\n(%d/%d ready)\n\n", readyCount, connectedCount);

    // Debug info at bottom of screen
    int sent, received;
    Multiplayer_GetDebugStats(&sent, &received);

    // Low-level socket debug
    int recvCalls, recvSuccess, recvFiltered;
    getReceiveDebugStats(&recvCalls, &recvSuccess, &recvFiltered);

    printf("--------------------------------\n");
    printf("DEBUG: MyID=%d Connected=%d\n", myID, connectedCount);
    printf("AllReady=%d Countdown=%d\n", allReady ? 1 : 0, countdownActive ? 1 : 0);
    printf("Packets: Sent=%d Recv=%d\n", sent, received);
    printf("Socket: Calls=%d OK=%d Filt=%d\n", recvCalls, recvSuccess, recvFiltered);
    unsigned long myIP = Wifi_GetIP();
    unsigned char macAddr[6];
    Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, macAddr);
    printf("IP: %lu.%lu.%lu.%lu\n",
           (myIP & 0xFF), ((myIP >> 8) & 0xFF),
           ((myIP >> 16) & 0xFF), ((myIP >> 24) & 0xFF));
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           macAddr[0], macAddr[1], macAddr[2],
           macAddr[3], macAddr[4], macAddr[5]);

    // If someone drops or unreadies, cancel the countdown
    if (countdownActive && (!allReady || connectedCount < 2)) {
        countdownActive = false;
        countdownTimer = 0;
    }

    // Handle countdown or show instructions
    if (!countdownActive) {
        // Not in countdown - show instructions
        printf("Press SELECT when ready\n");
        printf("Press B to cancel\n");

        // Start countdown if all players are ready
        if (allReady && connectedCount >= 2) {
            countdownActive = true;
            countdownTimer = 180;  // 3 seconds at 60Hz (3 * 60 = 180 frames)
        }
    } else {
        // Countdown active - show countdown
        int secondsLeft = (countdownTimer / 60) + 1;  // Round up

        printf("\nStarting in %d...\n", secondsLeft);

        countdownTimer--;
        if (countdownTimer <= 0) {
            // Countdown finished - start race!
            Multiplayer_StartRace();  // Clear pending lobby ACKs
            GameContext_SetMap(ScorchingSands);  // not sure about this
            return GAMEPLAY;
        }
    }

    return MULTIPLAYER_LOBBY;
}

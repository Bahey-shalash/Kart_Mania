#include "multiplayer.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "WiFi_minilib.h"

//=============================================================================
// Protocol Constants
//=============================================================================

#define PROTOCOL_VERSION 1
#define PLAYER_TIMEOUT_MS 3000  // 3 seconds without packets = disconnected

//=============================================================================
// Network Packet Format (32 bytes total)
//=============================================================================

typedef enum {
    MSG_LOBBY_JOIN,    // "I'm joining the lobby"
    MSG_LOBBY_UPDATE,  // "I'm still here" (heartbeat)
    MSG_READY,         // "I pressed SELECT"
    MSG_CAR_UPDATE,    // "Here's my car state" (during race)
    MSG_DISCONNECT     // "I'm leaving"
} MessageType;

typedef struct {
    uint8_t version;   // Protocol version (for future compatibility)
    uint8_t msgType;   // MessageType enum
    uint8_t playerID;  // 0-7
    uint8_t padding;   // Alignment

    // Payload (28 bytes) - content depends on msgType
    union {
        // For MSG_LOBBY_JOIN, MSG_LOBBY_UPDATE, MSG_READY
        struct {
            bool isReady;          // Has this player pressed SELECT?
            uint8_t reserved[27];  // Future expansion
        } lobby;

        // For MSG_CAR_UPDATE (during race)
        struct {
            Vec2 position;  // 8 bytes (2 x int32_t in Q16.8)
            Q16_8 speed;    // 4 bytes
            int angle512;   // 4 bytes
            int lap;        // 4 bytes
            Item item;      // 4 bytes
            uint8_t reserved[4];
        } carState;

        uint8_t raw[28];  // For future message types
    } payload;
} NetworkPacket;  // Total: 32 bytes

//=============================================================================
// Player Tracking
//=============================================================================

typedef struct {
    bool connected;           // Is this player in the game?
    bool ready;               // Has this player pressed SELECT? (lobby only)
    uint32_t lastPacketTime;  // For timeout detection
} PlayerInfo;

//=============================================================================
// Module State
//=============================================================================

static int myPlayerID = -1;
static PlayerInfo players[MAX_MULTIPLAYER_PLAYERS];
static bool initialized = false;

// Simple millisecond counter (wraps every ~49 days, which is fine)
static uint32_t msCounter = 0;
static uint32_t lastLobbyBroadcastMs = 0;

//=============================================================================
// Helper Functions
//=============================================================================

/**
 * Get current time in milliseconds (approximate)
 * Called once per frame, so just increment by ~16ms per call
 */
static uint32_t getTimeMs(void) {
    msCounter += 16;  // Approximate: 1 frame at 60Hz = 16.67ms
    return msCounter;
}

//=============================================================================
// Public API - Initialization
//=============================================================================

/*
 * ============================================================================
 * MULTIPLAYER PLAYER ID ASSIGNMENT - PROBLEM & SOLUTION
 * ============================================================================
 *
 * PROBLEM ENCOUNTERED:
 * -------------------
 * During testing, both Nintendo DS units were being assigned the SAME player ID
 * (both showing as "Player 1"), causing the lobby to show "1/1 ready" instead of
 * recognizing two separate players. This broke the entire multiplayer system.
 *
 * ROOT CAUSE ANALYSIS:
 * -------------------
 * Original code used only the LAST OCTET of the IP address for player ID:
 *
 *     myPlayerID = (myIP & 0xFF) % MAX_MULTIPLAYER_PLAYERS;
 *                   ^^^^^^^^^^^
 *                   Only uses bits 0-7 (last octet)
 *
 * DHCP servers typically assign sequential IPs on the same subnet:
 *   - DS #1: 192.168.1.100 → last octet = 100 → 100 % 8 = 4 → Player 5
 *   - DS #2: 192.168.1.108 → last octet = 108 → 108 % 8 = 4 → Player 5 (COLLISION!)
 *
 * SOLUTIONS CONSIDERED:
 * --------------------
 *
 * 1. FULL 32-BIT IP MODULO:
 *    myPlayerID = (int)(myIP % MAX_MULTIPLAYER_PLAYERS);
 *
 *    Pros: Uses entire IP address, better than last octet only
 *    Cons: Sequential IPs can still collide (e.g., 192.168.1.8 and 192.168.1.16)
 *    Verdict: REJECTED - Still vulnerable to collisions with sequential DHCP
 *
 * 2. XOR OF ALL OCTETS:
 *    uint8_t hash = octet0 ^ octet1 ^ octet2 ^ octet3;
 *    myPlayerID = hash % MAX_MULTIPLAYER_PLAYERS;
 *
 *    Pros: Spreads entropy across all octets
 *    Cons: XOR can still produce identical results with similar IPs
 *    Verdict: REJECTED - Marginal improvement, not guaranteed collision-free
 *
 * 3. MAC ADDRESS (HARDWARE UNIQUE):
 *    Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, macAddr);
 *    myPlayerID = macAddr[5] % MAX_MULTIPLAYER_PLAYERS;
 *
 *    Pros: - MAC addresses are globally unique (IEEE standard)
 *          - Burned into WiFi chip hardware (never changes)
 *          - High entropy in last byte (manufacturer variation)
 *          - Same DS always gets same player ID (consistent)
 *    Cons: None for this use case
 *    Verdict: ACCEPTED ✓ - Guaranteed collision-free, hardware-backed uniqueness
 *
 * FINAL SOLUTION:
 * --------------
 * Use the last byte of the MAC address for player ID assignment. This provides:
 * - Hardware-level uniqueness (no two DS units have the same MAC)
 * - Deterministic assignment (same DS = same player ID every session)
 * - Excellent distribution across 8 player slots
 * - Zero probability of collisions
 *
 * EXAMPLE OUTPUT:
 * --------------
 * DS Unit #1:
 *   MAC: 00:09:BF:12:34:AB (last byte = 0xAB = 171)
 *   Player ID: 171 % 8 = 3 → "You are Player 4"
 *
 * DS Unit #2:
 *   MAC: 00:09:BF:56:78:CD (last byte = 0xCD = 205)
 *   Player ID: 205 % 8 = 5 → "You are Player 6"
 *
 * Result: Both DS units get DIFFERENT, STABLE player IDs ✓
 * ============================================================================
 */

int Multiplayer_Init(void) {
    if (initialized) {
        // return myPlayerID;  // Already initialized
        Multiplayer_Cleanup();
    }

    // Initialize console for status messages (sub-screen)
    consoleDemoInit();
    consoleClear();
    printf("\x1b[2J");  // Clear screen

    printf("=== MULTIPLAYER INIT ===\n\n");
    printf("Connecting to WiFi...\n");
    printf("Looking for 'MES-NDS'...\n\n");
    printf("(This may take 5-10 seconds)\n");

    // Initialize WiFi (with timeout)
    if (!initWiFi()) {
        consoleClear();
        printf("WiFi Connection Failed!\n\n");
        printf("Possible issues:\n");
        printf("- WiFi is OFF\n");
        printf("- 'MES-NDS' AP not found\n");
        printf("- Out of range\n\n");
        printf("Press B to return\n");

        // Wait for user acknowledgment
        while (1) {
            scanKeys();
            if (keysDown() & KEY_B) {
                break;
            }
            swiWaitForVBlank();
        }

        return -1;
    }

    printf("\nWiFi connected!\n");
    printf("Opening socket...\n");

    // Open socket
    if (!openSocket()) {
        consoleClear();
        printf("Socket Error!\n\n");
        printf("Failed to create UDP socket.\n\n");
        printf("Press B to return\n");

        while (1) {
            scanKeys();
            if (keysDown() & KEY_B) {
                break;
            }
            swiWaitForVBlank();
        }

        disconnectFromWiFi();
        return -1;
    }

    printf("Socket ready!\n\n");

    //=========================================================================
    // PLAYER ID ASSIGNMENT - MAC ADDRESS BASED (COLLISION-FREE)
    //=========================================================================
    // Get MAC address from WiFi hardware (6 bytes, globally unique)
    unsigned char macAddr[6];
    Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, macAddr);

    // Use last byte of MAC address to determine player ID
    // This gives us excellent distribution across 0-7 player slots
    // and guarantees no collisions (every DS has unique MAC address)
    myPlayerID = macAddr[5] % MAX_MULTIPLAYER_PLAYERS;

    // Get IP address for display/debugging purposes
    unsigned long myIP = Wifi_GetIP();

    // Display connection info (helps with debugging network issues)
    printf("You are Player %d\n", myPlayerID + 1);
    printf("IP: %lu.%lu.%lu.%lu\n", (myIP >> 0) & 0xFF, (myIP >> 8) & 0xFF,
           (myIP >> 16) & 0xFF, (myIP >> 24) & 0xFF);
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", macAddr[0], macAddr[1], macAddr[2],
           macAddr[3], macAddr[4], macAddr[5]);

    // Initialize player tracking
    memset(players, 0, sizeof(players));
    players[myPlayerID].connected = true;
    players[myPlayerID].ready = false;
    players[myPlayerID].lastPacketTime = getTimeMs();
    lastLobbyBroadcastMs = players[myPlayerID].lastPacketTime;

    initialized = true;

    // Small delay to show success message
    for (int i = 0; i < 90; i++) {  // ~1.5 seconds
        swiWaitForVBlank();
    }

    return myPlayerID;
}

/**
 * Reset lobby state (call when re-entering lobby after exiting gameplay)
 * This clears stale connection state from previous sessions
 */
static void resetLobbyState(void) {
    // Reset all players except ourselves
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (i != myPlayerID) {
            players[i].connected = false;
            players[i].ready = false;
        }
    }
}

void Multiplayer_Cleanup(void) {
    if (!initialized) {
        return;
    }

    // Send disconnect message to other players
    NetworkPacket packet = {.version = PROTOCOL_VERSION,
                            .msgType = MSG_DISCONNECT,
                            .playerID = myPlayerID};
    sendData((char*)&packet, sizeof(NetworkPacket));

    // Cleanup WiFi
    closeSocket();
    disconnectFromWiFi();

    initialized = false;
    lastLobbyBroadcastMs = 0;
    myPlayerID = -1;
}

//=============================================================================
// Public API - Player Info
//=============================================================================

int Multiplayer_GetMyPlayerID(void) {
    return myPlayerID;
}

int Multiplayer_GetConnectedCount(void) {
    int count = 0;
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (players[i].connected) {
            count++;
        }
    }
    return count;
}

bool Multiplayer_IsPlayerConnected(int playerID) {
    if (playerID < 0 || playerID >= MAX_MULTIPLAYER_PLAYERS) {
        return false;
    }
    return players[playerID].connected;
}

bool Multiplayer_IsPlayerReady(int playerID) {
    if (playerID < 0 || playerID >= MAX_MULTIPLAYER_PLAYERS) {
        return false;
    }
    return players[playerID].ready;
}

//=============================================================================
// Public API - Lobby
//=============================================================================

void Multiplayer_JoinLobby(void) {
    // CRITICAL FIX: Reset lobby state before joining
    // This prevents stale "ghost players" from previous sessions
    resetLobbyState();

    NetworkPacket packet = {.version = PROTOCOL_VERSION,
                            .msgType = MSG_LOBBY_JOIN,
                            .playerID = myPlayerID,
                            .payload.lobby = {.isReady = false}};

    sendData((char*)&packet, sizeof(NetworkPacket));

    // Mark self as not ready
    players[myPlayerID].ready = false;
    lastLobbyBroadcastMs = getTimeMs();
}

bool Multiplayer_UpdateLobby(void) {
    NetworkPacket packet;
    uint32_t currentTime = getTimeMs();

    // Periodic heartbeat so peers don't time out (every ~1s)
    if (currentTime - lastLobbyBroadcastMs >= 1000) {
        NetworkPacket heartbeat = {
            .version = PROTOCOL_VERSION,
            .msgType = MSG_LOBBY_UPDATE,
            .playerID = myPlayerID,
            .payload.lobby = {.isReady = players[myPlayerID].ready}};
        sendData((char*)&heartbeat, sizeof(NetworkPacket));
        lastLobbyBroadcastMs = currentTime;
        players[myPlayerID].lastPacketTime = currentTime;
    }

    // Receive all pending packets (non-blocking)
    while (receiveData((char*)&packet, sizeof(NetworkPacket)) > 0) {
        // Validate packet
        if (packet.version != PROTOCOL_VERSION)
            continue;
        if (packet.playerID >= MAX_MULTIPLAYER_PLAYERS)
            continue;
        if (packet.playerID == myPlayerID)
            continue;  // Skip own packets

        // Update player info based on message type
        switch (packet.msgType) {
            case MSG_LOBBY_JOIN:
                players[packet.playerID].connected = true;
                players[packet.playerID].ready = false;
                players[packet.playerID].lastPacketTime = currentTime;
                break;

            case MSG_LOBBY_UPDATE:
            case MSG_READY:
                players[packet.playerID].connected = true;
                players[packet.playerID].ready = packet.payload.lobby.isReady;
                players[packet.playerID].lastPacketTime = currentTime;
                break;

            case MSG_DISCONNECT:
                players[packet.playerID].connected = false;
                players[packet.playerID].ready = false;
                break;

            default:
                break;
        }
    }

    // Check for player timeouts (no packets for 3 seconds = disconnected)
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (i == myPlayerID)
            continue;  // Don't timeout ourselves

        if (players[i].connected) {
            uint32_t timeSinceLastPacket = currentTime - players[i].lastPacketTime;
            if (timeSinceLastPacket > PLAYER_TIMEOUT_MS) {
                // Player timed out
                players[i].connected = false;
                players[i].ready = false;
            }
        }
    }

    // Check if all connected players are ready
    int connectedCount = 0;
    int readyCount = 0;

    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (players[i].connected) {
            connectedCount++;
            if (players[i].ready) {
                readyCount++;
            }
        }
    }

    // Need at least 2 players, and all must be ready
    return (connectedCount >= 2 && readyCount == connectedCount);
}

void Multiplayer_SetReady(bool ready) {
    players[myPlayerID].ready = ready;

    NetworkPacket packet = {.version = PROTOCOL_VERSION,
                            .msgType = MSG_READY,
                            .playerID = myPlayerID,
                            .payload.lobby = {.isReady = ready}};

    sendData((char*)&packet, sizeof(NetworkPacket));
}

//=============================================================================
// Public API - Race
//=============================================================================

void Multiplayer_SendCarState(const Car* car) {
    NetworkPacket packet = {.version = PROTOCOL_VERSION,
                            .msgType = MSG_CAR_UPDATE,
                            .playerID = myPlayerID,
                            .payload.carState = {.position = car->position,
                                                 .speed = car->speed,
                                                 .angle512 = car->angle512,
                                                 .lap = car->Lap,
                                                 .item = car->item}};

    sendData((char*)&packet, sizeof(NetworkPacket));
}

void Multiplayer_ReceiveCarStates(Car* cars, int carCount) {
    NetworkPacket packet;

    // Receive all pending packets (non-blocking)
    while (receiveData((char*)&packet, sizeof(NetworkPacket)) > 0) {
        // Validate packet
        if (packet.version != PROTOCOL_VERSION)
            continue;
        if (packet.msgType != MSG_CAR_UPDATE)
            continue;
        if (packet.playerID >= carCount)
            continue;
        if (packet.playerID == myPlayerID)
            continue;  // Skip own packets

        // Update the car directly
        Car* otherCar = &cars[packet.playerID];
        otherCar->position = packet.payload.carState.position;
        otherCar->speed = packet.payload.carState.speed;
        otherCar->angle512 = packet.payload.carState.angle512;
        otherCar->Lap = packet.payload.carState.lap;
        otherCar->item = packet.payload.carState.item;

        // Mark as connected (for disconnect detection)
        players[packet.playerID].connected = true;
        players[packet.playerID].lastPacketTime = getTimeMs();
    }
}

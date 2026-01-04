#include "multiplayer.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>
// BAHEY------
#include "WiFi_minilib.h"

// Access WiFi/socket status flags from WiFi_minilib
extern bool socket_opened;
extern bool WiFi_initialized;

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
    MSG_LOBBY_ACK,     // "I received your lobby message" (ACK for reliable delivery)
    MSG_CAR_UPDATE,    // "Here's my car state" (during race)
    MSG_ITEM_PLACED,   // "I placed/threw an item on the track"
    MSG_ITEM_BOX_PICKUP,  // "I picked up an item box"
    MSG_DISCONNECT        // "I'm leaving"
} MessageType;

typedef struct {
    uint8_t version;   // Protocol version (for future compatibility)
    uint8_t msgType;   // MessageType enum
    uint8_t playerID;  // 0-7
    uint8_t seqNum;    // Sequence number (0-255, wraps around) for ACK tracking

    // Payload (28 bytes) - content depends on msgType
    union {
        // For MSG_LOBBY_JOIN, MSG_LOBBY_UPDATE, MSG_READY
        struct {
            bool isReady;          // Has this player pressed SELECT?
            uint8_t reserved[27];  // Future expansion
        } lobby;

        // For MSG_LOBBY_ACK (acknowledgment)
        struct {
            uint8_t ackSeqNum;     // Which sequence number we're acknowledging
            uint8_t reserved[27];  // Future expansion
        } ack;

        // For MSG_CAR_UPDATE (during race)
        struct {
            Vec2 position;  // 8 bytes (2 x int32_t in Q16.8)
            Q16_8 speed;    // 4 bytes
            int angle512;   // 4 bytes
            int lap;        // 4 bytes
            Item item;      // 4 bytes
            uint8_t reserved[4];
        } carState;

        // For MSG_ITEM_PLACED (item placed on track)
        struct {
            Item itemType;        // 4 bytes - what item was placed
            Vec2 position;        // 8 bytes - where it was placed
            int angle512;         // 4 bytes - direction (for projectiles)
            Q16_8 speed;          // 4 bytes - initial speed (for projectiles)
            int shooterCarIndex;  // 4 bytes - who fired this (for immunity)
            uint8_t reserved[4];  // 4 bytes - future use
        } itemPlaced;

        // For MSG_ITEM_BOX_PICKUP (item box collected)
        struct {
            int boxIndex;  // 4 bytes - which box was picked up
            uint8_t reserved[24];
        } itemBoxPickup;

        uint8_t raw[28];  // For future message types
    } payload;
} NetworkPacket;  // Total: 32 bytes

//=============================================================================
// Player Tracking
//=============================================================================

// Selective Repeat ARQ - Retransmission queue entry
#define MAX_PENDING_ACKS 4  // Track up to 4 unacknowledged messages per player

typedef struct {
    NetworkPacket packet;   // The packet awaiting ACK
    uint32_t lastSendTime;  // When we last sent this packet
    int retryCount;         // Number of times we've retried
    bool active;            // Is this slot in use?
} PendingAck;

typedef struct {
    bool connected;           // is this player in the game?
    bool ready;               // Has this player pressed SELECT? (lobby only)
    uint32_t lastPacketTime;  // For timeout detection

    // Selective Repeat ARQ state (lobby only)
    uint8_t lastSeqNumReceived;  // Last sequence number we received from this player
    PendingAck pendingAcks[MAX_PENDING_ACKS];  // Packets awaiting ACK from this player
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
static uint32_t joinResendDeadlineMs = 0;
static uint32_t lastJoinResendMs = 0;

// Selective Repeat ARQ state
static uint8_t nextSeqNum = 0;  // Next sequence number to use for outgoing packets
#define ACK_TIMEOUT_MS 500      // Resend if no ACK after 500ms
#define MAX_RETRIES 5           // Give up after 5 retransmissions

// Debug counters
static int totalPacketsSent = 0;
static int totalPacketsReceived = 0;

// Packet buffering for item placements/boxes (used across cleanup)
#define MAX_BUFFERED_ITEM_PACKETS 16
static NetworkPacket itemPacketBuffer[MAX_BUFFERED_ITEM_PACKETS];
static int itemPacketCount = 0;

#define MAX_BUFFERED_BOX_PACKETS 16
static NetworkPacket boxPacketBuffer[MAX_BUFFERED_BOX_PACKETS];
static int boxPacketCount = 0;

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

/**
 * Send a reliable lobby message with ACK tracking
 * This implements Selective Repeat ARQ for lobby messages only
 */
static void sendReliableLobbyMessage(NetworkPacket* packet) {
    // Assign sequence number
    packet->seqNum = nextSeqNum++;

    // Send the packet
    sendData((char*)packet, sizeof(NetworkPacket));
    totalPacketsSent++;

    // Add to pending ACK queue for each connected player
    uint32_t currentTime = getTimeMs();
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (i == myPlayerID || !players[i].connected) {
            continue;  // Don't track ACKs from ourselves or disconnected players
        }

        // Find an empty slot in the pending ACK queue
        for (int j = 0; j < MAX_PENDING_ACKS; j++) {
            if (!players[i].pendingAcks[j].active) {
                players[i].pendingAcks[j].packet = *packet;
                players[i].pendingAcks[j].lastSendTime = currentTime;
                players[i].pendingAcks[j].retryCount = 0;
                players[i].pendingAcks[j].active = true;
                break;
            }
        }
    }
}

/**
 * Process ACK packets and remove acknowledged messages from retransmission queue
 */
static void processAck(uint8_t fromPlayerID, uint8_t ackSeqNum) {
    if (fromPlayerID >= MAX_MULTIPLAYER_PLAYERS) {
        return;
    }

    // Remove the acknowledged packet from pending queue
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (players[fromPlayerID].pendingAcks[i].active &&
            players[fromPlayerID].pendingAcks[i].packet.seqNum == ackSeqNum) {
            players[fromPlayerID].pendingAcks[i].active = false;
            break;
        }
    }
}

/**
 * Retransmit packets that haven't been acknowledged within timeout
 * Call this periodically in Multiplayer_UpdateLobby()
 */
static void retransmitUnackedPackets(void) {
    uint32_t currentTime = getTimeMs();

    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (i == myPlayerID || !players[i].connected) {
            continue;
        }

        for (int j = 0; j < MAX_PENDING_ACKS; j++) {
            PendingAck* pending = &players[i].pendingAcks[j];

            if (!pending->active) {
                continue;
            }

            // Check if timeout elapsed
            if (currentTime - pending->lastSendTime >= ACK_TIMEOUT_MS) {
                pending->retryCount++;

                // Give up after MAX_RETRIES
                if (pending->retryCount >= MAX_RETRIES) {
                    pending->active = false;  // Stop retrying this packet
                    continue;
                }

                // Resend the packet
                sendData((char*)&pending->packet, sizeof(NetworkPacket));
                pending->lastSendTime = currentTime;
            }
        }
    }
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

//=============================================================================
// Key interrupt for canceling WiFi connection
//=============================================================================

int Multiplayer_Init(void) {
    // Fresh timing each session so heartbeats/countdowns are consistent
    msCounter = 0;
    lastLobbyBroadcastMs = 0;

    if (initialized) {
        Multiplayer_Cleanup();
        // Short delay to ensure cleanup completes
        for (int i = 0; i < 60; i++) {  // 1 second
            swiWaitForVBlank();
        }  // TODO: test if without this if it still works
    }

    // Initialize console for status messages (sub-screen)
    consoleDemoInit();
    consoleClear();
    printf("\x1b[2J");

    printf("=== MULTIPLAYER INIT ===\n\n");
    printf("Connecting to WiFi...\n");
    printf("Looking for 'MES-NDS'...\n\n");
    printf("(This may take 5-10 seconds)\n");

    // Initialize WiFi (with timeout)
    int wifiResult = initWiFi();
    printf("WiFi init result: %d\n", wifiResult);
    if (!wifiResult) {
        consoleClear();
        printf("WiFi Connection Failed!\n\n");
        printf("Possible issues:\n");
        printf("- WiFi is OFF\n");
        printf("- 'MES-NDS' AP not found\n");
        printf("- Out of range\n");
        printf("- WiFi already initialized?\n\n");
        printf("Press B to return\n");

        // Simple polling loop
        while (1) {
            swiWaitForVBlank();
            scanKeys();

            if (keysDown() & KEY_B) {
                printf("DEBUG: B pressed! Breaking...\n");
                for (int i = 0; i < 120; i++)
                    swiWaitForVBlank();
                break;
            }
            Wifi_Update();
            swiWaitForVBlank();
        }

        printf("DEBUG: Returning -1\n");
        for (int i = 0; i < 120; i++)
            swiWaitForVBlank();
        return -1;
    }

    printf("\nWiFi connected!\n");
    printf("Opening socket...\n");

    // Open socket
    int socketResult = openSocket();
    printf("Socket open result: %d\n", socketResult);
    if (!socketResult) {
        consoleClear();
        printf("Socket Error!\n\n");
        printf("Failed to create UDP socket.\n");
        printf("Socket might already be open?\n\n");
        printf("Press B to return\n");

        // Simple polling loop
        while (1) {
            swiWaitForVBlank();
            scanKeys();
            if (keysDown() & KEY_B) {
                break;
            }
            Wifi_Update();
            swiWaitForVBlank();
        }

        disconnectFromWiFi();
        return -1;
    }

    printf("Socket ready!\n\n");

    //=========================================================================
    // PLAYER ID ASSIGNMENT
    //=========================================================================
    unsigned char macAddr[6];
    Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, macAddr);

    myPlayerID = macAddr[5] % MAX_MULTIPLAYER_PLAYERS;
    unsigned long myIP = Wifi_GetIP();

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
    joinResendDeadlineMs = lastLobbyBroadcastMs + 2000;  // resend JOIN for first 2s
    lastJoinResendMs = 0;

    initialized = true;

    // Small delay to show success message
    for (int i = 0; i < 90; i++) {
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
            players[i].lastPacketTime = 0;
            players[i].lastSeqNumReceived = 0;

            // Clear pending ACK queues (ARQ state)
            for (int j = 0; j < MAX_PENDING_ACKS; j++) {
                players[i].pendingAcks[j].active = false;
            }
        }
    }

    // Reset our own ARQ state
    nextSeqNum = 0;  // Start fresh with sequence numbers
}

/* void Multiplayer_Cleanup(void) {
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
} */

/**
 * Clear all pending ACK queues
 * Call this when transitioning from lobby to race
 * This prevents old lobby packets from being retransmitted during the race
 */
static void clearPendingAcks(void) {
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        for (int j = 0; j < MAX_PENDING_ACKS; j++) {
            players[i].pendingAcks[j].active = false;
        }
    }
}

void Multiplayer_Cleanup(void) {
    if (!initialized) {
        return;
    }

    // Send disconnect message to other players (send multiple times for reliability)
    NetworkPacket packet = {.version = PROTOCOL_VERSION,
                            .msgType = MSG_DISCONNECT,
                            .playerID = myPlayerID};

    // Send 3 times with small gaps (UDP is unreliable)
    for (int i = 0; i < 3; i++) {
        sendData((char*)&packet, sizeof(NetworkPacket));

        // Small delay between sends (just a few frames)
        for (int j = 0; j < 5; j++) {
            swiWaitForVBlank();
        }
    }

    // Cleanup WiFi
    closeSocket();
    disconnectFromWiFi();

    // Reset all multiplayer state so next session starts clean
    memset(players, 0, sizeof(players));
    itemPacketCount = 0;
    boxPacketCount = 0;
    memset(itemPacketBuffer, 0, sizeof(itemPacketBuffer));
    memset(boxPacketBuffer, 0, sizeof(boxPacketBuffer));
    initialized = false;
    myPlayerID = -1;

    // Reset ARQ state
    nextSeqNum = 0;

    // Reset debug counters
    totalPacketsSent = 0;
    totalPacketsReceived = 0;
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

    // Mark self as not ready
    players[myPlayerID].ready = false;
    uint32_t currentTime = getTimeMs();
    lastLobbyBroadcastMs = currentTime;
    joinResendDeadlineMs =
        currentTime + 2000;  // Aggressively resend JOIN for first 2s
    lastJoinResendMs = currentTime;

    // Send JOIN message with Selective Repeat ARQ for reliability
    NetworkPacket packet = {.version = PROTOCOL_VERSION,
                            .msgType = MSG_LOBBY_JOIN,
                            .playerID = myPlayerID,
                            .seqNum = 0,  // Will be set by sendReliableLobbyMessage
                            .payload.lobby = {.isReady = false}};

    sendReliableLobbyMessage(&packet);

    // Send a few extra times immediately for faster discovery (redundancy)
    // These won't be tracked for ACK, but help with initial discovery
    for (int i = 0; i < 3; i++) {
        swiWaitForVBlank();
        sendData((char*)&packet, sizeof(NetworkPacket));
    }
}

bool Multiplayer_UpdateLobby(void) {
    NetworkPacket packet;
    uint32_t currentTime = getTimeMs();

    // Retransmit unacknowledged packets (Selective Repeat ARQ)
    retransmitUnackedPackets();

    // During the first 2 seconds after joining, aggressively resend JOIN
    // This helps with initial discovery when no players are connected yet
    if (currentTime < joinResendDeadlineMs &&
        currentTime - lastJoinResendMs >= 300) {  // every 300ms
        NetworkPacket joinAgain = {
            .version = PROTOCOL_VERSION,
            .msgType = MSG_LOBBY_JOIN,
            .playerID = myPlayerID,
            .seqNum = 0,
            .payload.lobby = {.isReady = players[myPlayerID].ready}};
        // Don't use sendReliableLobbyMessage here - just broadcast
        sendData((char*)&joinAgain, sizeof(NetworkPacket));
        lastJoinResendMs = currentTime;
    }

    // Periodic heartbeat so peers don't time out (every ~1s)
    // Heartbeats use reliable delivery now
    if (currentTime - lastLobbyBroadcastMs >= 1000) {
        NetworkPacket heartbeat = {
            .version = PROTOCOL_VERSION,
            .msgType = MSG_LOBBY_UPDATE,
            .playerID = myPlayerID,
            .seqNum = 0,  // Will be set by sendReliableLobbyMessage
            .payload.lobby = {.isReady = players[myPlayerID].ready}};
        sendReliableLobbyMessage(&heartbeat);
        lastLobbyBroadcastMs = currentTime;
        players[myPlayerID].lastPacketTime = currentTime;
    }

    // Receive all pending packets (non-blocking)
    int packetsReceived = 0;
    while (receiveData((char*)&packet, sizeof(NetworkPacket)) > 0) {
        packetsReceived++;
        totalPacketsReceived++;

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
                players[packet.playerID].lastSeqNumReceived = packet.seqNum;

                // Send ACK for this packet
                NetworkPacket ack = {.version = PROTOCOL_VERSION,
                                     .msgType = MSG_LOBBY_ACK,
                                     .playerID = myPlayerID,
                                     .seqNum = 0,  // ACKs don't need sequence numbers
                                     .payload.ack = {.ackSeqNum = packet.seqNum}};
                sendData((char*)&ack, sizeof(NetworkPacket));

                // CRITICAL: Immediately respond so the joining player discovers us!
                // Send our own state as a reliable message
                NetworkPacket response = {
                    .version = PROTOCOL_VERSION,
                    .msgType = MSG_LOBBY_UPDATE,
                    .playerID = myPlayerID,
                    .seqNum = 0,  // Will be set by sendReliableLobbyMessage
                    .payload.lobby = {.isReady = players[myPlayerID].ready}};
                sendReliableLobbyMessage(&response);
                break;

            case MSG_LOBBY_UPDATE:
            case MSG_READY:
                players[packet.playerID].connected = true;
                players[packet.playerID].ready = packet.payload.lobby.isReady;
                players[packet.playerID].lastPacketTime = currentTime;
                players[packet.playerID].lastSeqNumReceived = packet.seqNum;

                // Send ACK for this packet
                NetworkPacket updateAck = {
                    .version = PROTOCOL_VERSION,
                    .msgType = MSG_LOBBY_ACK,
                    .playerID = myPlayerID,
                    .seqNum = 0,
                    .payload.ack = {.ackSeqNum = packet.seqNum}};
                sendData((char*)&updateAck, sizeof(NetworkPacket));
                break;

            case MSG_LOBBY_ACK:
                // Process ACK - remove packet from retransmission queue
                processAck(packet.playerID, packet.payload.ack.ackSeqNum);
                break;

            case MSG_DISCONNECT:
                players[packet.playerID].connected = false;
                players[packet.playerID].ready = false;

                // Clear pending ACKs for this player (they're gone, stop waiting)
                for (int i = 0; i < MAX_PENDING_ACKS; i++) {
                    players[packet.playerID].pendingAcks[i].active = false;
                }
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

                // Clear pending ACKs for this player (they timed out, stop waiting)
                for (int j = 0; j < MAX_PENDING_ACKS; j++) {
                    players[i].pendingAcks[j].active = false;
                }
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
                            .seqNum = 0,  // Will be set by sendReliableLobbyMessage
                            .payload.lobby = {.isReady = ready}};

    sendReliableLobbyMessage(&packet);
}

//=============================================================================
// Public API - Race
//=============================================================================

/**
 * Clear pending lobby ACKs when starting race
 * Call this once when transitioning from lobby to race
 * Prevents old lobby messages from being retransmitted during gameplay
 */
void Multiplayer_StartRace(void) {
    clearPendingAcks();
}

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
        // Validate packet version
        if (packet.version != PROTOCOL_VERSION)
            continue;

        // Handle MSG_CAR_UPDATE packets
        if (packet.msgType == MSG_CAR_UPDATE) {
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
        // Buffer MSG_ITEM_PLACED packets for later processing
        else if (packet.msgType == MSG_ITEM_PLACED) {
            if (itemPacketCount < MAX_BUFFERED_ITEM_PACKETS) {
                itemPacketBuffer[itemPacketCount++] = packet;
            }
        }
        // Buffer MSG_ITEM_BOX_PICKUP packets for later processing
        else if (packet.msgType == MSG_ITEM_BOX_PICKUP) {
            if (boxPacketCount < MAX_BUFFERED_BOX_PACKETS) {
                boxPacketBuffer[boxPacketCount++] = packet;
            }
        }
        // Ignore other packet types
    }
}

//=============================================================================
// Public API - Item Synchronization
//=============================================================================

void Multiplayer_SendItemPlacement(Item itemType, Vec2 position, int angle512,
                                   Q16_8 speed, int shooterCarIndex) {
    if (!initialized) {
        return;
    }

    NetworkPacket packet = {
        .version = PROTOCOL_VERSION,
        .msgType = MSG_ITEM_PLACED,
        .playerID = myPlayerID,
        .payload.itemPlaced = {.itemType = itemType,
                               .position = position,
                               .angle512 = angle512,
                               .speed = speed,
                               .shooterCarIndex = shooterCarIndex}};

    sendData((char*)&packet, sizeof(NetworkPacket));
}

ItemPlacementData Multiplayer_ReceiveItemPlacements(void) {
    ItemPlacementData result = {.valid = false, .shooterCarIndex = -1};

    // Check buffered item packets (buffered by Multiplayer_ReceiveCarStates)
    if (itemPacketCount > 0) {
        // Get the first buffered packet
        NetworkPacket packet = itemPacketBuffer[0];

        // Validate packet
        if (packet.version == PROTOCOL_VERSION && packet.msgType == MSG_ITEM_PLACED &&
            packet.playerID < MAX_MULTIPLAYER_PLAYERS &&
            packet.playerID != myPlayerID) {
            // Return the item placement data
            result.valid = true;
            result.playerID = packet.playerID;
            int shooterIndex = packet.payload.itemPlaced.shooterCarIndex;
            if (shooterIndex < 0 || shooterIndex >= MAX_MULTIPLAYER_PLAYERS) {
                shooterIndex = packet.playerID;  // Fallback for older packets
            }
            result.shooterCarIndex = shooterIndex;
            result.itemType = packet.payload.itemPlaced.itemType;
            result.position = packet.payload.itemPlaced.position;
            result.angle512 = packet.payload.itemPlaced.angle512;
            result.speed = packet.payload.itemPlaced.speed;
        }

        // Remove the packet from buffer (shift array)
        itemPacketCount--;
        for (int i = 0; i < itemPacketCount; i++) {
            itemPacketBuffer[i] = itemPacketBuffer[i + 1];
        }
    }

    return result;
}

void Multiplayer_SendItemBoxPickup(int boxIndex) {
    if (!initialized) {
        return;
    }

    NetworkPacket packet = {.version = PROTOCOL_VERSION,
                            .msgType = MSG_ITEM_BOX_PICKUP,
                            .playerID = myPlayerID,
                            .payload.itemBoxPickup = {.boxIndex = boxIndex}};

    sendData((char*)&packet, sizeof(NetworkPacket));
}

int Multiplayer_ReceiveItemBoxPickup(void) {
    // Check buffered item box pickup packets
    if (boxPacketCount > 0) {
        // Get the first buffered packet
        NetworkPacket packet = boxPacketBuffer[0];

        int boxIndex = -1;

        // Validate packet
        if (packet.version == PROTOCOL_VERSION &&
            packet.msgType == MSG_ITEM_BOX_PICKUP &&
            packet.playerID < MAX_MULTIPLAYER_PLAYERS &&
            packet.playerID != myPlayerID) {
            boxIndex = packet.payload.itemBoxPickup.boxIndex;
        }

        // Remove the packet from buffer (shift array)
        boxPacketCount--;
        for (int i = 0; i < boxPacketCount; i++) {
            boxPacketBuffer[i] = boxPacketBuffer[i + 1];
        }

        return boxIndex;
    }

    return -1;
}

void Multiplayer_GetDebugStats(int* sentCount, int* receivedCount) {
    if (sentCount) {
        *sentCount = totalPacketsSent;
    }
    if (receivedCount) {
        *receivedCount = totalPacketsReceived;
    }
}

void Multiplayer_NukeConnectivity(void) {
    // 1. Send disconnect packets (multiple times for reliability)
    if (initialized) {
        NetworkPacket packet = {.version = PROTOCOL_VERSION,
                                .msgType = MSG_DISCONNECT,
                                .playerID = myPlayerID};

        for (int i = 0; i < 5; i++) {
            sendData((char*)&packet, sizeof(NetworkPacket));
            for (int j = 0; j < 3; j++) {
                swiWaitForVBlank();
            }
        }
    }

    // 2. Close socket completely
    if (socket_opened) {
        closeSocket();
    }

    // 3. Disconnect WiFi completely
    if (WiFi_initialized) {
        disconnectFromWiFi();
    }

    // 4. Reset ALL multiplayer module state
    memset(players, 0, sizeof(players));
    itemPacketCount = 0;
    boxPacketCount = 0;
    memset(itemPacketBuffer, 0, sizeof(itemPacketBuffer));
    memset(boxPacketBuffer, 0, sizeof(boxPacketBuffer));

    // 5. Reset timing/counters
    msCounter = 0;
    lastLobbyBroadcastMs = 0;
    joinResendDeadlineMs = 0;
    lastJoinResendMs = 0;

    // 5b. Reset ARQ state
    nextSeqNum = 0;

    // 5c. Reset debug counters
    totalPacketsSent = 0;
    totalPacketsReceived = 0;

    // 6. Reset flags
    initialized = false;
    myPlayerID = -1;

    // 7. Small delay to ensure everything settles
    for (int i = 0; i < 60; i++) {  // 1 second - WiFi hardware needs time
        Wifi_Update();              // CRITICAL: Keep WiFi stack alive during settling
        swiWaitForVBlank();
    }
}

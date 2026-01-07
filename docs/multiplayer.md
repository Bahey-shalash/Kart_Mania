# Multiplayer System

## Overview

The multiplayer system implements peer-to-peer racing for 2-8 Nintendo DS units over local WiFi. Each DS runs the full game independently, broadcasting car states and synchronizing item events via UDP. The system uses Selective Repeat ARQ for reliable lobby messaging and unreliable high-frequency broadcasts for race updates.

**Implementation files:**
- [multiplayer.h](../source/network/multiplayer.h) - Public API
- [multiplayer.c](../source/network/multiplayer.c) - Implementation

**Authors:** Bahey Shalash, Hugo Svolgaard

**Key Features:**
- Peer-to-peer architecture (no dedicated server)
- MAC address-based player ID (prevents DHCP collisions)
- Reliable lobby synchronization (Selective Repeat ARQ)
- 15Hz car state updates (unreliable broadcast)
- Item synchronization (placement, box pickups)
- 3-second timeout detection
- Automatic disconnect handling

## Architecture

### Network Topology

**Peer-to-Peer Broadcast:**
```
DS #1 (Player 0)  ←────┐
      ↓                 │
   UDP Broadcast        │  All devices receive
 255.255.255.255:8888   │  all broadcasts
      ↓                 │
DS #2 (Player 3)  ←────┤
      ↓                 │
DS #3 (Player 5)  ←────┘
```

**No Central Server:** Each DS is authoritative for its own car. Other players' cars are updated by receiving their broadcast packets.

### Player ID Assignment

**Critical Design Decision:** Use MAC address instead of IP address to avoid player ID collisions.

**Problem Encountered:**
During testing, both DS units were assigned the same player ID (both showing "Player 1") because DHCP servers assign sequential IPs:
- DS #1: 192.168.1.100 → `100 % 8 = 4` → Player 5
- DS #2: 192.168.1.108 → `108 % 8 = 4` → Player 5 (COLLISION!)

**Solution ([multiplayer.c:413-416](../source/network/multiplayer.c#L413-L416)):**
```c
unsigned char macAddr[6];
Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, macAddr);
myPlayerID = macAddr[5] % MAX_MULTIPLAYER_PLAYERS;
```

**Why MAC Address:**
- **Hardware-unique:** Burned into WiFi chip, globally unique (IEEE)
- **Deterministic:** Same DS always gets same player ID
- **High entropy:** Last byte varies significantly across devices
- **Very low collision risk:** IDs still use `last_byte % MAX_MULTIPLAYER_PLAYERS`, so modulo collisions are theoretically possible if two devices share the same last-byte modulo; widening `MAX_MULTIPLAYER_PLAYERS` reduces that chance.

**Example:**
```
DS Unit #1:
  MAC: 00:09:BF:12:34:AB (last byte = 0xAB = 171)
  Player ID: 171 % 8 = 3 → "You are Player 4"

DS Unit #2:
  MAC: 00:09:BF:56:78:CD (last byte = 0xCD = 205)
  Player ID: 205 % 8 = 5 → "You are Player 6"
```

### Network Protocol

**Packet Format (32 bytes total):**

**Defined in:** [multiplayer.c:35-83](../source/network/multiplayer.c#L35-L83)

```c
typedef struct {
    uint8_t version;   // Protocol version (1)
    uint8_t msgType;   // MessageType enum
    uint8_t playerID;  // 0-7
    uint8_t seqNum;    // For ACK tracking (lobby only)

    union {
        // 28 bytes payload - depends on msgType
        struct { bool isReady; } lobby;            // LOBBY_JOIN, LOBBY_UPDATE, READY
        struct { uint8_t ackSeqNum; } ack;         // LOBBY_ACK
        struct { Vec2 position; Q16_8 speed; ... } carState;  // CAR_UPDATE
        struct { Item itemType; Vec2 position; ... } itemPlaced;  // ITEM_PLACED
        struct { int boxIndex; } itemBoxPickup;    // ITEM_BOX_PICKUP
    } payload;
} NetworkPacket;  // Total: 32 bytes
```

**Message Types:**
- `MSG_LOBBY_JOIN` - "I'm joining the lobby"
- `MSG_LOBBY_UPDATE` - "I'm still here" (heartbeat)
- `MSG_READY` - "I pressed SELECT" (ready to race)
- `MSG_LOBBY_ACK` - "I received your message" (acknowledgment)
- `MSG_CAR_UPDATE` - "Here's my car state" (position, speed, angle, lap, item)
- `MSG_ITEM_PLACED` - "I placed/threw an item"
- `MSG_ITEM_BOX_PICKUP` - "I picked up an item box"
- `MSG_DISCONNECT` - "I'm leaving"

**Why 32 bytes:** Efficient for DS WiFi hardware while providing enough space for all message types.

### Reliability Strategy

**Lobby Messages: Selective Repeat ARQ**

**Defined in:** [multiplayer.c:89-127](../source/network/multiplayer.c#L89-L127)

Lobby messages (join, ready, heartbeat) use Selective Repeat ARQ for reliability:

```
Sender                          Receiver
  │                                │
  ├─ MSG_LOBBY_JOIN (seq=1) ─────→│
  │                                ├─ Store seq=1
  │                                │  Send ACK
  │←──────── MSG_LOBBY_ACK ────────┤
  ├─ Remove from pending queue     │
  │                                │
  │  (500ms timeout, no ACK)       │
  ├─ MSG_LOBBY_JOIN (seq=1) ─────→│ (retransmit)
  │←──────── MSG_LOBBY_ACK ────────┤
  │                                │
```

**ARQ Configuration:**
- Timeout: 500ms (resend if no ACK)
- Max retries: 5 attempts
- Pending queue: 4 slots per remote player
- Sequence numbers: 0-255 (wraps around)

**Implementation ([multiplayer.c:158-239](../source/network/multiplayer.c#L158-L239)):**

```c
static void sendReliableLobbyMessage(NetworkPacket* packet) {
    packet->seqNum = nextSeqNum++;
    sendData((char*)packet, sizeof(NetworkPacket));

    // Add to pending ACK queue for each connected player
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (i != myPlayerID && players[i].connected) {
            // Store packet in retransmission queue
            players[i].pendingAcks[slot].packet = *packet;
            players[i].pendingAcks[slot].lastSendTime = currentTime;
            players[i].pendingAcks[slot].active = true;
        }
    }
}

static void retransmitUnackedPackets(void) {
    for each player {
        for each pending packet {
            if (timeout elapsed && retries < MAX_RETRIES) {
                sendData(&packet);  // Retransmit
                retryCount++;
            }
        }
    }
}
```

**Race Messages: Unreliable Broadcast**

Car updates during race are sent at 15Hz (every 4 frames) with **no ACKs, no retransmits**.

**Why unreliable:**
- High frequency (15Hz) compensates for packet loss
- Next update arrives quickly (66ms later)
- Adding ACKs would triple bandwidth (send, ACK, potential retransmit)
- Visual glitches from dropped packets are acceptable

### Timing & Synchronization

**Frame-Based Timing:**

**Defined in:** [multiplayer.c:118-119](../source/network/multiplayer.c#L118-L119)

```c
static uint32_t msCounter = 0;  // Approximate milliseconds

static uint32_t getTimeMs(void) {
    msCounter += 16;  // ~16.67ms per frame at 60Hz
    return msCounter;
}
```

**Not Real-Time Accurate:** Good enough for timeouts and heartbeats. Wraps every ~49 days (acceptable for game sessions).

**Lobby Timing:**
- **First 2 seconds:** Aggressive JOIN broadcasting (300ms interval)
- **Ongoing:** Heartbeat every 1000ms
- **Timeout:** 3000ms without packets = player disconnected

**Race Timing:**
- **Car updates:** Every 4 frames (15Hz at 60 FPS)
- **Item events:** Immediate broadcast when placed/picked up
- **Timeout:** 3000ms without packets = player disconnected

## Lobby Flow

### 1. Initialization

**Function:** `Multiplayer_Init()`
**Defined in:** [multiplayer.c:323-442](../source/network/multiplayer.c#L323-L442)

```c
int playerID = Multiplayer_Init();
if (playerID < 0) {
    // WiFi failed (timeout, AP not found, socket error)
    return;
}
// playerID is 0-7, assigned by MAC address
```

**Process:**
1. Resets timing counters (`msCounter = 0`)
2. Calls `initWiFi()` - connects to "MES-NDS" (5s timeout)
3. Calls `openSocket()` - creates UDP socket on port 8888
4. Retrieves MAC address and assigns player ID
5. Displays IP, MAC, and player number on console
6. Initializes player tracking arrays
7. Returns assigned player ID

**Shows on console:**
```
=== MULTIPLAYER INIT ===

Connecting to WiFi...
Looking for 'MES-NDS'...

WiFi connected!
Opening socket...
Socket ready!

You are Player 4
IP: 192.168.1.100
MAC: 00:09:BF:12:34:AB
```

### 2. Joining Lobby

**Function:** `Multiplayer_JoinLobby()`
**Defined in:** [multiplayer.c:577-605](../source/network/multiplayer.c#L577-L605)

```c
Multiplayer_JoinLobby();
// Broadcasts presence to other players
```

**Process:**
1. **Resets lobby state** - clears stale "ghost players" from previous sessions
2. Marks self as not ready
3. Sends `MSG_LOBBY_JOIN` with Selective Repeat ARQ
4. Sends 3 extra immediate broadcasts (redundancy for fast discovery)

**Critical Fix ([multiplayer.c:579](../source/network/multiplayer.c#L579)):**
```c
resetLobbyState();  // Prevents "ghost players" bug
```

Without this, players from previous sessions would still show as "connected" even though they're not actually in the lobby.

### 3. Lobby Update Loop

**Function:** `Multiplayer_UpdateLobby()`
**Defined in:** [multiplayer.c:607-756](../source/network/multiplayer.c#L607-L756)

Call every frame while in lobby:

```c
while (inLobby) {
    bool allReady = Multiplayer_UpdateLobby();

    // Display lobby status
    int connected = Multiplayer_GetConnectedCount();
    printf("%d/8 players\n", connected);

    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            char* status = Multiplayer_IsPlayerReady(i) ? "READY" : "waiting";
            printf("Player %d: %s\n", i + 1, status);
        }
    }

    if (allReady && connected >= 2) {
        // Start race!
        break;
    }
}
```

**Update Loop Does:**
1. **Retransmits unacked packets** - Selective Repeat ARQ maintenance
2. **Aggressive JOIN resending** - first 2 seconds, every 300ms
3. **Heartbeat** - every 1000ms, broadcasts `MSG_LOBBY_UPDATE`
4. **Receives packets** - processes join, ready, ACK, disconnect messages
5. **Timeout detection** - 3 seconds without packets = disconnected
6. **Checks ready status** - returns true if all connected players are ready

**Packet Processing ([multiplayer.c:658-718](../source/network/multiplayer.c#L658-L718)):**

```c
switch (packet.msgType) {
    case MSG_LOBBY_JOIN:
        players[playerID].connected = true;
        players[playerID].ready = false;
        // Send ACK
        // CRITICAL: Immediately respond with own state!
        sendReliableLobbyMessage(&response);
        break;

    case MSG_LOBBY_UPDATE:
    case MSG_READY:
        players[playerID].connected = true;
        players[playerID].ready = packet.payload.lobby.isReady;
        // Send ACK
        break;

    case MSG_LOBBY_ACK:
        processAck(playerID, ackSeqNum);  // Remove from pending queue
        break;

    case MSG_DISCONNECT:
        players[playerID].connected = false;
        // Clear pending ACKs for this player
        break;
}
```

**Immediate Response to JOIN:** When a new player joins, all existing players immediately broadcast their state so the new player discovers them quickly.

### 4. Marking Ready

**Function:** `Multiplayer_SetReady(bool ready)`
**Defined in:** [multiplayer.c:758-768](../source/network/multiplayer.c#L758-L768)

```c
// When player presses SELECT
if (keysDown() & KEY_SELECT) {
    bool newReady = !myReady;
    Multiplayer_SetReady(newReady);
}
```

Broadcasts `MSG_READY` with Selective Repeat ARQ to all players.

## Race Flow

### 1. Starting Race

**Function:** `Multiplayer_StartRace()`
**Defined in:** [multiplayer.c:779-781](../source/network/multiplayer.c#L779-L781)

```c
Multiplayer_StartRace();
// Clears pending lobby ACK queues
```

**Why Clear ACKs:** Prevents old lobby messages from being retransmitted during gameplay, which would waste bandwidth and cause confusion.

### 2. Sending Car State

**Function:** `Multiplayer_SendCarState(const Car* car)`
**Defined in:** [multiplayer.c:783-794](../source/network/multiplayer.c#L783-L794)

Call every 4 frames (15Hz):

```c
static int networkCounter = 0;

void gameplay_update() {
    // ... physics, input, collisions ...

    // Network update every 4 frames
    networkCounter++;
    if (networkCounter >= 4) {
        networkCounter = 0;
        Multiplayer_SendCarState(&myCar);
        Multiplayer_ReceiveCarStates(allCars, MAX_CARS);
    }
}
```

**Packet Content:**
- Position (Vec2, 8 bytes)
- Speed (Q16_8, 4 bytes)
- Angle (int, 4 bytes)
- Lap (int, 4 bytes)
- Current item (Item, 4 bytes)

**No ACKs, No Retransmits:** Unreliable broadcast for performance.

### 3. Receiving Car States

**Function:** `Multiplayer_ReceiveCarStates(Car* cars, int carCount)`
**Defined in:** [multiplayer.c:796-838](../source/network/multiplayer.c#L796-L838)

Receives all pending car update packets and directly updates the cars array:

```c
void Multiplayer_ReceiveCarStates(Car* cars, int carCount) {
    NetworkPacket packet;

    while (receiveData((char*)&packet, sizeof(NetworkPacket)) > 0) {
        if (packet.msgType == MSG_CAR_UPDATE) {
            if (packet.playerID == myPlayerID) continue;  // Skip own

            Car* otherCar = &cars[packet.playerID];
            otherCar->position = packet.payload.carState.position;
            otherCar->speed = packet.payload.carState.speed;
            otherCar->angle512 = packet.payload.carState.angle512;
            otherCar->Lap = packet.payload.carState.lap;
            otherCar->item = packet.payload.carState.item;

            players[packet.playerID].connected = true;
            players[packet.playerID].lastPacketTime = getTimeMs();
        }
        // Buffer item/box packets for later processing
        else if (packet.msgType == MSG_ITEM_PLACED) { ... }
        else if (packet.msgType == MSG_ITEM_BOX_PICKUP) { ... }
    }
}
```

**Direct Array Update:** No buffering for car states. Each packet immediately updates the corresponding car in the array.

**Packet Buffering:** Item placement and box pickup packets are buffered for processing by separate functions.

## Item Synchronization

### Item Placement

**Function:** `Multiplayer_SendItemPlacement(...)`
**Defined in:** [multiplayer.c:844-861](../source/network/multiplayer.c#L844-L861)

```c
// When throwing a green shell
Vec2 position = car.position;
int angle = car.angle512;
Q16_8 speed = IntToFixed(10);

Multiplayer_SendItemPlacement(ITEM_GREEN_SHELL, position,
                              angle, speed, myPlayerID);

// Locally create the shell (authoritative for own items)
createShell(position, angle, speed, myPlayerID);
```

**Broadcast Content:**
- Item type (ITEM_BANANA, ITEM_GREEN_SHELL, etc.)
- Position where placed (Vec2)
- Angle/direction (for projectiles)
- Initial speed (for projectiles)
- Shooter car index (for immunity - can't hit own projectile)

**Function:** `Multiplayer_ReceiveItemPlacements()`
**Defined in:** [multiplayer.c:863-897](../source/network/multiplayer.c#L863-L897)

```c
// Call every frame during race
ItemPlacementData data = Multiplayer_ReceiveItemPlacements();
if (data.valid) {
    // Create the item on local track
    createItem(data.itemType, data.position, data.angle,
               data.speed, data.shooterCarIndex);
}
```

**Buffered Processing:** Item placement packets are buffered by `ReceiveCarStates()` and retrieved one at a time by this function.

**Returns:** `ItemPlacementData` struct with `valid` flag. If `valid` is false, no item packet available.

### Item Box Pickup

**Function:** `Multiplayer_SendItemBoxPickup(int boxIndex)`
**Defined in:** [multiplayer.c:899-910](../source/network/multiplayer.c#L899-L910)

```c
// When picking up box #5
if (collidedWithBox(5)) {
    Multiplayer_SendItemBoxPickup(5);
    deactivateBox(5);  // Locally deactivate
}
```

**Function:** `Multiplayer_ReceiveItemBoxPickup()`
**Defined in:** [multiplayer.c:912-938](../source/network/multiplayer.c#L912-L938)

```c
// Call every frame during race
int boxIndex = Multiplayer_ReceiveItemBoxPickup();
if (boxIndex >= 0) {
    deactivateBox(boxIndex);  // Other player picked it up
}
```

**Returns:** Box index if available, -1 otherwise.

**Synchronization:** All players deactivate the same box when any player picks it up. Prevents multiple players from getting the same box.

## Cleanup & Disconnection

### Normal Cleanup

**Function:** `Multiplayer_Cleanup()`
**Defined in:** [multiplayer.c:500-539](../source/network/multiplayer.c#L500-L539)

```c
Multiplayer_Cleanup();
// Sends disconnect, closes socket, disconnects WiFi
```

**Process:**
1. Sends `MSG_DISCONNECT` 3 times with delays (UDP unreliability)
2. Calls `closeSocket()` - releases port 8888
3. Calls `disconnectFromWiFi()` - disconnects AP, keeps stack alive
4. Resets all multiplayer state (players, buffers, counters)
5. Clears flags (`initialized = false`, `myPlayerID = -1`)

**Triple Disconnect:** Sending disconnect 3 times compensates for UDP packet loss, ensuring other players detect the disconnect.

### Nuclear Cleanup

**Function:** `Multiplayer_NukeConnectivity()`
**Defined in:** [multiplayer.c:949-1003](../source/network/multiplayer.c#L949-L1003)

```c
Multiplayer_NukeConnectivity();
// Complete reset of all WiFi/multiplayer state
```

**Use Case:** When returning to home page or when things are stuck.

**Process:**
1. Sends `MSG_DISCONNECT` 5 times (extra reliability)
2. Closes socket completely
3. Disconnects WiFi completely
4. Resets ALL module state (players, buffers, counters, flags)
5. **Pumps `Wifi_Update()` for 1 second** during settling

**Critical Detail ([multiplayer.c:999-1002](../source/network/multiplayer.c#L999-L1002)):**
```c
for (int i = 0; i < 60; i++) {  // 1 second
    Wifi_Update();  // CRITICAL: Keep WiFi stack alive
    swiWaitForVBlank();
}
```

Even during "nuclear" cleanup, we keep pumping `Wifi_Update()` to prevent WiFi stack from getting stuck.

## Player Tracking

### PlayerInfo Structure

**Defined in:** [multiplayer.c:99-107](../source/network/multiplayer.c#L99-L107)

```c
typedef struct {
    bool connected;           // Is this player in the game?
    bool ready;               // Has this player pressed SELECT? (lobby only)
    uint32_t lastPacketTime;  // For timeout detection

    // Selective Repeat ARQ state (lobby only)
    uint8_t lastSeqNumReceived;  // Last sequence number from this player
    PendingAck pendingAcks[MAX_PENDING_ACKS];  // Packets awaiting ACK
} PlayerInfo;

static PlayerInfo players[MAX_MULTIPLAYER_PLAYERS];  // Track all 8 players
```

### Query Functions

**Get My Player ID:**
```c
int myID = Multiplayer_GetMyPlayerID();
// Returns 0-7, or -1 if not initialized
```

**Get Connected Count:**
```c
int count = Multiplayer_GetConnectedCount();
printf("%d/8 players\n", count);
```

**Check if Player Connected:**
```c
for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
    if (Multiplayer_IsPlayerConnected(i)) {
        printf("Player %d is online\n", i + 1);
    }
}
```

**Check if Player Ready:**
```c
for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
    if (Multiplayer_IsPlayerConnected(i)) {
        bool ready = Multiplayer_IsPlayerReady(i);
        printf("Player %d: %s\n", i + 1, ready ? "READY" : "waiting");
    }
}
```

### Timeout Detection

**Defined in:** [multiplayer.c:721-739](../source/network/multiplayer.c#L721-L739)

```c
#define PLAYER_TIMEOUT_MS 3000  // 3 seconds

// In Multiplayer_UpdateLobby():
for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
    if (i == myPlayerID) continue;

    if (players[i].connected) {
        uint32_t timeSinceLastPacket = currentTime - players[i].lastPacketTime;
        if (timeSinceLastPacket > PLAYER_TIMEOUT_MS) {
            // Player timed out
            players[i].connected = false;
            players[i].ready = false;

            // Clear pending ACKs (stop waiting for dead player)
            for (int j = 0; j < MAX_PENDING_ACKS; j++) {
                players[i].pendingAcks[j].active = false;
            }
        }
    }
}
```

**Updates Last Packet Time:**
- When receiving lobby packets (join, ready, heartbeat)
- When receiving car updates during race

**Timeout Actions:**
- Mark player as disconnected
- Clear ready status
- Clear pending ACK queue (stop retransmitting to them)

## Debug & Troubleshooting

### Debug Statistics

**Function:** `Multiplayer_GetDebugStats(int* sentCount, int* receivedCount)`
**Defined in:** [multiplayer.c:940-947](../source/network/multiplayer.c#L940-L947)

```c
int sent, received;
Multiplayer_GetDebugStats(&sent, &received);
printf("Sent: %d, Received: %d\n", sent, received);
```

**Tracks:**
- Total packets sent (incremented in `sendReliableLobbyMessage()`)
- Total packets received (incremented in `UpdateLobby()` and `ReceiveCarStates()`)

### Common Issues

**Issue: Players can't see each other in lobby**

**Check:**
1. Are both players calling `Multiplayer_UpdateLobby()` every frame?
2. Is WiFi connected? (check `initWiFi()` return value)
3. Is socket open? (check `openSocket()` return value)
4. Are debug stats showing sent > 0 but received = 0?
   - If yes: WiFi receiving broken, check [wifi.md](wifi.md) troubleshooting
5. Did player join lobby with `Multiplayer_JoinLobby()`?

**Issue: "Ghost players" from previous session**

**Check:**
1. Is `Multiplayer_JoinLobby()` being called? (resets lobby state)
2. Is `resetLobbyState()` being called in `JoinLobby()`? (line 579)

**Issue: Reconnection doesn't work**

**Check:**
1. Is `Wifi_InitDefault()` called only ONCE at startup? (not in `Multiplayer_Init()`)
2. Is `Wifi_Update()` called every frame? (main loop)
3. Is `Wifi_DisableWifi()` being called? (should not be!)
4. See [wifi.md](wifi.md) for complete WiFi troubleshooting

**Issue: Race desyncs (cars teleporting)**

**Check:**
1. Are packets being sent every 4 frames? (15Hz)
2. Check debug stats - is `received` incrementing during race?
3. Network congestion? (too many broadcasts on WiFi)
4. Are other apps using port 8888? (close them)

## Usage Examples

### Complete Session Flow

```c
// === INITIALIZATION (Home Page) ===
int myPlayerID = Multiplayer_Init();
if (myPlayerID < 0) {
    showError("WiFi connection failed");
    return;
}

// === LOBBY ===
Multiplayer_JoinLobby();

bool inLobby = true;
while (inLobby) {
    scanKeys();
    int keys = keysDown();

    // Toggle ready status
    if (keys & KEY_SELECT) {
        myReady = !myReady;
        Multiplayer_SetReady(myReady);
    }

    // Update lobby (receives packets, checks timeouts)
    bool allReady = Multiplayer_UpdateLobby();

    // Display status
    int count = Multiplayer_GetConnectedCount();
    printf("%d/8 players\n", count);
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            bool ready = Multiplayer_IsPlayerReady(i);
            printf("Player %d: %s\n", i + 1, ready ? "READY" : "waiting");
        }
    }

    // Start race if all ready
    if (allReady && count >= 2) {
        inLobby = false;
    }

    // Exit lobby
    if (keys & KEY_B) {
        Multiplayer_Cleanup();
        return;
    }

    swiWaitForVBlank();
}

// === RACE START ===
Multiplayer_StartRace();  // Clear lobby ACK queues
initRace();

// === RACE LOOP ===
int networkCounter = 0;
while (racing) {
    updatePhysics(&myCar);
    handleInput(&myCar);
    checkCollisions(&myCar, allCars, MAX_CARS);

    // Network update every 4 frames
    networkCounter++;
    if (networkCounter >= 4) {
        networkCounter = 0;

        // Send my car state
        Multiplayer_SendCarState(&myCar);

        // Receive other players' car states
        Multiplayer_ReceiveCarStates(allCars, MAX_CARS);

        // Process item events
        ItemPlacementData itemData = Multiplayer_ReceiveItemPlacements();
        if (itemData.valid) {
            createItem(itemData.itemType, itemData.position,
                      itemData.angle, itemData.speed,
                      itemData.shooterCarIndex);
        }

        int boxIndex = Multiplayer_ReceiveItemBoxPickup();
        if (boxIndex >= 0) {
            deactivateBox(boxIndex);
        }
    }

    render(allCars, MAX_CARS);
    swiWaitForVBlank();
}

// === CLEANUP ===
Multiplayer_Cleanup();
```

### Item Usage Example

```c
// When player uses banana
if (myCar.item == ITEM_BANANA && keysDown() & KEY_A) {
    // Place banana behind kart
    Vec2 behindPos = Vec2_Add(myCar.position,
                              Vec2_Scale(Vec2_FromAngle(myCar.angle512 + 256),
                                        IntToFixed(-10)));

    // Broadcast placement
    Multiplayer_SendItemPlacement(ITEM_BANANA, behindPos, 0, 0, myPlayerID);

    // Create locally
    createBanana(behindPos);

    myCar.item = ITEM_NONE;
}

// When player throws green shell
if (myCar.item == ITEM_GREEN_SHELL && keysDown() & KEY_A) {
    Vec2 shellPos = myCar.position;
    int shellAngle = myCar.angle512;
    Q16_8 shellSpeed = IntToFixed(15);

    // Broadcast placement
    Multiplayer_SendItemPlacement(ITEM_GREEN_SHELL, shellPos,
                                  shellAngle, shellSpeed, myPlayerID);

    // Create locally
    createShell(shellPos, shellAngle, shellSpeed, myPlayerID);

    myCar.item = ITEM_NONE;
}
```

### Item Box Collision

```c
// Check item box collisions
for (int i = 0; i < MAX_ITEM_BOXES; i++) {
    if (!itemBoxes[i].active) continue;

    if (checkCollision(&myCar, &itemBoxes[i])) {
        // Broadcast pickup
        Multiplayer_SendItemBoxPickup(i);

        // Deactivate locally
        itemBoxes[i].active = false;

        // Give player random item
        myCar.item = getRandomItem();

        break;
    }
}
```

## Cross-References

- **WiFi Library:** [wifi.md](wifi.md) - Low-level WiFi and socket management
- **Initialization:** [init.md](init.md) - Calls `Wifi_InitDefault()` once at startup
- **Main Loop:** [main.md](main.md) - Pumps `Wifi_Update()` every frame
- **State Machine:** [state_machine.md](state_machine.md) - Manages multiplayer states (lobby, race)
- **Car System:** gameplay/Car.h - Car struct with position, speed, angle, lap, item
- **Reconnection Flow:** [wifi.md#reconnection](wifi.md#reconnection) - How reconnect logic works


---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)

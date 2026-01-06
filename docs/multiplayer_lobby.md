# Multiplayer Lobby System

## Introduction

The multiplayer lobby is the pre-race synchronization screen where 2-8 players on local WiFi discover each other, mark themselves ready, and start a race together. It provides a simple console-based UI for player coordination and extensive debug information for troubleshooting connectivity issues.

---

## Quick Facts

- **Location:** `source/ui/multiplayer_lobby.h` and `source/ui/multiplayer_lobby.c`
- **Network Protocol:** UDP broadcast (see [Multiplayer System](multiplayer.md))
- **Reliability:** Selective Repeat ARQ (lobby only - race uses unreliable)
- **Player Discovery:** Automatic via broadcast messages
- **Countdown:** 3 seconds (180 frames at 60 FPS)
- **Min Players:** 2 (race won't start with 1 player)
- **Max Players:** 8 (matches MAX_CARS and MAX_MULTIPLAYER_PLAYERS)

---

## Table of Contents

1. [UI Flow](#ui-flow)
2. [Network Architecture](#network-architecture)
3. [Reliability Strategy](#reliability-strategy)
4. [Countdown Mechanism](#countdown-mechanism)
5. [Display Layout](#display-layout)
6. [Debug Information](#debug-information)
7. [API Reference](#api-reference)
8. [Integration with Other Systems](#integration-with-other-systems)
9. [Common Issues](#common-issues)

---

## UI Flow

### 1. Entering the Lobby

When a player selects "Multiplayer" from the home page:

```
HOME_PAGE → Multiplayer_Init() → MULTIPLAYER_LOBBY → MultiplayerLobby_Init()
```

**What happens:**
- WiFi connects to "MES-NDS" access point (see [WiFi_minilib](wifi_minilib.md))
- Player ID assigned based on MAC address (see [Multiplayer System](multiplayer.md#player-id-assignment))
- UDP socket opened on port 8888 for broadcast
- `MSG_LOBBY_JOIN` broadcast sent (with Selective Repeat ARQ)
- Lobby state reset (clears any stale connections from previous sessions)

### 2. Player Discovery

**Aggressive JOIN Broadcasting (First 2 Seconds):**
- Local player broadcasts `MSG_LOBBY_JOIN` every 300ms
- Other players receive JOIN and immediately respond with their own state
- ACKs exchanged to confirm reliable delivery

**Heartbeat (Ongoing):**
- After initial 2 seconds, `MSG_LOBBY_UPDATE` sent every 1000ms
- Keeps connection alive, prevents 3-second timeout

See [Multiplayer System - Lobby Discovery](multiplayer.md#lobby-discovery) for protocol details.

### 3. Ready State

**Player presses SELECT:**
- Toggles local ready state (false → true or true → false)
- Sends `MSG_READY` packet (reliable, with ACK)
- All connected players see updated status

**UI shows:**
```
Player 1: [READY]    (YOU)
Player 3: [WAITING]

(1/2 ready)
```

### 4. Countdown

**Triggered when:**
- All connected players marked ready
- At least 2 players connected

**Countdown behavior:**
- Runs for 180 frames (3 seconds at 60 FPS)
- SELECT button disabled (prevents toggling during countdown)
- B button still works (cancel any time)

**Auto-cancellation:**
- Any player unreadies (toggles SELECT again)
- Any player disconnects (3-second timeout)
- Player count drops below 2

### 5. Race Start

**When countdown reaches 0:**
- Calls `Multiplayer_StartRace()` - clears pending ACK queues
- Sets map to ScorchingSands (TODO: add map selection)
- Transitions to GAMEPLAY state

**Network mode changes:**
- Lobby: Reliable messaging with Selective Repeat ARQ
- Race: Unreliable 15Hz car state broadcasts (no ACKs)

---

## Network Architecture

The lobby uses a **peer-to-peer** architecture with **reliable messaging**, unlike the unreliable race updates.

### Lobby Messages (Reliable)

All lobby messages use **Selective Repeat ARQ** for guaranteed delivery:

| Message Type | Sent When | Payload | ACK Required |
|--------------|-----------|---------|--------------|
| `MSG_LOBBY_JOIN` | Enter lobby, first 2s (every 300ms) | isReady flag | Yes |
| `MSG_LOBBY_UPDATE` | Heartbeat (every 1000ms) | isReady flag | Yes |
| `MSG_READY` | Player presses SELECT | isReady flag | Yes |
| `MSG_LOBBY_ACK` | Received any lobby message | ackSeqNum | No (it's the ACK itself) |
| `MSG_DISCONNECT` | Player presses B or cleanup | - | No (sent 3x for reliability) |

See [Multiplayer System - Network Protocol](multiplayer.md#network-protocol) for packet format.

### Race Messages (Unreliable)

Once the race starts, the protocol switches to **unreliable UDP**:

| Message Type | Frequency | ACK Required |
|--------------|-----------|--------------|
| `MSG_CAR_UPDATE` | 15Hz (every 4 frames) | No |
| `MSG_ITEM_PLACED` | On item use | No |
| `MSG_ITEM_BOX_PICKUP` | On box pickup | No |

**Why the switch?**
- **Lobby:** Low frequency, state changes matter (must ensure all players see ready status)
- **Race:** High frequency, loss tolerable (missing 1 car update out of 15/second is fine)

See [Multiplayer System - Race Functions](multiplayer.md#race-functions) for details.

---

## Reliability Strategy

### Selective Repeat ARQ (Lobby Only)

The lobby implements **Selective Repeat ARQ** (Automatic Repeat reQuest) to ensure all players see each other's ready state changes. This is critical because:
- Players need accurate ready counts to know when race can start
- A missed JOIN message means a player appears offline
- A missed READY message means countdown won't start

**How it works:**

1. **Sender assigns sequence number:**
   ```c
   packet.seqNum = nextSeqNum++;  // 0-255, wraps around
   ```

2. **Sender tracks pending ACKs:**
   - Adds packet to `pendingAcks[]` queue for each remote player
   - Records send time and retry count

3. **Receiver sends ACK:**
   ```c
   NetworkPacket ack = {
       .msgType = MSG_LOBBY_ACK,
       .payload.ack.ackSeqNum = packet.seqNum
   };
   sendData(&ack);
   ```

4. **Sender processes ACK:**
   - Removes acknowledged packet from queue
   - Stops retransmitting

5. **Timeout retransmission:**
   - If no ACK after 500ms, resend packet
   - Retry up to 5 times, then give up

**Implementation:** See [multiplayer.c](../source/network/multiplayer.c) lines 228-254 (sendReliableLobbyMessage) and 278-309 (retransmitUnackedPackets).

### Unreliable Race Updates (No ARQ)

During the race, **no ACKs are used**:
- Car states broadcast at 15Hz (high frequency compensates for loss)
- Missing 1-2 packets is visually acceptable (car jumps slightly)
- No retransmission overhead (keeps bandwidth low)

**Why this works:**
- 15 updates/second means 66ms between updates
- 1 lost packet = car position 66ms stale (barely noticeable)
- Next update arrives and corrects position

**Trade-off:**
- Lobby: Low frequency + high reliability = ARQ needed
- Race: High frequency + loss tolerable = ARQ unnecessary (would add overhead)

See [Multiplayer System - Reliability Strategy](multiplayer.md#reliability-strategy) for full analysis.

---

## Countdown Mechanism

### Trigger Conditions

Countdown starts when **all conditions met:**
```c
if (allReady && connectedCount >= 2) {
    countdownActive = true;
    countdownTimer = 180;  // 3 seconds at 60 FPS
}
```

- `allReady`: Every connected player has `isReady = true`
- `connectedCount >= 2`: At least 2 players (can't race alone)

### Countdown Execution

**Frame-based countdown:**
- Timer starts at 180 frames
- Decrements every frame (60 FPS)
- Displays seconds rounded up: `(countdownTimer / 60) + 1`

**Frame-to-seconds mapping:**
| Frames Remaining | Display |
|------------------|---------|
| 180-121 | "Starting in 3..." |
| 120-61 | "Starting in 2..." |
| 60-1 | "Starting in 1..." |
| 0 | Transition to GAMEPLAY |

### Auto-Cancellation

Countdown **immediately cancels** if:
```c
if (countdownActive && (!allReady || connectedCount < 2)) {
    countdownActive = false;
    countdownTimer = 0;
}
```

**Scenarios:**
- Player unreadies (toggles SELECT again)
- Player disconnects (3-second timeout, see [WiFi_minilib](wifi_minilib.md#timeout))
- Player count drops below 2

### Input During Countdown

| Button | Behavior |
|--------|----------|
| SELECT | **Disabled** (prevents toggling during countdown) |
| B | **Enabled** (cancel and return to HOME_PAGE) |

**Why disable SELECT?**
- Prevents accidental unready during countdown
- Forces players to commit once countdown starts
- Use B if you genuinely want to cancel

---

## Display Layout

The lobby uses the **sub-screen console** for all UI:

```
=== MULTIPLAYER LOBBY ===

Player 1: [READY]    (YOU)
Player 3: [WAITING]

(1/2 ready)

--------------------------------
DEBUG: MyID=0 Connected=2
AllReady=0 Countdown=0
Packets: Sent=42 Recv=38
Socket: Calls=150 OK=38 Filt=12
IP: 192.168.1.100
MAC: 00:09:BF:12:34:AB

Press SELECT when ready
Press B to cancel
```

### Top Section: Player List

- Shows all connected players (1-8)
- Ready status: `[READY]` or `[WAITING]`
- Local player marked with `(YOU)`
- Ready count: `(X/Y ready)` where Y = connected, X = ready

### Middle Section: Instructions/Countdown

**Before countdown:**
```
Press SELECT when ready
Press B to cancel
```

**During countdown:**
```
Starting in 3...
```

### Bottom Section: Debug Info

See [Debug Information](#debug-information) below.

---

## Debug Information

The lobby displays extensive debug stats for troubleshooting connectivity issues:

### Player State
```
DEBUG: MyID=0 Connected=2
```
- **MyID:** Local player ID (0-7, assigned from MAC address)
- **Connected:** Number of players in lobby

See [Multiplayer System - Player ID Assignment](multiplayer.md#player-id-assignment) for MAC-based ID logic.

### Lobby State
```
AllReady=0 Countdown=0
```
- **AllReady:** 1 if all players ready, 0 otherwise
- **Countdown:** 1 if countdown active, 0 otherwise

### High-Level Network Stats
```
Packets: Sent=42 Recv=38
```
- **Sent:** Total packets sent by multiplayer.c (JOIN, UPDATE, READY, ACK, etc.)
- **Recv:** Total packets received from other players

Source: `Multiplayer_GetDebugStats()` in [multiplayer.c](../source/network/multiplayer.c) lines 1010-1017.

### Low-Level Socket Stats
```
Socket: Calls=150 OK=38 Filt=12
```
- **Calls:** Total `recvfrom()` calls (includes no-data returns)
- **OK:** Successful receives (data available)
- **Filt:** Packets filtered (our own broadcasts)

Source: `getReceiveDebugStats()` in [WiFi_minilib.c](../source/network/WiFi_minilib.c) lines 385-392.

**Interpreting:**
- `Calls >> OK`: Normal (non-blocking socket returns 0 when no data)
- `Filt > 0`: Normal (we filter our own UDP broadcasts)
- `OK == 0` for long time: **Problem** - no packets being received

See [WiFi_minilib - Packet Filtering](wifi_minilib.md#self-packet-filtering) for details.

### Network Addressing
```
IP: 192.168.1.100
MAC: 00:09:BF:12:34:AB
```
- **IP:** IPv4 address assigned by DHCP
- **MAC:** WiFi hardware address (used for player ID)

**Why show MAC?**
- Debugging player ID collisions
- Verifying correct device
- Confirming WiFi radio is active

---

## API Reference

### MultiplayerLobby_Init

```c
void MultiplayerLobby_Init(void);
```

**Description:** Initializes the multiplayer lobby screen and joins the network lobby.

**Actions:**
1. Clears console on sub-screen
2. Calls `Multiplayer_JoinLobby()` (broadcasts `MSG_LOBBY_JOIN`)
3. Sets default map to ScorchingSands
4. Resets countdown state

**Prerequisites:**
- `Multiplayer_Init()` must have been called successfully
- WiFi connection must be active

**Call frequency:** Once when entering MULTIPLAYER_LOBBY state

**Source:** [multiplayer_lobby.c:74-95](../source/ui/multiplayer_lobby.c#L74-L95)

---

### MultiplayerLobby_Update

```c
GameState MultiplayerLobby_Update(void);
```

**Description:** Updates lobby state and handles user input. Call once per frame at 60Hz.

**Input Handling:**
- **SELECT:** Toggles local player ready state (disabled during countdown)
- **B:** Cancels lobby, cleans up multiplayer, returns to HOME_PAGE

**Behavior:**
1. Receives network packets via `Multiplayer_UpdateLobby()` (see [multiplayer.c](../source/network/multiplayer.c) lines 677-826)
2. Updates player connection/ready status
3. Displays player list with ready indicators
4. Shows debug statistics
5. Starts 3-second countdown when all players ready (minimum 2 players)
6. Cancels countdown if any player unreadies or disconnects
7. Transitions to GAMEPLAY when countdown completes

**Returns:**
- `MULTIPLAYER_LOBBY`: Stay in lobby
- `GAMEPLAY`: Countdown finished, start race
- `HOME_PAGE`: Player pressed B to cancel

**Notes:**
- Countdown runs at 60 FPS (180 frames = 3 seconds)
- Automatically cancels countdown if players < 2 or not all ready
- Debug info displayed at bottom of screen for troubleshooting

**Call frequency:** Every frame (60 FPS)

**Source:** [multiplayer_lobby.c:106-240](../source/ui/multiplayer_lobby.c#L106-L240)

---

## Integration with Other Systems

### WiFi System

The lobby depends on [WiFi_minilib](wifi_minilib.md) for low-level networking:

```c
// In Multiplayer_Init() (called before lobby)
initWiFi();      // Connect to "MES-NDS" AP, get IP via DHCP
openSocket();    // Create UDP socket on port 8888
```

**Lobby usage:**
- `sendData()`: Broadcasts lobby messages
- `receiveData()`: Receives packets from other players
- `getReceiveDebugStats()`: Gets socket statistics for debug display
- `Wifi_GetIP()`: Gets local IP for debug display
- `Wifi_GetData(WIFIGETDATA_MACADDRESS)`: Gets MAC for debug display

See [WiFi_minilib Documentation](wifi_minilib.md) for function details.

### Multiplayer System

The lobby is the primary user of [Multiplayer System](multiplayer.md) lobby functions:

```c
// Initialization
Multiplayer_JoinLobby();     // Broadcast presence, reset lobby state

// Every frame
Multiplayer_UpdateLobby();   // Receive packets, check timeouts, retransmit

// User input
Multiplayer_SetReady(true);  // Toggle ready state

// Transition to race
Multiplayer_StartRace();     // Clear pending ACK queues

// Cancel
Multiplayer_Cleanup();       // Disconnect WiFi, close socket
```

**Critical integration points:**

1. **Lobby State Reset:**
   - `Multiplayer_JoinLobby()` calls `resetLobbyState()` (see [multiplayer.c:517-535](../source/network/multiplayer.c#L517-L535))
   - Prevents "ghost players" from previous sessions

2. **ACK Queue Management:**
   - `Multiplayer_StartRace()` calls `clearPendingAcks()` (see [multiplayer.c:562-568](../source/network/multiplayer.c#L562-L568))
   - Prevents lobby retransmits during race

3. **Timeout Detection:**
   - `Multiplayer_UpdateLobby()` checks 3-second timeout (see [multiplayer.c:792-809](../source/network/multiplayer.c#L792-L809))
   - Disconnects players who haven't sent packets

### Context System

The lobby uses [Context System](context.md) for global state:

```c
// Set multiplayer mode
GameContext_SetMultiplayerMode(true);   // Before entering lobby

// Set map selection
GameContext_SetMap(ScorchingSands);     // In lobby init and race start

// Cancel lobby
GameContext_SetMultiplayerMode(false);  // When B pressed
```

### State Machine

The lobby is a state in the [State Machine](state_machine.md):

```c
// State transitions
HOME_PAGE → MULTIPLAYER_LOBBY   // User selects "Multiplayer"
MULTIPLAYER_LOBBY → GAMEPLAY    // Countdown completes
MULTIPLAYER_LOBBY → HOME_PAGE   // User presses B
```

**Init/Cleanup:**
- `StateMachine_Init(MULTIPLAYER_LOBBY)` calls `MultiplayerLobby_Init()`
- `StateMachine_Cleanup(MULTIPLAYER_LOBBY)` handles multiplayer cleanup (conditional on next state)

---

## Common Issues

### "Ghost Players" (Stale Connections)

**Symptom:** Lobby shows players from previous session who aren't actually connected.

**Cause:** Lobby state not reset when re-entering lobby.

**Fix:** `Multiplayer_JoinLobby()` now calls `resetLobbyState()` which clears all remote player state.

**Code:** [multiplayer.c:517-535](../source/network/multiplayer.c#L517-L535)

---

### Countdown Won't Start

**Symptom:** All players show `[READY]` but countdown doesn't trigger.

**Possible causes:**

1. **Player count < 2:**
   - Check `Connected=` in debug info
   - Need at least 2 players

2. **`allReady` is false:**
   - Check `AllReady=` in debug info
   - One player might not actually be ready (network desync)

3. **Network issue:**
   - Check `Packets: Recv=` - should be increasing
   - Check `Socket: OK=` - should be > 0

**Debug:**
```c
printf("AllReady=%d Countdown=%d\n", allReady ? 1 : 0, countdownActive ? 1 : 0);
printf("Connected=%d Ready=%d\n", connectedCount, readyCount);
```

---

### Countdown Cancels Randomly

**Symptom:** Countdown starts but immediately cancels.

**Cause:** Player timeout or unready event.

**Check:**
- `Connected=` value - did a player disconnect?
- `AllReady=` value - did a player unready?
- 3-second timeout might be too aggressive on poor WiFi

**Code:** Countdown cancellation logic at [multiplayer_lobby.c:199-204](../source/ui/multiplayer_lobby.c#L199-L204)

---

### No Packets Received

**Symptom:** `Packets: Recv=0` and `Socket: OK=0` stay at 0.

**Possible causes:**

1. **Wrong WiFi network:**
   - Check `IP:` - should be on same subnet as other DS units
   - Example: 192.168.1.100 and 192.168.1.101 = good
   - Example: 192.168.1.100 and 10.0.0.5 = bad (different networks)

2. **Broadcast filtering:**
   - Router might be blocking 255.255.255.255 broadcasts
   - See [WiFi_minilib - Broadcast Address](wifi_minilib.md#broadcast-address)

3. **Socket not opened:**
   - Check console output during `Multiplayer_Init()`
   - Should see "Socket ready!"

**Debug:**
```c
Socket: Calls=150 OK=0 Filt=0
```
- `Calls > 0`: Socket is being polled ✓
- `OK = 0`: No data arriving ✗

---

### Player ID Collision

**Symptom:** Two DS units show same player ID.

**Cause:** Extremely rare (MAC addresses should be unique).

**Check:**
```
MAC: 00:09:BF:12:34:AB  (last byte = 0xAB = 171)
MyID = 171 % 8 = 3
```

If two DS units have MAC addresses ending in bytes that are congruent modulo 8, they'll collide.

**Example collision:**
- DS1: `XX:XX:XX:XX:XX:03` → 3 % 8 = 3
- DS2: `XX:XX:XX:XX:XX:0B` → 11 % 8 = 3

**Fix:** Increase `MAX_MULTIPLAYER_PLAYERS` to 16 (reduces collision probability).

See [Multiplayer System - Player ID Assignment](multiplayer.md#player-id-assignment) for full analysis.

---

### Lobby Retransmits During Race

**Symptom:** High packet counts, stuttering during race.

**Cause:** Pending ACK queues not cleared on race start.

**Fix:** `Multiplayer_StartRace()` calls `clearPendingAcks()` to stop retransmitting old lobby messages.

**Code:** [multiplayer.c:562-568](../source/network/multiplayer.c#L562-L568)

**Why this matters:**
- Lobby messages retransmit every 500ms if no ACK
- During race, no one is sending lobby ACKs
- Old lobby packets spam network, waste bandwidth
- Clearing queues stops retransmission

---

## Performance Considerations

### Network Bandwidth

**Lobby phase:**
- JOIN: 32 bytes × 3.33/sec (every 300ms for first 2s) = ~100 bytes/sec
- Heartbeat: 32 bytes × 1/sec = 32 bytes/sec
- ACKs: 32 bytes × (# of packets received from others)
- **Total:** <200 bytes/sec per player (very light)

**Race phase (for comparison):**
- Car updates: 32 bytes × 15/sec = 480 bytes/sec
- Item events: Variable, bursty
- **Total:** ~500-800 bytes/sec per player

See [Multiplayer System - Performance](multiplayer.md#network-bandwidth) for detailed analysis.

### Frame Budget

**Lobby update (per frame):**
- Input handling: <0.01ms
- Network receive: ~0.1ms (non-blocking)
- Display update: ~0.2ms (printf to console)
- Countdown logic: <0.01ms
- **Total:** ~0.3ms per frame

**Race update (for comparison):**
- Car physics: ~1.3ms (8 cars)
- Item updates: ~0.5ms
- Rendering: ~5ms
- **Total:** ~7ms per frame

Lobby is very lightweight compared to gameplay.

---

## Related Documentation

- **[Multiplayer System](multiplayer.md)** - Network protocol, ARQ implementation, race updates
- **[WiFi_minilib](wifi_minilib.md)** - Low-level WiFi and UDP socket library
- **[Context System](context.md)** - Global game state management
- **[State Machine](state_machine.md)** - Game state transitions and lifecycle

---

## Authors

- **Bahey Shalash**
- **Hugo Svolgaard**

**Version:** 1.0
**Last Updated:** 06.01.2026

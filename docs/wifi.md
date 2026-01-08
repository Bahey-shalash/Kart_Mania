# WiFi Library (WiFi_minilib)

## Overview

The WiFi library provides simplified WiFi connection and UDP socket communication for Nintendo DS multiplayer. It wraps the low-level DSWifi library with timeout protection, reconnection support, and broadcast messaging capabilities.

**Implementation files:**
- [WiFi_minilib.h](../source/network/WiFi_minilib.h) - Public API
- [WiFi_minilib.c](../source/network/WiFi_minilib.c) - Implementation

**Original Source:** EPFL ESL Lab educational materials (Moodle)

**Adapted by:** Bahey Shalash, Hugo Svolgaard with critical bug fixes for production use

**Key Features:**
- WiFi connection with timeout watchdogs (5s scan, 10s connect)
- UDP socket creation and broadcast messaging (port 8888)
- Non-blocking I/O (doesn't freeze game loop)
- Self-packet filtering (discards own broadcasts)
- Socket reconnection support (SO_REUSEADDR)
- Debug statistics tracking

## Architecture

### Network Configuration

**Access Point:**
- SSID: "MES-NDS" (hardcoded)
- Security: None (WEPMODE_NONE)
- IP Assignment: DHCP automatic

**Socket Configuration:**
- Protocol: UDP (SOCK_DGRAM)
- Local Port: 8888 (receiving)
- Remote Port: 8888 (sending)
- Broadcast Address: 255.255.255.255 (limited broadcast)
- Mode: Non-blocking I/O

### WiFi Lifecycle

The library implements a careful lifecycle to avoid "works once per boot" bugs on Nintendo DS hardware:

1. **One-Time Initialization** (at game startup in [init.c](../source/core/init.c#L78)):
   ```c
   Wifi_InitDefault(false);  // Called ONCE, never again
   ```

2. **Per-Session Connection** (when entering multiplayer):
   ```c
   initWiFi();      // Enables radio, scans, connects
   openSocket();    // Creates UDP socket
   ```

3. **Continuous Pumping** (multiple locations - see "Where Wifi_Update() is Called" below):
   ```c
   Wifi_Update();   // Services ARM7 WiFi firmware
   ```

4. **Per-Session Disconnect** (when leaving multiplayer):
   ```c
   closeSocket();         // Closes socket
   disconnectFromWiFi();  // Disconnects AP, keeps stack alive
   ```

**Critical Rule:** `Wifi_InitDefault()` is called ONCE at program start. Calling it multiple times breaks the ARM7 WiFi firmware irreparably.

## Major Adaptations from Original

### 1. Timeout Watchdogs (Critical Fix)

**Problem:** Original code used infinite loops waiting for WiFi connection. If WiFi was off or AP unavailable, the game would freeze forever.

**Solution:** Added frame-based timeout counters (defined in
[game_constants.h:269-273](../source/core/game_constants.h#L269-L273)):

```c
#define WIFI_SCAN_TIMEOUT_FRAMES 300    // 5 seconds at 60Hz
#define WIFI_CONNECT_TIMEOUT_FRAMES 600 // 10 seconds at 60Hz
```

**AP Scanning Watchdog** ([WiFi_minilib.c](../source/network/WiFi_minilib.c)):
```c
int scanAttempts = 0;
while (found == 0 && scanAttempts < WIFI_SCAN_TIMEOUT_FRAMES) {
    count = Wifi_GetNumAP();
    // ... check for "MES-NDS" ...

    if (!found) {
        Wifi_Update();       // Update WiFi state
        swiWaitForVBlank();  // Wait 1/60th second
        scanAttempts++;      // Increment watchdog
    }
}

if (!found) {
    return 0;  // Timeout - AP not found
}
```

**Connection Watchdog** ([WiFi_minilib.c](../source/network/WiFi_minilib.c)):
```c
int connectAttempts = 0;
while ((status != ASSOCSTATUS_ASSOCIATED) &&
       (status != ASSOCSTATUS_CANNOTCONNECT) &&
       (connectAttempts < WIFI_CONNECT_TIMEOUT_FRAMES)) {
    status = Wifi_AssocStatus();
    Wifi_Update();
    swiWaitForVBlank();
    connectAttempts++;
}
```

**Impact:** Game returns gracefully to menu after timeout instead of freezing indefinitely.

### 2. WiFi Lifecycle Management (Hardware Bug Fix)

**Problem:** Original called `Wifi_InitDefault()` inside `initWiFi()`, causing "works once" bugs where reconnection would fail on real DS hardware due to ARM7 firmware desynchronization.

**Solution:** Separated initialization into one-time and per-session:

**One-time (in init.c):**
```c
Wifi_InitDefault(false);  // Initialize ARM7↔ARM9 communication ONCE
```

**Per-session (in WiFi_minilib.c:87):**
```c
int initWiFi() {
    Wifi_EnableWifi();  // Only enable radio, don't re-initialize
    // ... scan and connect ...
}
```

**WiFi pumping:** `Wifi_Update()` is called in multiple places throughout the codebase:
- **Main loop** ([main.c:29](../source/core/main.c#L29)): Baseline 60Hz heartbeat
- **WiFi operations** (scanning, connecting, cleanup): Tight loops for faster servicing
- See "Where Wifi_Update() is Called" section below for complete list

**Disconnect (in WiFi_minilib.c:294-317):**
```c
void disconnectFromWiFi() {
    Wifi_DisconnectAP();

    // Pump WiFi stack for 1 second to settle
    for (int i = 0; i < 60; i++) {
        Wifi_Update();  // Keep WiFi alive!
        swiWaitForVBlank();
    }

    // DO NOT call Wifi_DisableWifi() - keeps stack alive
    WiFi_initialized = false;
}
```

**Why This Works:** WiFi firmware stays initialized for entire program lifetime. Only the radio and connection state are toggled, never the underlying stack.

### 3. Socket Reconnection Support

**Problem:** Original didn't support reopening sockets after disconnect. Port 8888 would show "address already in use" error when trying to reconnect.

**Solution:** Added socket options and cleanup ([WiFi_minilib.c:207-235](../source/network/WiFi_minilib.c#L207-L235)):

```c
int openSocket() {
    // Force close if somehow still open
    if (socket_opened) {
        closeSocket();
    }

    // Clear socket address structures (critical for reconnection!)
    memset(&sa_in, 0, sizeof(sa_in));
    memset(&sa_out, 0, sizeof(sa_out));

    socket_id = socket(AF_INET, SOCK_DGRAM, 0);

    // Enable socket address reuse
    int reuse = 1;
    setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR,
               (char*)&reuse, sizeof(reuse));

    // ... bind and configure ...
}
```

**Impact:** Socket can be closed and reopened multiple times within the same program session.

### 4. Broadcast Address Change

**Problem:** Original calculated subnet-specific broadcast (e.g., 192.168.1.255) using bitwise operations on IP and netmask. This had endianness issues between `Wifi_GetIP()` (host order) and `snmask.s_addr` (network order).

**Solution:** Changed to limited broadcast ([WiFi_minilib.c:251-262](../source/network/WiFi_minilib.c#L251-L262)):

```c
// Old (endianness issues):
unsigned long ip = Wifi_GetIP();
unsigned long mask = snmask.s_addr;
unsigned long broadcast_addr = ip | ~mask;  // WRONG

// New (works everywhere):
sa_out.sin_addr.s_addr = htonl(0xFFFFFFFF);  // 255.255.255.255
```

**Why 255.255.255.255:** Limited broadcast works on all IPv4 networks regardless of subnet configuration, avoiding endianness conversion issues.

### 5. Debug Instrumentation

**Problem:** Original had no way to diagnose multiplayer connectivity issues.

**Solution:** Added debug counters and console output:

**Statistics Tracking** ([WiFi_minilib.c:78-81](../source/network/WiFi_minilib.c#L78-L81)):
```c
static int total_recvfrom_calls = 0;
static int total_recvfrom_success = 0;
static int total_filtered_own = 0;
```

**Statistics API** ([WiFi_minilib.c:371-378](../source/network/WiFi_minilib.c#L371-L378)):
```c
void getReceiveDebugStats(int* calls, int* success, int* filtered) {
    if (calls) *calls = total_recvfrom_calls;
    if (success) *success = total_recvfrom_success;
    if (filtered) *filtered = total_filtered_own;
}
```

**Console Messages:**
- Socket creation/binding status
- IP address and broadcast address
- Error diagnostics

### 6. Minor Fixes

**recvfrom() Flags Fix:**

The original library used `~MSG_PEEK` (bitwise NOT of MSG_PEEK) as the flags parameter:

```c
// Original:
recvfrom(socket_id, data_buff, bytes, ~MSG_PEEK, ...);

// Adapted:
recvfrom(socket_id, data_buff, bytes, 0, ...);
```

**Why this change:** `MSG_PEEK` is a flag that previews incoming data without removing it from the receive queue. The bitwise NOT operator (`~`) inverts all bits, creating a value like `0xFFFFFFFD` that sets nearly every possible socket flag simultaneously. This is non-standard and undefined behavior.

Since the socket is already configured for non-blocking I/O via `ioctl(socket_id, FIONBIO, &nonblock)`, the correct flags parameter is `0` (no special flags), which uses standard receive behavior: read data and remove it from the queue.

**shutdown() Removal** ([WiFi_minilib.c:286-290](../source/network/WiFi_minilib.c#L286-L290)):
```c
// Skip shutdown() - can hang on DS hardware
closesocket(socket_id);
```

**Broadcast Permission** ([WiFi_minilib.c:263-266](../source/network/WiFi_minilib.c#L263-L266)):
```c
int broadcast_permission = 1;
setsockopt(socket_id, SOL_SOCKET, SO_BROADCAST,
           (char*)&broadcast_permission, sizeof(broadcast_permission));
```

## Function Reference

### initWiFi

**Signature:** `int initWiFi(void)`
**Defined in:** [WiFi_minilib.c:87-201](../source/network/WiFi_minilib.c#L87-L201)

Initializes WiFi connection to "MES-NDS" access point with timeout protection.

**Process:**
1. Checks if already initialized (returns 0 if so)
2. Enables WiFi radio (`Wifi_EnableWifi()`)
3. Scans for "MES-NDS" AP (5-second timeout)
4. Connects using DHCP (10-second timeout)

**Returns:**
- `1` - Successfully connected
- `0` - Failed (timeout, AP not found, or already initialized)

**Important:** Can only be called once per session. Use `disconnectFromWiFi()` first if reconnecting.

**Example:**
```c
if (!initWiFi()) {
    // Show error: "WiFi unavailable"
    return;
}
// WiFi connected, proceed to openSocket()
```

### openSocket

**Signature:** `int openSocket(void)`
**Defined in:** [WiFi_minilib.c:207-274](../source/network/WiFi_minilib.c#L207-L274)

Creates and configures UDP socket for broadcast communication on port 8888.

**Configuration Applied:**
- `SO_REUSEADDR` - Allows rebinding after close
- `SO_BROADCAST` - Enables broadcast permission
- `FIONBIO` - Sets non-blocking mode
- Binds to `0.0.0.0:8888` (receive from anyone)
- Sends to `255.255.255.255:8888` (broadcast to all)

**Returns:**
- `1` - Socket opened successfully
- `0` - Failed (socket creation failed, bind failed, or already open)

**Important:** Must be called after `initWiFi()`. Use `closeSocket()` first if reopening.

**Example:**
```c
if (!openSocket()) {
    // Show error: "Socket creation failed"
    disconnectFromWiFi();
    return;
}
// Socket ready for sendData()/receiveData()
```

### sendData

**Signature:** `int sendData(char* data_buff, int bytes)`
**Defined in:** [WiFi_minilib.c:319-333](../source/network/WiFi_minilib.c#L319-L333)

Sends data via UDP broadcast to all devices on port 8888.

**Parameters:**
- `data_buff` - Buffer containing data to send
- `bytes` - Number of bytes to send

**Returns:**
- `1` - Send completed (does not guarantee delivery - UDP is unreliable)
- `-1` - Error (socket not opened)

**Behavior:**
- Broadcasts to `255.255.255.255:8888`
- All devices on network receive (including sender)
- Non-blocking (returns immediately)

**Example:**
```c
NetworkPacket packet = { ... };
sendData((char*)&packet, sizeof(NetworkPacket));
```

### receiveData

**Signature:** `int receiveData(char* data_buff, int bytes)`
**Defined in:** [WiFi_minilib.c:336-369](../source/network/WiFi_minilib.c#L336-L369)

Non-blocking receive from UDP socket with self-packet filtering.

**Parameters:**
- `data_buff` - Buffer to store received data
- `bytes` - Maximum bytes to receive

**Returns:**
- `>0` - Number of bytes received from another device
- `0` - No data available OR received own broadcast (filtered out)
- `-1` - Error (socket not opened)

**Behavior:**
- Returns immediately (non-blocking)
- Filters out packets from own IP address
- Updates debug statistics

**Example:**
```c
NetworkPacket packet;
int bytesReceived = receiveData((char*)&packet, sizeof(NetworkPacket));
if (bytesReceived > 0) {
    // Process packet from another player
}
```

### closeSocket

**Signature:** `void closeSocket(void)`
**Defined in:** [WiFi_minilib.c:277-292](../source/network/WiFi_minilib.c#L277-L292)

Closes the UDP socket and releases port 8888.

**Behavior:**
- Closes socket descriptor
- Clears `socket_opened` flag
- Safe to call multiple times (no-op if already closed)

**Important:** Must be called before `openSocket()` if reconnecting.

**Example:**
```c
closeSocket();  // Safe to call even if already closed
```

### disconnectFromWiFi

**Signature:** `void disconnectFromWiFi(void)`
**Defined in:** [WiFi_minilib.c:294-317](../source/network/WiFi_minilib.c#L294-L317)

Disconnects from WiFi access point while keeping WiFi stack alive.

**Behavior:**
- Calls `Wifi_DisconnectAP()` to disconnect
- Pumps `Wifi_Update()` for ~1 second to let stack settle
- **DOES NOT** call `Wifi_DisableWifi()` (keeps stack alive)
- Clears `WiFi_initialized` flag

**Important:** WiFi stack remains alive and ready for reconnection. This is critical for hardware stability.

**Example:**
```c
disconnectFromWiFi();
// WiFi stack still alive, can call initWiFi() again
```

### getReceiveDebugStats

**Signature:** `void getReceiveDebugStats(int* calls, int* success, int* filtered)`
**Defined in:** [WiFi_minilib.c:371-378](../source/network/WiFi_minilib.c#L371-L378)

Retrieves low-level packet reception statistics for debugging.

**Parameters (all optional, pass NULL to skip):**
- `calls` - Out: Total `recvfrom()` calls made
- `success` - Out: Calls that received data
- `filtered` - Out: Packets filtered (own broadcasts)

**Use Case:** Debugging multiplayer connectivity by checking if packets are received but filtered, or not arriving at all.

**Example:**
```c
int calls, success, filtered;
getReceiveDebugStats(&calls, &success, &filtered);
printf("Recv: %d/%d (filtered: %d)\n", success, calls, filtered);
```

## Where Wifi_Update() is Called

`Wifi_Update()` services the ARM7 WiFi firmware and must be called frequently. The codebase calls it in **multiple locations** to ensure responsive WiFi communication:

### 1. Main Loop Heartbeat
**Location:** [main.c:29](../source/core/main.c#L29)
**Frequency:** 60Hz (once per VBlank)
**Purpose:** Provides **baseline heartbeat** when no WiFi operations are active
```c
while (true) {
    Wifi_Update();  // Baseline 60Hz
    // ... state updates ...
    swiWaitForVBlank();
}
```

### 2. Access Point Scanning
**Location:** [WiFi_minilib.c:132](../source/network/WiFi_minilib.c#L132)
**Frequency:** Up to 60Hz during scan (5 second timeout)
**Purpose:** Keep WiFi responsive while waiting for AP list
```c
while (found == 0 && scanAttempts < WIFI_SCAN_TIMEOUT_FRAMES) {
    Wifi_Update();       // Fast pumping during scan
    swiWaitForVBlank();
    scanAttempts++;
}
```

### 3. Connection Establishment
**Location:** [WiFi_minilib.c:186](../source/network/WiFi_minilib.c#L186)
**Frequency:** Up to 60Hz during connect (10 second timeout)
**Purpose:** Service WiFi stack during DHCP negotiation
```c
while (status != ASSOCSTATUS_ASSOCIATED && connectAttempts < WIFI_CONNECT_TIMEOUT_FRAMES) {
    Wifi_Update();       // Fast pumping during connect
    swiWaitForVBlank();
    connectAttempts++;
}
```

### 4. Disconnect Settling
**Location:** [WiFi_minilib.c:310](../source/network/WiFi_minilib.c#L310)
**Frequency:** 60Hz for exactly 1 second (60 frames)
**Purpose:** Let WiFi stack settle after disconnect to prevent corruption
```c
for (int i = 0; i < 60; i++) {
    Wifi_Update();  // Pump during 1-second settle
    swiWaitForVBlank();
}
```

### 5. Multiplayer Cleanup Settling
**Location:** [multiplayer.c:1071](../source/network/multiplayer.c#L1071)
**Frequency:** 60Hz for exactly 1 second (60 frames)
**Purpose:** Keep WiFi alive during nuclear cleanup after multiplayer session
```c
for (int i = 0; i < 60; i++) {
    Wifi_Update();  // CRITICAL: Keep WiFi stack alive
    swiWaitForVBlank();
}
```

### 6. Multiplayer Sync Timeouts
**Locations:** [multiplayer.c:582, 612](../source/network/multiplayer.c#L582)
**Frequency:** 60Hz during player sync (up to 10 second timeout)
**Purpose:** Service WiFi while waiting for other players
```c
while (elapsed < MULTIPLAYER_SYNC_TIMEOUT_FRAMES) {
    Wifi_Update();       // Keep WiFi alive during sync
    // ... check for packets ...
    swiWaitForVBlank();
}
```

### Summary: Multiple Pumping Strategies

The key insight: **`Wifi_Update()` is NOT just called in the main loop** - it's called:
- **Main loop**: Baseline 60Hz heartbeat (minimum frequency)
- **WiFi operations**: Tight loops at 60Hz for responsive communication
- **Cleanup operations**: Settle periods to prevent stack corruption

This multi-location approach ensures WiFi remains responsive during blocking operations while maintaining a baseline heartbeat when idle.

## Nintendo DS WiFi Best Practices

Based on extensive debugging of the "works once per boot" bug, here are the critical rules for DS WiFi:

### ✅ DO:

1. **Initialize `Wifi_InitDefault()` ONCE at program startup** (in main.c)
2. **Call `Wifi_Update()` frequently** - main loop provides 60Hz baseline, operations pump in tight loops
3. **Only enable/disable the radio**, never destroy the WiFi stack
4. **Pump `Wifi_Update()` during cleanup** for at least 1 second
5. **Use limited broadcast (255.255.255.255)** for maximum compatibility
6. **Add SO_REUSEADDR** to sockets for quick reconnection

### ❌ DON'T:

1. **Never call `Wifi_InitDefault()` more than once** per program execution
2. **Never call `Wifi_DisableWifi()`** if you want to reconnect
3. **Never stop calling `Wifi_Update()`** - must be pumped continuously
4. **Never use calculated subnet broadcast** (endianness issues)
5. **Never skip settle delays** after disconnect

### Why These Rules Exist

The Nintendo DS has dual-processor architecture:
- **ARM9**: Main CPU running game code
- **ARM7**: I/O processor handling WiFi firmware

`Wifi_InitDefault()` sets up communication between ARM9 and ARM7. Calling it multiple times desynchronizes this communication, leaving ARM7 WiFi firmware in an undefined state. **The only way to recover is to reboot the entire DS.**

By initializing once and keeping the stack alive via continuous `Wifi_Update()` calls, we maintain stable ARM9↔ARM7 communication throughout the program lifetime.

## Usage Examples

### Basic Connection Flow

```c
// At game startup (main.c):
Wifi_InitDefault(false);  // One-time initialization

// When entering multiplayer:
if (!initWiFi()) {
    printf("WiFi connection failed\n");
    return;
}

if (!openSocket()) {
    printf("Socket creation failed\n");
    disconnectFromWiFi();
    return;
}

printf("Multiplayer ready!\n");

// Main loop (every frame):
Wifi_Update();  // Keep WiFi alive

// When leaving multiplayer:
closeSocket();
disconnectFromWiFi();
```

### Broadcast Communication

```c
// Sending:
typedef struct {
    uint8_t playerID;
    Vec2 position;
} PlayerPacket;

PlayerPacket outgoing = { myID, myPosition };
sendData((char*)&outgoing, sizeof(PlayerPacket));

// Receiving:
PlayerPacket incoming;
int bytes = receiveData((char*)&incoming, sizeof(PlayerPacket));
if (bytes > 0) {
    // Update other player's position
    otherPlayers[incoming.playerID].position = incoming.position;
}
```

### Reconnection

```c
// First connection:
initWiFi();
openSocket();
// ... play multiplayer ...
closeSocket();
disconnectFromWiFi();

// Second connection (same session):
initWiFi();    // Works! WiFi stack still alive
openSocket();  // Works! SO_REUSEADDR allows rebind
// ... play multiplayer again ...
```

## Debugging Connection Issues

### Problem: No packets received after reconnection

**Check:**
1. Is `Wifi_Update()` called every frame? 
2. Is `Wifi_InitDefault()` called only once? (not in initWiFi)
3. Is `Wifi_DisableWifi()` being called? (should be removed)
4. Are there settle delays after disconnect? (60 frames minimum)

### Problem: Socket bind fails

**Check:**
1. Is `SO_REUSEADDR` set before `bind()`?
2. Is `closeSocket()` called before `openSocket()`?
3. Are sockaddr structures cleared with `memset()`?

### Problem: Broadcasts not reaching other devices

**Check:**
1. Is broadcast address `255.255.255.255`? (not subnet-specific)
2. Is `SO_BROADCAST` permission enabled?
3. Are all devices on same WiFi network?
4. Is firewall blocking UDP port 8888?

### Debug Statistics

```c
int calls, success, filtered;
getReceiveDebugStats(&calls, &success, &filtered);

if (calls > 0 && success == 0) {
    printf("Receiving broken - check WiFi stack\n");
} else if (success > 0 && filtered == success) {
    printf("Only receiving own packets - check broadcast address\n");
} else if (success > 0 && filtered < success) {
    printf("Receiving OK - %d remote packets\n", success - filtered);
}
```

## Cross-References

- **Multiplayer System:** [multiplayer.md](multiplayer.md) - Uses WiFi_minilib for network communication
- **Initialization:** [init.md](init.md) - Calls `Wifi_InitDefault()` once at startup
- **Main Loop:** [main.md](main.md) - Provides baseline 60Hz `Wifi_Update()` heartbeat
- **Where Wifi_Update() is Called:** See section above for complete list of pumping locations
- **State Machine:** [state_machine.md](state_machine.md) - Manages multiplayer connection lifecycle
- **Reconnection Flow:** [Reconnection](#reconnection) - Current reconnect behavior and usage


---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)

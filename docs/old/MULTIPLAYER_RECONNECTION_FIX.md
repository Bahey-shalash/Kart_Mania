# Multiplayer Reconnection Fix - Complete Documentation

## Problem Statement

**Symptom**: Multiplayer lobby worked perfectly on the **first connection**, but after disconnecting (pressing B to leave lobby) and attempting to reconnect, players could no longer see each other. This "works once per boot" bug is a classic issue with Nintendo DS WiFi networking.

**Impact**: Players had to restart the entire game to reconnect to multiplayer, making the feature nearly unusable.

---

## Diagnostic Journey

### Initial Investigation

** Report**:
> "When leaving the lobby and re-entering, players are not detected by each other - it only works the first time."

**Observable Symptoms**:
- First connection: Both DS units see each other immediately ✅
- After disconnect + reconnect:
  - `Sent` packets increasing (sending works) ✅
  - `Recv` packets staying at 0 (receiving broken) ❌
  - `Calls` increasing (recvfrom being called) ✅
  - `OK` staying at 0 (recvfrom returns no data) ❌

This indicated that **the socket was not receiving UDP broadcast packets after reconnection**.

---

## Attempted Fixes (What Didn't Work)

### Attempt 1: Selective Repeat ARQ (Reliability Layer)
**Theory**: Packets were being lost due to UDP unreliability.

**Implementation**:
- Added sequence numbers to lobby packets
- Implemented ACK/NACK system
- Added retransmission queue with 500ms timeout
- Broadcasted immediate responses to JOIN messages

**Result**: ❌ Failed - The problem wasn't packet loss, it was that packets weren't being received at all after reconnection.

---

### Attempt 2: Socket Address Structure Cleanup
**Theory**: Stale `sa_in` and `sa_out` structures were causing bind/broadcast issues.

**Implementation**:
```c
// In openSocket()
memset(&sa_in, 0, sizeof(sa_in));
memset(&sa_out, 0, sizeof(sa_out));
```

**Result**: ❌ Failed - While good practice, this didn't solve the reconnection issue.

---

### Attempt 3: Broadcast Address Endianness Fix
**Theory**: Calculated subnet broadcast address had endianness issues between `Wifi_GetIP()` and `snmask.s_addr`.

**Implementation**:
```c
// Changed from calculated subnet broadcast:
unsigned long broadcast_addr = ip | ~mask;  // WRONG - endianness mismatch

// To true broadcast:
sa_out.sin_addr.s_addr = htonl(0xFFFFFFFF);  // 255.255.255.255
```

**Result**: ✅ Improved reliability but didn't fix reconnection.

---

### Attempt 4: Remove Problematic shutdown() Call
**Theory**: `shutdown(socket_id, SHUT_RDWR)` can leave sockets in weird states on DS.

**Implementation**:
```c
void closeSocket() {
    // shutdown(socket_id, SHUT_RDWR);  // REMOVED
    closesocket(socket_id);
    socket_id = -1;
    socket_opened = false;
}
```

**Result**: ✅ Improved cleanup but didn't fix reconnection.

---

### Attempt 5: Socket Reuse Options
**Theory**: Port 8888 was staying bound after close, preventing rebind.

**Implementation**:
```c
int reuse = 1;
setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
```

**Result**: ✅ Good defensive coding but didn't fix reconnection.

---

### Attempt 6: WiFi Settle Delays
**Theory**: WiFi hardware needed time to settle after disconnect.

**Implementation**:
```c
void disconnectFromWiFi() {
    Wifi_DisconnectAP();

    // Let DSWifi settle
    for (int i = 0; i < 30; i++) {
        Wifi_Update();
        swiWaitForVBlank();
    }

    Wifi_DisableWifi();  // Still disabling here - WRONG!
}
```

**Result**: ❌ Failed - Still had the fundamental "works once" problem.

---

## Root Cause Analysis

After extensive debugging, the issue was identified as a **classic Nintendo DS WiFi stack limitation**:

### The "Works Once Per Boot" Bug

**Problem #1: Multiple `Wifi_InitDefault()` Calls**
```c
// WRONG - Called on every multiplayer init
int initWiFi() {
    Wifi_InitDefault(false);  // Re-initializing stack = BAD
    // ... rest of init
}
```

On Nintendo DS, `Wifi_InitDefault()` is **not designed to be called multiple times** in a single program execution. Re-initializing the WiFi stack puts the ARM7 WiFi firmware in a broken state.

**Problem #2: Disabling WiFi Stack**
```c
// WRONG - Destroying the WiFi stack
void disconnectFromWiFi() {
    Wifi_DisconnectAP();
    Wifi_DisableWifi();  // Fully shuts down WiFi firmware - BAD
}
```

Calling `Wifi_DisableWifi()` tears down the entire WiFi stack. Combined with multiple `Wifi_InitDefault()` calls, this created an unrecoverable state.

**Problem #3: Not Pumping WiFi State Machine**
```c
// WRONG - WiFi stack not being updated
while (true) {
    // Wifi_Update();  // MISSING!
    GameState nextState = update_state(ctx->currentGameState);
    // ...
}
```

The DSWifi library requires `Wifi_Update()` to be called **every frame** to service the ARM7 WiFi firmware. Without continuous pumping, the WiFi stack becomes unresponsive, especially during disconnect/reconnect cycles.

---

## The Final Solution

### Fix #1: Initialize WiFi Stack ONCE at Startup ⭐ CRITICAL

**File**: `source/main.c`

```c
int main(void) {
    // ... storage, sound, context init ...

    // Initialize WiFi stack ONCE at program start
    // DO NOT call Wifi_InitDefault() again later!
    Wifi_InitDefault(false);

    init_state(ctx->currentGameState);

    while (true) {
        // ... game loop
    }
}
```

**Why This Works**: The WiFi firmware is initialized once and stays alive for the entire program lifetime. No more re-initialization that breaks the stack.

---

### Fix #2: Remove WiFi Stack Re-initialization ⭐ CRITICAL

**File**: `source/WiFi_minilib.c`

```c
int initWiFi() {
    if (WiFi_initialized == true)
        return 0;

    // ONLY enable the radio - stack already initialized in main()
    // DO NOT call Wifi_InitDefault() here!
    Wifi_EnableWifi();

    // ... scan for AP and connect ...
}
```

**Why This Works**: We only enable/disable the radio, never destroying the underlying WiFi stack.

---

### Fix #3: Keep WiFi Stack Alive During Disconnect ⭐ CRITICAL

**File**: `source/WiFi_minilib.c`

```c
void disconnectFromWiFi() {
    if (WiFi_initialized == false) {
        return;
    }

    // Disconnect from the access point
    Wifi_DisconnectAP();

    // Pump WiFi stack for 1 second to process disconnect
    for (int i = 0; i < 60; i++) {
        Wifi_Update();  // Keep WiFi alive!
        swiWaitForVBlank();
    }

    // DO NOT call Wifi_DisableWifi() here!
    // Keep the WiFi stack alive for reconnection

    WiFi_initialized = false;
}
```

**Why This Works**: The WiFi stack stays alive and can be reconnected to without re-initialization.

---

### Fix #4: Continuous WiFi Pumping ⭐ CRITICAL

**File**: `source/main.c`

```c
while (true) {
    // Update DSWifi state machine EVERY FRAME
    Wifi_Update();  // Keeps ARM7 WiFi firmware alive

    GameState nextState = update_state(ctx->currentGameState);

    // ... state transitions ...

    swiWaitForVBlank();
}
```

**Why This Works**: The ARM7 WiFi firmware requires regular servicing. Calling `Wifi_Update()` every frame ensures the WiFi stack never gets stuck, even during disconnects and reconnects.

---

### Fix #5: Nuclear Cleanup with WiFi Pumping

**File**: `source/multiplayer.c`

```c
void Multiplayer_NukeConnectivity(void) {
    // 1. Send disconnect packets
    if (initialized) {
        // ... send disconnect messages ...
    }

    // 2. Close socket
    if (socket_opened) {
        closeSocket();
    }

    // 3. Disconnect WiFi (keeps stack alive)
    if (WiFi_initialized) {
        disconnectFromWiFi();
    }

    // 4. Reset all state
    // ... memset, counters, flags ...

    // 5. Settle with WiFi pumping
    for (int i = 0; i < 60; i++) {
        Wifi_Update();  // Keep WiFi alive during cleanup!
        swiWaitForVBlank();
    }
}
```

**Why This Works**: Even during "nuclear" cleanup, we keep pumping `Wifi_Update()` so the WiFi stack stays healthy.

---

### Fix #6: Remove Duplicate Cleanup Call

**File**: `source/main.c`

```c
if (nextState != ctx->currentGameState) {
    cleanup_state(ctx->currentGameState, nextState);
    ctx->currentGameState = nextState;

    // REMOVED: Multiplayer_NukeConnectivity();
    // HomePage_initialize() already calls this!

    video_nuke();
    init_state(nextState);
}
```

**Why This Works**: Prevents calling `Multiplayer_NukeConnectivity()` twice in quick succession, which could cause timing issues.

---

## Additional Improvements (Already Applied)

### True Broadcast Address
```c
// Use 255.255.255.255 instead of calculated subnet broadcast
sa_out.sin_addr.s_addr = htonl(0xFFFFFFFF);
```
Avoids endianness issues between `Wifi_GetIP()` and network byte order.

### Socket Reuse
```c
int reuse = 1;
setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
```
Allows immediate rebinding to port 8888 after socket close.

### Remove shutdown() Call
```c
// Skip shutdown() - can be problematic on DS
closesocket(socket_id);
socket_id = -1;
```
Cleaner socket close without potential DS-specific shutdown() issues.

---

## Key Takeaways: Nintendo DS WiFi Best Practices

### ✅ DO:
1. **Initialize `Wifi_InitDefault()` ONCE at program startup**
2. **Call `Wifi_Update()` every single frame** (even on menus, home screen, everywhere)
3. **Only enable/disable the radio**, never destroy the WiFi stack
4. **Pump `Wifi_Update()` during cleanup** for at least 1 second
5. **Use true broadcast (255.255.255.255)** for maximum compatibility
6. **Add SO_REUSEADDR** to sockets for quick reconnection

### ❌ DON'T:
1. **Never call `Wifi_InitDefault()` more than once per program execution**
2. **Never call `Wifi_DisableWifi()` if you want to reconnect**
3. **Never stop calling `Wifi_Update()`** - it must run continuously
4. **Never use calculated subnet broadcast** (endianness issues on DS)
5. **Never skip settle delays** after disconnect

---

## Why This Bug Exists

The Nintendo DS has a **dual-processor architecture**:
- **ARM9**: Main CPU running your game code
- **ARM7**: I/O processor handling WiFi firmware

`Wifi_InitDefault()` sets up communication between ARM9 and ARM7. When called multiple times, the inter-processor communication gets desynchronized, leaving the ARM7 WiFi firmware in an undefined state. The only way to recover is to **reboot the entire DS**.

By initializing once and keeping the WiFi stack alive (via continuous `Wifi_Update()` calls), we maintain stable ARM9↔ARM7 communication throughout the program lifetime.

---

## Testing Results

### Before Fix:
- ✅ First connection: Works perfectly
- ❌ Second connection: 0 packets received, players invisible
- ❌ Third+ connections: Broken, requires DS reboot

### After Fix:
- ✅ First connection: Works perfectly
- ✅ Second connection: Works perfectly
- ✅ Third+ connections: Works perfectly
- ✅ Unlimited reconnections: **All work reliably! 🎉**

---

## Files Modified

| File | Changes |
|------|---------|
| `source/main.c` | Added `Wifi_InitDefault()` at startup, `Wifi_Update()` in main loop, removed duplicate cleanup call |
| `source/WiFi_minilib.c` | Removed `Wifi_InitDefault()` from `initWiFi()`, removed `Wifi_DisableWifi()` from disconnect, added settle delays with `Wifi_Update()` |
| `source/multiplayer.c` | Added `Wifi_Update()` to nuclear cleanup settling loop, increased delay to 1 second |

---

## Conclusion

The "works once per boot" bug was caused by **repeatedly initializing and destroying the Nintendo DS WiFi stack**. The fix is simple but requires understanding DS-specific WiFi architecture:

**Initialize the WiFi stack once, keep it alive, and pump it continuously.**

This approach is **mandatory** for reliable multiplayer on Nintendo DS hardware. Trying to "bring WiFi back from the dead" (re-initialize after disable) is **not possible** on this platform due to ARM7 firmware limitations.

---

## Credits

**Debugging Session**: January 2026
**Platform**: Nintendo DS (ARM9/ARM7)
**WiFi Library**: DSWifi (dswifi9.h)
**Networking**: UDP broadcast peer-to-peer (port 8888)

Special thanks to the DSWifi library maintainers and the Nintendo DS homebrew community for documenting these hardware quirks.

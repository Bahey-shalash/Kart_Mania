/**
 * File: WiFi_minilib.h
 * --------------------
 * Description: Simplified WiFi and UDP socket library for Nintendo DS multiplayer.
 *              Provides high-level interface for WiFi connection, UDP broadcast
 *              communication, and socket management. Designed for local multiplayer
 *              racing games using broadcast messaging.
 *
 * Original Source: EPFL ESL Lab educational materials (Moodle)
 * Adapted by: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 *
 * Key Adaptations from Original:
 *   1. Added timeout watchdogs to prevent infinite loops when WiFi is unavailable
 *   2. Added socket address reuse (SO_REUSEADDR) for reconnection support
 *   3. Changed to 255.255.255.255 broadcast instead of subnet-specific broadcast
 *   4. Added debug statistics tracking for packet reception
 *   5. Fixed WiFi lifecycle management to avoid "works once" bugs on hardware
 *   6. Added extensive inline documentation explaining DSWifi quirks
 *
 * Original Library Design:
 *   - Simple init/open/send/receive/close pattern
 *   - Single SSID connection ("MES-NDS")
 *   - UDP broadcast on port 8888
 *   - Non-blocking socket I/O
 *   - Automatic self-packet filtering
 */

#ifndef WIFI_MINILIB_H
#define WIFI_MINILIB_H

#include <nds.h>
#include <dswifi9.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

/*=============================================================================
 * CONFIGURATION
 *===========================================================================*/

/** Target WiFi access point SSID */
#define SSID "MES-NDS"

/*=============================================================================
 * FUNCTION PROTOTYPES
 *===========================================================================*/

/**
 * Function: initWiFi
 * ------------------
 * Initializes WiFi connection to predefined access point with timeout protection.
 *
 * Process:
 *   1. Enables WiFi radio (assumes Wifi_InitDefault called once at startup)
 *   2. Scans for access point with SSID "MES-NDS" (up to 5 seconds)
 *   3. Connects using DHCP for IP assignment (up to 10 seconds)
 *   4. Returns immediately on timeout instead of freezing
 *
 * Returns:
 *   1 - WiFi initialized and connected successfully
 *   0 - Failed (timeout, AP not found, or already initialized)
 *
 * IMPORTANT: Can only be called once. Returns 0 if already initialized.
 *            Use disconnectFromWiFi() first if reconnection is needed.
 *
 * Adaptation: Added WIFI_SCAN_TIMEOUT_FRAMES and WIFI_CONNECT_TIMEOUT_FRAMES
 *             to prevent infinite loops when WiFi is off or AP is unavailable.
 */
int initWiFi();

/**
 * Function: openSocket
 * --------------------
 * Creates and configures UDP socket for broadcast communication on port 8888.
 *
 * Configuration:
 *   - Protocol: UDP (SOCK_DGRAM)
 *   - Local port: 8888 (receiving)
 *   - Remote port: 8888 (sending)
 *   - Broadcast: 255.255.255.255 (all devices on network)
 *   - Mode: Non-blocking I/O
 *   - Options: SO_REUSEADDR, SO_BROADCAST enabled
 *
 * Returns:
 *   1 - Socket opened and bound successfully
 *   0 - Failed (socket creation failed, bind failed, or already open)
 *
 * IMPORTANT: Can only be called once. Returns 0 if already opened.
 *            Use closeSocket() first if reopening is needed.
 *
 * Adaptation: Added SO_REUSEADDR to allow quick reconnection after disconnect.
 *             Changed from subnet broadcast to 255.255.255.255 for reliability.
 */
int openSocket();

/**
 * Function: receiveData
 * ---------------------
 * Non-blocking receive from UDP socket with self-packet filtering.
 *
 * Parameters:
 *   data_buff - Buffer to store received data
 *   bytes     - Maximum bytes to receive
 *
 * Returns:
 *   >0 - Number of bytes received from another device
 *    0 - No data available OR received our own broadcast (filtered out)
 *   -1 - Error (socket not opened)
 *
 * Behavior:
 *   - Returns immediately (non-blocking mode)
 *   - Automatically filters out packets sent by this DS (compares sender IP)
 *   - Updates debug statistics (total calls, successful receives, filtered packets)
 *
 * Adaptation: Changed from ~MSG_PEEK flag to 0 (standard non-blocking receive).
 *             Added debug statistics tracking.
 */
int receiveData(char* data_buff, int bytes);

/**
 * Function: sendData
 * ------------------
 * Sends data via UDP broadcast to all devices on port 8888.
 *
 * Parameters:
 *   data_buff - Buffer containing data to send
 *   bytes     - Number of bytes to send
 *
 * Returns:
 *   1 - Send completed (does not guarantee delivery - UDP is unreliable)
 *  -1 - Error (socket not opened)
 *
 * Behavior:
 *   - Broadcasts to 255.255.255.255:8888
 *   - All devices on network receive (including sender - filtered by receiveData)
 *   - Returns immediately (non-blocking)
 *
 * Adaptation: Changed broadcast address from calculated subnet broadcast to
 *             255.255.255.255 for better cross-network compatibility.
 */
int sendData(char* data_buff, int bytes);

/**
 * Function: closeSocket
 * ---------------------
 * Closes the UDP socket and releases port 8888.
 *
 * Behavior:
 *   - Closes socket descriptor
 *   - Clears socket_opened flag
 *   - Safe to call multiple times (no-op if already closed)
 *
 * IMPORTANT: Must be called before openSocket() if reconnecting.
 *
 * Adaptation: Removed shutdown() call which can be problematic on DS hardware.
 */
void closeSocket();

/**
 * Function: disconnectFromWiFi
 * ----------------------------
 * Disconnects from WiFi access point while keeping WiFi stack alive.
 *
 * Behavior:
 *   - Calls Wifi_DisconnectAP() to disconnect from network
 *   - Waits ~1 second with Wifi_Update() to let stack settle
 *   - DOES NOT disable WiFi radio (prevents "works once" bugs)
 *   - Clears WiFi_initialized flag
 *   - Safe to call multiple times (no-op if already disconnected)
 *
 * IMPORTANT: WiFi stack remains alive and ready for reconnection.
 *            This is critical for hardware stability on Nintendo DS.
 *            Must be called before initWiFi() if reconnecting.
 *
 * Adaptation: Added Wifi_Update() pump during disconnect to prevent hardware bugs.
 *             Removed Wifi_DisableWifi() call to keep stack alive.
 */
void disconnectFromWiFi();

/**
 * Function: getReceiveDebugStats
 * ------------------------------
 * Retrieves low-level packet reception statistics for debugging.
 *
 * Parameters:
 *   calls    - Out: Total recvfrom() calls made (NULL to skip)
 *   success  - Out: Calls that received data (NULL to skip)
 *   filtered - Out: Packets filtered (our own broadcasts) (NULL to skip)
 *
 * Use Case:
 *   Debugging multiplayer connectivity issues by checking if packets
 *   are being received but filtered, or not arriving at all.
 *
 * Adaptation: New function not present in original library.
 */
void getReceiveDebugStats(int* calls, int* success, int* filtered);

#endif  // WIFI_MINILIB_H
#include "WiFi_minilib.h"
#include <stdio.h>

// Socket port
#define LOCAL_PORT 8888
#define OUT_PORT 8888

//=============================================================================
// TIMEOUT CONFIGURATION
//=============================================================================
// These timeouts prevent the game from freezing if WiFi is off or unavailable.
// Values are in VBlank frames (60 frames = 1 second on Nintendo DS).

// Maximum time to search for the access point before giving up
#define WIFI_SCAN_TIMEOUT_FRAMES 300  // 5 seconds at 60Hz

// Maximum time to wait for connection to complete before giving up
#define WIFI_CONNECT_TIMEOUT_FRAMES 600  // 10 seconds at 60Hz

//=============================================================================
// MODULE STATE
//=============================================================================

// Socket i/o configuration
struct sockaddr_in sa_out, sa_in;
int socket_id;

// Flags indicating whether the WiFi or the socket has been initialized
bool socket_opened = false;
bool WiFi_initialized = false;

// Debug: Track raw recvfrom calls
static int total_recvfrom_calls = 0;
static int total_recvfrom_success = 0;
static int total_filtered_own = 0;

//=============================================================================
// WiFi Initialization with Timeout Watchdogs
//=============================================================================

int initWiFi() {
    // If WiFi already initialized return 0 (error)
    if (WiFi_initialized == true)
        return 0;

    // Ensure the radio is enabled (Wifi_InitDefault called once at startup)
    // DO NOT call Wifi_InitDefault() here - it causes "works once" bugs
    Wifi_EnableWifi();

    // Access point information structure
    Wifi_AccessPoint ap;

    // Indicates if the access point has been found
    int found = 0, count = 0, i = 0;

    // Set scan mode to find APs
    Wifi_ScanMode();

    //=========================================================================
    // TIMEOUT WATCHDOG #1: Access Point Scanning
    //=========================================================================
    // Problem: If WiFi is OFF or the AP doesn't exist, the original code
    //          would loop forever waiting to find "MES-NDS".
    // Solution: Add a frame counter that gives up after WIFI_SCAN_TIMEOUT_FRAMES.
    //
    // How it works:
    // - scanAttempts starts at 0
    // - Each iteration through the loop increments scanAttempts
    // - If we've tried WIFI_SCAN_TIMEOUT_FRAMES times without finding the AP,
    //   we exit the loop and return an error
    // - swiWaitForVBlank() makes each attempt last exactly 1/60th of a second
    //=========================================================================

    int scanAttempts = 0;
    while (found == 0 && scanAttempts < WIFI_SCAN_TIMEOUT_FRAMES) {
        // Get visible APs and check their SSID with our predefined one
        count = Wifi_GetNumAP();
        for (i = 0; (i < count) && (found == 0); i++) {
            Wifi_GetAPData(i, &ap);
            if (strcmp(SSID, ap.ssid) == 0)
                found = 1;  // Our predefined AP has been found
        }

        // If not found yet, wait one frame and increment counter
        if (!found) {
            Wifi_Update();       // Update DSWifi state
            swiWaitForVBlank();  // Wait 1/60th second
            scanAttempts++;      // Increment watchdog counter
        }
    }

    //=========================================================================
    // Check if we timed out during scanning
    //=========================================================================
    // If scanAttempts reached the timeout limit, we never found the AP
    // Return 0 to indicate failure
    if (!found) {
        return 0;  // Timeout - AP not found
    }

    //=========================================================================
    // AP Found - Attempt Connection
    //=========================================================================
    // Use DHCP to get an IP on the network and connect to the AP
    Wifi_SetIP(0, 0, 0, 0, 0);
    Wifi_ConnectAP(&ap, WEPMODE_NONE, 0, 0);

    //=========================================================================
    // TIMEOUT WATCHDOG #2: Connection Establishment
    //=========================================================================
    // Problem: If the AP is found but connection fails (wrong password,
    //          network issues, etc.), the original code would loop forever
    //          waiting for ASSOCSTATUS_ASSOCIATED.
    // Solution: Add another frame counter that gives up after
    //           WIFI_CONNECT_TIMEOUT_FRAMES.
    //
    // How it works:
    // - connectAttempts starts at 0
    // - Each iteration checks the connection status and increments the counter
    // - We exit the loop if:
    //   a) Successfully connected (ASSOCSTATUS_ASSOCIATED)
    //   b) Connection definitely failed (ASSOCSTATUS_CANNOTCONNECT)
    //   c) Timeout reached (connectAttempts >= WIFI_CONNECT_TIMEOUT_FRAMES)
    //=========================================================================

    int status = ASSOCSTATUS_DISCONNECTED;
    int connectAttempts = 0;

    // Keep trying to connect while:
    // - Not yet connected AND
    // - Not failed permanently AND
    // - Haven't timed out
    while ((status != ASSOCSTATUS_ASSOCIATED) &&
           (status != ASSOCSTATUS_CANNOTCONNECT) &&
           (connectAttempts < WIFI_CONNECT_TIMEOUT_FRAMES)) {
        // Check current connection status
        status = Wifi_AssocStatus();

        // Update WiFi state and wait for a VBlank (1/60th second)
        Wifi_Update();
        swiWaitForVBlank();

        // Increment watchdog counter
        connectAttempts++;
    }

    //=========================================================================
    // Check Final Connection Status
    //=========================================================================
    // WiFi_initialized will be true only if we actually connected
    // (not if we timed out or got CANNOTCONNECT)
    WiFi_initialized = (status == ASSOCSTATUS_ASSOCIATED);

    return WiFi_initialized;
}

//=============================================================================
// Socket Management (unchanged from original)
//=============================================================================

int openSocket() {
    // Safety: force close if somehow still open
    if (socket_opened) {
        iprintf("WARNING: socket still open, forcing close...\n");
        closeSocket();
    }

    // Clear socket address structures (critical for reconnection!)
    memset(&sa_in, 0, sizeof(sa_in));
    memset(&sa_out, 0, sizeof(sa_out));

    socket_id = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket

    if (socket_id < 0) {
        // DEBUG: Socket creation failed
        iprintf("ERROR: socket() failed: %d\n", socket_id);
        return 0;  // Failed to create socket
    }

    // DEBUG: Print socket ID (can help diagnose issues)
    iprintf("Socket created: ID=%d\n", socket_id);

    //-----------Configure receiving side---------------------//

    // Enable socket address reuse to prevent "address already in use" errors
    // This is critical for allowing quick reconnection after disconnect
    int reuse = 1;
    setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    sa_in.sin_family = AF_INET;          // Type of address (Inet)
    sa_in.sin_port = htons(LOCAL_PORT);  // set input port
    sa_in.sin_addr.s_addr = 0x00000000;  // Receive data from any address
    // Bind the socket
    if (bind(socket_id, (struct sockaddr*)&sa_in, sizeof(sa_in)) < 0) {
        // Cleanup the socket we just created before returning error
        iprintf("ERROR: bind() failed!\n");
        closesocket(socket_id);
        return 0;  // Error binding the socket
    }

    iprintf("Socket bound to port %d\n", LOCAL_PORT);

    //-----------Configure sending side-----------------------//

    sa_out.sin_family = AF_INET;        // Type of address (Inet)
    sa_out.sin_port = htons(OUT_PORT);  // set output port (same as input)

    // Derive broadcast address from current IP/mask (endian-correct)
    struct in_addr gateway, snmask, dns1, dns2;
    Wifi_GetIPInfo(&gateway, &snmask, &dns1, &dns2);

    unsigned long ip_host = Wifi_GetIP();            // DSWifi returns host-order
    unsigned long mask_host = ntohl(snmask.s_addr);  // Convert mask to host order
    unsigned long broadcast_host = ip_host | ~mask_host;

    // Fallback in case Wifi_GetIP() returns 0 unexpectedly
    if (ip_host == 0) {
        broadcast_host = 0xFFFFFFFF;
    }

    sa_out.sin_addr.s_addr = htonl(broadcast_host);

    // DEBUG: Print network info
    iprintf("IP: %lu.%lu.%lu.%lu\n", (ip_host & 0xFF), ((ip_host >> 8) & 0xFF),
            ((ip_host >> 16) & 0xFF), ((ip_host >> 24) & 0xFF));
    iprintf("Mask: %lu.%lu.%lu.%lu\n", (mask_host & 0xFF), ((mask_host >> 8) & 0xFF),
            ((mask_host >> 16) & 0xFF), ((mask_host >> 24) & 0xFF));
    iprintf("Broadcast: %lu.%lu.%lu.%lu\n", (broadcast_host & 0xFF),
            ((broadcast_host >> 8) & 0xFF), ((broadcast_host >> 16) & 0xFF),
            ((broadcast_host >> 24) & 0xFF));

    // Enable broadcast permission on the socket
    int broadcast_permission = 1;
    setsockopt(socket_id, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast_permission,
               sizeof(broadcast_permission));

    // Set socket to be non-blocking
    char nonblock = 1;
    ioctl(socket_id, FIONBIO, &nonblock);

    // Return successful flag
    socket_opened = 1;
    return socket_opened;
}

void closeSocket() {
    // If socket not opened, nothing to do
    if (socket_opened == false) {
        iprintf("closeSocket: already closed\n");
        return;
    }

    iprintf("Closing socket ID=%d\n", socket_id);

    // Close the socket (skip shutdown() - can be problematic on DS)
    closesocket(socket_id);
    socket_id = -1;
    socket_opened = false;

    iprintf("Socket closed\n");
}

void disconnectFromWiFi() {
    // If Wi-Fi not connected, nothing to do
    if (WiFi_initialized == false) {
        iprintf("WiFi: already disconnected\n");
        return;
    }

    iprintf("Disconnecting WiFi...\n");

    // Disconnect from the access point
    Wifi_DisconnectAP();

    // Let DSWifi settle (~1 second with Wifi_Update)
    // IMPORTANT: Keep WiFi stack alive and pumping - DON'T disable it!
    // Disabling causes "works once" bugs on DS hardware
    for (int i = 0; i < 60; i++) {
        Wifi_Update();
        swiWaitForVBlank();
    }

    WiFi_initialized = false;

    iprintf("WiFi disconnected (stack still alive)\n");
}

int sendData(char* data_buff, int bytes) {
    // If no socket is opened return (error)
    if (socket_opened == false)
        return -1;

    // Send the data
    sendto(socket_id,                  // Socket id
           data_buff,                  // buffer of data
           bytes,                      // Bytes to send
           0,                          // Flags (none)
           (struct sockaddr*)&sa_out,  // Output side of the socket
           sizeof(sa_out));            // Size of the structure

    // Return always true
    return 1;
}

int receiveData(char* data_buff, int bytes) {
    int received_bytes;
    int info_size = sizeof(sa_in);

    // If no socket is opened, return (error)
    if (socket_opened == false)
        return -1;

    // Try to receive the data
    total_recvfrom_calls++;
    received_bytes = recvfrom(socket_id,  // Socket id
                              data_buff,  // Buffer where to put the data
                              bytes,      // Bytes to receive (at most)
                              0,  // Non-blocking handled by ioctl; no flags needed
                              (struct sockaddr*)&sa_in,  // Sender information
                              &info_size);               // Sender info size

    // If nothing received, return
    if (received_bytes <= 0) {
        return received_bytes;
    }

    total_recvfrom_success++;

    // Discard data sent by ourselves
    unsigned long myIP = Wifi_GetIP();
    if (sa_in.sin_addr.s_addr == myIP) {
        total_filtered_own++;
        return 0;  // Filter out our own packets
    }

    // Return the amount of received bytes
    return received_bytes;
}

// Debug function to get low-level receive stats
void getReceiveDebugStats(int* calls, int* success, int* filtered) {
    if (calls)
        *calls = total_recvfrom_calls;
    if (success)
        *success = total_recvfrom_success;
    if (filtered)
        *filtered = total_filtered_own;
}

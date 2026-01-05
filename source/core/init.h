/**
 * File: init.h
 * ------------
 * Description: Application initialization interface. Handles one-time setup of all
 *              subsystems (storage, audio, WiFi, context) at program startup.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#ifndef INIT_H
#define INIT_H

/**
 * Function: InitGame
 * ------------------
 * Performs all one-time initialization for the game. Must be called once at
 * program startup before entering the main game loop.
 *
 * Initialization order:
 *   1. Storage system (File Allocation Table [FAT] filesystem)
 *   2. Game context (defaults + saved settings)
 *   3. Audio system (MaxMod + soundbank)
 *   4. WiFi stack (critical: only initialized once!)
 *   5. Initial game state (HOME_PAGE)
 *
 * IMPORTANT: WiFi stack is initialized here and kept alive for the entire
 * program lifetime. See wifi.md for details on reconnection handling.
 */
void InitGame(void);

#endif  // INIT_H

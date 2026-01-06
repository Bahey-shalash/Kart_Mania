# Kart Mania - Technical Documentation Wiki

Welcome to the comprehensive technical documentation for Kart Mania. This wiki provides in-depth coverage of all game systems, architecture decisions, and implementation details.

## Quick Navigation

- [Core Systems](#core-systems) - Main loop, state machine, initialization
- [Graphics & Rendering](#graphics--rendering) - Display systems and visual rendering
- [Gameplay Systems](#gameplay-systems) - Racing physics, collision, terrain
- [Items & Power-ups](#items--power-ups) - Item system architecture
- [User Interface](#user-interface) - All menu screens and UI patterns
- [Multiplayer & Networking](#multiplayer--networking) - WiFi synchronization
- [Storage & Persistence](#storage--persistence) - Save data management
- [Audio](#audio) - Sound effects and music
- [Utilities](#utilities) - Math libraries and helper systems
- [Development Tools](#development-tools) - Custom development scripts

---

## Core Systems

The foundational systems that drive the game architecture.

### [Main Loop & State Machine](main.md)
The game's central control flow and state management system. Handles initialization, update loops, and transitions between different game states.

**Topics covered:**
- Game state enumeration (HOME_PAGE, GAMEPLAY, MULTIPLAYER_LOBBY, etc.)
- State machine update loop (60 Hz)
- State initialization and cleanup
- VBlank synchronization

### [State Machine](state_machine.md)
Detailed architecture of the state management system that coordinates screen transitions and lifecycle management.

**Topics covered:**
- State transition logic and dispatch
- Per-state initialization/cleanup hooks
- Conditional cleanup (e.g., WiFi preservation across multiplayer flow)
- REINIT_HOME pattern for error recovery

### [Initialization](init.md)
System startup sequence and hardware initialization.

**Topics covered:**
- Storage + context defaults
- Audio system initialization
- One-time WiFi stack initialization
- First-state (HOME_PAGE) setup via state machine

### [Timers & Interrupts](timer.md)
Interrupt-driven timing systems for precise race timing and display updates.

**Topics covered:**
- VBlank interrupt (60 Hz) for rendering
- Chrono interrupt (1000 Hz) for race timing
- Timer configuration and ISR routing
- Per-state VBlank handlers

### [Game Context](context.md)
Global game state management and configuration.

**Topics covered:**
- User settings (WiFi, sound, music)
- Current map selection
- Singleplayer vs multiplayer mode
- Session-persistent state

---

## Graphics & Rendering

Dual-screen rendering systems and visual effects.

### [Graphics Overview](graphics.md)
High-level overview of the graphics architecture and rendering pipeline.

**Topics covered:**
- Main screen vs sub screen roles
- Background layer system (BG0-BG3)
- Sprite rendering (OAM)
- VRAM allocation strategy

### [Gameplay Rendering](gameplay.md)
The core rendering system for the racing screen.

**Topics covered:**
- **Camera system** - Scrolling to follow player
- **Quadrant loading** - Dynamic 256×256 map sections
- **Sprite rendering** - Kart rotation and item display
- **Sub-screen HUD** - Timer, lap counter, item display
- **Countdown display** - "3, 2, 1, 0" sequence
- **Final time display** - Race completion screen

**Key systems:**
- 1024×1024 world divided into 9 quadrants (256×256 each)
- Dynamic VRAM loading based on camera position
- Affine sprite transformations for rotation
- Dual-timer system (lap time + total time)

---

## Gameplay Systems

Racing mechanics, physics, and collision detection.

### [Gameplay Logic](gameplay_logic.md)
The physics and race state management system (separate from rendering).

**Topics covered:**
- Car physics updates
- Input handling
- Collision detection integration
- Lap tracking and finish line detection
- Network synchronization hooks
- Race state management

**Separation of concerns:**
- `gameplay.c` - Rendering only
- `gameplay_logic.c` - Physics and state

### [Car System](car_overview.md)
Player kart physics and control.

**Topics covered:**
- Car state structure
- Movement and steering
- Speed and acceleration
- Item effects on car

**Related:**
- [Car API](car_api.md) - Public interface documentation

### [Collision Detection](wall_collision.md)
Wall and boundary collision system.

**Topics covered:**
- Pixel-perfect wall detection
- Bounce-back mechanics
- Collision response
- Track boundary enforcement

### [Terrain Detection](terrain_detection.md)
Surface type detection and speed modifiers.

**Topics covered:**
- Terrain type enumeration (road, sand, grass)
- Speed multipliers per surface
- Terrain map lookup system
- Integration with car physics

---

## Items & Power-ups

Complete item system architecture.

### [Items Overview](items_overview.md)
High-level introduction to the item system and its role in gameplay.

### [Items Architecture](items_architecture.md)
Detailed system design and module organization.

**Topics covered:**
- Item type enumeration (8 items)
- State management for active items
- Module breakdown (spawning, inventory, effects, update, render)
- Memory pools for item entities

### [Items API](items_api.md)
Public interface for item system integration.

**Functions covered:**
- `Items_Initialize()` - Setup item boxes and sprites
- `Items_Update()` - Physics and collision updates
- `Items_Render()` - Sprite rendering with camera offset
- `Items_CheckCollisions()` - Item box pickup + projectile/hazard collisions
- `Items_Reset()` - Clear all items between races

> Module-specific docs live inline in the code (e.g., `items_spawning.c`, `items_inventory.c`, `item_navigation.c`). No separate markdown pages exist yet for those subsystems.

---

## User Interface

All menu screens and interaction systems.

### [Home Page](home_page.md)
Main menu interface with Singleplayer, Multiplayer, and Settings options.

**Topics covered:**
- Dual-screen layout (animated kart on top, menu on bottom)
- Selection highlighting with palette tricks
- WiFi connectivity check before multiplayer
- Touch and D-pad input handling
- REINIT_HOME pattern for multiplayer failures

### Map Selection
Track selection screen (currently "Scorching Sands" only). Implemented in `source/ui/map_selection.c` (no standalone doc yet).

### [Settings](settings.md)
Configuration menu for WiFi, sound, and music settings.

**Topics covered:**
- Toggle controls for each setting
- Persistent storage integration
- Setting validation and feedback

### [Multiplayer Lobby](multiplayer_lobby.md)
WiFi lobby for waiting for players to connect.

**Topics covered:**
- Player connection display
- Ready status indicators
- Host vs client roles
- Countdown synchronization
- Disconnection handling

### [Play Again](play_again.md)
Post-race menu for replaying or returning to home.

**Topics covered:**
- YES/NO selection
- Race reset logic
- Return to home flow

---

## Multiplayer & Networking

WiFi networking and synchronization systems.

### [Multiplayer System](multiplayer.md)
High-level multiplayer architecture and protocol design.

**Topics covered:**
- Connection establishment
- Player ID assignment
- Race state synchronization
- Item synchronization
- Disconnection recovery

### [WiFi Low-Level](wifi.md)
Nintendo DS WiFi library integration and configuration.

**Topics covered:**
- dswifi setup
- Connection management
- Packet sending/receiving
- Network state queries

---

## Storage & Persistence

Save data systems for settings and records.

### [Storage System](storage.md)
General storage architecture using FAT filesystem.

**Topics covered:**
- libfat integration
- File I/O operations
- SD card access
- Data serialization

### Personal Best Storage
Time trial record keeping system implemented in `source/storage/storage_pb.c` / `.h` (no separate doc yet).

---

## Audio

Sound effects and music playback.

### [Sound System](sound.md)
Audio engine using maxmod library.

**Topics covered:**
- Sound effect playback (SFX)
- Background music (BGM)
- Volume control
- Audio mixing
- Soundbank structure

**Key sounds:**
- Menu navigation clicks
- Item collection
- Item effects (shell launch, bomb explosion)
- Speed boost
- Background race music

---

## Utilities

Helper libraries and mathematical systems.

### [Fixed-Point Math](fixedmath.md)
Custom Q16.8 fixed-point arithmetic library for physics calculations.

**Topics covered:**
- Q16.8 format (1/256 resolution) with binary angles (0-511)
- Arithmetic helpers (add, sub, mul, div, abs)
- Trig via quarter-wave LUT (sin, cos) and binary-search atan2 approximation
- Conversion to/from integers
- Why fixed-point? (No FPU on ARM9)

**Use cases:**
- Car position and velocity
- Item trajectories
- Collision calculations
- Smooth interpolation

---

## Development Tools

Custom Python scripts for asset conversion, debugging, and testing.

### [Development Tools](development_tools.md)
Complete guide to all custom tools built for Kart Mania development.

**Tools covered:**
- **Audio Conversion** (`mp3_to_wav.py`) - MP3 to WAV converter with DS-compatible settings
- **Image Processing** (`swap_rb_grit_fix.py`) - RGB swap fix for macOS grit, transparency correction
- **Pixel Picker** (`pick_pixel_xy.py`) - Interactive coordinate/color picker for map editing
- **Network Simulator** (`simulate_ds_player.py`) - Simulate DS players for multiplayer testing
- **Math Generation** (`gen_sin_lut_q16_8.py`) - Sine lookup table generator for fixed-point math

**Key features:**
- Solves macOS grit RGB swap bug
- DS player simulator for solo multiplayer testing
- Interactive pixel coordinate picker for hitbox placement
- Automated audio format conversion
- Pre-computed sin/cos lookup tables

---

## Additional Resources

### [Useful Links](Useful_Links.md)
External references, libraries, and helpful documentation.


---

## Development Notes

### Architecture Philosophy

Kart Mania follows several key design principles:

1. **Separation of Concerns** - Rendering (`gameplay.c`) is separate from logic (`gameplay_logic.c`)
2. **State Machine Design** - Clean state transitions with init/update/cleanup lifecycle
3. **Modular Systems** - Each subsystem (items, car, network) is self-contained
4. **Hardware-Aware** - Leverages DS-specific features (dual screens, interrupts, DMA)
5. **Performance First** - Fixed-point math, quadrant loading, and interrupt-driven updates

### Memory Constraints

The Nintendo DS has limited resources:
- **4 MB RAM** (main memory)
- **656 KB VRAM** (video memory)
- **No FPU** (floating-point unit)

These constraints drive many architectural decisions:
- Dynamic quadrant loading (VRAM constraint)
- Fixed-point math (no FPU)
- Object pools for items (memory fragmentation)
- Efficient sprite/tile reuse

### Performance Targets

- **60 FPS** - VBlank-synchronized rendering at 60 Hz
- **1000 Hz timing** - Millisecond-precise race timers
- **8-player sync** - Network updates without frame drops
- **Instant response** - Input handling with no perceptible lag

---

## Contributing to Documentation

When documenting new systems, please follow these guidelines:

1. **Use markdown** (.md files) with clear headings
2. **Include code snippets** with syntax highlighting
3. **Explain the "why"** not just the "what"
4. **Link related docs** to help navigation
5. **Document assumptions** and constraints
6. **Add file/line references** for source code

---

## Contact

**Authors:** Bahey Shalash, Hugo Svolgaard

For questions about the codebase or documentation, please refer to the specific module documentation or reach out to the development team.

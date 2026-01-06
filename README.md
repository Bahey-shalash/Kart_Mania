# Kart Mania

Nintendo DS homebrew kart racer with singleplayer time trials and local WiFi multiplayer (2-8 players). Built for real hardware with devkitARM/libnds.

![Homepage](figures/pictures_taken/homepage.png)

## Highlights
- Singleplayer 2-lap time trial on **Scorching Sands** with countdown + stopwatch and personal best tracking.
- Local WiFi multiplayer (up to 8 players) on MES-NDS; lobby ready flow and 5-lap race.
- Full item set: bananas, oil, turbo mushroom (control invert), speed boost, green/red shells, bombs, missiles.
- Animated dual-screen menus (home, settings, map select, play again) with touch or D-pad navigation.
- Persistent settings/best times saved to SD; WiFi preference stored but stack always stays alive.

## Documentation
- Main wiki: [docs/wiki.md](docs/wiki.md)
- Key pages: [state machine](docs/state_machine.md), [context](docs/context.md), [timer system](docs/timer.md), [gameplay logic](docs/gameplay_logic.md), [dev tools](docs/development_tools.md).

## Modes and Screens

### Settings
- WiFi: must be ON to enter Multiplayer.
- Sound FX: toggle on/off.
- Music: toggle on/off.
- Save icon persists settings to `/kart-mania/settings.txt` so they load on boot. Home/Return leaves the screen without overwriting saved defaults.

<p align="center">
  <img src="figures/pictures_taken/settings.png" alt="Settings screen" width="55%">
</p>

### Singleplayer

<p align="center">
  <img src="figures/pictures_taken/map_selection.png" alt="Singleplayer map selection" width="47%">
  <img src="figures/pictures_taken/singleplayer_with_banana.png" alt="Singleplayer gameplay" width="47%">
</p>

- Map select shows three options; only **Scorching Sands** is implemented. Selecting the others returns to the home page.
- 2-lap solo time trial. AI bots are still in development.
- Countdown (3, 2, 1, 0) appears on the bottom screen, then a stopwatch tracks your lap.
- Finish screen compares your time to your personal best; new records save to `/kart-mania/best_times.txt`.
- Item boxes grant one item at a time; press **L** to use it.
- Item set:
  - **Banana** - drop behind you to spin/slow
  - **Oil Slick** - slippery patch that slows on contact
  - **Turbo Mushroom** - inverts D-pad controls for a few seconds
  - **Speed Boost** - doubles top speed briefly

<p align="center">
  <img src="figures/pictures_taken/END_OF_SINGLE_PLAYER_BEST_TIME.png" alt="Best time comparison" width="47%">
  <img src="figures/pictures_taken/PLAY_again_menu.png" alt="Play again prompt" width="47%">
</p>

<p align="center">
  <img src="figures/pictures_taken/PLAY_AGAIN_selecting_NO.png" alt="Play again - No selected" width="47%">
  <img src="figures/pictures_taken/single_player.png" alt="Singleplayer HUD" width="47%">
</p>

**Item gallery**

<p align="center">
  <img src="figures/entities/item_box.png" alt="Item box" width="64">
  <img src="figures/entities/banana.png" alt="Banana" width="64">
  <img src="figures/entities/oil_slick.png" alt="Oil slick" width="64">
  <img src="figures/entities/turbo_mushroom.png" alt="Turbo mushroom" width="64">
  <img src="figures/entities/speed_boost.png" alt="Speed boost" width="64">
  <img src="figures/entities/green_shell.png" alt="Green shell" width="64">
  <img src="figures/entities/red_shell.png" alt="Red shell" width="64">
  <img src="figures/entities/bomb.png" alt="Bomb" width="64">
  <img src="figures/entities/missile.png" alt="Missile" width="64">
</p>

### Multiplayer (WiFi)

<p align="center">
  <img src="figures/pictures_taken/MP_lobby.png" alt="Multiplayer lobby" width="32%">
  <img src="figures/pictures_taken/multiplayer_5_players.png" alt="Lobby with five players" width="32%">
  <img src="figures/pictures_taken/multiplayer_turn_one_item_collected.png" alt="Multiplayer race" width="32%">
</p>

- Requires WiFi enabled and connection to `MES-NDS`. If connection fails, the bottom screen shows the error and you can exit.
- Supports up to 8 players; the lobby lists who joined.
- Press **SELECT** to mark ready. When everyone is ready, the race starts automatically.
- Map: Scorching Sands.
- Laps: 5.
- Item set: full arsenal (bananas, oil, mushrooms, speed boosts, green/red shells, bombs, missiles).

## Controls
- Menus: D-pad to highlight, **A** to select, or tap on the touchscreen.
- Racing: **A** accelerate, **B** brake, D-pad steer (mushroom can invert), **L** use item, **SELECT** return home when playing, **START** pauses locally (in multiplayer others keep moving).

## Saving and Persistence
- Settings are stored on SD under `/kart-mania/settings.txt` with defaults in `/kart-mania/default_settings.txt`.
- Start+Select while tapping Save resets your settings back to the defaults file.
- Personal best times are stored in `/kart-mania/best_times.txt` after singleplayer runs.
- The `/kart-mania` folder is created automatically when storage initializes.

## Build and Run
Prerequisites:
- devkitPro with devkitARM toolchain
- libnds, libfat, maxmod, dswifi
- grit (for asset conversion)

Commands:
```bash
make                 # Release build (-O2)
make BUILD_MODE=debug
make clean
```

Output: `kart-mania.nds` (artifacts in `build/`). To play, copy the NDS to your flashcart. WiFi features require a DS-compatible access point. On first boot the game writes its `/kart-mania` folder for settings and best times.

Running notes:
- Hardware: Tested on real DS/DS Lite. Join `MES-NDS` for multiplayer.
- Emulation: Emulators vary in WiFi support; singleplayer generally works, multiplayer may not.

## Project Structure
```
kart-mania/
├── source/          # Core game code (state machine, UI, gameplay, networking)
│   ├── gameplay/    # Racing logic, items, physics
│   ├── ui/          # Menus and screens
│   ├── graphics/    # Rendering helpers
│   ├── audio/       # Sound effects and music playback
│   ├── network/     # WiFi multiplayer
│   └── storage/     # Settings and personal best persistence
├── data/            # Graphics assets (PNG + grit)
├── audio/           # Music/SFX sources (soundbank)
├── docs/            # Additional documentation
└── figures/         # Screenshots
```

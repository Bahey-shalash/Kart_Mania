# State Machine System

## Overview

The state machine owns all screen/game-mode transitions. It dispatches per-frame updates, initializes resources when entering a state, and cleans them up on exit. The functions live in [state_machine.c](../source/core/state_machine.c) with the public interface in [state_machine.h](../source/core/state_machine.h). Called every frame from the main loop (see [main.md](main.md)).

- Update dispatch: [state_machine.c:30-54](../source/core/state_machine.c#L30-L54)
- Initialization hooks: [state_machine.c:60-86](../source/core/state_machine.c#L60-L86)
- Cleanup rules: [state_machine.c:93-133](../source/core/state_machine.c#L93-L133)

## Game States

`GameState` values are defined in [game_types.h:5-26](../source/core/game_types.h#L5-L26). Brief behavior:
- **HOME_PAGE / REINIT_HOME** — Main menu; handles singleplayer/multiplayer selection and kicks off WiFi lobby initialization. VBlank updates animate the kart (`HomePage_OnVBlank()`).
- **MAPSELECTION** — Lets the player choose a track; `Map_selection_update()` can jump straight to `GAMEPLAY` (Scorching Sands) or back home.
- **MULTIPLAYER_LOBBY** — Text-console lobby where players ready up; can cancel to `HOME_PAGE` or advance to `GAMEPLAY` when everyone is ready.
- **GAMEPLAY** — Active race. Graphics/timers are set up by `Graphical_Gameplay_initialize()` and torn down by `Gameplay_Cleanup()` / `Race_Stop()`. Race tick timers come from the timer system (see [timer.md](timer.md)).
- **PLAYAGAIN** — Post-race yes/no prompt to replay or return home. Keeps multiplayer alive intentionally.
- **SETTINGS** — Toggle WiFi/music/SFX preferences and save to storage.

## Update Dispatch

`StateMachine_Update(state)` routes to the current state’s `*_update()` function and returns the next `GameState`. `REINIT_HOME` is treated identically to `HOME_PAGE`, allowing the home screen to be reloaded after a failed multiplayer init without special-case logic.

## Initialization Hooks

`StateMachine_Init(state)` prepares graphics/resources for the target screen:
- Home/reinit: `HomePage_initialize()` sets up menus and starts VBlank handling for the animated top screen.
- Map selection: `Map_Selection_initialize()` prepares highlight layers and scrollable clouds.
- Lobby: `MultiplayerLobby_Init()` joins the lobby, prints status, and defaults to the Scorching Sands map.
- Gameplay: `Graphical_Gameplay_initialize()` configures race graphics, loads tiles/quadrants, and starts race timers (see [timer.md](timer.md)).
- Settings: `Settings_initialize()` draws toggles and selection masks.
- Play Again: `PlayAgain_Initialize()` prepares the replay prompt overlays.

## Cleanup Rules

`StateMachine_Cleanup(state, nextState)` ensures resources and network state are sane before the next screen:
- Home/reinit: `HomePage_Cleanup()` frees menu sprites.
- Map selection / Settings: no explicit cleanup needed.
- Lobby: keeps the WiFi connection alive when heading into `GAMEPLAY`; only calls `Multiplayer_Cleanup()` and clears multiplayer mode when exiting elsewhere (avoids breaking reconnection).
- Gameplay: stops race timers (`RaceTick_TimerStop()`), frees gameplay graphics (`Gameplay_Cleanup()`), halts race logic (`Race_Stop()`), and tears down multiplayer if it was active.
- Play Again: intentionally skips multiplayer cleanup so a replay can start instantly; timers are stopped when the player chooses to quit in that state’s logic.

## Lifecycle with main()

`main()` calls `StateMachine_Update()` each frame and, when a state change is requested, executes `StateMachine_Cleanup() -> video_nuke() -> StateMachine_Init()` ([main.c:31-39](../source/core/main.c#L31-L39)). This keeps VBlank routing, timers, and multiplayer connections consistent across transitions (see [main.md](main.md) and [init.md](init.md)).

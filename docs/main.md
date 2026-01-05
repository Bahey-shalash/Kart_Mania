# Main Game Loop

## Overview

`main.c` contains the top-level orchestrator for Kart Mania: it runs one-time setup, pumps the WiFi stack every frame, dispatches the state machine, and synchronizes to the Nintendo DS display refresh. The loop is intentionally small so that state-specific code lives in the state machine and UI/gameplay modules.

- Entry point: [main.c:20-44](../source/core/main.c#L20-L44)
- Calls [`InitGame()`](../source/core/init.c#L85-L98) once to bring up storage, context, audio, WiFi, and the first screen (see [init.md](init.md))
- Uses the global context singleton to track the active `GameState` (see [context.md](context.md))

## Frame Flow

Every frame the loop performs four predictable steps:
1. **Pump WiFi** — `Wifi_Update()` must run continuously to keep the DSWifi firmware alive for multiplayer ([main.c:27-30](../source/core/main.c#L27-L30)). See the reconnection notes in `docs/old/MULTIPLAYER_RECONNECTION_FIX.md`.
2. **Update current state** — `StateMachine_Update()` executes the active screen’s logic and returns the next `GameState` ([main.c:31-33](../source/core/main.c#L31-L33)). Details in [state_machine.md](state_machine.md).
3. **Handle transitions** — when the state changes, the loop calls `StateMachine_Cleanup(...)`, updates `ctx->currentGameState`, nukes video memory via `video_nuke()`, and re-initializes the new state with `StateMachine_Init(...)` ([main.c:34-39](../source/core/main.c#L34-L39)).
4. **Sync to VBlank** — `swiWaitForVBlank()` locks the loop to 60Hz so VBlank handlers (see [timer.md](timer.md)) stay in sync ([main.c:42-43](../source/core/main.c#L42-L43)).

## Transition Handling

`main()` centralizes state transitions instead of letting each state jump directly. This guarantees:
- Cleanup always runs before initialization, preventing leaked sprites or timers when swapping screens.
- `video_nuke()` clears VRAM between screens to avoid graphical artifacts.
- The context’s `currentGameState` is updated exactly once per transition, keeping the state machine and VBlank routing in agreement.

## Cross-References

- Startup flow lives in [init.md](init.md); control returns here to enter the loop.
- State lifecycle (update/init/cleanup) is defined in [state_machine.md](state_machine.md).
- WiFi behavior and why `Wifi_Update()` cannot pause are documented in `docs/old/MULTIPLAYER_RECONNECTION_FIX.md`.

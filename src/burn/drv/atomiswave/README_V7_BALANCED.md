# V7 balanced render defaults

V7 changes the default for heavy Atomiswave games such as `samsptk`, `kov7sprt`, and `ngbc`.

V6 `flycast` preset improved speed slightly, but the log and user report show picture artifacts. V7 defaults those heavy games to a safer balanced preset:

- alpha sorting: `per-triangle (normal)`
- RTTB: `enabled`
- threaded rendering: `disabled`
- synchronous rendering: `disabled`
- direct FBNeo ROM archive remains enabled

This keeps the layer/RTT behavior closer to the previous accurate mode while avoiding the heaviest per-pixel alpha path.

## Recommended test command

```bat
@echo off
set FBNEO_AWAVE_PROFILE=1
set FBNEO_AWAVE_RENDER_PRESET=balanced
fbneo64d.exe
pause
```

## Compare modes

Accurate/correctness baseline:

```bat
set FBNEO_AWAVE_RENDER_PRESET=accurate
```

V6 fast-but-risky mode:

```bat
set FBNEO_AWAVE_RENDER_PRESET=flycast
```

Force only one option:

```bat
set FBNEO_AWAVE_ALPHA_SORTING=triangle
set FBNEO_AWAVE_RTTB=1
set FBNEO_AWAVE_THREADED_RENDERING=0
```

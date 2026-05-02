# V6 Flycast-like render defaults

This package keeps the V5 diagnostics and direct FBNeo ROM archive path, but changes heavy Atomiswave/NAOMI titles to a render preset closer to standalone Flycast defaults.

Default heavy-title preset applies to:

- `samsptk`
- `kov7sprt`
- `ngbc`

For those titles the wrapper now prefers:

- `reicast_alpha_sorting = per-triangle (normal)`
- `reicast_enable_rttb = disabled`
- `reicast_threaded_rendering = enabled`
- `reicast_synchronous_rendering = disabled`

## Environment switches

Force Flycast-like preset for all games:

```bat
set FBNEO_AWAVE_RENDER_PRESET=flycast
```

Force accurate/old behavior:

```bat
set FBNEO_AWAVE_RENDER_PRESET=accurate
set FBNEO_AWAVE_RTTB=1
set FBNEO_AWAVE_ALPHA_SORTING=pixel
set FBNEO_AWAVE_THREADED_RENDERING=0
```

Try fastest alpha sorting:

```bat
set FBNEO_AWAVE_ALPHA_SORTING=strip
```

Force normal alpha sorting:

```bat
set FBNEO_AWAVE_ALPHA_SORTING=triangle
```

Re-enable RTTB if a game has missing backgrounds/layers:

```bat
set FBNEO_AWAVE_RTTB=1
```

Disable threaded rendering if visual artifacts appear:

```bat
set FBNEO_AWAVE_THREADED_RENDERING=0
```

## Suggested test command

```bat
@echo off
set FBNEO_AWAVE_PROFILE=1
set FBNEO_AWAVE_RENDER_PRESET=flycast
fbneo64d.exe
pause
```

The log should show something like:

```text
AWAVE profile: visual mode=cpu_readback; render_preset=flycast, RTTB=disabled, alpha=per-triangle (normal), threaded_rendering=enabled, direct_rom=enabled
```

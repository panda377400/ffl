# V9 no-spam diagnostics

V8 proved that speed preset and audio latency were active, but its per-frame `AWAVE slow-run` logging became extremely noisy when the timer produced invalid/huge deltas. Writing thousands of HTML log lines can itself make FBNeo-QT sluggish.

V9 changes:

- `AWAVE slow-run` per-frame logging is disabled by default.
- Set `FBNEO_AWAVE_TRACE_SLOWRUN=1` only when you need detailed slow-run tracing.
- Profile summaries now show `slow=`, `slowMax=` and `invalidTime=` every 120 frames instead of spamming every frame.
- Invalid/huge timing deltas are ignored for averages instead of making later profile rows become all zero.

Recommended test:

```bat
@echo off
set FBNEO_AWAVE_PROFILE=1
set FBNEO_AWAVE_RENDER_PRESET=speed
set FBNEO_AWAVE_AUDIO_LATENCY_FRAMES=1
fbneo64d.exe
pause
```

Do not set `FBNEO_AWAVE_TRACE_SLOWRUN` for normal testing.

# V8 speed/audio experiment

This package keeps the V7 balanced visual baseline, then adds a separate `speed` preset and lower-latency audio bridge.

## Recommended test

```bat
@echo off
set FBNEO_AWAVE_PROFILE=1
set FBNEO_AWAVE_RENDER_PRESET=speed
set FBNEO_AWAVE_AUDIO_LATENCY_FRAMES=1
fbneo64d.exe
pause
```

Expected log header:

```text
render_preset=speed
speed_hacks=enabled
audio_latency_frames=1
```

## What changed from V7

- `speed` preset now enables speed hacks without forcing threaded rendering.
- `speed` uses `per-strip` alpha by default. If graphics break, set:

```bat
set FBNEO_AWAVE_ALPHA_SORTING=triangle
```

- `speed_hacks` disables Flycast options that may cost CPU in this embedded wrapper:
  - `div_matching=disabled`
  - `enable_dsp=disabled`
  - `mipmapping=disabled`
  - `fog=disabled`
  - `pvr2_filtering=disabled`
  - `delay_frame_swapping=disabled`
  - `frame_skipping=disabled`

- Audio queue target is no longer allowed to grow into a multi-frame FIFO by default.
  The previous logs showed `audioQueued` climbing to about 2205 frames. V8 targets
  1 frame in `speed` mode and 2 frames otherwise.

## If picture is wrong

Use this safer speed test:

```bat
@echo off
set FBNEO_AWAVE_PROFILE=1
set FBNEO_AWAVE_RENDER_PRESET=speed
set FBNEO_AWAVE_ALPHA_SORTING=triangle
set FBNEO_AWAVE_RTTB=1
set FBNEO_AWAVE_AUDIO_LATENCY_FRAMES=1
fbneo64d.exe
pause
```

## What to send back

Send the new `zzBurnDebug.html`, especially lines containing:

- `AWAVE profile: visual mode=`
- `audioQueued=`
- `AWAVE slow-run`
- final `frames per second`

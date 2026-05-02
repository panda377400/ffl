# V4 pacing / architecture fixes

This version is built on V3 and addresses the new skip_readback log.

## What the new log proved

`FBNEO_AWAVE_VIDEO_MODE=skip_readback` worked: `readpix=0.000`, `pbo=0`, and the screen went black, but samsptk was still slow. That means PBO/glReadPixels is not the only bottleneck.

## Changes

1. `awave_wrap_config.h` now detects 64-bit Windows by `_WIN64`, `_M_X64`, `__x86_64__`, and `__amd64__`, instead of relying on `BUILD_X64_EXE`. A 64-bit `fbneo64d.exe` must not compile Flycast wrapper files as `TARGET_WIN86`.

2. `awave_core.cpp` now includes `awave_wrap_config.h` before diagnostics, so the log should report `TARGET_WIN64=1` and `HAVE_OPENGL=1` in a 64-bit OpenGL build.

3. Added `NaomiCoreWantsRedraw()`. In `skip_readback` and valid `direct_texture` modes, the driver no longer calls `BurnDrvRedraw()`, avoiding FBNeo/SoftFX redraw cost for a black or stale CPU framebuffer.

## Test

```bat
set FBNEO_AWAVE_VIDEO_MODE=skip_readback
set FBNEO_AWAVE_PROFILE=1
fbneo64d.exe
```

Expected log:

```text
AWAVE profile: TARGET_WIN64=1
AWAVE profile: HAVE_OPENGL=1
AWAVE profile: visual mode=skip_readback
readpix=0.000
pbo=0
```

If it is still slow after this, the remaining bottleneck is probably outside the atomiswave directory: FBNeo-QT main loop/vblank/audio pacing or how the embedded libretro core is driven per frame.

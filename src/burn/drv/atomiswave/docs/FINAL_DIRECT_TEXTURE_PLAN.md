# Final direct-texture present plan

This version adds the atomiswave/Flycast side of the correct final path:

```text
Flycast PVR -> OpenGL FBO/color texture -> FBNeo-QT draws that texture directly
```

It avoids the expensive old path:

```text
Flycast FBO -> glReadPixels/PBO -> CPU NaomiVideo -> pBurnDraw -> SoftFX/window blit
```

## Runtime modes

Default remains compatible CPU readback:

```bash
FBNEO_AWAVE_VIDEO_MODE=cpu_readback
```

Use the final intended path after the frontend context bridge is wired:

```bash
FBNEO_AWAVE_VIDEO_MODE=direct_texture
```

Diagnostic mode to prove readback is the bottleneck:

```bash
FBNEO_AWAVE_VIDEO_MODE=skip_readback
```

`skip_readback` intentionally gives stale/blank normal FBNeo video, but Flycast still runs. If audio/core speed becomes full speed in this mode, then GPU-to-CPU readback/display is confirmed as the bottleneck.

## Required frontend integration

The atomiswave directory cannot, by itself, show an OpenGL texture inside FBNeo-QT. The frontend must:

1. Register the frontend/shared OpenGL context before `NaomiCoreInit()`:
   - `OGLSetContext(void*)`
   - `OGLSetMakeCurrent(void (*)(void*))`
   - `OGLSetDoneCurrent(void (*)(void*))`
2. Set `FBNEO_AWAVE_VIDEO_MODE=direct_texture`.
3. After `NaomiCoreFrame()`, call `NaomiCoreGetHwVideoInfo()`.
4. If `info.valid != 0`, draw `info.texture` directly as a GL_TEXTURE_2D and skip the normal pBurnDraw/SoftFX blit for that frame.

If the frontend does not provide a GL context, this version logs a warning and safely falls back to CPU readback so it will not black-screen.

## Added public API

```cpp
struct NaomiCoreHwVideoInfo {
    UINT32 size;
    UINT32 texture;
    UINT32 framebuffer;
    INT32 width;
    INT32 height;
    INT32 upsideDown;
    INT32 valid;
};

INT32 NaomiCoreGetHwVideoInfo(NaomiCoreHwVideoInfo* info);
INT32 NaomiCoreUsingHwDirectPresent();
```

`upsideDown=1` means the OpenGL texture has the normal GL bottom-left origin; the frontend should flip vertically when drawing to a top-left UI framebuffer.

## What this package does not do automatically

It does not patch the exact FBNeo-QT renderer file, because only the `atomiswave` directory was provided. See `frontend_integration/fbneo_qt_awave_direct_texture_example.cpp` for the intended integration pattern.

# atomiswave v3 - direct texture present side

This package keeps the v2 direct-ROM loader and adds the atomiswave side of the final correct video architecture.

New mode:

```bash
FBNEO_AWAVE_VIDEO_MODE=direct_texture
```

This mode avoids readback only when the frontend has supplied a shared/current OpenGL context. Without frontend support it logs a warning and falls back to CPU readback so the build remains usable.

For testing bottlenecks:

```bash
FBNEO_AWAVE_VIDEO_MODE=skip_readback
FBNEO_AWAVE_PROFILE=1
```

If `skip_readback` runs full speed while normal mode is slow, then the slowdown is definitely the readback/display bridge, not the Flycast core.

Files to read:

- `docs/FINAL_DIRECT_TEXTURE_PLAN.md`
- `frontend_integration/fbneo_qt_awave_direct_texture_example.cpp`

Important: a true final result needs a small FBNeo-QT renderer patch outside this directory. The new exported API is in `awave_core.h`.

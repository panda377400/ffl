# Atomiswave v2 fixes

Changes in this package:

1. Replaces per-run temp zip creation with an in-memory FBNeo ROM archive. `flycast_retro_load_game()` now receives a synthetic path like `fbneo-awave://samsptk.zip`; Flycast `OpenArchive()` resolves that path through callbacks into ROM data loaded by `BurnLoadRom()`.
2. Keeps `FBNEO_AWAVE_USE_DIRECT_ROM=0` as an emergency fallback to the old on-disk zip cache. The fallback now uses a stable `content/<game>.zip` path instead of `content/run-*`.
3. Clears the temporary in-memory ROM archive immediately after `flycast_retro_load_game()` succeeds, so large games do not keep a second full ROM copy during gameplay.
4. Adds a default fast-visual profile for `samsptk`: RTTB stays enabled, but alpha sorting changes from `per-pixel (accurate)` to `per-triangle (normal)`. This targets the observed ~48 FPS result in the uploaded log. Set `FBNEO_AWAVE_FAST_VISUALS=0` to restore per-pixel alpha for all games.
5. Forces anisotropic filtering to disabled to avoid hidden frontend/default cost on older GPUs.

Known limitation: this was source-patched in the uploaded atomiswave tree and not full-FBNeo compiled in this environment. If direct archive loading fails, run once with `FBNEO_AWAVE_USE_DIRECT_ROM=0` and send the new `zzBurnDebug.html`.

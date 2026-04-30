# mGBA Porting Workspace

This directory is the staging area for bringing mGBA into FBNeo as a
whole-system driver, similar in shape to:

- `src/burn/drv/nes`
- `src/burn/drv/snes`
- `src/burn/drv/megadrive`

It is not a MAME-style board-driver port. The correct mental model is
"embed a console core, then wrap it with FBNeo driver glue."

## Current State

The embedded source tree now covers the minimum FBNeo-facing subset for:

- GBA
- GB
- GBC

The mirrored upstream subtree currently includes:

- `upstream/include/mgba`
- `upstream/include/mgba-util`
- `upstream/src/arm`
- `upstream/src/core`
- `upstream/src/gb`
- `upstream/src/gba`
- `upstream/src/sm83`
- `upstream/src/util`
- `upstream/src/platform/windows`

The copied subtree intentionally excludes:

- `src/platform/libretro`
- `src/platform/qt`
- `src/platform/sdl`
- debugger/scripting/frontends not needed for first bring-up

Additional dependencies mirrored locally:

- `upstream/src/third-party/blip_buf`
- `upstream/src/third-party/inih`

The copied source is now wrapped by generated root-level shim files:

- `mgba_wrap_*.c`

These are generated from `standalone_sources.txt` by:

- `generate_wrappers.ps1`

That keeps upstream sync manageable while avoiding basename collisions in
FBNeo's legacy makefiles. The wrapper generator also dispatches the
Windows-only mGBA sources to portable fallbacks on non-Windows targets, so
the same FBNeo source list can build on Win32, libretro Android/Linux, and
other non-Windows ports without maintaining duplicate wrapper sets.

## Why It Started With GBA

mGBA exposes a clean embeddable `mCore` API:

- `mCoreCreate(mPLATFORM_GBA)`
- `loadROM`
- `loadBIOS`
- `reset`
- `runFrame`
- `setVideoBuffer`
- `setKeys`
- `saveState`
- `loadState`

That made a GBA-only first stage the lowest-risk path. GB/GBC support has
now been added on top of the same wrapper/build pipeline.

## Implemented First Pass

The first end-to-end embedded bring-up is now in place:

- `d_mgba.cpp`
- `mgba_core.cpp`
- `mgba_core.h`
- `mgba_wrap_config.h`

It currently exposes three generic FBNeo drivers:

- `mgba_gba`
- `mgba_gb`
- `mgba_gbc`

The generic loaders expect ROM zips named:

- `roms/mgba_gba.zip`
- `roms/mgba_gb.zip`
- `roms/mgba_gbc.zip`

With these contents:

- `mgba_gba`: required `mgba_cart.gba`, optional `gba_bios.bin`
- `mgba_gb`: required `mgba_cart.gb`, optional `gb_bios.bin`
- `mgba_gbc`: required `mgba_cart.gbc`, optional `gbc_bios.bin`

For easier day-to-day use on Windows, a helper packer now exists at:

- `fbneo/mgba_pack_cart.ps1`
- `fbneo/mgba_pack_cart.bat`

For the preferred "generic driver + RomData" workflow, a dedicated helper now
exists too:

- `fbneo/mgba_make_romdata.ps1`
- `fbneo/mgba_make_romdata.bat`

Example:

- `powershell -ExecutionPolicy Bypass -File .\\mgba_pack_cart.ps1 C:\\roms\\mario.gba`
- `powershell -ExecutionPolicy Bypass -File .\\mgba_pack_cart.ps1 C:\\roms\\mario.gba C:\\bios\\gba_bios.bin`
- `powershell -ExecutionPolicy Bypass -File .\\mgba_make_romdata.ps1 C:\\roms\\mario.gba`
- `powershell -ExecutionPolicy Bypass -File .\\mgba_make_romdata.ps1 C:\\roms\\mario.gba C:\\bios\\gba_bios.bin`

The bridge currently supports:

- ROM load from FBNeo memory
- optional BIOS load
- GBA, GB, and GBC video output
- digital handheld pad input
- stereo audio through mGBA blip buffers
- savestate scan via `mCoreSaveStateNamed` / `mCoreLoadStateNamed`
- battery save scan via `savedataClone` / `savedataRestore`

## Recommended Porting Stages

1. Add a thin FBNeo wrapper pair:
   - `d_mgba.cpp`
   - `mgba_core.cpp` / `mgba_core.h`

2. Make the wrapper own one `mCore*` instance and drive it through:
   - `Init`
   - `Exit`
   - `Frame`
   - `Draw`
   - `Scan`

3. Start with one generic GBA test target, not a full ROM database.

4. After the core is stable, decide whether to:
   - generate a GBA driver list, or
   - keep mGBA behind a generic loader path

## First-pass Compile Targets

The first compile pass should only aim for:

- cartridge ROM load from memory
- video output
- digital input
- basic audio
- save SRAM/state

It should not try to solve on day one:

- link cable
- camera
- motion sensor
- full BIOS management UI
- huge No-Intro style ROM lists

## Candidate Compile Policy

For the current FBNeo integration pass, keep mGBA narrow:

- `M_CORE_GBA` enabled
- `M_CORE_GB` enabled
- `USE_DEBUGGERS` disabled
- `ENABLE_SCRIPTING` disabled
- `USE_SQLITE3` disabled
- `USE_ELF` disabled
- `MINIMAL_CORE=2`

The goal is to avoid pulling in optional subsystems until the basic GBA
frame loop is alive inside FBNeo.

## Build Integration

Build integration is now wired into:

- `makefile.burn_rules`
- `src/burner/libretro/Makefile.common`
- `projectfiles/visualstudio-2026/fbneo_vs2026.vcxproj`

The wrapper set is intentionally rooted in `src/burn/drv/mgba` so that
future upstream updates only need:

1. run `sync_from_upstream.ps1`
2. run `generate_wrappers.ps1`
3. rebuild

## Known Integration Gaps

These still remain for a more complete port:

- a proper per-game list instead of only generic cartridge entries
- better front-end UX for arbitrary cart naming / CRC-less generic loading
- link cable and specialty peripherals
- richer BIOS/UI handling
- broader non-Windows runtime validation beyond compile coverage

## Front-end Integration

The generic loader now shows up under the Win32 hardware filter as:

- `Hardware -> GBA`

The intended low-maintenance workflow is now:

1. run `mgba_make_romdata.bat` on a `.gba`
2. it creates `roms/gba/*.zip`
3. it creates `support/romdata/mgba_*.dat`
4. open `Misc -> Open RomData manager`
5. scan dats and launch the generated entry

## License Note

Upstream mGBA is MPL-2.0. The upstream license file was copied into:

- `upstream/LICENSE`

If source files are modified in-place, keep their notices intact and do
not collapse them into unrelated FBNeo source files.

## Validation Status

Standalone validation completed:

- `build_standalone.bat` succeeds
- `mgba_standalone_test.exe` boots a small upstream GBA test ROM

FBNeo validation completed:

- `fbneo64d.exe` now builds with the embedded mGBA subset
- `BurnDrvmgba_gba` appears in `driverlist.h`
- `BurnDrvmgba_gb` appears in `driverlist.h`
- `BurnDrvmgba_gbc` appears in `driverlist.h`
- a temporary `mgba_gba.zip` smoke test was launched in windowed debug mode
- process stayed alive for 8 seconds without an immediate crash

## Next Practical Step

The next useful expansion is:

1. decide whether to keep the generic loader model or generate a GBA set list
2. improve generic cart UX if the one-driver model is kept
3. verify libretro and Visual Studio builds against the same source set

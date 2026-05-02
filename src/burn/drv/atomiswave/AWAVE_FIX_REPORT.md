# atomiswave fixed package report

- NaomiSafeProfileMs inserted: 1
- HLE BIOS option made env-selectable
- startup skip/probe replacements: 2
- audio underrun drain patched
- profile output guarded replacements: 8
- awave_core.cpp written
- awave_core.h DREAMCAST_PAD enum added

## Notes

- I kept all Flycast core/dependency wrapper files because this port still builds a temporary zip and embeds Flycast runtime paths. Removing libzip/archive/zlib/PVR/AICA/ARM7/SH4/Maple files would likely break compilation or boot.
- Safe cleanup target is generated logs only. No source files were removed.
- If NAOMI BIOS errors remain, provide the proper BIOS archive/file set; source cannot synthesize missing BIOS ROMs.

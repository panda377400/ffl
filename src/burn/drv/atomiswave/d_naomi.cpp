// Stable NAOMI FBNeo driver bridge, first ROM-cart batch.
// This file is designed for the clean awave_core.cpp + flycast_shim bridge.
// It intentionally does not include Flycast source, libretro callbacks, or awave_wrap_*.
// Source references for ROM names/CRCs: MAME src/mame/sega/naomi.cpp, parsed 2026-05-03.

#include "burnint.h"
#include "awave_core.h"

static UINT8 NaomiRecalc = 0;
static UINT8 NaomiReset = 0;
static UINT8 NaomiJoy1[16];
static UINT8 NaomiJoy2[16];
static UINT8 NaomiJoy3[16];
static UINT8 NaomiJoy4[16];
static INT16 NaomiAnalog[4][4];

static struct BurnInputInfo Naomi2PInputList[] = {
    { "P1 Coin",     BIT_DIGITAL, NaomiJoy1 + 0,  "p1 coin"   },
    { "P1 Start",    BIT_DIGITAL, NaomiJoy1 + 1,  "p1 start"  },
    { "P1 Up",       BIT_DIGITAL, NaomiJoy1 + 2,  "p1 up"     },
    { "P1 Down",     BIT_DIGITAL, NaomiJoy1 + 3,  "p1 down"   },
    { "P1 Left",     BIT_DIGITAL, NaomiJoy1 + 4,  "p1 left"   },
    { "P1 Right",    BIT_DIGITAL, NaomiJoy1 + 5,  "p1 right"  },
    { "P1 Button 1", BIT_DIGITAL, NaomiJoy1 + 6,  "p1 fire 1" },
    { "P1 Button 2", BIT_DIGITAL, NaomiJoy1 + 7,  "p1 fire 2" },
    { "P1 Button 3", BIT_DIGITAL, NaomiJoy1 + 8,  "p1 fire 3" },
    { "P1 Button 4", BIT_DIGITAL, NaomiJoy1 + 9,  "p1 fire 4" },
    { "P1 Button 5", BIT_DIGITAL, NaomiJoy1 + 10, "p1 fire 5" },
    { "P1 Button 6", BIT_DIGITAL, NaomiJoy1 + 11, "p1 fire 6" },
    { "P1 Service",  BIT_DIGITAL, NaomiJoy1 + 12, "service"   },

    { "P2 Coin",     BIT_DIGITAL, NaomiJoy2 + 0,  "p2 coin"   },
    { "P2 Start",    BIT_DIGITAL, NaomiJoy2 + 1,  "p2 start"  },
    { "P2 Up",       BIT_DIGITAL, NaomiJoy2 + 2,  "p2 up"     },
    { "P2 Down",     BIT_DIGITAL, NaomiJoy2 + 3,  "p2 down"   },
    { "P2 Left",     BIT_DIGITAL, NaomiJoy2 + 4,  "p2 left"   },
    { "P2 Right",    BIT_DIGITAL, NaomiJoy2 + 5,  "p2 right"  },
    { "P2 Button 1", BIT_DIGITAL, NaomiJoy2 + 6,  "p2 fire 1" },
    { "P2 Button 2", BIT_DIGITAL, NaomiJoy2 + 7,  "p2 fire 2" },
    { "P2 Button 3", BIT_DIGITAL, NaomiJoy2 + 8,  "p2 fire 3" },
    { "P2 Button 4", BIT_DIGITAL, NaomiJoy2 + 9,  "p2 fire 4" },
    { "P2 Button 5", BIT_DIGITAL, NaomiJoy2 + 10, "p2 fire 5" },
    { "P2 Button 6", BIT_DIGITAL, NaomiJoy2 + 11, "p2 fire 6" },
    { "P2 Service",  BIT_DIGITAL, NaomiJoy2 + 12, "service"   },

    { "Test",        BIT_DIGITAL, NaomiJoy1 + 13, "diag"      },
    { "Reset",       BIT_DIGITAL, &NaomiReset,    "reset"     },
};
STDINPUTINFO(Naomi2P)

static struct BurnInputInfo Naomi4PInputList[] = {
    { "P1 Coin",     BIT_DIGITAL, NaomiJoy1 + 0,  "p1 coin"   }, { "P1 Start",    BIT_DIGITAL, NaomiJoy1 + 1,  "p1 start"  },
    { "P1 Up",       BIT_DIGITAL, NaomiJoy1 + 2,  "p1 up"     }, { "P1 Down",     BIT_DIGITAL, NaomiJoy1 + 3,  "p1 down"   },
    { "P1 Left",     BIT_DIGITAL, NaomiJoy1 + 4,  "p1 left"   }, { "P1 Right",    BIT_DIGITAL, NaomiJoy1 + 5,  "p1 right"  },
    { "P1 Button 1", BIT_DIGITAL, NaomiJoy1 + 6,  "p1 fire 1" }, { "P1 Button 2", BIT_DIGITAL, NaomiJoy1 + 7,  "p1 fire 2" },
    { "P1 Button 3", BIT_DIGITAL, NaomiJoy1 + 8,  "p1 fire 3" }, { "P1 Button 4", BIT_DIGITAL, NaomiJoy1 + 9,  "p1 fire 4" },
    { "P2 Coin",     BIT_DIGITAL, NaomiJoy2 + 0,  "p2 coin"   }, { "P2 Start",    BIT_DIGITAL, NaomiJoy2 + 1,  "p2 start"  },
    { "P2 Up",       BIT_DIGITAL, NaomiJoy2 + 2,  "p2 up"     }, { "P2 Down",     BIT_DIGITAL, NaomiJoy2 + 3,  "p2 down"   },
    { "P2 Left",     BIT_DIGITAL, NaomiJoy2 + 4,  "p2 left"   }, { "P2 Right",    BIT_DIGITAL, NaomiJoy2 + 5,  "p2 right"  },
    { "P2 Button 1", BIT_DIGITAL, NaomiJoy2 + 6,  "p2 fire 1" }, { "P2 Button 2", BIT_DIGITAL, NaomiJoy2 + 7,  "p2 fire 2" },
    { "P2 Button 3", BIT_DIGITAL, NaomiJoy2 + 8,  "p2 fire 3" }, { "P2 Button 4", BIT_DIGITAL, NaomiJoy2 + 9,  "p2 fire 4" },
    { "P3 Coin",     BIT_DIGITAL, NaomiJoy3 + 0,  "p3 coin"   }, { "P3 Start",    BIT_DIGITAL, NaomiJoy3 + 1,  "p3 start"  },
    { "P3 Up",       BIT_DIGITAL, NaomiJoy3 + 2,  "p3 up"     }, { "P3 Down",     BIT_DIGITAL, NaomiJoy3 + 3,  "p3 down"   },
    { "P3 Left",     BIT_DIGITAL, NaomiJoy3 + 4,  "p3 left"   }, { "P3 Right",    BIT_DIGITAL, NaomiJoy3 + 5,  "p3 right"  },
    { "P3 Button 1", BIT_DIGITAL, NaomiJoy3 + 6,  "p3 fire 1" }, { "P3 Button 2", BIT_DIGITAL, NaomiJoy3 + 7,  "p3 fire 2" },
    { "P3 Button 3", BIT_DIGITAL, NaomiJoy3 + 8,  "p3 fire 3" }, { "P3 Button 4", BIT_DIGITAL, NaomiJoy3 + 9,  "p3 fire 4" },
    { "P4 Coin",     BIT_DIGITAL, NaomiJoy4 + 0,  "p4 coin"   }, { "P4 Start",    BIT_DIGITAL, NaomiJoy4 + 1,  "p4 start"  },
    { "P4 Up",       BIT_DIGITAL, NaomiJoy4 + 2,  "p4 up"     }, { "P4 Down",     BIT_DIGITAL, NaomiJoy4 + 3,  "p4 down"   },
    { "P4 Left",     BIT_DIGITAL, NaomiJoy4 + 4,  "p4 left"   }, { "P4 Right",    BIT_DIGITAL, NaomiJoy4 + 5,  "p4 right"  },
    { "P4 Button 1", BIT_DIGITAL, NaomiJoy4 + 6,  "p4 fire 1" }, { "P4 Button 2", BIT_DIGITAL, NaomiJoy4 + 7,  "p4 fire 2" },
    { "P4 Button 3", BIT_DIGITAL, NaomiJoy4 + 8,  "p4 fire 3" }, { "P4 Button 4", BIT_DIGITAL, NaomiJoy4 + 9,  "p4 fire 4" },
    { "Test",        BIT_DIGITAL, NaomiJoy1 + 13, "diag"      }, { "Reset",       BIT_DIGITAL, &NaomiReset,    "reset"     },
};
STDINPUTINFO(Naomi4P)

static struct BurnInputInfo NaomiLightgunInputList[] = {
    { "P1 Coin",     BIT_DIGITAL, NaomiJoy1 + 0, "p1 coin"   },
    { "P1 Start",    BIT_DIGITAL, NaomiJoy1 + 1, "p1 start"  },
    { "P1 Trigger",  BIT_DIGITAL, NaomiJoy1 + 6, "p1 fire 1" },
    { "P1 Reload",   BIT_DIGITAL, NaomiJoy1 + 7, "p1 fire 2" },
    { "P1 X",        BIT_ANALOG_REL, (UINT8*)&NaomiAnalog[0][0], "mouse x-axis" },
    { "P1 Y",        BIT_ANALOG_REL, (UINT8*)&NaomiAnalog[0][1], "mouse y-axis" },
    { "P2 Coin",     BIT_DIGITAL, NaomiJoy2 + 0, "p2 coin"   },
    { "P2 Start",    BIT_DIGITAL, NaomiJoy2 + 1, "p2 start"  },
    { "P2 Trigger",  BIT_DIGITAL, NaomiJoy2 + 6, "p2 fire 1" },
    { "P2 Reload",   BIT_DIGITAL, NaomiJoy2 + 7, "p2 fire 2" },
    { "P2 X",        BIT_ANALOG_REL, (UINT8*)&NaomiAnalog[1][0], "p2 x-axis" },
    { "P2 Y",        BIT_ANALOG_REL, (UINT8*)&NaomiAnalog[1][1], "p2 y-axis" },
    { "Test",        BIT_DIGITAL, NaomiJoy1 + 13, "diag" },
    { "Reset",       BIT_DIGITAL, &NaomiReset, "reset" },
};
STDINPUTINFO(NaomiLightgun)

static void NaomiClearInputs()
{
    memset(NaomiJoy1, 0, sizeof(NaomiJoy1));
    memset(NaomiJoy2, 0, sizeof(NaomiJoy2));
    memset(NaomiJoy3, 0, sizeof(NaomiJoy3));
    memset(NaomiJoy4, 0, sizeof(NaomiJoy4));
    memset(NaomiAnalog, 0, sizeof(NaomiAnalog));
}

static UINT32 PackNaomiDigital(const UINT8* joy)
{
    UINT32 s = 0;
    if (joy[0])  s |= NAOMI_INPUT_COIN;
    if (joy[1])  s |= NAOMI_INPUT_START;
    if (joy[2])  s |= NAOMI_INPUT_UP;
    if (joy[3])  s |= NAOMI_INPUT_DOWN;
    if (joy[4])  s |= NAOMI_INPUT_LEFT;
    if (joy[5])  s |= NAOMI_INPUT_RIGHT;
    if (joy[6])  s |= NAOMI_INPUT_BTN1;
    if (joy[7])  s |= NAOMI_INPUT_BTN2 | NAOMI_INPUT_RELOAD;
    if (joy[8])  s |= NAOMI_INPUT_BTN3;
    if (joy[9])  s |= NAOMI_INPUT_BTN4;
    if (joy[10]) s |= NAOMI_INPUT_BTN5;
    if (joy[11]) s |= NAOMI_INPUT_BTN6;
    if (joy[12]) s |= NAOMI_INPUT_SERVICE;
    if (joy[13]) s |= NAOMI_INPUT_TEST;
    return s;
}

static INT32 NaomiFrameCommon()
{
    if (NaomiReset) NaomiCoreReset();
    NaomiCoreSetPadState(0, PackNaomiDigital(NaomiJoy1));
    NaomiCoreSetPadState(1, PackNaomiDigital(NaomiJoy2));
    NaomiCoreSetPadState(2, PackNaomiDigital(NaomiJoy3));
    NaomiCoreSetPadState(3, PackNaomiDigital(NaomiJoy4));
    NaomiCoreSetAnalogState(0, 0, NaomiAnalog[0][0]);
    NaomiCoreSetAnalogState(0, 1, NaomiAnalog[0][1]);
    NaomiCoreSetAnalogState(1, 0, NaomiAnalog[1][0]);
    NaomiCoreSetAnalogState(1, 1, NaomiAnalog[1][1]);
    NaomiCoreSetLightgunState(0, NaomiAnalog[0][0], NaomiAnalog[0][1], NaomiJoy1[7], NaomiJoy1[7]);
    NaomiCoreSetLightgunState(1, NaomiAnalog[1][0], NaomiAnalog[1][1], NaomiJoy2[7], NaomiJoy2[7]);
    return NaomiCoreFrame();
}

static INT32 NaomiExitCommon() { NaomiClearInputs(); return NaomiCoreExit(); }
static INT32 NaomiDrawCommon() { return NaomiCoreDraw(); }
static INT32 NaomiScanCommon(INT32 nAction, INT32* pnMin) { return NaomiCoreScan(nAction, pnMin); }

#define NAOMI_BIOS_COUNT 4
#define NAOMI_BIOS_ROMS \
    { "epr-21576h.ic27", 0x200000, 0xd4895685, BRF_ESS | BRF_BIOS }, \
    { "epr-21578h.ic27", 0x200000, 0x7b452946, BRF_ESS | BRF_BIOS }, \
    { "epr-21577h.ic27", 0x200000, 0xfdf17452, BRF_ESS | BRF_BIOS }, \
    { "315-6188.ic31",   0x002034, 0x7c9fea46, BRF_ESS | BRF_BIOS }

#define HOTD2_BIOS_COUNT 4
#define HOTD2_BIOS_ROMS \
    { "epr-21331.ic27", 0x200000, 0x065f8500, BRF_ESS | BRF_BIOS }, \
    { "epr-21330.ic27", 0x200000, 0x9e3bfa1b, BRF_ESS | BRF_BIOS }, \
    { "epr-21329.ic27", 0x200000, 0xd99e5b9b, BRF_ESS | BRF_BIOS }, \
    { "epr-21332.ic27", 0x200000, 0xbd6ce0ec, BRF_ESS | BRF_BIOS }

static const UINT32 NaomiDigitalMap[] = {
    NAOMI_INPUT_BTN1, NAOMI_INPUT_BTN2, NAOMI_INPUT_BTN3, NAOMI_INPUT_BTN4,
    NAOMI_INPUT_BTN5, NAOMI_INPUT_BTN6, NAOMI_INPUT_START, NAOMI_INPUT_COIN,
    NAOMI_INPUT_SERVICE, NAOMI_INPUT_TEST, NAOMI_INPUT_UP, NAOMI_INPUT_DOWN,
    NAOMI_INPUT_LEFT, NAOMI_INPUT_RIGHT
};

#define ZIPENTRY(name, idx) { name, idx }
#define ZIPEND { NULL, -1 }

#define NAOMI_INIT_FUNC(shortname, zipentries, inputtype, bioszip, systemname, runtime, platform) \
static INT32 shortname##Init() { \
    static NaomiGameConfig cfg = { #shortname, #shortname ".zip", zipentries, bioszip, systemname, runtime, NaomiDigitalMap, sizeof(NaomiDigitalMap)/sizeof(NaomiDigitalMap[0]), NULL, 0, inputtype, platform, NULL }; \
    return NaomiCoreInit(&cfg); \
}


// BIOS-only entries.
static struct BurnRomInfo naomiRomDesc[] = { NAOMI_BIOS_ROMS };
STD_ROM_PICK(naomi)
STD_ROM_FN(naomi)
static INT32 naomiInit() { return 1; }
struct BurnDriver BurnDrvNaomiNaomi = {
    "naomi", NULL, NULL, NULL, "1998", "Sega NAOMI BIOS\0", "BIOS only", "Sega", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_BOARDROM, 0, HARDWARE_MISC_POST90S, 0, 0,
    NULL, naomiRomInfo, naomiRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    naomiInit, NaomiExitCommon, NULL, NULL, NULL, NULL, 0, 640, 480, 4, 3
};

static struct BurnRomInfo hod2biosRomDesc[] = { HOTD2_BIOS_ROMS };
STD_ROM_PICK(hod2bios)
STD_ROM_FN(hod2bios)
static INT32 hod2biosInit() { return 1; }
struct BurnDriver BurnDrvNaomiHod2bios = {
    "hod2bios", NULL, NULL, NULL, "1998", "The House of the Dead 2 BIOS\0", "BIOS only", "Sega", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_BOARDROM, 0, HARDWARE_MISC_POST90S, 0, 0,
    NULL, hod2biosRomInfo, hod2biosRomName, NULL, NULL, NULL, NULL, NaomiLightgunInputInfo, NULL,
    hod2biosInit, NaomiExitCommon, NULL, NULL, NULL, NULL, 0, 640, 480, 4, 3
};

// Power Stone
static struct BurnRomInfo pstoneRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-21597.ic22", 0x0200000, 0x62c7acc0, BRF_PRG | BRF_ESS },
    { "mpr-21589.ic1", 0x0800000, 0x2fa66608, BRF_PRG | BRF_ESS },
    { "mpr-21590.ic2", 0x0800000, 0x6341b399, BRF_PRG | BRF_ESS },
    { "mpr-21591.ic3", 0x0800000, 0x7f2d99aa, BRF_PRG | BRF_ESS },
    { "mpr-21592.ic4", 0x0800000, 0x6ebe3b25, BRF_PRG | BRF_ESS },
    { "mpr-21593.ic5", 0x0800000, 0x84366f3e, BRF_PRG | BRF_ESS },
    { "mpr-21594.ic6", 0x0800000, 0xddfa0467, BRF_PRG | BRF_ESS },
    { "mpr-21595.ic7", 0x0800000, 0x7ab218f7, BRF_PRG | BRF_ESS },
    { "mpr-21596.ic8", 0x0800000, 0xf27dbdc5, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(pstone)
STD_ROM_FN(pstone)
static NaomiZipEntry pstoneZipEntries[] = {
    ZIPENTRY("epr-21597.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-21589.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-21590.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-21591.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-21592.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-21593.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-21594.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-21595.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-21596.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPEND
};
NAOMI_INIT_FUNC(pstone, pstoneZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomipstone = {
    "pstone", NULL, "naomi", NULL, "1999", "Power Stone" "\0", NULL, "Capcom", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, pstoneRomInfo, pstoneRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    pstoneInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Dead or Alive 2 Millennium
static struct BurnRomInfo doa2mRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "doa2verm.ic22", 0x0400000, 0x94b16f08, BRF_PRG | BRF_ESS },
    { "mpr-22100.ic1", 0x0800000, 0x92a53e5e, BRF_PRG | BRF_ESS },
    { "mpr-22101.ic2", 0x0800000, 0x14cd7dce, BRF_PRG | BRF_ESS },
    { "mpr-22102.ic3", 0x0800000, 0x34e778b1, BRF_PRG | BRF_ESS },
    { "mpr-22103.ic4", 0x0800000, 0x6f3db8df, BRF_PRG | BRF_ESS },
    { "mpr-22104.ic5", 0x0800000, 0xfcc2787f, BRF_PRG | BRF_ESS },
    { "mpr-22105.ic6", 0x0800000, 0x3e2da942, BRF_PRG | BRF_ESS },
    { "mpr-22106.ic7", 0x0800000, 0x03aceaaf, BRF_PRG | BRF_ESS },
    { "mpr-22107.ic8", 0x0800000, 0x6f1705e4, BRF_PRG | BRF_ESS },
    { "mpr-22108.ic9", 0x0800000, 0xd34d3d8a, BRF_PRG | BRF_ESS },
    { "mpr-22109.ic10", 0x0800000, 0x00ef44dd, BRF_PRG | BRF_ESS },
    { "mpr-22110.ic11", 0x0800000, 0xa193b577, BRF_PRG | BRF_ESS },
    { "mpr-22111.ic12s", 0x0800000, 0x55dddebf, BRF_PRG | BRF_ESS },
    { "mpr-22112.ic13s", 0x0800000, 0xc5ffe564, BRF_PRG | BRF_ESS },
    { "mpr-22113.ic14s", 0x0800000, 0x12e7adf0, BRF_PRG | BRF_ESS },
    { "mpr-22114.ic15s", 0x0800000, 0xd181d0a0, BRF_PRG | BRF_ESS },
    { "mpr-22115.ic16s", 0x0800000, 0xee2c842d, BRF_PRG | BRF_ESS },
    { "mpr-22116.ic17s", 0x0800000, 0x224ab770, BRF_PRG | BRF_ESS },
    { "mpr-22117.ic18s", 0x0800000, 0x884a45a9, BRF_PRG | BRF_ESS },
    { "mpr-22118.ic19s", 0x0800000, 0x8d631cbf, BRF_PRG | BRF_ESS },
    { "mpr-22119.ic20s", 0x0800000, 0xd608fa86, BRF_PRG | BRF_ESS },
    { "mpr-22120.ic21s", 0x0800000, 0xa30facb4, BRF_PRG | BRF_ESS },
    { "841-0003.sf", 0x0000084, 0x3a119a17, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(doa2m)
STD_ROM_FN(doa2m)
static NaomiZipEntry doa2mZipEntries[] = {
    ZIPENTRY("doa2verm.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-22100.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-22101.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-22102.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-22103.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-22104.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-22105.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-22106.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-22107.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-22108.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-22109.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-22110.ic11", NAOMI_BIOS_COUNT + 11),
    ZIPENTRY("mpr-22111.ic12s", NAOMI_BIOS_COUNT + 12),
    ZIPENTRY("mpr-22112.ic13s", NAOMI_BIOS_COUNT + 13),
    ZIPENTRY("mpr-22113.ic14s", NAOMI_BIOS_COUNT + 14),
    ZIPENTRY("mpr-22114.ic15s", NAOMI_BIOS_COUNT + 15),
    ZIPENTRY("mpr-22115.ic16s", NAOMI_BIOS_COUNT + 16),
    ZIPENTRY("mpr-22116.ic17s", NAOMI_BIOS_COUNT + 17),
    ZIPENTRY("mpr-22117.ic18s", NAOMI_BIOS_COUNT + 18),
    ZIPENTRY("mpr-22118.ic19s", NAOMI_BIOS_COUNT + 19),
    ZIPENTRY("mpr-22119.ic20s", NAOMI_BIOS_COUNT + 20),
    ZIPENTRY("mpr-22120.ic21s", NAOMI_BIOS_COUNT + 21),
    ZIPENTRY("841-0003.sf", NAOMI_BIOS_COUNT + 22),
    ZIPEND
};
NAOMI_INIT_FUNC(doa2m, doa2mZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomidoa2m = {
    "doa2m", NULL, "naomi", NULL, "2000", "Dead or Alive 2 Millennium" "\0", NULL, "Tecmo", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, doa2mRomInfo, doa2mRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    doa2mInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Dead or Alive 2
static struct BurnRomInfo doa2RomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-22207.ic22", 0x0400000, 0x313d0e55, BRF_PRG | BRF_ESS },
    { "mpr-22100.ic1", 0x0800000, 0x92a53e5e, BRF_PRG | BRF_ESS },
    { "mpr-22101.ic2", 0x0800000, 0x14cd7dce, BRF_PRG | BRF_ESS },
    { "mpr-22102.ic3", 0x0800000, 0x34e778b1, BRF_PRG | BRF_ESS },
    { "mpr-22103.ic4", 0x0800000, 0x6f3db8df, BRF_PRG | BRF_ESS },
    { "mpr-22104.ic5", 0x0800000, 0xfcc2787f, BRF_PRG | BRF_ESS },
    { "mpr-22105.ic6", 0x0800000, 0x3e2da942, BRF_PRG | BRF_ESS },
    { "mpr-22106.ic7", 0x0800000, 0x03aceaaf, BRF_PRG | BRF_ESS },
    { "mpr-22107.ic8", 0x0800000, 0x6f1705e4, BRF_PRG | BRF_ESS },
    { "mpr-22108.ic9", 0x0800000, 0xd34d3d8a, BRF_PRG | BRF_ESS },
    { "mpr-22109.ic10", 0x0800000, 0x00ef44dd, BRF_PRG | BRF_ESS },
    { "mpr-22110.ic11", 0x0800000, 0xa193b577, BRF_PRG | BRF_ESS },
    { "mpr-22111.ic12s", 0x0800000, 0x55dddebf, BRF_PRG | BRF_ESS },
    { "mpr-22112.ic13s", 0x0800000, 0xc5ffe564, BRF_PRG | BRF_ESS },
    { "mpr-22113.ic14s", 0x0800000, 0x12e7adf0, BRF_PRG | BRF_ESS },
    { "mpr-22114.ic15s", 0x0800000, 0xd181d0a0, BRF_PRG | BRF_ESS },
    { "mpr-22115.ic16s", 0x0800000, 0xee2c842d, BRF_PRG | BRF_ESS },
    { "mpr-22116.ic17s", 0x0800000, 0x224ab770, BRF_PRG | BRF_ESS },
    { "mpr-22117.ic18s", 0x0800000, 0x884a45a9, BRF_PRG | BRF_ESS },
    { "mpr-22118.ic19s", 0x0800000, 0x8d631cbf, BRF_PRG | BRF_ESS },
    { "mpr-22119.ic20s", 0x0800000, 0xd608fa86, BRF_PRG | BRF_ESS },
    { "mpr-22120.ic21s", 0x0800000, 0xa30facb4, BRF_PRG | BRF_ESS },
    { "841-0003.sf", 0x0000084, 0x3a119a17, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(doa2)
STD_ROM_FN(doa2)
static NaomiZipEntry doa2ZipEntries[] = {
    ZIPENTRY("epr-22207.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-22100.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-22101.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-22102.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-22103.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-22104.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-22105.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-22106.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-22107.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-22108.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-22109.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-22110.ic11", NAOMI_BIOS_COUNT + 11),
    ZIPENTRY("mpr-22111.ic12s", NAOMI_BIOS_COUNT + 12),
    ZIPENTRY("mpr-22112.ic13s", NAOMI_BIOS_COUNT + 13),
    ZIPENTRY("mpr-22113.ic14s", NAOMI_BIOS_COUNT + 14),
    ZIPENTRY("mpr-22114.ic15s", NAOMI_BIOS_COUNT + 15),
    ZIPENTRY("mpr-22115.ic16s", NAOMI_BIOS_COUNT + 16),
    ZIPENTRY("mpr-22116.ic17s", NAOMI_BIOS_COUNT + 17),
    ZIPENTRY("mpr-22117.ic18s", NAOMI_BIOS_COUNT + 18),
    ZIPENTRY("mpr-22118.ic19s", NAOMI_BIOS_COUNT + 19),
    ZIPENTRY("mpr-22119.ic20s", NAOMI_BIOS_COUNT + 20),
    ZIPENTRY("mpr-22120.ic21s", NAOMI_BIOS_COUNT + 21),
    ZIPENTRY("841-0003.sf", NAOMI_BIOS_COUNT + 22),
    ZIPEND
};
NAOMI_INIT_FUNC(doa2, doa2ZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomidoa2 = {
    "doa2", "doa2m", "naomi", NULL, "1999", "Dead or Alive 2" "\0", NULL, "Tecmo", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, doa2RomInfo, doa2RomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    doa2Init, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Spawn: In the Demon's Hand (Rev B)
static struct BurnRomInfo spawnRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-22977b.ic22", 0x0400000, 0x814ff5d1, BRF_PRG | BRF_ESS },
    { "mpr-22967.ic1", 0x0800000, 0x78c7d914, BRF_PRG | BRF_ESS },
    { "mpr-22968.ic2", 0x0800000, 0x8c4ae1bb, BRF_PRG | BRF_ESS },
    { "mpr-22969.ic3", 0x0800000, 0x2928627c, BRF_PRG | BRF_ESS },
    { "mpr-22970.ic4", 0x0800000, 0x12e27ffd, BRF_PRG | BRF_ESS },
    { "mpr-22971.ic5", 0x0800000, 0x993d2bce, BRF_PRG | BRF_ESS },
    { "mpr-22972.ic6", 0x0800000, 0xe0f75067, BRF_PRG | BRF_ESS },
    { "mpr-22973.ic7", 0x0800000, 0x698498ca, BRF_PRG | BRF_ESS },
    { "mpr-22974.ic8", 0x0800000, 0x20983c51, BRF_PRG | BRF_ESS },
    { "mpr-22975.ic9", 0x0800000, 0x0d3c70d1, BRF_PRG | BRF_ESS },
    { "mpr-22976.ic10", 0x0800000, 0x092d8063, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(spawn)
STD_ROM_FN(spawn)
static NaomiZipEntry spawnZipEntries[] = {
    ZIPENTRY("epr-22977b.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-22967.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-22968.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-22969.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-22970.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-22971.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-22972.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-22973.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-22974.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-22975.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-22976.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPEND
};
NAOMI_INIT_FUNC(spawn, spawnZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomispawn = {
    "spawn", NULL, "naomi", NULL, "1999", "Spawn: In the Demon's Hand (Rev B)" "\0", NULL, "Todd McFarlane / Capcom", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 4, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, spawnRomInfo, spawnRomName, NULL, NULL, NULL, NULL, Naomi4PInputInfo, NULL,
    spawnInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Marvel Vs. Capcom 2: New Age of Heroes (Export, Korea, Rev A)
static struct BurnRomInfo mvsc2RomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23085a.ic11", 0x0400000, 0x5d5b7ad1, BRF_PRG | BRF_ESS },
    { "mpr-23048.ic17s", 0x0800000, 0x93d7a63a, BRF_PRG | BRF_ESS },
    { "mpr-23049.ic18", 0x0800000, 0x003dcce0, BRF_PRG | BRF_ESS },
    { "mpr-23050.ic19s", 0x0800000, 0x1d6b88a7, BRF_PRG | BRF_ESS },
    { "mpr-23051.ic20", 0x0800000, 0x01226aaa, BRF_PRG | BRF_ESS },
    { "mpr-23052.ic21s", 0x0800000, 0x74bee120, BRF_PRG | BRF_ESS },
    { "mpr-23053.ic22", 0x0800000, 0xd92d4401, BRF_PRG | BRF_ESS },
    { "mpr-23054.ic23s", 0x0800000, 0x78ba02e8, BRF_PRG | BRF_ESS },
    { "mpr-23055.ic24", 0x0800000, 0x84319604, BRF_PRG | BRF_ESS },
    { "mpr-23056.ic25s", 0x0800000, 0xd7386034, BRF_PRG | BRF_ESS },
    { "mpr-23057.ic26", 0x0800000, 0xa3f087db, BRF_PRG | BRF_ESS },
    { "mpr-23058.ic27s", 0x0800000, 0x61a6cc5d, BRF_PRG | BRF_ESS },
    { "mpr-23059.ic28", 0x0800000, 0x64808024, BRF_PRG | BRF_ESS },
    { "mpr-23060.ic29", 0x0800000, 0x67519942, BRF_PRG | BRF_ESS },
    { "mpr-23061.ic30s", 0x0800000, 0xfb1844c4, BRF_PRG | BRF_ESS },
    { "mpr-23083.ic31", 0x0400000, 0xc61d2dfe, BRF_PRG | BRF_ESS },
    { "mpr-23083.ic31", 0x0400000, 0xc61d2dfe, BRF_PRG | BRF_ESS },
    { "mpr-23084.ic32s", 0x0400000, 0x4ebbbdd9, BRF_PRG | BRF_ESS },
    { "25lc040.ic13s", 0x0000200, 0xdc449637, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(mvsc2)
STD_ROM_FN(mvsc2)
static NaomiZipEntry mvsc2ZipEntries[] = {
    ZIPENTRY("epr-23085a.ic11", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23048.ic17s", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23049.ic18", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23050.ic19s", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23051.ic20", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23052.ic21s", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-23053.ic22", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-23054.ic23s", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-23055.ic24", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-23056.ic25s", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-23057.ic26", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-23058.ic27s", NAOMI_BIOS_COUNT + 11),
    ZIPENTRY("mpr-23059.ic28", NAOMI_BIOS_COUNT + 12),
    ZIPENTRY("mpr-23060.ic29", NAOMI_BIOS_COUNT + 13),
    ZIPENTRY("mpr-23061.ic30s", NAOMI_BIOS_COUNT + 14),
    ZIPENTRY("mpr-23083.ic31", NAOMI_BIOS_COUNT + 15),
    ZIPENTRY("mpr-23083.ic31", NAOMI_BIOS_COUNT + 16),
    ZIPENTRY("mpr-23084.ic32s", NAOMI_BIOS_COUNT + 17),
    ZIPENTRY("25lc040.ic13s", NAOMI_BIOS_COUNT + 18),
    ZIPEND
};
NAOMI_INIT_FUNC(mvsc2, mvsc2ZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomimvsc2 = {
    "mvsc2", NULL, "naomi", NULL, "2000", "Marvel Vs. Capcom 2: New Age of Heroes (Export, Korea, Rev A)" "\0", NULL, "Capcom / Marvel", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, mvsc2RomInfo, mvsc2RomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    mvsc2Init, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Marvel Vs. Capcom 2: New Age of Heroes (USA, Rev A)
static struct BurnRomInfo mvsc2uRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23062a.ic22", 0x0400000, 0x96038276, BRF_PRG | BRF_ESS },
    { "mpr-23048.ic1", 0x0800000, 0x93d7a63a, BRF_PRG | BRF_ESS },
    { "mpr-23049.ic2", 0x0800000, 0x003dcce0, BRF_PRG | BRF_ESS },
    { "mpr-23050.ic3", 0x0800000, 0x1d6b88a7, BRF_PRG | BRF_ESS },
    { "mpr-23051.ic4", 0x0800000, 0x01226aaa, BRF_PRG | BRF_ESS },
    { "mpr-23052.ic5", 0x0800000, 0x74bee120, BRF_PRG | BRF_ESS },
    { "mpr-23053.ic6", 0x0800000, 0xd92d4401, BRF_PRG | BRF_ESS },
    { "mpr-23054.ic7", 0x0800000, 0x78ba02e8, BRF_PRG | BRF_ESS },
    { "mpr-23055.ic8", 0x0800000, 0x84319604, BRF_PRG | BRF_ESS },
    { "mpr-23056.ic9", 0x0800000, 0xd7386034, BRF_PRG | BRF_ESS },
    { "mpr-23057.ic10", 0x0800000, 0xa3f087db, BRF_PRG | BRF_ESS },
    { "mpr-23058.ic11", 0x0800000, 0x61a6cc5d, BRF_PRG | BRF_ESS },
    { "mpr-23059.ic12s", 0x0800000, 0x64808024, BRF_PRG | BRF_ESS },
    { "mpr-23060.ic13s", 0x0800000, 0x67519942, BRF_PRG | BRF_ESS },
    { "mpr-23061.ic14s", 0x0800000, 0xfb1844c4, BRF_PRG | BRF_ESS },
    { "sflash.ic37", 0x0000084, 0x37a66f3c, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(mvsc2u)
STD_ROM_FN(mvsc2u)
static NaomiZipEntry mvsc2uZipEntries[] = {
    ZIPENTRY("epr-23062a.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23048.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23049.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23050.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23051.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23052.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-23053.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-23054.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-23055.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-23056.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-23057.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-23058.ic11", NAOMI_BIOS_COUNT + 11),
    ZIPENTRY("mpr-23059.ic12s", NAOMI_BIOS_COUNT + 12),
    ZIPENTRY("mpr-23060.ic13s", NAOMI_BIOS_COUNT + 13),
    ZIPENTRY("mpr-23061.ic14s", NAOMI_BIOS_COUNT + 14),
    ZIPENTRY("sflash.ic37", NAOMI_BIOS_COUNT + 15),
    ZIPEND
};
NAOMI_INIT_FUNC(mvsc2u, mvsc2uZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomimvsc2u = {
    "mvsc2u", "mvsc2", "naomi", NULL, "2000", "Marvel Vs. Capcom 2: New Age of Heroes (USA, Rev A)" "\0", NULL, "Capcom / Marvel", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, mvsc2uRomInfo, mvsc2uRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    mvsc2uInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Power Stone 2
static struct BurnRomInfo pstone2RomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23127.ic22", 0x0400000, 0x185761d6, BRF_PRG | BRF_ESS },
    { "mpr-23118.ic1", 0x0800000, 0xc69f3c3c, BRF_PRG | BRF_ESS },
    { "mpr-23119.ic2", 0x0800000, 0xa80d444d, BRF_PRG | BRF_ESS },
    { "mpr-23120.ic3", 0x0800000, 0xc285dd64, BRF_PRG | BRF_ESS },
    { "mpr-23121.ic4", 0x0800000, 0x1f3f6505, BRF_PRG | BRF_ESS },
    { "mpr-23122.ic5", 0x0800000, 0x5e403a12, BRF_PRG | BRF_ESS },
    { "mpr-23123.ic6", 0x0800000, 0x4b71078b, BRF_PRG | BRF_ESS },
    { "mpr-23124.ic7", 0x0800000, 0x508c0207, BRF_PRG | BRF_ESS },
    { "mpr-23125.ic8", 0x0800000, 0xb9938bbc, BRF_PRG | BRF_ESS },
    { "mpr-23126.ic9", 0x0800000, 0xfbb0325b, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(pstone2)
STD_ROM_FN(pstone2)
static NaomiZipEntry pstone2ZipEntries[] = {
    ZIPENTRY("epr-23127.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23118.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23119.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23120.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23121.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23122.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-23123.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-23124.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-23125.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-23126.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPEND
};
NAOMI_INIT_FUNC(pstone2, pstone2ZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomipstone2 = {
    "pstone2", NULL, "naomi", NULL, "2000", "Power Stone 2" "\0", NULL, "Capcom", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, pstone2RomInfo, pstone2RomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    pstone2Init, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Capcom Vs. SNK: Millennium Fight 2000 (Rev C)
static struct BurnRomInfo capsnkRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23511c.ic22", 0x0400000, 0x3dbf8eb2, BRF_PRG | BRF_ESS },
    { "mpr-23504.ic1", 0x1000000, 0xe01a31d2, BRF_PRG | BRF_ESS },
    { "mpr-23505.ic2", 0x1000000, 0x3a34d5fe, BRF_PRG | BRF_ESS },
    { "mpr-23506.ic3", 0x1000000, 0x9cbab27d, BRF_PRG | BRF_ESS },
    { "mpr-23507.ic4", 0x1000000, 0x363c1734, BRF_PRG | BRF_ESS },
    { "mpr-23508.ic5", 0x1000000, 0x0a3590aa, BRF_PRG | BRF_ESS },
    { "mpr-23509.ic6", 0x1000000, 0x281d633d, BRF_PRG | BRF_ESS },
    { "mpr-23510.ic7", 0x1000000, 0xb856fef5, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(capsnk)
STD_ROM_FN(capsnk)
static NaomiZipEntry capsnkZipEntries[] = {
    ZIPENTRY("epr-23511c.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23504.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23505.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23506.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23507.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23508.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-23509.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-23510.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPEND
};
NAOMI_INIT_FUNC(capsnk, capsnkZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomicapsnk = {
    "capsnk", NULL, "naomi", NULL, "2000", "Capcom Vs. SNK: Millennium Fight 2000 (Rev C)" "\0", NULL, "Capcom / SNK", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, capsnkRomInfo, capsnkRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    capsnkInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Cannon Spike / Gun Spike
static struct BurnRomInfo cspikeRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23210.ic22", 0x0400000, 0xa15c54b5, BRF_PRG | BRF_ESS },
    { "mpr-23198.ic1", 0x0800000, 0xce8d3edf, BRF_PRG | BRF_ESS },
    { "mpr-23199.ic2", 0x0800000, 0x0979392a, BRF_PRG | BRF_ESS },
    { "mpr-23200.ic3", 0x0800000, 0xe4b2db33, BRF_PRG | BRF_ESS },
    { "mpr-23201.ic4", 0x0800000, 0xc55ca0fa, BRF_PRG | BRF_ESS },
    { "mpr-23202.ic5", 0x0800000, 0x983bb21c, BRF_PRG | BRF_ESS },
    { "mpr-23203.ic6", 0x0800000, 0xf61b8d96, BRF_PRG | BRF_ESS },
    { "mpr-23204.ic7", 0x0800000, 0x03593ecd, BRF_PRG | BRF_ESS },
    { "mpr-23205.ic8", 0x0800000, 0xe8c9349b, BRF_PRG | BRF_ESS },
    { "mpr-23206.ic9", 0x0800000, 0x8089d80f, BRF_PRG | BRF_ESS },
    { "mpr-23207.ic10", 0x0800000, 0x39f692a1, BRF_PRG | BRF_ESS },
    { "mpr-23208.ic11", 0x0800000, 0xb9494f4b, BRF_PRG | BRF_ESS },
    { "mpr-23209.ic12s", 0x0800000, 0x560188c0, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(cspike)
STD_ROM_FN(cspike)
static NaomiZipEntry cspikeZipEntries[] = {
    ZIPENTRY("epr-23210.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23198.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23199.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23200.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23201.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23202.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-23203.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-23204.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-23205.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-23206.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-23207.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-23208.ic11", NAOMI_BIOS_COUNT + 11),
    ZIPENTRY("mpr-23209.ic12s", NAOMI_BIOS_COUNT + 12),
    ZIPEND
};
NAOMI_INIT_FUNC(cspike, cspikeZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomicspike = {
    "cspike", NULL, "naomi", NULL, "2000", "Cannon Spike / Gun Spike" "\0", NULL, "Psikyo / Capcom", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, cspikeRomInfo, cspikeRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    cspikeInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Guilty Gear X
static struct BurnRomInfo ggxRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23356.ic22", 0x0400000, 0xed2d289f, BRF_PRG | BRF_ESS },
    { "mpr-23342.ic1", 0x0800000, 0x4fd89557, BRF_PRG | BRF_ESS },
    { "mpr-23343.ic2", 0x0800000, 0x2e4417b6, BRF_PRG | BRF_ESS },
    { "mpr-23344.ic3", 0x0800000, 0x968eea3b, BRF_PRG | BRF_ESS },
    { "mpr-23345.ic4", 0x0800000, 0x30efe1ec, BRF_PRG | BRF_ESS },
    { "mpr-23346.ic5", 0x0800000, 0xb34d9461, BRF_PRG | BRF_ESS },
    { "mpr-23347.ic6", 0x0800000, 0x5a254cd1, BRF_PRG | BRF_ESS },
    { "mpr-23348.ic7", 0x0800000, 0xaff43142, BRF_PRG | BRF_ESS },
    { "mpr-23349.ic8", 0x0800000, 0xe83871c7, BRF_PRG | BRF_ESS },
    { "mpr-23350.ic9", 0x0800000, 0x4237010b, BRF_PRG | BRF_ESS },
    { "mpr-23351.ic10", 0x0800000, 0xb096f712, BRF_PRG | BRF_ESS },
    { "mpr-23352.ic11", 0x0800000, 0x1a01ab38, BRF_PRG | BRF_ESS },
    { "mpr-23353.ic12s", 0x0800000, 0xdaa0ca24, BRF_PRG | BRF_ESS },
    { "mpr-23354.ic13s", 0x0800000, 0xcea127f7, BRF_PRG | BRF_ESS },
    { "mpr-23355.ic14s", 0x0800000, 0xe809685f, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(ggx)
STD_ROM_FN(ggx)
static NaomiZipEntry ggxZipEntries[] = {
    ZIPENTRY("epr-23356.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23342.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23343.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23344.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23345.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23346.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-23347.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-23348.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-23349.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-23350.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-23351.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-23352.ic11", NAOMI_BIOS_COUNT + 11),
    ZIPENTRY("mpr-23353.ic12s", NAOMI_BIOS_COUNT + 12),
    ZIPENTRY("mpr-23354.ic13s", NAOMI_BIOS_COUNT + 13),
    ZIPENTRY("mpr-23355.ic14s", NAOMI_BIOS_COUNT + 14),
    ZIPEND
};
NAOMI_INIT_FUNC(ggx, ggxZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomiggx = {
    "ggx", NULL, "naomi", NULL, "2000", "Guilty Gear X" "\0", NULL, "Arc System Works", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, ggxRomInfo, ggxRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    ggxInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Giga Wing 2
static struct BurnRomInfo gwing2RomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-22270.ic22", 0x0200000, 0x876b3c97, BRF_PRG | BRF_ESS },
    { "mpr-22271.ic1", 0x1000000, 0x9a072af5, BRF_PRG | BRF_ESS },
    { "mpr-22272.ic2", 0x1000000, 0x1e816ab1, BRF_PRG | BRF_ESS },
    { "mpr-22273.ic3", 0x1000000, 0xcd633dcf, BRF_PRG | BRF_ESS },
    { "mpr-22274.ic4", 0x1000000, 0xf8daaaf3, BRF_PRG | BRF_ESS },
    { "mpr-22275.ic5", 0x1000000, 0x61aa1521, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(gwing2)
STD_ROM_FN(gwing2)
static NaomiZipEntry gwing2ZipEntries[] = {
    ZIPENTRY("epr-22270.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-22271.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-22272.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-22273.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-22274.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-22275.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPEND
};
NAOMI_INIT_FUNC(gwing2, gwing2ZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomigwing2 = {
    "gwing2", NULL, "naomi", NULL, "2000", "Giga Wing 2" "\0", NULL, "Takumi / Capcom", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, gwing2RomInfo, gwing2RomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    gwing2Init, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Project Justice / Moero! Justice Gakuen (Rev B)
static struct BurnRomInfo pjusticRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23548b.ic22", 0x0400000, 0xd0dbdf40, BRF_PRG | BRF_ESS },
    { "mpr-23537.ic1", 0x1000000, 0xa2462770, BRF_PRG | BRF_ESS },
    { "mpr-23538.ic2", 0x1000000, 0xe4480832, BRF_PRG | BRF_ESS },
    { "mpr-23539.ic3", 0x1000000, 0x97e3f7f5, BRF_PRG | BRF_ESS },
    { "mpr-23540.ic4", 0x1000000, 0xb9e92d21, BRF_PRG | BRF_ESS },
    { "mpr-23541.ic5", 0x1000000, 0x95b8a9c6, BRF_PRG | BRF_ESS },
    { "mpr-23542.ic6", 0x1000000, 0xdfd490f5, BRF_PRG | BRF_ESS },
    { "mpr-23543.ic7", 0x1000000, 0x66847ebd, BRF_PRG | BRF_ESS },
    { "mpr-23544.ic8", 0x1000000, 0xd1f5b460, BRF_PRG | BRF_ESS },
    { "mpr-23545.ic9", 0x1000000, 0x60bd692f, BRF_PRG | BRF_ESS },
    { "mpr-23546.ic10", 0x1000000, 0x85db2248, BRF_PRG | BRF_ESS },
    { "mpr-23547.ic11", 0x1000000, 0x18b369c7, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(pjustic)
STD_ROM_FN(pjustic)
static NaomiZipEntry pjusticZipEntries[] = {
    ZIPENTRY("epr-23548b.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23537.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23538.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23539.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23540.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23541.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-23542.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-23543.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-23544.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-23545.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-23546.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-23547.ic11", NAOMI_BIOS_COUNT + 11),
    ZIPEND
};
NAOMI_INIT_FUNC(pjustic, pjusticZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomipjustic = {
    "pjustic", NULL, "naomi", NULL, "2000", "Project Justice / Moero! Justice Gakuen (Rev B)" "\0", NULL, "Capcom", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, pjusticRomInfo, pjusticRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    pjusticInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Zero Gunner 2
static struct BurnRomInfo zerogu2RomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-23689.ic22", 0x0400000, 0xba42267c, BRF_PRG | BRF_ESS },
    { "mpr-23684.ic1", 0x1000000, 0x035aec98, BRF_PRG | BRF_ESS },
    { "mpr-23685.ic2", 0x1000000, 0xd878ff99, BRF_PRG | BRF_ESS },
    { "mpr-23686.ic3", 0x1000000, 0xa61b4d49, BRF_PRG | BRF_ESS },
    { "mpr-23687.ic4", 0x1000000, 0xe125439a, BRF_PRG | BRF_ESS },
    { "mpr-23688.ic5", 0x1000000, 0x38412edf, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(zerogu2)
STD_ROM_FN(zerogu2)
static NaomiZipEntry zerogu2ZipEntries[] = {
    ZIPENTRY("epr-23689.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-23684.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-23685.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-23686.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-23687.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-23688.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPEND
};
NAOMI_INIT_FUNC(zerogu2, zerogu2ZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomizerogu2 = {
    "zerogu2", NULL, "naomi", NULL, "2001", "Zero Gunner 2" "\0", NULL, "Psikyo", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, zerogu2RomInfo, zerogu2RomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    zerogu2Init, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Virtua Tennis / Power Smash
static struct BurnRomInfo vtennisRomDesc[] = {
    NAOMI_BIOS_ROMS,
    { "epr-22927.ic22", 0x0400000, 0x89781723, BRF_PRG | BRF_ESS },
    { "mpr-22916.ic1", 0x0800000, 0x903873e5, BRF_PRG | BRF_ESS },
    { "mpr-22917.ic2", 0x0800000, 0x5f020fa6, BRF_PRG | BRF_ESS },
    { "mpr-22918.ic3", 0x0800000, 0x3c3bf533, BRF_PRG | BRF_ESS },
    { "mpr-22919.ic4", 0x0800000, 0x3d8dd003, BRF_PRG | BRF_ESS },
    { "mpr-22920.ic5", 0x0800000, 0xefd781d4, BRF_PRG | BRF_ESS },
    { "mpr-22921.ic6", 0x0800000, 0x79e75be1, BRF_PRG | BRF_ESS },
    { "mpr-22922.ic7", 0x0800000, 0x44bd3883, BRF_PRG | BRF_ESS },
    { "mpr-22923.ic8", 0x0800000, 0x9ebdf0f8, BRF_PRG | BRF_ESS },
    { "mpr-22924.ic9", 0x0800000, 0xecde9d57, BRF_PRG | BRF_ESS },
    { "mpr-22925.ic10", 0x0800000, 0x81057e42, BRF_PRG | BRF_ESS },
    { "mpr-22926.ic11", 0x0800000, 0x57eec89d, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(vtennis)
STD_ROM_FN(vtennis)
static NaomiZipEntry vtennisZipEntries[] = {
    ZIPENTRY("epr-22927.ic22", NAOMI_BIOS_COUNT + 0),
    ZIPENTRY("mpr-22916.ic1", NAOMI_BIOS_COUNT + 1),
    ZIPENTRY("mpr-22917.ic2", NAOMI_BIOS_COUNT + 2),
    ZIPENTRY("mpr-22918.ic3", NAOMI_BIOS_COUNT + 3),
    ZIPENTRY("mpr-22919.ic4", NAOMI_BIOS_COUNT + 4),
    ZIPENTRY("mpr-22920.ic5", NAOMI_BIOS_COUNT + 5),
    ZIPENTRY("mpr-22921.ic6", NAOMI_BIOS_COUNT + 6),
    ZIPENTRY("mpr-22922.ic7", NAOMI_BIOS_COUNT + 7),
    ZIPENTRY("mpr-22923.ic8", NAOMI_BIOS_COUNT + 8),
    ZIPENTRY("mpr-22924.ic9", NAOMI_BIOS_COUNT + 9),
    ZIPENTRY("mpr-22925.ic10", NAOMI_BIOS_COUNT + 10),
    ZIPENTRY("mpr-22926.ic11", NAOMI_BIOS_COUNT + 11),
    ZIPEND
};
NAOMI_INIT_FUNC(vtennis, vtennisZipEntries, NAOMI_GAME_INPUT_DIGITAL, "naomi.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomivtennis = {
    "vtennis", NULL, "naomi", NULL, "1999", "Virtua Tennis / Power Smash" "\0", NULL, "Sega", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, vtennisRomInfo, vtennisRomName, NULL, NULL, NULL, NULL, Naomi2PInputInfo, NULL,
    vtennisInit, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// The House of the Dead 2 (USA)
static struct BurnRomInfo hotd2RomDesc[] = {
    HOTD2_BIOS_ROMS,
    { "epr-21585.ic22", 0x0200000, 0xb23d1a0c, BRF_PRG | BRF_ESS },
    { "mpr-21386.ic1", 0x0800000, 0x88fb0562, BRF_PRG | BRF_ESS },
    { "mpr-21387.ic2", 0x0800000, 0x5f4dd576, BRF_PRG | BRF_ESS },
    { "mpr-21388.ic3", 0x0800000, 0x3e62fca4, BRF_PRG | BRF_ESS },
    { "mpr-21389.ic4", 0x0800000, 0x6f73a852, BRF_PRG | BRF_ESS },
    { "mpr-21390.ic5", 0x0800000, 0xc7950445, BRF_PRG | BRF_ESS },
    { "mpr-21391.ic6", 0x0800000, 0x5a812247, BRF_PRG | BRF_ESS },
    { "mpr-21392.ic7", 0x0800000, 0x17e9414a, BRF_PRG | BRF_ESS },
    { "mpr-21393.ic8", 0x0800000, 0x5d2d8134, BRF_PRG | BRF_ESS },
    { "mpr-21394.ic9", 0x0800000, 0xeacaf26d, BRF_PRG | BRF_ESS },
    { "mpr-21395.ic10", 0x0800000, 0x1e3686be, BRF_PRG | BRF_ESS },
    { "mpr-21396.ic11", 0x0800000, 0x5ada00a2, BRF_PRG | BRF_ESS },
    { "mpr-21397.ic12s", 0x0800000, 0x9eff6247, BRF_PRG | BRF_ESS },
    { "mpr-21398.ic13s", 0x0800000, 0x8a80b16a, BRF_PRG | BRF_ESS },
    { "mpr-21399.ic14s", 0x0800000, 0x7ae20daf, BRF_PRG | BRF_ESS },
    { "mpr-21400.ic15s", 0x0800000, 0xfbb8641b, BRF_PRG | BRF_ESS },
    { "mpr-21401.ic16s", 0x0800000, 0x3881ec23, BRF_PRG | BRF_ESS },
    { "mpr-21402.ic17s", 0x0800000, 0x66bff6e4, BRF_PRG | BRF_ESS },
    { "mpr-21403.ic18s", 0x0800000, 0x8cd2f654, BRF_PRG | BRF_ESS },
    { "mpr-21404.ic19s", 0x0800000, 0x6cf6e705, BRF_PRG | BRF_ESS },
    { "mpr-21405.ic20s", 0x0800000, 0x495e6265, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(hotd2)
STD_ROM_FN(hotd2)
static NaomiZipEntry hotd2ZipEntries[] = {
    ZIPENTRY("epr-21585.ic22", HOTD2_BIOS_COUNT + 0),
    ZIPENTRY("mpr-21386.ic1", HOTD2_BIOS_COUNT + 1),
    ZIPENTRY("mpr-21387.ic2", HOTD2_BIOS_COUNT + 2),
    ZIPENTRY("mpr-21388.ic3", HOTD2_BIOS_COUNT + 3),
    ZIPENTRY("mpr-21389.ic4", HOTD2_BIOS_COUNT + 4),
    ZIPENTRY("mpr-21390.ic5", HOTD2_BIOS_COUNT + 5),
    ZIPENTRY("mpr-21391.ic6", HOTD2_BIOS_COUNT + 6),
    ZIPENTRY("mpr-21392.ic7", HOTD2_BIOS_COUNT + 7),
    ZIPENTRY("mpr-21393.ic8", HOTD2_BIOS_COUNT + 8),
    ZIPENTRY("mpr-21394.ic9", HOTD2_BIOS_COUNT + 9),
    ZIPENTRY("mpr-21395.ic10", HOTD2_BIOS_COUNT + 10),
    ZIPENTRY("mpr-21396.ic11", HOTD2_BIOS_COUNT + 11),
    ZIPENTRY("mpr-21397.ic12s", HOTD2_BIOS_COUNT + 12),
    ZIPENTRY("mpr-21398.ic13s", HOTD2_BIOS_COUNT + 13),
    ZIPENTRY("mpr-21399.ic14s", HOTD2_BIOS_COUNT + 14),
    ZIPENTRY("mpr-21400.ic15s", HOTD2_BIOS_COUNT + 15),
    ZIPENTRY("mpr-21401.ic16s", HOTD2_BIOS_COUNT + 16),
    ZIPENTRY("mpr-21402.ic17s", HOTD2_BIOS_COUNT + 17),
    ZIPENTRY("mpr-21403.ic18s", HOTD2_BIOS_COUNT + 18),
    ZIPENTRY("mpr-21404.ic19s", HOTD2_BIOS_COUNT + 19),
    ZIPENTRY("mpr-21405.ic20s", HOTD2_BIOS_COUNT + 20),
    ZIPEND
};
NAOMI_INIT_FUNC(hotd2, hotd2ZipEntries, NAOMI_GAME_INPUT_LIGHTGUN, "hod2bios.zip", "naomi", "naomi", AWAVE_PLATFORM_NAOMI)
struct BurnDriver BurnDrvNaomihotd2 = {
    "hotd2", NULL, "hod2bios", NULL, "1998", "The House of the Dead 2 (USA)" "\0", NULL, "Sega", "Sega NAOMI",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
    NULL, hotd2RomInfo, hotd2RomName, NULL, NULL, NULL, NULL, NaomiLightgunInputInfo, NULL,
    hotd2Init, NaomiExitCommon, NaomiFrameCommon, NaomiDrawCommon, NaomiScanCommon, &NaomiRecalc,
    0x1000000, 640, 480, 4, 3
};
// Stable Atomiswave FBNeo driver bridge.
// This file intentionally contains only FBNeo/Burn declarations and ROM metadata.
// Flycast integration lives behind awave_core.cpp + flycast_shim/fbneo_flycast_api.*.

#include "burnint.h"
#include "awave_core.h"
#include "awave_platform.h"

static UINT8 AwaveRecalc = 0;
static UINT8 AwaveReset = 0;
static UINT8 AwaveJoy1[16];
static UINT8 AwaveJoy2[16];
static UINT8 AwaveJoy3[16];
static UINT8 AwaveJoy4[16];
static INT16 AwaveAnalog[4][4];

static struct BurnInputInfo Awave2PInputList[] = {
    { "P1 Coin",    BIT_DIGITAL, AwaveJoy1 + 0,  "p1 coin"   },
    { "P1 Start",   BIT_DIGITAL, AwaveJoy1 + 1,  "p1 start"  },
    { "P1 Up",      BIT_DIGITAL, AwaveJoy1 + 2,  "p1 up"     },
    { "P1 Down",    BIT_DIGITAL, AwaveJoy1 + 3,  "p1 down"   },
    { "P1 Left",    BIT_DIGITAL, AwaveJoy1 + 4,  "p1 left"   },
    { "P1 Right",   BIT_DIGITAL, AwaveJoy1 + 5,  "p1 right"  },
    { "P1 Button 1",BIT_DIGITAL, AwaveJoy1 + 6,  "p1 fire 1" },
    { "P1 Button 2",BIT_DIGITAL, AwaveJoy1 + 7,  "p1 fire 2" },
    { "P1 Button 3",BIT_DIGITAL, AwaveJoy1 + 8,  "p1 fire 3" },
    { "P1 Button 4",BIT_DIGITAL, AwaveJoy1 + 9,  "p1 fire 4" },
    { "P1 Button 5",BIT_DIGITAL, AwaveJoy1 + 10, "p1 fire 5" },
    { "P1 Service", BIT_DIGITAL, AwaveJoy1 + 11, "service"   },

    { "P2 Coin",    BIT_DIGITAL, AwaveJoy2 + 0,  "p2 coin"   },
    { "P2 Start",   BIT_DIGITAL, AwaveJoy2 + 1,  "p2 start"  },
    { "P2 Up",      BIT_DIGITAL, AwaveJoy2 + 2,  "p2 up"     },
    { "P2 Down",    BIT_DIGITAL, AwaveJoy2 + 3,  "p2 down"   },
    { "P2 Left",    BIT_DIGITAL, AwaveJoy2 + 4,  "p2 left"   },
    { "P2 Right",   BIT_DIGITAL, AwaveJoy2 + 5,  "p2 right"  },
    { "P2 Button 1",BIT_DIGITAL, AwaveJoy2 + 6,  "p2 fire 1" },
    { "P2 Button 2",BIT_DIGITAL, AwaveJoy2 + 7,  "p2 fire 2" },
    { "P2 Button 3",BIT_DIGITAL, AwaveJoy2 + 8,  "p2 fire 3" },
    { "P2 Button 4",BIT_DIGITAL, AwaveJoy2 + 9,  "p2 fire 4" },
    { "P2 Button 5",BIT_DIGITAL, AwaveJoy2 + 10, "p2 fire 5" },
    { "P2 Service", BIT_DIGITAL, AwaveJoy2 + 11, "service"   },

    { "Test",       BIT_DIGITAL, AwaveJoy1 + 12, "diag"      },
    { "Reset",      BIT_DIGITAL, &AwaveReset,    "reset"     },
};
STDINPUTINFO(Awave2P)

static struct BurnInputInfo Awave4PInputList[] = {
    { "P1 Coin",    BIT_DIGITAL, AwaveJoy1 + 0,  "p1 coin"   }, { "P1 Start", BIT_DIGITAL, AwaveJoy1 + 1, "p1 start" },
    { "P1 Up",      BIT_DIGITAL, AwaveJoy1 + 2,  "p1 up"     }, { "P1 Down",  BIT_DIGITAL, AwaveJoy1 + 3, "p1 down"  },
    { "P1 Left",    BIT_DIGITAL, AwaveJoy1 + 4,  "p1 left"   }, { "P1 Right", BIT_DIGITAL, AwaveJoy1 + 5, "p1 right" },
    { "P1 Button 1",BIT_DIGITAL, AwaveJoy1 + 6,  "p1 fire 1" }, { "P1 Button 2",BIT_DIGITAL,AwaveJoy1 + 7, "p1 fire 2" },
    { "P1 Button 3",BIT_DIGITAL, AwaveJoy1 + 8,  "p1 fire 3" }, { "P1 Button 4",BIT_DIGITAL,AwaveJoy1 + 9, "p1 fire 4" },
    { "P2 Coin",    BIT_DIGITAL, AwaveJoy2 + 0,  "p2 coin"   }, { "P2 Start", BIT_DIGITAL, AwaveJoy2 + 1, "p2 start" },
    { "P2 Up",      BIT_DIGITAL, AwaveJoy2 + 2,  "p2 up"     }, { "P2 Down",  BIT_DIGITAL, AwaveJoy2 + 3, "p2 down"  },
    { "P2 Left",    BIT_DIGITAL, AwaveJoy2 + 4,  "p2 left"   }, { "P2 Right", BIT_DIGITAL, AwaveJoy2 + 5, "p2 right" },
    { "P2 Button 1",BIT_DIGITAL, AwaveJoy2 + 6,  "p2 fire 1" }, { "P2 Button 2",BIT_DIGITAL,AwaveJoy2 + 7, "p2 fire 2" },
    { "P2 Button 3",BIT_DIGITAL, AwaveJoy2 + 8,  "p2 fire 3" }, { "P2 Button 4",BIT_DIGITAL,AwaveJoy2 + 9, "p2 fire 4" },
    { "P3 Coin",    BIT_DIGITAL, AwaveJoy3 + 0,  "p3 coin"   }, { "P3 Start", BIT_DIGITAL, AwaveJoy3 + 1, "p3 start" },
    { "P3 Up",      BIT_DIGITAL, AwaveJoy3 + 2,  "p3 up"     }, { "P3 Down",  BIT_DIGITAL, AwaveJoy3 + 3, "p3 down"  },
    { "P3 Left",    BIT_DIGITAL, AwaveJoy3 + 4,  "p3 left"   }, { "P3 Right", BIT_DIGITAL, AwaveJoy3 + 5, "p3 right" },
    { "P3 Button 1",BIT_DIGITAL, AwaveJoy3 + 6,  "p3 fire 1" }, { "P3 Button 2",BIT_DIGITAL,AwaveJoy3 + 7, "p3 fire 2" },
    { "P3 Button 3",BIT_DIGITAL, AwaveJoy3 + 8,  "p3 fire 3" }, { "P3 Button 4",BIT_DIGITAL,AwaveJoy3 + 9, "p3 fire 4" },
    { "P4 Coin",    BIT_DIGITAL, AwaveJoy4 + 0,  "p4 coin"   }, { "P4 Start", BIT_DIGITAL, AwaveJoy4 + 1, "p4 start" },
    { "P4 Up",      BIT_DIGITAL, AwaveJoy4 + 2,  "p4 up"     }, { "P4 Down",  BIT_DIGITAL, AwaveJoy4 + 3, "p4 down"  },
    { "P4 Left",    BIT_DIGITAL, AwaveJoy4 + 4,  "p4 left"   }, { "P4 Right", BIT_DIGITAL, AwaveJoy4 + 5, "p4 right" },
    { "P4 Button 1",BIT_DIGITAL, AwaveJoy4 + 6,  "p4 fire 1" }, { "P4 Button 2",BIT_DIGITAL,AwaveJoy4 + 7, "p4 fire 2" },
    { "P4 Button 3",BIT_DIGITAL, AwaveJoy4 + 8,  "p4 fire 3" }, { "P4 Button 4",BIT_DIGITAL,AwaveJoy4 + 9, "p4 fire 4" },
    { "Test",       BIT_DIGITAL, AwaveJoy1 + 12, "diag"      }, { "Reset", BIT_DIGITAL, &AwaveReset, "reset" },
};
STDINPUTINFO(Awave4P)

static struct BurnInputInfo AwaveRacingInputList[] = {
    { "P1 Coin",    BIT_DIGITAL, AwaveJoy1 + 0, "p1 coin"   },
    { "P1 Start",   BIT_DIGITAL, AwaveJoy1 + 1, "p1 start"  },
    { "P1 Button 1",BIT_DIGITAL, AwaveJoy1 + 6, "p1 fire 1" },
    { "P1 Button 2",BIT_DIGITAL, AwaveJoy1 + 7, "p1 fire 2" },
    { "P1 Steering",BIT_ANALOG_REL, (UINT8*)&AwaveAnalog[0][0], "p1 x-axis" },
    { "P1 Gas",     BIT_ANALOG_REL, (UINT8*)&AwaveAnalog[0][1], "p1 z-axis" },
    { "P1 Brake",   BIT_ANALOG_REL, (UINT8*)&AwaveAnalog[0][2], "p1 rz-axis" },
    { "Test",       BIT_DIGITAL, AwaveJoy1 + 12, "diag" },
    { "Reset",      BIT_DIGITAL, &AwaveReset, "reset" },
};
STDINPUTINFO(AwaveRacing)

static struct BurnInputInfo AwaveLightgunInputList[] = {
    { "P1 Coin",    BIT_DIGITAL, AwaveJoy1 + 0, "p1 coin"   },
    { "P1 Start",   BIT_DIGITAL, AwaveJoy1 + 1, "p1 start"  },
    { "P1 Trigger", BIT_DIGITAL, AwaveJoy1 + 6, "p1 fire 1" },
    { "P1 Reload",  BIT_DIGITAL, AwaveJoy1 + 7, "p1 fire 2" },
    { "P1 X",       BIT_ANALOG_REL, (UINT8*)&AwaveAnalog[0][0], "mouse x-axis" },
    { "P1 Y",       BIT_ANALOG_REL, (UINT8*)&AwaveAnalog[0][1], "mouse y-axis" },
    { "P2 Coin",    BIT_DIGITAL, AwaveJoy2 + 0, "p2 coin"   },
    { "P2 Start",   BIT_DIGITAL, AwaveJoy2 + 1, "p2 start"  },
    { "P2 Trigger", BIT_DIGITAL, AwaveJoy2 + 6, "p2 fire 1" },
    { "P2 Reload",  BIT_DIGITAL, AwaveJoy2 + 7, "p2 fire 2" },
    { "P2 X",       BIT_ANALOG_REL, (UINT8*)&AwaveAnalog[1][0], "p2 x-axis" },
    { "P2 Y",       BIT_ANALOG_REL, (UINT8*)&AwaveAnalog[1][1], "p2 y-axis" },
    { "Test",       BIT_DIGITAL, AwaveJoy1 + 12, "diag" },
    { "Reset",      BIT_DIGITAL, &AwaveReset, "reset" },
};
STDINPUTINFO(AwaveLightgun)

static void AwaveClearInputs()
{
    memset(AwaveJoy1, 0, sizeof(AwaveJoy1));
    memset(AwaveJoy2, 0, sizeof(AwaveJoy2));
    memset(AwaveJoy3, 0, sizeof(AwaveJoy3));
    memset(AwaveJoy4, 0, sizeof(AwaveJoy4));
    memset(AwaveAnalog, 0, sizeof(AwaveAnalog));
}

static UINT32 PackDigital(const UINT8* joy)
{
    UINT32 s = 0;
    if (joy[0])  s |= AWAVE_INPUT_COIN;
    if (joy[1])  s |= AWAVE_INPUT_START;
    if (joy[2])  s |= AWAVE_INPUT_UP;
    if (joy[3])  s |= AWAVE_INPUT_DOWN;
    if (joy[4])  s |= AWAVE_INPUT_LEFT;
    if (joy[5])  s |= AWAVE_INPUT_RIGHT;
    if (joy[6])  s |= AWAVE_INPUT_BTN1 | AWAVE_INPUT_TRIGGER;
    if (joy[7])  s |= AWAVE_INPUT_BTN2;
    if (joy[8])  s |= AWAVE_INPUT_BTN3;
    if (joy[9])  s |= AWAVE_INPUT_BTN4;
    if (joy[10]) s |= AWAVE_INPUT_BTN5;
    if (joy[11]) s |= AWAVE_INPUT_SERVICE;
    if (joy[12]) s |= AWAVE_INPUT_TEST;
    return s;
}

static INT32 AwaveFrameCommon()
{
    if (AwaveReset) NaomiCoreReset();
    NaomiCoreSetPadState(0, PackDigital(AwaveJoy1));
    NaomiCoreSetPadState(1, PackDigital(AwaveJoy2));
    NaomiCoreSetPadState(2, PackDigital(AwaveJoy3));
    NaomiCoreSetPadState(3, PackDigital(AwaveJoy4));
    NaomiCoreSetAnalogState(0, 0, AwaveAnalog[0][0]);
    NaomiCoreSetAnalogState(0, 1, AwaveAnalog[0][1]);
    NaomiCoreSetAnalogState(0, 2, AwaveAnalog[0][2]);
    NaomiCoreSetLightgunState(0, AwaveAnalog[0][0], AwaveAnalog[0][1], AwaveJoy1[7], AwaveJoy1[7]);
    NaomiCoreSetLightgunState(1, AwaveAnalog[1][0], AwaveAnalog[1][1], AwaveJoy2[7], AwaveJoy2[7]);
    return NaomiCoreFrame();
}

static INT32 AwaveExitCommon() { AwaveClearInputs(); return NaomiCoreExit(); }
static INT32 AwaveDrawCommon() { return NaomiCoreDraw(); }
static INT32 AwaveScanCommon(INT32 nAction, INT32* pnMin) { return NaomiCoreScan(nAction, pnMin); }

#define AWAVE_BIOS_ROMS \
    { "bios0.ic23",      0x020000, 0x719b2b0b, BRF_ESS | BRF_BIOS }, \
    { "bios1.ic23",      0x020000, 0xd3e80a9f, BRF_ESS | BRF_BIOS }, \
    { "fpr-24363.ic48",  0x020000, 0x82a105f0, BRF_ESS | BRF_BIOS }

static const UINT32 AwaveDigitalMap[] = {
    AWAVE_INPUT_BTN1, AWAVE_INPUT_BTN2, AWAVE_INPUT_BTN3, AWAVE_INPUT_BTN4, AWAVE_INPUT_BTN5,
    AWAVE_INPUT_START, AWAVE_INPUT_COIN, AWAVE_INPUT_SERVICE, AWAVE_INPUT_TEST,
    AWAVE_INPUT_UP, AWAVE_INPUT_DOWN, AWAVE_INPUT_LEFT, AWAVE_INPUT_RIGHT
};

#define ZIPENTRY(name, idx) { name, idx }
#define ZIPEND { NULL, -1 }
#define AWAVE_INIT_FUNC(shortname, zipentries, inputtype) \
static INT32 shortname##Init() { \
    static NaomiGameConfig cfg = { #shortname, #shortname ".zip", zipentries, "awbios.zip", "atomiswave", "atomiswave", AwaveDigitalMap, sizeof(AwaveDigitalMap)/sizeof(AwaveDigitalMap[0]), NULL, 0, inputtype, AWAVE_PLATFORM_ATOMISWAVE, NULL }; \
    return NaomiCoreInit(&cfg); \
}


// BIOS-only entry.
static struct BurnRomInfo awbiosRomDesc[] = { AWAVE_BIOS_ROMS };
STD_ROM_PICK(awbios)
STD_ROM_FN(awbios)
static INT32 awbiosInit() { return 1; }
struct BurnDriver BurnDrvAwaveAwbios = {
    "awbios", NULL, NULL, NULL, "2001", "Atomiswave BIOS\0", "BIOS only", "Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_BOARDROM, 0, HARDWARE_SAMMY_ATOMISWAVE, GBF_BIOS, 0,
    NULL, awbiosRomInfo, awbiosRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    awbiosInit, AwaveExitCommon, NULL, NULL, NULL, NULL, 0, 640, 480, 4, 3
};

// Sports Shooting USA
static struct BurnRomInfo sprtshotRomDesc[] = {
    { "ax0101p01.ic18", 0x0800000, 0xb3642b5d, BRF_PRG | BRF_ESS },
    { "ax0101m01.ic11", 0x1000000, 0x1e39184d, BRF_PRG | BRF_ESS },
    { "ax0102m01.ic12", 0x1000000, 0x700764d1, BRF_PRG | BRF_ESS },
    { "ax0103m01.ic13", 0x1000000, 0x6144e7a8, BRF_PRG | BRF_ESS },
    { "ax0104m01.ic14", 0x1000000, 0xccb72150, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(sprtshot)
STD_ROM_FN(sprtshot)
static NaomiZipEntry sprtshotZipEntries[] = {
    ZIPENTRY("ax0101p01.ic18", 3), ZIPENTRY("ax0101m01.ic11", 4), ZIPENTRY("ax0102m01.ic12", 5), ZIPENTRY("ax0103m01.ic13", 6), ZIPENTRY("ax0104m01.ic14", 7), ZIPEND
};
AWAVE_INIT_FUNC(sprtshot, sprtshotZipEntries, NAOMI_GAME_INPUT_LIGHTGUN)
struct BurnDriver BurnDrvAwavesprtshot = {
    "sprtshot", NULL, "awbios", NULL, "2003", "Sports Shooting USA" "\0", NULL, "Sammy USA", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, sprtshotRomInfo, sprtshotRomName, NULL, NULL, NULL, NULL, AwaveLightgunInputInfo, NULL,
    sprtshotInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Dolphin Blue
static struct BurnRomInfo dolphinRomDesc[] = {
    { "ax0401p01.ic18", 0x0800000, 0x195d6328, BRF_PRG | BRF_ESS },
    { "ax0401m01.ic11", 0x1000000, 0x5e5dca57, BRF_PRG | BRF_ESS },
    { "ax0402m01.ic12", 0x1000000, 0x77dd4771, BRF_PRG | BRF_ESS },
    { "ax0403m01.ic13", 0x1000000, 0x911d0674, BRF_PRG | BRF_ESS },
    { "ax0404m01.ic14", 0x1000000, 0xf82a4ca3, BRF_PRG | BRF_ESS },
    { "ax0405m01.ic15", 0x1000000, 0xb88298d7, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(dolphin)
STD_ROM_FN(dolphin)
static NaomiZipEntry dolphinZipEntries[] = {
    ZIPENTRY("ax0401p01.ic18", 3), ZIPENTRY("ax0401m01.ic11", 4), ZIPENTRY("ax0402m01.ic12", 5), ZIPENTRY("ax0403m01.ic13", 6), ZIPENTRY("ax0404m01.ic14", 7), ZIPENTRY("ax0405m01.ic15", 8), ZIPEND
};
AWAVE_INIT_FUNC(dolphin, dolphinZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwavedolphin = {
    "dolphin", NULL, "awbios", NULL, "2003", "Dolphin Blue" "\0", NULL, "Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, dolphinRomInfo, dolphinRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    dolphinInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Demolish Fist
static struct BurnRomInfo demofistRomDesc[] = {
    { "ax0601p01.ic18", 0x0800000, 0x0efb38ad, BRF_PRG | BRF_ESS },
    { "ax0601m01.ic11", 0x1000000, 0x12fda2c7, BRF_PRG | BRF_ESS },
    { "ax0602m01.ic12", 0x1000000, 0xaea61fdf, BRF_PRG | BRF_ESS },
    { "ax0603m01.ic13", 0x1000000, 0xd5879d35, BRF_PRG | BRF_ESS },
    { "ax0604m01.ic14", 0x1000000, 0xa7b09048, BRF_PRG | BRF_ESS },
    { "ax0605m01.ic15", 0x1000000, 0x18d8437e, BRF_PRG | BRF_ESS },
    { "ax0606m01.ic16", 0x1000000, 0x42c81617, BRF_PRG | BRF_ESS },
    { "ax0607m01.ic17", 0x1000000, 0x96e5aa84, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(demofist)
STD_ROM_FN(demofist)
static NaomiZipEntry demofistZipEntries[] = {
    ZIPENTRY("ax0601p01.ic18", 3), ZIPENTRY("ax0601m01.ic11", 4), ZIPENTRY("ax0602m01.ic12", 5), ZIPENTRY("ax0603m01.ic13", 6), ZIPENTRY("ax0604m01.ic14", 7), ZIPENTRY("ax0605m01.ic15", 8), ZIPENTRY("ax0606m01.ic16", 9), ZIPENTRY("ax0607m01.ic17", 10), ZIPEND
};
AWAVE_INIT_FUNC(demofist, demofistZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwavedemofist = {
    "demofist", NULL, "awbios", NULL, "2003", "Demolish Fist" "\0", NULL, "Polygon Magic / Dimps", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, demofistRomInfo, demofistRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    demofistInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Ranger Mission
static struct BurnRomInfo rangrmsnRomDesc[] = {
    { "ax1601p01.ic18", 0x0800000, 0x00a74fbb, BRF_PRG | BRF_ESS },
    { "ax1601m01.ic11", 0x1000000, 0xf34eed33, BRF_PRG | BRF_ESS },
    { "ax1602m01.ic12", 0x1000000, 0xa7d59efb, BRF_PRG | BRF_ESS },
    { "ax1603m01.ic13", 0x1000000, 0x7c0aa241, BRF_PRG | BRF_ESS },
    { "ax1604m01.ic14", 0x1000000, 0xd2369144, BRF_PRG | BRF_ESS },
    { "ax1605m01.ic15", 0x1000000, 0x0c11c1f9, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(rangrmsn)
STD_ROM_FN(rangrmsn)
static NaomiZipEntry rangrmsnZipEntries[] = {
    ZIPENTRY("ax1601p01.ic18", 3), ZIPENTRY("ax1601m01.ic11", 4), ZIPENTRY("ax1602m01.ic12", 5), ZIPENTRY("ax1603m01.ic13", 6), ZIPENTRY("ax1604m01.ic14", 7), ZIPENTRY("ax1605m01.ic15", 8), ZIPEND
};
AWAVE_INIT_FUNC(rangrmsn, rangrmsnZipEntries, NAOMI_GAME_INPUT_LIGHTGUN)
struct BurnDriver BurnDrvAwaverangrmsn = {
    "rangrmsn", NULL, "awbios", NULL, "2004", "Ranger Mission" "\0", NULL, "RIZ Inc. / Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, rangrmsnRomInfo, rangrmsnRomName, NULL, NULL, NULL, NULL, AwaveLightgunInputInfo, NULL,
    rangrmsnInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// The Rumble Fish
static struct BurnRomInfo rumblefRomDesc[] = {
    { "ax1801p01.ic18", 0x0800000, 0x2f7fb163, BRF_PRG | BRF_ESS },
    { "ax1801m01.ic11", 0x1000000, 0xc38aa61c, BRF_PRG | BRF_ESS },
    { "ax1802m01.ic12", 0x1000000, 0x72e0ebc8, BRF_PRG | BRF_ESS },
    { "ax1803m01.ic13", 0x1000000, 0xd0f59d98, BRF_PRG | BRF_ESS },
    { "ax1804m01.ic14", 0x1000000, 0x15595cba, BRF_PRG | BRF_ESS },
    { "ax1805m01.ic15", 0x1000000, 0x3d3f8e0d, BRF_PRG | BRF_ESS },
    { "ax1806m01.ic16", 0x1000000, 0xac2751bb, BRF_PRG | BRF_ESS },
    { "ax1807m01.ic17", 0x1000000, 0x3b2fbdb0, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(rumblef)
STD_ROM_FN(rumblef)
static NaomiZipEntry rumblefZipEntries[] = {
    ZIPENTRY("ax1801p01.ic18", 3), ZIPENTRY("ax1801m01.ic11", 4), ZIPENTRY("ax1802m01.ic12", 5), ZIPENTRY("ax1803m01.ic13", 6), ZIPENTRY("ax1804m01.ic14", 7), ZIPENTRY("ax1805m01.ic15", 8), ZIPENTRY("ax1806m01.ic16", 9), ZIPENTRY("ax1807m01.ic17", 10), ZIPEND
};
AWAVE_INIT_FUNC(rumblef, rumblefZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwaverumblef = {
    "rumblef", NULL, "awbios", NULL, "2004", "The Rumble Fish" "\0", NULL, "Sammy / Dimps", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, rumblefRomInfo, rumblefRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    rumblefInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Fist Of The North Star / Hokuto no Ken
static struct BurnRomInfo fotnsRomDesc[] = {
    { "ax1901p01.ic18", 0x0800000, 0xa06998b0, BRF_PRG | BRF_ESS },
    { "ax1901m01.ic11", 0x1000000, 0xff5a1642, BRF_PRG | BRF_ESS },
    { "ax1902m01.ic12", 0x1000000, 0xd9aae8a9, BRF_PRG | BRF_ESS },
    { "ax1903m01.ic13", 0x1000000, 0x1711b23d, BRF_PRG | BRF_ESS },
    { "ax1904m01.ic14", 0x1000000, 0x443bfb26, BRF_PRG | BRF_ESS },
    { "ax1905m01.ic15", 0x1000000, 0xeb1cada0, BRF_PRG | BRF_ESS },
    { "ax1906m01.ic16", 0x1000000, 0xfe6da168, BRF_PRG | BRF_ESS },
    { "ax1907m01.ic17", 0x1000000, 0x9d3a0520, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(fotns)
STD_ROM_FN(fotns)
static NaomiZipEntry fotnsZipEntries[] = {
    ZIPENTRY("ax1901p01.ic18", 3), ZIPENTRY("ax1901m01.ic11", 4), ZIPENTRY("ax1902m01.ic12", 5), ZIPENTRY("ax1903m01.ic13", 6), ZIPENTRY("ax1904m01.ic14", 7), ZIPENTRY("ax1905m01.ic15", 8), ZIPENTRY("ax1906m01.ic16", 9), ZIPENTRY("ax1907m01.ic17", 10), ZIPEND
};
AWAVE_INIT_FUNC(fotns, fotnsZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwavefotns = {
    "fotns", NULL, "awbios", NULL, "2005", "Fist Of The North Star / Hokuto no Ken" "\0", NULL, "Arc System Works / Sega", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, fotnsRomInfo, fotnsRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    fotnsInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// The King of Fighters Neowave
static struct BurnRomInfo kofnwRomDesc[] = {
    { "ax2201en_p01.ic18", 0x0800000, 0x27aab918, BRF_PRG | BRF_ESS },
    { "ax2201m01.ic11",    0x1000000, 0x22ea665b, BRF_PRG | BRF_ESS },
    { "ax2202m01.ic12",    0x1000000, 0x7fad1bea, BRF_PRG | BRF_ESS },
    { "ax2203m01.ic13",    0x1000000, 0x78986ca4, BRF_PRG | BRF_ESS },
    { "ax2204m01.ic14",    0x1000000, 0x6ffbeb04, BRF_PRG | BRF_ESS },
    { "ax2205m01.ic15",    0x1000000, 0x2851b791, BRF_PRG | BRF_ESS },
    { "ax2206m01.ic16",    0x1000000, 0xe53eb965, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(kofnw)
STD_ROM_FN(kofnw)
static NaomiZipEntry kofnwZipEntries[] = {
    ZIPENTRY("ax2201en_p01.ic18", 3), ZIPENTRY("ax2201m01.ic11", 4), ZIPENTRY("ax2202m01.ic12", 5), ZIPENTRY("ax2203m01.ic13", 6), ZIPENTRY("ax2204m01.ic14", 7), ZIPENTRY("ax2205m01.ic15", 8), ZIPENTRY("ax2206m01.ic16", 9), ZIPEND
};
AWAVE_INIT_FUNC(kofnw, kofnwZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwavekofnw = {
    "kofnw", NULL, "awbios", NULL, "2004", "The King of Fighters Neowave" "\0", NULL, "Sammy / SNK Playmore", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, kofnwRomInfo, kofnwRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    kofnwInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Extreme Hunting
static struct BurnRomInfo xtrmhuntRomDesc[] = {
    { "ax2401p01.ic18", 0x0800000, 0x8e2a11f5, BRF_PRG | BRF_ESS },
    { "ax2401m01.ic11", 0x1000000, 0x76dbc286, BRF_PRG | BRF_ESS },
    { "ax2402m01.ic12", 0x1000000, 0xcd590ea2, BRF_PRG | BRF_ESS },
    { "ax2403m01.ic13", 0x1000000, 0x06f62eb5, BRF_PRG | BRF_ESS },
    { "ax2404m01.ic14", 0x1000000, 0x759ef5cb, BRF_PRG | BRF_ESS },
    { "ax2405m01.ic15", 0x1000000, 0x940d77f1, BRF_PRG | BRF_ESS },
    { "ax2406m01.ic16", 0x1000000, 0xcbcf2c5d, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(xtrmhunt)
STD_ROM_FN(xtrmhunt)
static NaomiZipEntry xtrmhuntZipEntries[] = {
    ZIPENTRY("ax2401p01.ic18", 3), ZIPENTRY("ax2401m01.ic11", 4), ZIPENTRY("ax2402m01.ic12", 5), ZIPENTRY("ax2403m01.ic13", 6), ZIPENTRY("ax2404m01.ic14", 7), ZIPENTRY("ax2405m01.ic15", 8), ZIPENTRY("ax2406m01.ic16", 9), ZIPEND
};
AWAVE_INIT_FUNC(xtrmhunt, xtrmhuntZipEntries, NAOMI_GAME_INPUT_LIGHTGUN)
struct BurnDriver BurnDrvAwavextrmhunt = {
    "xtrmhunt", NULL, "awbios", NULL, "2004", "Extreme Hunting" "\0", NULL, "Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, xtrmhuntRomInfo, xtrmhuntRomName, NULL, NULL, NULL, NULL, AwaveLightgunInputInfo, NULL,
    xtrmhuntInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Extreme Hunting 2
static struct BurnRomInfo xtrmhnt2RomDesc[] = {
    { "610-0752.u3",  0x1000000, 0xbab6182e, BRF_PRG | BRF_ESS },
    { "610-0752.u1",  0x1000000, 0x3086bc47, BRF_PRG | BRF_ESS },
    { "610-0752.u4",  0x1000000, 0x9787f145, BRF_PRG | BRF_ESS },
    { "610-0752.u2",  0x1000000, 0xd3a88b31, BRF_PRG | BRF_ESS },
    { "610-0752.u15", 0x1000000, 0x864a6342, BRF_PRG | BRF_ESS },
    { "610-0752.u17", 0x1000000, 0xa79fb1fa, BRF_PRG | BRF_ESS },
    { "610-0752.u14", 0x1000000, 0xce83bcc7, BRF_PRG | BRF_ESS },
    { "610-0752.u16", 0x1000000, 0x8ac71c76, BRF_PRG | BRF_ESS },
    { "fpr-24330a.ic2", 0x400000, 0x8d89877e, BRF_PRG | BRF_OPT },
    { "flash128.ic4s",  0x1000000, 0x866ed675, BRF_PRG | BRF_OPT },
};
STD_ROM_PICK(xtrmhnt2)
STD_ROM_FN(xtrmhnt2)
static NaomiZipEntry xtrmhnt2ZipEntries[] = {
    ZIPENTRY("610-0752.u3", 3), ZIPENTRY("610-0752.u1", 4), ZIPENTRY("610-0752.u4", 5), ZIPENTRY("610-0752.u2", 6), ZIPENTRY("610-0752.u15", 7), ZIPENTRY("610-0752.u17", 8), ZIPENTRY("610-0752.u14", 9), ZIPENTRY("610-0752.u16", 10), ZIPEND
};
AWAVE_INIT_FUNC(xtrmhnt2, xtrmhnt2ZipEntries, NAOMI_GAME_INPUT_LIGHTGUN)
struct BurnDriver BurnDrvAwavextrmhnt2 = {
    "xtrmhnt2", NULL, "awbios", NULL, "2006", "Extreme Hunting 2" "\0", NULL, "Sega", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, xtrmhnt2RomInfo, xtrmhnt2RomName, NULL, NULL, NULL, NULL, AwaveLightgunInputInfo, NULL,
    xtrmhnt2Init, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Animal Basket / Hustle Tamaire Kyousou (24 Jan 2005)
static struct BurnRomInfo anmlbsktRomDesc[] = {
    { "vm2001f01.u3",  0x800000, 0x4fb33380, BRF_PRG | BRF_ESS },
    { "vm2001f01.u4",  0x800000, 0x7cb2e7c3, BRF_PRG | BRF_ESS },
    { "vm2001f01.u2",  0x800000, 0x386070a1, BRF_PRG | BRF_ESS },
    { "vm2001f01.u15", 0x800000, 0x2bb1be28, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(anmlbskt)
STD_ROM_FN(anmlbskt)
static NaomiZipEntry anmlbsktZipEntries[] = {
    ZIPENTRY("vm2001f01.u3", 3), ZIPENTRY("vm2001f01.u4", 4), ZIPENTRY("vm2001f01.u2", 5), ZIPENTRY("vm2001f01.u15", 6), ZIPEND
};
AWAVE_INIT_FUNC(anmlbskt, anmlbsktZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwaveanmlbskt = {
    "anmlbskt", NULL, "awbios", NULL, "2005", "Animal Basket / Hustle Tamaire Kyousou (24 Jan 2005)" "\0", NULL, "MOSS / Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, anmlbsktRomInfo, anmlbsktRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    anmlbsktInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Block Pong-Pong
static struct BurnRomInfo blokpongRomDesc[] = {
    { "u3", 0x1000000, 0xdebaf8bd, BRF_PRG | BRF_ESS },
    { "u1", 0x1000000, 0xca097a3f, BRF_PRG | BRF_ESS },
    { "u4", 0x1000000, 0xd235dd29, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(blokpong)
STD_ROM_FN(blokpong)
static NaomiZipEntry blokpongZipEntries[] = {
    ZIPENTRY("u3", 3), ZIPENTRY("u1", 4), ZIPENTRY("u4", 5), ZIPEND
};
AWAVE_INIT_FUNC(blokpong, blokpongZipEntries, NAOMI_GAME_INPUT_BLOCKPONG)
struct BurnDriver BurnDrvAwaveblokpong = {
    "blokpong", NULL, "awbios", NULL, "2004", "Block Pong-Pong" "\0", NULL, "MOSS / Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, blokpongRomInfo, blokpongRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    blokpongInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Knights of Valour - The Seven Spirits
static struct BurnRomInfo kov7sprtRomDesc[] = {
    { "ax1301p01.ic18", 0x0800000, 0x6833a334, BRF_PRG | BRF_ESS },
    { "ax1301m01.ic11", 0x1000000, 0x58ae7ca1, BRF_PRG | BRF_ESS },
    { "ax1301m02.ic12", 0x1000000, 0x871ea03f, BRF_PRG | BRF_ESS },
    { "ax1301m03.ic13", 0x1000000, 0xabc328bc, BRF_PRG | BRF_ESS },
    { "ax1301m04.ic14", 0x1000000, 0x25a176d1, BRF_PRG | BRF_ESS },
    { "ax1301m05.ic15", 0x1000000, 0xe6573a93, BRF_PRG | BRF_ESS },
    { "ax1301m06.ic16", 0x1000000, 0xcb8cacb4, BRF_PRG | BRF_ESS },
    { "ax1301m07.ic17", 0x1000000, 0x0ca92213, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(kov7sprt)
STD_ROM_FN(kov7sprt)
static NaomiZipEntry kov7sprtZipEntries[] = {
    ZIPENTRY("ax1301p01.ic18", 3), ZIPENTRY("ax1301m01.ic11", 4), ZIPENTRY("ax1301m02.ic12", 5), ZIPENTRY("ax1301m03.ic13", 6), ZIPENTRY("ax1301m04.ic14", 7), ZIPENTRY("ax1301m05.ic15", 8), ZIPENTRY("ax1301m06.ic16", 9), ZIPENTRY("ax1301m07.ic17", 10), ZIPEND
};
AWAVE_INIT_FUNC(kov7sprt, kov7sprtZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwavekov7sprt = {
    "kov7sprt", NULL, "awbios", NULL, "2003", "Knights of Valour - The Seven Spirits" "\0", NULL, "IGS / Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 2, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, kov7sprtRomInfo, kov7sprtRomName, NULL, NULL, NULL, NULL, Awave2PInputInfo, NULL,
    kov7sprtInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
// Guilty Gear Isuka
static struct BurnRomInfo ggisukaRomDesc[] = {
    { "ax1201p01.ic18", 0x0800000, 0x0a78d52c, BRF_PRG | BRF_ESS },
    { "ax1201m01.ic10", 0x1000000, 0xdf96ce30, BRF_PRG | BRF_ESS },
    { "ax1202m01.ic11", 0x1000000, 0xdfc6fd67, BRF_PRG | BRF_ESS },
    { "ax1203m01.ic12", 0x1000000, 0xbf623df9, BRF_PRG | BRF_ESS },
    { "ax1204m01.ic13", 0x1000000, 0xc80c3930, BRF_PRG | BRF_ESS },
    { "ax1205m01.ic14", 0x1000000, 0xe99a269d, BRF_PRG | BRF_ESS },
    { "ax1206m01.ic15", 0x1000000, 0x807ab795, BRF_PRG | BRF_ESS },
    { "ax1207m01.ic16", 0x1000000, 0x6636d1b8, BRF_PRG | BRF_ESS },
    { "ax1208m01.ic17", 0x1000000, 0x38bda476, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(ggisuka)
STD_ROM_FN(ggisuka)
static NaomiZipEntry ggisukaZipEntries[] = {
    ZIPENTRY("ax1201p01.ic18", 3), ZIPENTRY("ax1201m01.ic10", 4), ZIPENTRY("ax1202m01.ic11", 5), ZIPENTRY("ax1203m01.ic12", 6), ZIPENTRY("ax1204m01.ic13", 7), ZIPENTRY("ax1205m01.ic14", 8), ZIPENTRY("ax1206m01.ic15", 9), ZIPENTRY("ax1207m01.ic16", 10), ZIPENTRY("ax1208m01.ic17", 11), ZIPEND
};
AWAVE_INIT_FUNC(ggisuka, ggisukaZipEntries, NAOMI_GAME_INPUT_DIGITAL)
struct BurnDriver BurnDrvAwaveggisuka = {
    "ggisuka", NULL, "awbios", NULL, "2004", "Guilty Gear Isuka" "\0", NULL, "Arc System Works / Sammy", "Atomiswave",
    NULL, NULL, NULL, NULL, BDF_GAME_WORKING, 4, HARDWARE_SAMMY_ATOMISWAVE, GBF_MISC, 0,
    NULL, ggisukaRomInfo, ggisukaRomName, NULL, NULL, NULL, NULL, Awave4PInputInfo, NULL,
    ggisukaInit, AwaveExitCommon, AwaveFrameCommon, AwaveDrawCommon, AwaveScanCommon, &AwaveRecalc,
    0x1000000, 640, 480, 4, 3
};
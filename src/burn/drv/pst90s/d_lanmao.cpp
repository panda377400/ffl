// FinalBurn Neo driver module for MAME skeleton/lanmao.cpp

#include "tiles_generic.h"
#include "mcs51.h"
#include "ay8910.h"
#include "burn_ym2413.h"
#include "msm6295.h"
#include "../../../dep/libs/libspng/spng.h"
#include <stdio.h>

#define LANMAO_SCREEN_W 800
#define LANMAO_SCREEN_H 750
#ifndef DIRS_MAX
#define DIRS_MAX 20
#endif

static const UINT8 LanmaoPngSig[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };

extern TCHAR szAppArtworkPath[];
char* TCHARToANSI(const TCHAR* pszInString, char* pszOutString, int nOutSize);
INT32 __cdecl ZipLoadOneFile(char* arcName, const char* fileName, void** Dest, INT32* pnWrote);
#if defined(__LIBRETRO__)
extern char g_rom_dir[MAX_PATH];
#else
extern TCHAR szAppRomPaths[DIRS_MAX][MAX_PATH];
#endif

static UINT8 DrvReset;
static UINT8 SkeletonReset;

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *DrvMcuROM;
static UINT8 *DrvSndROM;
static UINT8 *DrvI2CMem;
static UINT8 *DrvNVRAM;
static UINT32 *DrvPalette;
static UINT16 *LanmaoArtwork;

static UINT8 DrvJoy1[8];
static UINT8 DrvJoy2[8];
static UINT8 DrvJoy3[8];
static UINT8 DrvJoy4[8];
static UINT8 DrvDips[1];
static UINT8 DrvInputs[4];

static UINT8 LanmaoDigits[32];
static UINT8 LanmaoLeds[31];
static UINT8 LanmaoKbdLine;
static UINT8 LanmaoOkiBank;
static UINT8 LanmaoHopperMotor;
static UINT8 LanmaoHopperSensor;
static UINT8 LanmaoHopperClock;
static UINT8 LanmaoPort1Latch;
static UINT8 LanmaoPort3Latch;
static UINT8 DrvRecalc;

struct LanmaoLayoutRect
{
	INT16 x, y, w, h;
};

// MAME can optionally load artwork/lanmao.zip for the decorated background while
// still rendering the 7-seg digits and LEDs dynamically. FBNeo keeps the dynamic
// overlay and, when the artwork zip is available, composites the background below it.
static const LanmaoLayoutRect LanmaoDigitLayout[26] = {
	{ 700, 666, 30, 37 }, { 660, 666, 30, 37 }, { 616, 666, 30, 37 }, { 576, 666, 30, 37 },
	{ 532, 666, 30, 37 }, { 492, 666, 30, 37 }, { 447, 666, 30, 37 }, { 407, 666, 30, 37 },
	{ 363, 666, 30, 37 }, { 323, 666, 30, 37 }, { 279, 666, 30, 37 }, { 239, 666, 30, 37 },
	{ 195, 666, 30, 37 }, { 155, 666, 30, 37 }, { 111, 666, 30, 37 }, {  71, 666, 30, 37 },
	{ 566,  60, 30, 37 }, { 526,  60, 30, 37 }, { 485,  60, 30, 37 }, { 445,  60, 30, 37 },
	{ 322,  60, 30, 37 }, { 282,  60, 30, 37 }, { 241,  60, 30, 37 }, { 200,  60, 30, 37 },
	{ 403, 409, 30, 37 }, { 363, 409, 30, 37 },
};

static const LanmaoLayoutRect LanmaoLedLayout[31] = {
	{ 174, 403, 23, 19 }, { 174, 333, 23, 19 }, { 174, 265, 23, 19 }, { 174, 194, 23, 19 },
	{ 175, 160, 23, 19 }, { 216, 160, 23, 19 }, { 299, 160, 23, 19 }, { 384, 160, 23, 19 },
	{ 467, 160, 23, 19 }, { 553, 160, 23, 19 }, { 594, 160, 23, 19 }, { 594, 194, 23, 19 },
	{ 594, 265, 23, 19 }, { 594, 333, 23, 19 }, { 594, 403, 23, 19 }, { 594, 471, 23, 19 },
	{ 595, 506, 23, 19 }, { 553, 506, 23, 19 }, { 469, 506, 23, 19 }, { 385, 506, 23, 19 },
	{ 301, 506, 23, 19 }, { 216, 506, 23, 19 }, { 176, 506, 23, 19 }, { 174, 471, 23, 19 },
	{ 596, 587, 23, 19 }, { 511, 587, 23, 19 }, { 427, 587, 23, 19 }, { 343, 587, 23, 19 },
	{ 258, 587, 23, 19 }, { 174, 587, 23, 19 }, { 384, 333, 23, 19 },
};

static struct BurnInputInfo SkeletonInputList[] = {
	{ "Reset"             , BIT_DIGITAL  , &SkeletonReset      , "reset"     },
};

STDINPUTINFO(Skeleton)

static struct BurnInputInfo LanmaoInputList[] = {
	{ "Bet 1",              BIT_DIGITAL  , DrvJoy1 + 0,         "p1 fire 1"  },
	{ "Bet 2",              BIT_DIGITAL  , DrvJoy1 + 1,         "p1 fire 2"  },
	{ "Bet 3",              BIT_DIGITAL  , DrvJoy1 + 2,         "p1 fire 3"  },
	{ "Bet 4",              BIT_DIGITAL  , DrvJoy1 + 3,         "p1 fire 4"  },
	{ "Bet 5",              BIT_DIGITAL  , DrvJoy1 + 4,         "p1 fire 5"  },
	{ "Bet 6",              BIT_DIGITAL  , DrvJoy1 + 5,         "p1 fire 6"  },
	{ "Bet 7",              BIT_DIGITAL  , DrvJoy1 + 6,         "p1 fire 7"  },
	{ "Bet 8",              BIT_DIGITAL  , DrvJoy1 + 7,         "p1 fire 8"  },
	{ "Start",              BIT_DIGITAL  , DrvJoy2 + 0,         "p1 start"   },
	{ "High",               BIT_DIGITAL  , DrvJoy2 + 1,         "p1 up"      },
	{ "Low",                BIT_DIGITAL  , DrvJoy2 + 2,         "p1 down"    },
	{ "Single",             BIT_DIGITAL  , DrvJoy2 + 3,         "p1 fire 9"  },
	{ "Shift Right",        BIT_DIGITAL  , DrvJoy2 + 4,         "p1 right"   },
	{ "Shift Left",         BIT_DIGITAL  , DrvJoy2 + 5,         "p1 left"    },
	{ "Payout",             BIT_DIGITAL  , DrvJoy2 + 6,         "p1 fire 10" },
	{ "Double",             BIT_DIGITAL  , DrvJoy2 + 7,         "p1 fire 11" },
	{ "K0",                 BIT_DIGITAL  , DrvJoy3 + 0,         "p1 fire 12" },
	{ "K1",                 BIT_DIGITAL  , DrvJoy3 + 1,         "p1 fire 13" },
	{ "K2",                 BIT_DIGITAL  , DrvJoy3 + 2,         "p1 fire 14" },
	{ "K3",                 BIT_DIGITAL  , DrvJoy3 + 3,         "p1 fire 15" },
	{ "Coin",               BIT_DIGITAL  , DrvJoy4 + 0,         "p1 coin"    },
	{ "Memory Reset",       BIT_DIGITAL  , DrvJoy4 + 1,         "service"    },
	{ "Reset",              BIT_DIGITAL  , &DrvReset,           "reset"      },
	{ "Dip A",              BIT_DIPSWITCH, DrvDips + 0,         "dip"        },
};

STDINPUTINFO(Lanmao)

static struct BurnDIPInfo LanmaoDIPList[]=
{
	{0x17, 0xff, 0xff, 0xff, NULL                         },

	{0   , 0xfe, 0   ,    4, "Running Lights Difficulty"  },
	{0x17, 0x01, 0x03, 0x03, "3"                          },
	{0x17, 0x01, 0x03, 0x02, "2"                          },
	{0x17, 0x01, 0x03, 0x01, "1"                          },
	{0x17, 0x01, 0x03, 0x00, "0"                          },

	{0   , 0xfe, 0   ,    2, "Demo Sounds"                },
	{0x17, 0x01, 0x04, 0x00, "Off"                        },
	{0x17, 0x01, 0x04, 0x04, "On"                         },

	{0   , 0xfe, 0   ,    2, "Double Up Difficulty"       },
	{0x17, 0x01, 0x08, 0x08, "1"                          },
	{0x17, 0x01, 0x08, 0x00, "0"                          },

	{0   , 0xfe, 0   ,    2, "Bet Ratio"                  },
	{0x17, 0x01, 0x10, 0x10, "1/1"                        },
	{0x17, 0x01, 0x10, 0x00, "1/5"                        },

	{0   , 0xfe, 0   ,    4, "Coinage"                    },
	{0x17, 0x01, 0x60, 0x00, "1C 5C"                      },
	{0x17, 0x01, 0x60, 0x60, "1C 10C"                     },
	{0x17, 0x01, 0x60, 0x20, "1C 50C"                     },
	{0x17, 0x01, 0x60, 0x40, "1C 100C"                    },

	{0   , 0xfe, 0   ,    2, "Maximum Bet"                },
	{0x17, 0x01, 0x80, 0x80, "60"                         },
	{0x17, 0x01, 0x80, 0x00, "95"                         },
};

STDDIPINFO(Lanmao)

static struct BurnRomInfo panda2RomDesc[] = {
	{ "w78e065",     0x008000, 0xe6a28090, 1 | BRF_PRG | BRF_ESS },
	{ "at28c16.u9",  0x000800, 0xbd67af27, 2 | BRF_PRG | BRF_ESS },
};

STD_ROM_PICK(panda2)
STD_ROM_FN(panda2)

static struct BurnRomInfo lanmaoRomDesc[] = {
	{ "w78e065",     0x010000, 0x57a89c7f, 1 | BRF_PRG | BRF_ESS },
	{ "w27e040-12",  0x080000, 0x4481a891, 2 | BRF_SND },
	{ "24c02",       0x000100, 0xdaf84e5e, 3 | BRF_PRG | BRF_ESS },
	{ "nvram",       0x000800, 0x5d051021, 4 | BRF_PRG | BRF_OPT },
};

STD_ROM_PICK(lanmao)
STD_ROM_FN(lanmao)

enum {
	LANMAO_I2C_IDLE = 0,
	LANMAO_I2C_DEVICEID,
	LANMAO_I2C_ADDR,
	LANMAO_I2C_WRITE,
	LANMAO_I2C_READSELACK,
	LANMAO_I2C_READ
};

static INT32 LanmaoI2cMode;
static UINT16 LanmaoI2cAddress;
static UINT8 LanmaoI2cShifter;
static INT32 LanmaoI2cBitCount;
static UINT8 LanmaoI2cSelectedDevice;
static UINT8 LanmaoI2cSda;
static UINT8 LanmaoI2cScl;
static UINT8 LanmaoI2cSdaRead;
static UINT8 LanmaoI2cPage[8];
static UINT8 LanmaoI2cPageOffset;
static UINT8 LanmaoI2cPageWrittenSize;

static void LanmaoI2cReset()
{
	LanmaoI2cMode = LANMAO_I2C_IDLE;
	LanmaoI2cAddress = 0;
	LanmaoI2cShifter = 0;
	LanmaoI2cBitCount = 0;
	LanmaoI2cSelectedDevice = 0;
	LanmaoI2cSda = 0;
	LanmaoI2cScl = 0;
	LanmaoI2cSdaRead = 1;
	memset(LanmaoI2cPage, 0, sizeof(LanmaoI2cPage));
	LanmaoI2cPageOffset = 0;
	LanmaoI2cPageWrittenSize = 0;
}

static UINT8 LanmaoI2cRead()
{
	return (LanmaoI2cMode == LANMAO_I2C_IDLE) ? LanmaoI2cSda : LanmaoI2cSdaRead;
}

static void LanmaoI2cWrite(INT32 sda, INT32 scl)
{
	if (sda != LanmaoI2cSda && scl && LanmaoI2cScl && LanmaoI2cSdaRead) {
		if (sda) {
			if (LanmaoI2cPageWrittenSize > 0) {
				UINT16 base = LanmaoI2cAddress & 0xff;
				UINT16 root = base & ~0x07;
				for (INT32 i = 0; i < LanmaoI2cPageWrittenSize; i++) {
					DrvI2CMem[root | ((base + i) & 0x07)] = LanmaoI2cPage[i];
				}
				LanmaoI2cPageWrittenSize = 0;
			}
			LanmaoI2cMode = LANMAO_I2C_IDLE;
		} else {
			LanmaoI2cMode = LANMAO_I2C_DEVICEID;
			LanmaoI2cBitCount = 0;
		}
		LanmaoI2cShifter = 0;
		LanmaoI2cSda = sda;
		LanmaoI2cScl = scl;
		return;
	}

	if (LanmaoI2cScl == scl) {
		LanmaoI2cSda = sda;
		LanmaoI2cScl = scl;
		return;
	}

	LanmaoI2cScl = scl;

	switch (LanmaoI2cMode)
	{
		case LANMAO_I2C_DEVICEID:
		case LANMAO_I2C_ADDR:
		case LANMAO_I2C_WRITE:
			if (LanmaoI2cBitCount < 8) {
				if (scl) {
					LanmaoI2cShifter = ((LanmaoI2cShifter << 1) | (sda & 1)) & 0xff;
					LanmaoI2cBitCount++;
				}
			} else {
				if (scl) {
					LanmaoI2cBitCount++;
				} else {
					if (LanmaoI2cBitCount == 8) {
						switch (LanmaoI2cMode)
						{
							case LANMAO_I2C_DEVICEID:
								LanmaoI2cSelectedDevice = LanmaoI2cShifter;
								if ((LanmaoI2cSelectedDevice & 0xfe) != 0xa0) {
									LanmaoI2cMode = LANMAO_I2C_IDLE;
								} else if (LanmaoI2cSelectedDevice & 1) {
									LanmaoI2cMode = LANMAO_I2C_READSELACK;
								} else {
									LanmaoI2cMode = LANMAO_I2C_ADDR;
								}
							break;

							case LANMAO_I2C_ADDR:
								LanmaoI2cAddress = LanmaoI2cShifter;
								LanmaoI2cPageOffset = 0;
								LanmaoI2cPageWrittenSize = 0;
								LanmaoI2cMode = LANMAO_I2C_WRITE;
							break;

							case LANMAO_I2C_WRITE:
								LanmaoI2cPage[LanmaoI2cPageOffset] = LanmaoI2cShifter;
								LanmaoI2cPageOffset = (LanmaoI2cPageOffset + 1) & 0x07;
								if (LanmaoI2cPageWrittenSize < 8) LanmaoI2cPageWrittenSize++;
							break;
						}

						if (LanmaoI2cMode != LANMAO_I2C_IDLE) {
							LanmaoI2cSdaRead = 0;
						}
					} else {
						LanmaoI2cBitCount = 0;
						LanmaoI2cSdaRead = 1;
					}
				}
			}
		break;

		case LANMAO_I2C_READSELACK:
			LanmaoI2cBitCount = 0;
			LanmaoI2cMode = LANMAO_I2C_READ;
		break;

		case LANMAO_I2C_READ:
			if (LanmaoI2cBitCount < 8) {
				if (scl) {
					LanmaoI2cBitCount++;
				} else {
					if (LanmaoI2cBitCount == 0) {
						LanmaoI2cShifter = DrvI2CMem[LanmaoI2cAddress & 0xff];
						LanmaoI2cAddress = (LanmaoI2cAddress + 1) & 0xff;
					}

					LanmaoI2cSdaRead = (LanmaoI2cShifter >> 7) & 1;
					LanmaoI2cShifter <<= 1;
				}
			} else {
				if (scl) {
					if (sda) {
						LanmaoI2cMode = LANMAO_I2C_IDLE;
					}
					LanmaoI2cBitCount = 0;
				} else {
					LanmaoI2cSdaRead = 1;
				}
			}
		break;
	}

	LanmaoI2cSda = sda;
}

static UINT8 Lanmao8279DisplayRam[16];
static UINT8 Lanmao8279SensorRam[8];
static UINT8 Lanmao8279Fifo[8];
static UINT8 Lanmao8279Cmd[8];
static UINT8 Lanmao8279Status;
static UINT8 Lanmao8279DisplayPtr;
static UINT8 Lanmao8279SensorPtr;
static UINT8 Lanmao8279Scanner;
static UINT8 Lanmao8279KeyDown;
static UINT8 Lanmao8279Debounce;
static UINT8 Lanmao8279CtrlKey;
static UINT8 Lanmao8279AutoInc;
static UINT8 Lanmao8279ReadFlag;
static UINT8 Lanmao8279SpecialError;

static void LanmaoDisplayWrite(UINT8 data)
{
	static const UINT8 patterns[16] = {
		0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07,
		0x7f, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	UINT32 digit = (LanmaoKbdLine & 0x0f) * 2;
	if (digit < 32) LanmaoDigits[digit + 0] = patterns[data & 0x0f];
	if (digit + 1 < 32) LanmaoDigits[digit + 1] = patterns[(data >> 4) & 0x0f];
}

static void LanmaoLedsWrite(UINT32 which, UINT32 data)
{
	for (UINT32 i = 0; i < 8; i++) {
		UINT32 index = i + (which * 8);
		if (index < 31) {
			LanmaoLeds[index] = (data >> i) & 1;
		}
	}
}

static UINT8 LanmaoKeyboardRead()
{
	switch (LanmaoKbdLine & 0x07)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			return DrvInputs[LanmaoKbdLine & 0x07];
	}

	return 0x00;
}

static void Lanmao8279Clear()
{
	static const UINT8 patterns[4] = { 0x00, 0x00, 0x20, 0xff };
	UINT8 data = patterns[(Lanmao8279Cmd[6] & 0x0c) >> 2];

	if (Lanmao8279Cmd[6] & 0x11) {
		for (INT32 i = 0; i < 16; i++) {
			Lanmao8279DisplayRam[i] = data;
		}
	}

	Lanmao8279Status &= 0x7f;
	Lanmao8279DisplayPtr = 0;

	if (Lanmao8279Cmd[6] & 0x03) {
		Lanmao8279Status &= 0x80;
		Lanmao8279SensorPtr = 0;
		Lanmao8279Debounce = 0;
	}
}

static void Lanmao8279PushFifo(UINT8 data)
{
	if (Lanmao8279Status & 0x20) return;
	if (Lanmao8279Status & 0x40) return;

	if (Lanmao8279Status & 0x08) {
		Lanmao8279Status |= 0x20;
		return;
	}

	UINT8 fifo_size = Lanmao8279Status & 7;
	Lanmao8279Fifo[fifo_size] = data;

	if (fifo_size == 7) {
		Lanmao8279Status |= 0x08;
	} else {
		Lanmao8279Status = (Lanmao8279Status & 0xe8) | (fifo_size + 1);
	}
}

static void Lanmao8279Reset()
{
	memset(Lanmao8279DisplayRam, 0, sizeof(Lanmao8279DisplayRam));
	memset(Lanmao8279SensorRam, 0, sizeof(Lanmao8279SensorRam));
	memset(Lanmao8279Fifo, 0, sizeof(Lanmao8279Fifo));
	memset(Lanmao8279Cmd, 0, sizeof(Lanmao8279Cmd));

	Lanmao8279Status = 0;
	Lanmao8279AutoInc = 1;
	Lanmao8279DisplayPtr = 0;
	Lanmao8279SensorPtr = 0;
	Lanmao8279ReadFlag = 0;
	Lanmao8279Scanner = 0;
	Lanmao8279CtrlKey = 1;
	Lanmao8279SpecialError = 0;
	Lanmao8279KeyDown = 0;
	Lanmao8279Debounce = 0;
	LanmaoKbdLine = 0;

	Lanmao8279Cmd[0] = 8;
	Lanmao8279Cmd[1] = 31;

	memset(LanmaoDigits, 0, sizeof(LanmaoDigits));
}

static void Lanmao8279Step()
{
	UINT8 scanner_mask = (Lanmao8279Cmd[0] & 1) ? 3 : ((Lanmao8279Cmd[0] & 8) ? 15 : 7);
	UINT8 decoded = Lanmao8279Cmd[0] & 1;
	UINT8 kbd_type = (Lanmao8279Cmd[0] >> 1) & 3;
	UINT8 shift_key = 1;
	UINT8 ctrl_key = 1;
	UINT8 strobe_pulse = ctrl_key && !Lanmao8279CtrlKey;
	Lanmao8279CtrlKey = ctrl_key;

	UINT8 rl = LanmaoKeyboardRead() ^ 0xff;
	UINT8 addr = Lanmao8279Scanner & 7;
	UINT8 keys_down = rl & ~Lanmao8279SensorRam[addr];

	switch (kbd_type)
	{
		case 0:
		case 1:
			if (keys_down != 0) {
				for (INT32 i = 0; i < 8; i++) {
					if ((keys_down >> i) & 1) {
						if (Lanmao8279Debounce == 0) {
							Lanmao8279KeyDown = (addr << 3) | i;
							Lanmao8279Debounce = 1;
						} else if (Lanmao8279KeyDown == ((addr << 3) | i)) {
							if (Lanmao8279Debounce++ > 1) {
								Lanmao8279PushFifo((ctrl_key << 7) | (shift_key << 6) | Lanmao8279KeyDown);
								Lanmao8279SensorRam[addr] |= 1 << i;
								Lanmao8279Debounce = 0;
							}
						}
					}
				}
			}
			if ((Lanmao8279KeyDown >> 3) == addr && ((rl >> (Lanmao8279KeyDown & 7)) & 1) == 0) {
				Lanmao8279Debounce = 0;
			}
			Lanmao8279SensorRam[addr] &= rl;
		break;

		case 2:
			if (keys_down != 0 && !Lanmao8279SpecialError) {
				Lanmao8279Status |= 0x40;
			}
			if (Lanmao8279SensorRam[addr] != rl) {
				Lanmao8279SensorRam[addr] = rl;
			}
		break;

		case 3:
			if (strobe_pulse) {
				Lanmao8279PushFifo(rl);
			}
			Lanmao8279SensorRam[addr] = rl;
		break;
	}

	Lanmao8279Scanner = (Lanmao8279Scanner + 1) & (decoded ? 3 : 15);
	LanmaoKbdLine = decoded ? (((1 << Lanmao8279Scanner) ^ 15) & 0x0f) : Lanmao8279Scanner;
	LanmaoDisplayWrite(Lanmao8279DisplayRam[Lanmao8279Scanner & scanner_mask]);
}

static UINT8 Lanmao8279DataRead()
{
	UINT8 data = 0;
	UINT8 sensor_mode = ((Lanmao8279Cmd[0] & 6) == 4);

	if (Lanmao8279ReadFlag) {
		data = Lanmao8279DisplayRam[Lanmao8279DisplayPtr & 0x0f];
		if (Lanmao8279AutoInc) {
			Lanmao8279DisplayPtr = (Lanmao8279DisplayPtr + 1) & 0x0f;
		}
		return data;
	}

	if (sensor_mode) {
		data = Lanmao8279SensorRam[Lanmao8279SensorPtr & 0x07];
		if (Lanmao8279AutoInc) {
			Lanmao8279SensorPtr = (Lanmao8279SensorPtr + 1) & 0x07;
		}
		return data;
	}

	data = Lanmao8279Fifo[0];
	UINT8 fifo_size = Lanmao8279Status & 7;

	switch (Lanmao8279Status & 0x38)
	{
		case 0x00:
			if (!fifo_size) {
				Lanmao8279Status |= 0x10;
			} else {
				for (INT32 i = 1; i < 8; i++) {
					Lanmao8279Fifo[i - 1] = Lanmao8279Fifo[i];
				}
				fifo_size--;
			}
		break;

		case 0x28:
		case 0x08:
			for (INT32 i = 1; i < 8; i++) {
				Lanmao8279Fifo[i - 1] = Lanmao8279Fifo[i];
			}
		break;

		default:
			if (fifo_size) {
				for (INT32 i = 1; i < 8; i++) {
					Lanmao8279Fifo[i - 1] = Lanmao8279Fifo[i];
				}
				fifo_size--;
			}
		break;
	}

	Lanmao8279Status = (Lanmao8279Status & 0xd0) | fifo_size;
	return data;
}

static void Lanmao8279CmdWrite(UINT8 data)
{
	UINT8 cmd = data >> 5;
	data &= 0x1f;
	Lanmao8279Cmd[cmd] = data;

	switch (cmd)
	{
		case 2:
			Lanmao8279ReadFlag = 0;
			if ((Lanmao8279Cmd[0] & 6) == 4) {
				Lanmao8279AutoInc = (data >> 4) & 1;
				Lanmao8279SensorPtr = data & 7;
			}
		break;

		case 3:
			Lanmao8279ReadFlag = 1;
			Lanmao8279DisplayPtr = data & 15;
			Lanmao8279AutoInc = (data >> 4) & 1;
		break;

		case 4:
			Lanmao8279DisplayPtr = data & 15;
			Lanmao8279AutoInc = (data >> 4) & 1;
		break;

		case 6:
			Lanmao8279Clear();
		break;

		case 7:
			Lanmao8279SpecialError = (data >> 4) & 1;
			Lanmao8279Status &= 0xbf;
		break;
	}
}

static void Lanmao8279DataWrite(UINT8 data)
{
	if (!(Lanmao8279Cmd[5] & 0x04)) {
		Lanmao8279DisplayRam[Lanmao8279DisplayPtr & 0x0f] = (Lanmao8279DisplayRam[Lanmao8279DisplayPtr & 0x0f] & 0xf0) | (data & 0x0f);
	}
	if (!(Lanmao8279Cmd[5] & 0x08)) {
		Lanmao8279DisplayRam[Lanmao8279DisplayPtr & 0x0f] = (Lanmao8279DisplayRam[Lanmao8279DisplayPtr & 0x0f] & 0x0f) | (data & 0xf0);
	}

	if (Lanmao8279AutoInc) {
		Lanmao8279DisplayPtr = (Lanmao8279DisplayPtr + 1) & 0x0f;
	}
}

static UINT8 Lanmao8279Read(UINT32 offset)
{
	return (offset & 1) ? Lanmao8279Status : Lanmao8279DataRead();
}

static void Lanmao8279Write(UINT32 offset, UINT8 data)
{
	if (offset & 1) {
		Lanmao8279CmdWrite(data);
	} else {
		Lanmao8279DataWrite(data);
	}
}

static void LanmaoSetOkiBank(UINT8 bank)
{
	LanmaoOkiBank = bank & 1;
	MSM6295SetBank(0, DrvSndROM + (LanmaoOkiBank * 0x40000), 0x00000, 0x3ffff);
}

static UINT8 LanmaoPort1Read()
{
	UINT8 external = 0xff;

	if (!LanmaoI2cRead()) external &= ~0x02;
	if (!LanmaoHopperSensor) external &= ~0x40;
	if (DrvJoy4[1]) external &= ~0x80;

	return LanmaoPort1Latch & external;
}

static UINT8 LanmaoPort3Read()
{
	UINT8 external = 0xff;

	external &= ~0x08;
	if (DrvJoy4[0]) external &= ~0x01;

	return LanmaoPort3Latch & external;
}

static void LanmaoPort1Write(UINT8 data)
{
	LanmaoPort1Latch = data;
	LanmaoI2cWrite((data >> 1) & 1, (data >> 2) & 1);
	LanmaoHopperMotor = (data >> 3) & 1;
}

static void LanmaoPort3Write(UINT8 data)
{
	LanmaoPort3Latch = data;
	LanmaoSetOkiBank((data >> 5) & 1);
}

static UINT8 LanmaoReadPort(INT32 address)
{
	switch (address)
	{
		case MCS51_PORT_P0:
			return 0xff;

		case MCS51_PORT_P1:
			return LanmaoPort1Read();

		case MCS51_PORT_P2:
			return 0xff;

		case MCS51_PORT_P3:
			return LanmaoPort3Read();
	}

	return 0xff;
}

static void LanmaoWritePort(INT32 address, UINT8 data)
{
	switch (address)
	{
		case MCS51_PORT_P1:
			LanmaoPort1Write(data);
		break;

		case MCS51_PORT_P3:
			LanmaoPort3Write(data);
		break;
	}
}

static UINT8 LanmaoReadByte(INT32 address)
{
	if (address >= 0x8000 && address <= 0x87ff) {
		return DrvNVRAM[address & 0x07ff];
	}

	switch (address)
	{
		case 0xb000:
		case 0xb001:
			return Lanmao8279Read(address & 1);

		case 0xd000:
			return MSM6295Read(0);
	}

	return 0xff;
}

static void LanmaoWriteByte(INT32 address, UINT8 data)
{
	if (address >= 0x8000 && address <= 0x87ff) {
		DrvNVRAM[address & 0x07ff] = data;
		return;
	}

	switch (address)
	{
		case 0x9000:
		case 0x9001:
			AY8910Write(0, address & 1, data);
		break;

		case 0x9002:
		case 0x9003:
			AY8910Write(1, address & 1, data);
		break;

		case 0xb000:
		case 0xb001:
			Lanmao8279Write(address & 1, data);
		break;

		case 0xc000:
		case 0xc001:
			BurnYM2413Write(address & 1, data);
		break;

		case 0xd000:
			MSM6295Write(0, data);
		break;
	}
}

static UINT8 LanmaoAyDummyRead(UINT32)
{
	return 0xff;
}

static UINT8 LanmaoRead(INT32 address)
{
	if (address >= MCS51_PORT_P0) {
		return LanmaoReadPort(address);
	}

	return LanmaoReadByte(address);
}

static void LanmaoWrite(INT32 address, UINT8 data)
{
	if (address >= MCS51_PORT_P0) {
		LanmaoWritePort(address, data);
		return;
	}

	LanmaoWriteByte(address, data);
}

static void LanmaoAy0PortAWrite(UINT32, UINT32 data) { LanmaoLedsWrite(0, data); }
static void LanmaoAy0PortBWrite(UINT32, UINT32 data) { LanmaoLedsWrite(1, data); }
static void LanmaoAy1PortAWrite(UINT32, UINT32 data) { LanmaoLedsWrite(2, data); }
static void LanmaoAy1PortBWrite(UINT32, UINT32 data) { LanmaoLedsWrite(3, data); }

static INT32 MemIndex()
{
	UINT8 *Next = AllMem;

	DrvMcuROM  = Next; Next += 0x010000;
	DrvSndROM  = Next; Next += 0x080000;
	DrvI2CMem  = Next; Next += 0x000100;
	DrvNVRAM   = Next; Next += 0x000800;
	DrvPalette = (UINT32*)Next; Next += 0x10000 * sizeof(UINT32);

	MemEnd = Next;

	return 0;
}

static inline UINT16 LanmaoPackColor(UINT8 r, UINT8 g, UINT8 b)
{
	return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

static inline bool LanmaoIsPathSeparator(TCHAR c)
{
	return c == _T('\\') || c == _T('/');
}

static inline TCHAR LanmaoPreferredPathSeparator()
{
#if defined(_WIN32)
	return _T('\\');
#else
	return _T('/');
#endif
}

static INT32 LanmaoDecodeArtwork(const UINT8* buffer, INT32 bufferLength)
{
	if (buffer == NULL || bufferLength <= 8) {
		return 1;
	}

	for (INT32 i = 0; i < 8; i++) {
		if (buffer[i] != LanmaoPngSig[i]) {
			return 1;
		}
	}

	spng_ctx* ctx = spng_ctx_new(0);
	if (ctx == NULL) {
		return 1;
	}

	spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
	size_t limit = 1024 * 1024 * 64;
	spng_set_chunk_limits(ctx, limit, limit);

	if (spng_set_png_buffer(ctx, buffer, bufferLength) != 0) {
		spng_ctx_free(ctx);
		return 1;
	}

	struct spng_ihdr ihdr;
	if (spng_get_ihdr(ctx, &ihdr) != 0) {
		spng_ctx_free(ctx);
		return 1;
	}

	const INT32 srcW = (INT32)ihdr.width;
	const INT32 srcH = (INT32)ihdr.height;
	if (srcW <= 0 || srcH <= 0) {
		spng_ctx_free(ctx);
		return 1;
	}

	size_t imageSize = 0;
	if (spng_decoded_image_size(ctx, SPNG_FMT_RGB8, &imageSize) != 0) {
		spng_ctx_free(ctx);
		return 1;
	}

	UINT8* decoded = (UINT8*)malloc(imageSize);
	if (decoded == NULL) {
		spng_ctx_free(ctx);
		return 1;
	}

	if (spng_decode_image(ctx, decoded, imageSize, SPNG_FMT_RGB8, SPNG_DECODE_TRNS) != 0) {
		free(decoded);
		spng_ctx_free(ctx);
		return 1;
	}

	LanmaoArtwork = (UINT16*)BurnMalloc(LANMAO_SCREEN_W * LANMAO_SCREEN_H * sizeof(UINT16));
	if (LanmaoArtwork == NULL) {
		free(decoded);
		spng_ctx_free(ctx);
		return 1;
	}

	if (srcW == LANMAO_SCREEN_W && srcH == LANMAO_SCREEN_H) {
		for (INT32 y = 0; y < LANMAO_SCREEN_H; y++) {
			const UINT8* src = decoded + (y * LANMAO_SCREEN_W * 3);
			for (INT32 x = 0; x < LANMAO_SCREEN_W; x++) {
				UINT8 r = src[(x * 3) + 0];
				UINT8 g = src[(x * 3) + 1];
				UINT8 b = src[(x * 3) + 2];
				LanmaoArtwork[(y * LANMAO_SCREEN_W) + x] = LanmaoPackColor(r, g, b);
			}
		}
	} else {
		for (INT32 y = 0; y < LANMAO_SCREEN_H; y++) {
			const INT32 srcY = (y * srcH) / LANMAO_SCREEN_H;
			const UINT8* srcRow = decoded + (srcY * srcW * 3);
			for (INT32 x = 0; x < LANMAO_SCREEN_W; x++) {
				const INT32 srcX = (x * srcW) / LANMAO_SCREEN_W;
				const UINT8* src = srcRow + (srcX * 3);
				UINT8 r = src[0];
				UINT8 g = src[1];
				UINT8 b = src[2];
				LanmaoArtwork[(y * LANMAO_SCREEN_W) + x] = LanmaoPackColor(r, g, b);
			}
		}
	}

	free(decoded);
	spng_ctx_free(ctx);
	return 0;
}

static void DrvPaletteUpdate()
{
	for (INT32 i = 0; i < 0x10000; i++) {
		UINT8 r = ((i >> 11) & 0x1f) * 255 / 31;
		UINT8 g = ((i >> 5) & 0x3f) * 255 / 63;
		UINT8 b = (i & 0x1f) * 255 / 31;
		DrvPalette[i] = BurnHighCol(r, g, b, 0);
	}
}

static void LanmaoBuildArchivePath(TCHAR* outPath, const TCHAR* basePath)
{
	_tcsncpy(outPath, basePath, MAX_PATH - 1);
	outPath[MAX_PATH - 1] = 0;

	INT32 len = _tcslen(outPath);
	if (len >= 4) {
		const TCHAR* tailZip = outPath + (len - 4);
		if (_tcsicmp(tailZip, _T(".zip")) == 0) {
			return;
		}
	}
	while (len > 0) {
		TCHAR c = outPath[len - 1];
		if (!LanmaoIsPathSeparator(c)) {
			break;
		}
		outPath[len - 1] = 0;
		len--;
	}

	if (len >= 6) {
		const TCHAR* tail = outPath + (len - 6);
		if (_tcsncmp(tail, _T("lanmao"), 6) == 0) {
			return;
		}
	}

	if (len < MAX_PATH - 1) {
		outPath[len++] = LanmaoPreferredPathSeparator();
		outPath[len] = 0;
	}

	_tcsncpy(outPath + len, _T("lanmao"), MAX_PATH - len - 1);
	outPath[MAX_PATH - 1] = 0;
}

static INT32 LanmaoTryLoadFromArchive(const TCHAR* archivePath)
{
	char szArchiveA[MAX_PATH] = { 0 };
	void* pngBuffer = NULL;
	INT32 wrote = 0;

	if (ZipLoadOneFile(TCHARToANSI(archivePath, szArchiveA, MAX_PATH), "lanmao/background.png", &pngBuffer, &wrote) != 0 || pngBuffer == NULL || wrote <= 0) {
		if (pngBuffer) {
			free(pngBuffer);
		}
		pngBuffer = NULL;
		wrote = 0;

		if (ZipLoadOneFile(TCHARToANSI(archivePath, szArchiveA, MAX_PATH), "background.png", &pngBuffer, &wrote) != 0 || pngBuffer == NULL || wrote <= 0) {
			if (pngBuffer) {
				free(pngBuffer);
			}
			return 1;
		}
	}

	INT32 ret = LanmaoDecodeArtwork((UINT8*)pngBuffer, wrote);
	free(pngBuffer);
	return ret;
}

static INT32 LanmaoTryLoadFromFilePath(const TCHAR* basePath)
{
	TCHAR folderPath[MAX_PATH] = { 0 };
	TCHAR filePath[MAX_PATH] = { 0 };
	char filePathA[MAX_PATH] = { 0 };

	LanmaoBuildArchivePath(folderPath, basePath);

	_tcsncpy(filePath, folderPath, MAX_PATH - 1);
	filePath[MAX_PATH - 1] = 0;

	INT32 len = _tcslen(filePath);
	if (len <= 0 || len >= MAX_PATH - 1) {
		return 1;
	}

	TCHAR tail = filePath[len - 1];
	if (!LanmaoIsPathSeparator(tail)) {
		if (len >= MAX_PATH - 1) {
			return 1;
		}
		filePath[len++] = LanmaoPreferredPathSeparator();
		filePath[len] = 0;
	}

	const TCHAR* leaf = _T("background.png");
	INT32 leafLen = _tcslen(leaf);
	if (len + leafLen >= MAX_PATH) {
		return 1;
	}

	_tcsncpy(filePath + len, leaf, MAX_PATH - len - 1);
	filePath[MAX_PATH - 1] = 0;

	FILE* fp = fopen(TCHARToANSI(filePath, filePathA, MAX_PATH), "rb");
	if (fp == NULL) {
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	if (fileSize <= 0) {
		fclose(fp);
		return 1;
	}
	fseek(fp, 0, SEEK_SET);

	UINT8* buffer = (UINT8*)malloc(fileSize);
	if (buffer == NULL) {
		fclose(fp);
		return 1;
	}

	size_t readCount = fread(buffer, 1, fileSize, fp);
	fclose(fp);

	if (readCount != (size_t)fileSize) {
		free(buffer);
		return 1;
	}

	INT32 ret = LanmaoDecodeArtwork(buffer, (INT32)fileSize);
	free(buffer);
	return ret;
}

static INT32 LanmaoTryLoadFromBasePath(const TCHAR* basePath)
{
	TCHAR archivePath[MAX_PATH] = { 0 };

	LanmaoBuildArchivePath(archivePath, basePath);
	if (LanmaoTryLoadFromArchive(archivePath) == 0) {
		return 0;
	}

	// Try explicit .zip variant when the base path points to a directory name.
	INT32 len = _tcslen(archivePath);
	if (len > 0 && len < MAX_PATH - 4) {
		archivePath[len + 0] = _T('.');
		archivePath[len + 1] = _T('z');
		archivePath[len + 2] = _T('i');
		archivePath[len + 3] = _T('p');
		archivePath[len + 4] = 0;
		if (LanmaoTryLoadFromArchive(archivePath) == 0) {
			return 0;
		}
	}

	if (LanmaoTryLoadFromFilePath(basePath) == 0) {
		return 0;
	}

	return 1;
}

static INT32 LanmaoLoadArtwork()
{
	if (LanmaoArtwork != NULL) {
		return 0;
	}

	if (LanmaoTryLoadFromBasePath(szAppArtworkPath) == 0) {
		return 0;
	}

#if defined(__LIBRETRO__)
	if (g_rom_dir[0] != 0) {
		if (LanmaoTryLoadFromBasePath(g_rom_dir) == 0) {
			return 0;
		}
	}
#else
	for (INT32 i = 0; i < DIRS_MAX; i++) {
		if (szAppRomPaths[i][0] == 0) {
			continue;
		}

		if (LanmaoTryLoadFromBasePath(szAppRomPaths[i]) == 0) {
			return 0;
		}
	}
#endif

	return 1;
}

static void LanmaoFreeArtwork()
{
	if (LanmaoArtwork) {
		BurnFree(LanmaoArtwork);
		LanmaoArtwork = NULL;
	}
}

static inline void DrawPixel(INT32 x, INT32 y, UINT16 c)
{
	if ((UINT32)x < (UINT32)nScreenWidth && (UINT32)y < (UINT32)nScreenHeight) {
		pTransDraw[(y * nScreenWidth) + x] = c;
	}
}

static void DrawRect(INT32 x, INT32 y, INT32 w, INT32 h, UINT16 c)
{
	for (INT32 yy = 0; yy < h; yy++) {
		for (INT32 xx = 0; xx < w; xx++) {
			DrawPixel(x + xx, y + yy, c);
		}
	}
}

static void DrawFilledCircle(INT32 cx, INT32 cy, INT32 radius, UINT16 c)
{
	for (INT32 y = -radius; y <= radius; y++) {
		for (INT32 x = -radius; x <= radius; x++) {
			if ((x * x) + (y * y) <= (radius * radius)) {
				DrawPixel(cx + x, cy + y, c);
			}
		}
	}
}

static void DrawDigit(const LanmaoLayoutRect *r, UINT8 pattern)
{
	const UINT16 on = LanmaoPackColor(0xff, 0x00, 0x00);
	const UINT16 off = LanmaoPackColor(0x28, 0x00, 0x00);
	const INT32 vthick = (r->w / 6) > 2 ? (r->w / 6) : 2;
	const INT32 hthick = (r->h / 9) > 2 ? (r->h / 9) : 2;
	const INT32 marginx = (r->w / 10) > 1 ? (r->w / 10) : 1;
	const INT32 marginy = (r->h / 12) > 1 ? (r->h / 12) : 1;
	const INT32 horizw = r->w - ((marginx * 2) + vthick);
	const INT32 midy = r->y + (r->h / 2) - (hthick / 2);
	const INT32 topy = r->y + marginy;
	const INT32 boty = r->y + r->h - marginy - hthick;
	const INT32 leftx = r->x + marginx;
	const INT32 rightx = r->x + r->w - marginx - vthick;
	const INT32 upperh = midy - topy - hthick + 1;
	const INT32 lowerh = boty - midy - hthick + 1;

	DrawRect(leftx + (vthick / 2), topy, horizw, hthick, (pattern & 0x01) ? on : off);
	DrawRect(rightx, topy + hthick, vthick, upperh, (pattern & 0x02) ? on : off);
	DrawRect(rightx, midy + hthick, vthick, lowerh, (pattern & 0x04) ? on : off);
	DrawRect(leftx + (vthick / 2), boty, horizw, hthick, (pattern & 0x08) ? on : off);
	DrawRect(leftx, midy + hthick, vthick, lowerh, (pattern & 0x10) ? on : off);
	DrawRect(leftx, topy + hthick, vthick, upperh, (pattern & 0x20) ? on : off);
	DrawRect(leftx + (vthick / 2), midy, horizw, hthick, (pattern & 0x40) ? on : off);
}

static void DrawLed(const LanmaoLayoutRect *r, UINT8 state)
{
	// marywu.lay defines the lamp element with state 1 = dark and state 0 = bright,
	// so the AY8910 output bits are effectively active-low at the layout level.
	const UINT16 color = state ? LanmaoPackColor(0x19, 0x06, 0x07) : LanmaoPackColor(0xff, 0x40, 0x47);
	const INT32 cx = r->x + (r->w / 2);
	const INT32 cy = r->y + (r->h / 2);
	const INT32 radius = ((r->w < r->h) ? r->w : r->h) / 4;

	DrawFilledCircle(cx, cy, radius + 2, LanmaoPackColor(0x08, 0x02, 0x02));
	DrawFilledCircle(cx, cy, radius, color);
}

static INT32 DrvDraw()
{
	if (DrvRecalc) {
		DrvPaletteUpdate();
		DrvRecalc = 0;
	}

	if (LanmaoArtwork) {
		memcpy(pTransDraw, LanmaoArtwork, LANMAO_SCREEN_W * LANMAO_SCREEN_H * sizeof(UINT16));
	} else {
		BurnTransferClear();
	}

	for (INT32 i = 0; i < 26; i++) {
		DrawDigit(&LanmaoDigitLayout[i], LanmaoDigits[i]);
	}

	for (INT32 i = 0; i < 31; i++) {
		DrawLed(&LanmaoLedLayout[i], LanmaoLeds[i]);
	}

	BurnTransferCopy(DrvPalette);

	return 0;
}

static INT32 DrvDoReset()
{
	DrvReset = 0;

	mcs51_reset();
	AY8910Reset(0);
	AY8910Reset(1);
	BurnYM2413Reset();
	MSM6295Reset(0);
	Lanmao8279Reset();
	LanmaoI2cReset();

	memset(LanmaoDigits, 0, sizeof(LanmaoDigits));
	memset(LanmaoLeds, 0, sizeof(LanmaoLeds));

	LanmaoKbdLine = 0;
	LanmaoOkiBank = 0;
	LanmaoHopperMotor = 0;
	LanmaoHopperSensor = 0;
	LanmaoHopperClock = 0;
	LanmaoPort1Latch = 0xff;
	LanmaoPort3Latch = 0xff;
	DrvRecalc = 1;

	LanmaoSetOkiBank(0);

	return 0;
}

static INT32 LanmaoInit()
{
	AllMem = (UINT8*)BurnMalloc(0x010000 + 0x080000 + 0x000100 + 0x000800 + (0x10000 * sizeof(UINT32)));
	if (AllMem == NULL) return 1;

	memset(AllMem, 0, 0x010000 + 0x080000 + 0x000100 + 0x000800 + (0x10000 * sizeof(UINT32)));
	MemIndex();

	if (BurnLoadRom(DrvMcuROM, 0, 1)) return 1;
	if (BurnLoadRom(DrvSndROM, 1, 1)) return 1;
	if (BurnLoadRom(DrvI2CMem, 2, 1)) return 1;
	if (BurnLoadRom(DrvNVRAM, 3, 1)) return 1;

	i8052_init();
	mcs51_set_program_data(DrvMcuROM);
	mcs51_set_write_handler(LanmaoWrite);
	mcs51_set_read_handler(LanmaoRead);

	AY8910Init(0, 10738635 / 6, 0);
	AY8910Init(1, 10738635 / 6, 1);
	AY8910SetPorts(0, LanmaoAyDummyRead, LanmaoAyDummyRead, LanmaoAy0PortAWrite, LanmaoAy0PortBWrite);
	AY8910SetPorts(1, LanmaoAyDummyRead, LanmaoAyDummyRead, LanmaoAy1PortAWrite, LanmaoAy1PortBWrite);
	AY8910SetAllRoutes(0, 0.25, BURN_SND_ROUTE_BOTH);
	AY8910SetAllRoutes(1, 0.25, BURN_SND_ROUTE_BOTH);
	AY8910SetBuffered(mcs51TotalCycles, 10738635 / 12);

	BurnYM2413Init(3579545, 1);
	BurnYM2413SetAllRoutes(0.50, BURN_SND_ROUTE_BOTH);

	MSM6295Init(0, 1000000 / MSM6295_PIN7_HIGH, 1);
	MSM6295SetRoute(0, 0.60, BURN_SND_ROUTE_BOTH);

	GenericTilesInit();
	BurnSetRefreshRate(60.00);
	LanmaoLoadArtwork();

	DrvDoReset();

	return 0;
}

static INT32 DrvExit()
{
	GenericTilesExit();

	MSM6295Exit(0);
	BurnYM2413Exit();
	AY8910Exit(0);
	AY8910Exit(1);
	mcs51_exit();

	BurnFree(AllMem);
	AllMem = NULL;

	DrvMcuROM = NULL;
	DrvSndROM = NULL;
	DrvI2CMem = NULL;
	DrvNVRAM = NULL;
	DrvPalette = NULL;
	LanmaoFreeArtwork();

	return 0;
}

static INT32 DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	DrvInputs[0] = 0xff;
	DrvInputs[1] = 0xff;
	DrvInputs[2] = DrvDips[0];
	DrvInputs[3] = 0xff;

	for (INT32 i = 0; i < 8; i++) {
		if (DrvJoy1[i]) DrvInputs[0] &= ~(1 << i);
		if (DrvJoy2[i]) DrvInputs[1] &= ~(1 << i);
		if (DrvJoy3[i]) DrvInputs[3] &= ~(1 << i);
	}

	mcs51NewFrame();

	const INT32 nInterleave = 16;
	const INT32 nCyclesTotal = (10738635 / 12) / 60;
	INT32 nCyclesDone = 0;

	for (INT32 i = 0; i < nInterleave; i++) {
		INT32 nSegment = ((i + 1) * nCyclesTotal / nInterleave) - nCyclesDone;
		nCyclesDone += mcs51Run(nSegment);
		Lanmao8279Step();
	}

	if (LanmaoHopperMotor) {
		LanmaoHopperClock = (LanmaoHopperClock + 1) & 0x07;
		LanmaoHopperSensor = (LanmaoHopperClock < 4);
	} else {
		LanmaoHopperClock = 0;
		LanmaoHopperSensor = 0;
	}

	if (pBurnSoundOut) {
		AY8910Render(pBurnSoundOut, nBurnSoundLen);
		BurnYM2413Render(pBurnSoundOut, nBurnSoundLen);
		MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	if (pnMin) {
		*pnMin = 0x029735;
	}

	if (nAction & ACB_NVRAM) {
		struct BurnArea ba;

		memset(&ba, 0, sizeof(ba));
		ba.Data = DrvNVRAM;
		ba.nLen = 0x0800;
		ba.szName = "Lanmao NVRAM";
		BurnAcb(&ba);

		memset(&ba, 0, sizeof(ba));
		ba.Data = DrvI2CMem;
		ba.nLen = 0x0100;
		ba.szName = "Lanmao 24C02";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		mcs51_scan(nAction);
		AY8910Scan(nAction, pnMin);
		BurnYM2413Scan(nAction, pnMin);
		MSM6295Scan(nAction, pnMin);

		SCAN_VAR(LanmaoDigits);
		SCAN_VAR(LanmaoLeds);
		SCAN_VAR(LanmaoKbdLine);
		SCAN_VAR(LanmaoOkiBank);
		SCAN_VAR(LanmaoHopperMotor);
		SCAN_VAR(LanmaoHopperSensor);
		SCAN_VAR(LanmaoHopperClock);
		SCAN_VAR(DrvRecalc);

		SCAN_VAR(LanmaoI2cMode);
		SCAN_VAR(LanmaoI2cAddress);
		SCAN_VAR(LanmaoI2cShifter);
		SCAN_VAR(LanmaoI2cBitCount);
		SCAN_VAR(LanmaoI2cSelectedDevice);
		SCAN_VAR(LanmaoI2cSda);
		SCAN_VAR(LanmaoI2cScl);
		SCAN_VAR(LanmaoI2cSdaRead);

		SCAN_VAR(Lanmao8279DisplayRam);
		SCAN_VAR(Lanmao8279SensorRam);
		SCAN_VAR(Lanmao8279Fifo);
		SCAN_VAR(Lanmao8279Cmd);
		SCAN_VAR(Lanmao8279Status);
		SCAN_VAR(Lanmao8279DisplayPtr);
		SCAN_VAR(Lanmao8279SensorPtr);
		SCAN_VAR(Lanmao8279Scanner);
		SCAN_VAR(Lanmao8279KeyDown);
		SCAN_VAR(Lanmao8279Debounce);
		SCAN_VAR(Lanmao8279CtrlKey);
		SCAN_VAR(Lanmao8279AutoInc);
		SCAN_VAR(Lanmao8279ReadFlag);
		SCAN_VAR(Lanmao8279SpecialError);
	}

	if (nAction & ACB_WRITE) {
		LanmaoSetOkiBank(LanmaoOkiBank);
	}

	return 0;
}

static INT32 Panda2Init()
{
	return 1;
}

static INT32 Panda2Exit()
{
	return 0;
}

static INT32 Panda2Frame()
{
	return 0;
}

static INT32 Panda2Draw()
{
	return 0;
}

static INT32 Panda2Scan(INT32, INT32*)
{
	return 0;
}

struct BurnDriver BurnDrvPanda2 = {
	"panda2", NULL, NULL, NULL, "1996",
	"Panda 2\0", NULL, "Kelly", "Skeleton",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING, 1, HARDWARE_SKELETON, GBF_MISC, 0,
	NULL, panda2RomInfo, panda2RomName, NULL, NULL, NULL, NULL, SkeletonInputInfo, NULL,
	Panda2Init, Panda2Exit, Panda2Frame, Panda2Draw, Panda2Scan, NULL, 0,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvLanmao = {
	"lanmao", NULL, NULL, NULL, "2003",
	"Lan Mao\0", NULL, "Changsheng Electric Company", "Skeleton",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_SKELETON, GBF_MISC, 0,
	NULL, lanmaoRomInfo, lanmaoRomName, NULL, NULL, NULL, NULL, LanmaoInputInfo, LanmaoDIPInfo,
	LanmaoInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x10,
	800, 750, 4, 3
};

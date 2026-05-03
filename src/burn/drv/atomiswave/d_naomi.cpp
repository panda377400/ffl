#include "burnint.h"
#include "awave_core.h"

static UINT8 NaomiInputPort0[14];
static UINT8 NaomiInputPort1[14];
static UINT8 NaomiReset;
static UINT8 NaomiResetLatch;
static UINT8 NaomiTest;
static UINT8 NaomiService;
static ClearOpposite<1, UINT32> NaomiClearOppositesP1;
static ClearOpposite<2, UINT32> NaomiClearOppositesP2;
static const NaomiGameConfig* NaomiCurrentGame = NULL;

static const UINT32 NaomiJoypadMap[] = {
	NAOMI_INPUT_BTN1,   // JOYPAD_B
	NAOMI_INPUT_BTN3,   // JOYPAD_Y
	NAOMI_INPUT_COIN,   // JOYPAD_SELECT
	NAOMI_INPUT_START,  // JOYPAD_START
	NAOMI_INPUT_UP,     // JOYPAD_UP
	NAOMI_INPUT_DOWN,   // JOYPAD_DOWN
	NAOMI_INPUT_LEFT,   // JOYPAD_LEFT
	NAOMI_INPUT_RIGHT,  // JOYPAD_RIGHT
	NAOMI_INPUT_BTN2,   // JOYPAD_A
	NAOMI_INPUT_BTN4,   // JOYPAD_X
	NAOMI_INPUT_BTN6,   // JOYPAD_L
	NAOMI_INPUT_BTN5,   // JOYPAD_R
	NAOMI_INPUT_BTN8,   // JOYPAD_L2
	NAOMI_INPUT_BTN7,   // JOYPAD_R2
	NAOMI_INPUT_TEST,   // JOYPAD_L3
	NAOMI_INPUT_SERVICE // JOYPAD_R3
};

static struct BurnInputInfo Naomi3BtnInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Button 3",  BIT_DIGITAL, NaomiInputPort0 + 6,  "p1 fire 3" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 12, "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 13, "p1 coin"   },

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Button 2",  BIT_DIGITAL, NaomiInputPort1 + 5,  "p2 fire 2" },
	{ "P2 Button 3",  BIT_DIGITAL, NaomiInputPort1 + 6,  "p2 fire 3" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 12, "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 13, "p2 coin"   },

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(Naomi3Btn)

static struct BurnInputInfo Naomi1BtnInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 12, "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 13, "p1 coin"   },

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 12, "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 13, "p2 coin"   },

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(Naomi1Btn)

static struct BurnInputInfo Naomi2BtnInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 12, "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 13, "p1 coin"   },

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Button 2",  BIT_DIGITAL, NaomiInputPort1 + 5,  "p2 fire 2" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 12, "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 13, "p2 coin"   },

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(Naomi2Btn)

static struct BurnInputInfo NaomiStd4BtnInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Button 3",  BIT_DIGITAL, NaomiInputPort0 + 6,  "p1 fire 3" },
	{ "P1 Button 4",  BIT_DIGITAL, NaomiInputPort0 + 7,  "p1 fire 4" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 12, "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 13, "p1 coin"   },

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Button 2",  BIT_DIGITAL, NaomiInputPort1 + 5,  "p2 fire 2" },
	{ "P2 Button 3",  BIT_DIGITAL, NaomiInputPort1 + 6,  "p2 fire 3" },
	{ "P2 Button 4",  BIT_DIGITAL, NaomiInputPort1 + 7,  "p2 fire 4" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 12, "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 13, "p2 coin"   },

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(NaomiStd4Btn)

static struct BurnInputInfo NaomiCapcom4BtnInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Button 3",  BIT_DIGITAL, NaomiInputPort0 + 7,  "p1 fire 3" },
	{ "P1 Button 4",  BIT_DIGITAL, NaomiInputPort0 + 8,  "p1 fire 4" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 12, "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 13, "p1 coin"   },

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Button 2",  BIT_DIGITAL, NaomiInputPort1 + 5,  "p2 fire 2" },
	{ "P2 Button 3",  BIT_DIGITAL, NaomiInputPort1 + 7,  "p2 fire 3" },
	{ "P2 Button 4",  BIT_DIGITAL, NaomiInputPort1 + 8,  "p2 fire 4" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 12, "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 13, "p2 coin"   },

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(NaomiCapcom4Btn)

static struct BurnInputInfo Naomi6BtnInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Button 3",  BIT_DIGITAL, NaomiInputPort0 + 6,  "p1 fire 3" },
	{ "P1 Button 4",  BIT_DIGITAL, NaomiInputPort0 + 7,  "p1 fire 4" },
	{ "P1 Button 5",  BIT_DIGITAL, NaomiInputPort0 + 8,  "p1 fire 5" },
	{ "P1 Button 6",  BIT_DIGITAL, NaomiInputPort0 + 9,  "p1 fire 6" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 12, "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 13, "p1 coin"   },

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Button 2",  BIT_DIGITAL, NaomiInputPort1 + 5,  "p2 fire 2" },
	{ "P2 Button 3",  BIT_DIGITAL, NaomiInputPort1 + 6,  "p2 fire 3" },
	{ "P2 Button 4",  BIT_DIGITAL, NaomiInputPort1 + 7,  "p2 fire 4" },
	{ "P2 Button 5",  BIT_DIGITAL, NaomiInputPort1 + 8,  "p2 fire 5" },
	{ "P2 Button 6",  BIT_DIGITAL, NaomiInputPort1 + 9,  "p2 fire 6" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 12, "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 13, "p2 coin"   },

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(Naomi6Btn)

static INT32 NaomiGetZipName(char** pszName, UINT32 i)
{
	if (pszName == NULL) {
		return 1;
	}

	if (i == 0) {
		*pszName = (char*)((NaomiCurrentGame != NULL && NaomiCurrentGame->biosZipName != NULL)
			? NaomiCurrentGame->biosZipName
			: "naomi");
		return 0;
	}

	if (i == 1) {
		*pszName = BurnDrvGetTextA(DRV_NAME);
		return (*pszName != NULL) ? 0 : 1;
	}

	if (i == 2) {
		*pszName = BurnDrvGetTextA(DRV_PARENT);
		return (*pszName != NULL) ? 0 : 1;
	}

	*pszName = NULL;
	return 1;
}

template<int N>
static UINT32 NaomiBuildPadStateTyped(UINT8* inputs, ClearOpposite<N, UINT32>& clearOpposites)
{
	UINT32 state = 0;

	if (inputs[0])  state |= NAOMI_INPUT_UP;
	if (inputs[1])  state |= NAOMI_INPUT_DOWN;
	if (inputs[2])  state |= NAOMI_INPUT_LEFT;
	if (inputs[3])  state |= NAOMI_INPUT_RIGHT;
	if (inputs[4])  state |= NAOMI_INPUT_BTN1;
	if (inputs[5])  state |= NAOMI_INPUT_BTN2;
	if (inputs[6])  state |= NAOMI_INPUT_BTN3;
	if (inputs[7])  state |= NAOMI_INPUT_BTN4;
	if (inputs[8])  state |= NAOMI_INPUT_BTN5;
	if (inputs[9])  state |= NAOMI_INPUT_BTN6;
	if (inputs[10]) state |= NAOMI_INPUT_BTN7;
	if (inputs[11]) state |= NAOMI_INPUT_BTN8;
	if (inputs[12]) state |= NAOMI_INPUT_START;
	if (inputs[13]) state |= NAOMI_INPUT_COIN;

	clearOpposites.neu(0, state, NAOMI_INPUT_UP, NAOMI_INPUT_DOWN, NAOMI_INPUT_LEFT, NAOMI_INPUT_RIGHT);
	return state;
}

static INT32 DrvInitCommon(const NaomiGameConfig* config)
{
	memset(NaomiInputPort0, 0, sizeof(NaomiInputPort0));
	memset(NaomiInputPort1, 0, sizeof(NaomiInputPort1));
	NaomiReset = 0;
	NaomiResetLatch = 0;
	NaomiTest = 0;
	NaomiService = 0;
	NaomiClearOppositesP1.reset();
	NaomiClearOppositesP2.reset();
	NaomiCurrentGame = config;

	if (NaomiCoreInit(config)) {
		NaomiCoreExit();
		return 1;
	}

	return 0;
}

static INT32 DrvExit()
{
	NaomiCurrentGame = NULL;
	return NaomiCoreExit();
}

static INT32 DrvDoReset()
{
	if (NaomiCurrentGame == NULL) {
		return 1;
	}

	memset(NaomiInputPort0, 0, sizeof(NaomiInputPort0));
	memset(NaomiInputPort1, 0, sizeof(NaomiInputPort1));
	NaomiReset = 0;
	NaomiResetLatch = 1;
	NaomiTest = 0;
	NaomiService = 0;
	NaomiClearOppositesP1.reset();
	NaomiClearOppositesP2.reset();

	NaomiCoreExit();
	return NaomiCoreInit(NaomiCurrentGame);
}

static INT32 DrvFrame()
{
	UINT32 p1State = NaomiBuildPadStateTyped(NaomiInputPort0, NaomiClearOppositesP1);
	UINT32 p2State = NaomiBuildPadStateTyped(NaomiInputPort1, NaomiClearOppositesP2);
	INT32 ret;

	if (NaomiTest) {
		p1State |= NAOMI_INPUT_TEST;
	}
	if (NaomiService) {
		p1State |= NAOMI_INPUT_SERVICE;
	}

	NaomiCoreSetPadState(0, p1State);
	NaomiCoreSetPadState(1, p2State);
	NaomiCoreSetPadState(2, 0);
	NaomiCoreSetPadState(3, 0);

	if (NaomiReset) {
		if (!NaomiResetLatch) {
			return DrvDoReset();
		}
	} else {
		NaomiResetLatch = 0;
	}

	ret = NaomiCoreFrame();

	if (ret == 0 && pBurnDraw && NaomiCoreWantsRedraw()) {
		BurnDrvRedraw();
	}

	return ret;
}

static INT32 DrvDraw()
{
	return NaomiCoreDraw();
}

static INT32 DrvScan(INT32 nAction, INT32* pnMin)
{
	if (pnMin) {
		*pnMin = 0x029900;
	}

	if (nAction & ACB_VOLATILE) {
		SCAN_VAR(NaomiReset);
		SCAN_VAR(NaomiResetLatch);
		SCAN_VAR(NaomiTest);
		SCAN_VAR(NaomiService);
		NaomiClearOppositesP1.scan();
		NaomiClearOppositesP2.scan();
	}

	return NaomiCoreScan(nAction, pnMin);
}

#define NAOMI_CONFIG(_var, _zipEntries) \
static const NaomiGameConfig NaomiGame##_var = { \
	#_var, #_var ".zip", _zipEntries, "naomi", "naomi", "naomi", \
	NaomiJoypadMap, sizeof(NaomiJoypadMap) / sizeof(NaomiJoypadMap[0]), \
	NULL, 0, NAOMI_GAME_INPUT_DIGITAL, AWAVE_PLATFORM_NAOMI, NULL, \
}

#define DEFINE_NAOMI_INIT(_var) \
static INT32 _var##Init() { return DrvInitCommon(&NaomiGame##_var); }

static const NaomiZipEntry CspikeZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23210.ic22",  1 },
	{ "mpr-23198.ic1",   2 },
	{ "mpr-23199.ic2",   3 },
	{ "mpr-23200.ic3",   4 },
	{ "mpr-23201.ic4",   5 },
	{ "mpr-23202.ic5",   6 },
	{ "mpr-23203.ic6",   7 },
	{ "mpr-23204.ic7",   8 },
	{ "mpr-23205.ic8",   9 },
	{ "mpr-23206.ic9",   10 },
	{ "mpr-23207.ic10",  11 },
	{ "mpr-23208.ic11",  12 },
	{ "mpr-23209.ic12s", 13 },
	{ NULL, -1 }
};

static const NaomiZipEntry Mvsc2ZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23062a.ic22", 1 },
	{ "mpr-23048.ic1",   2 },
	{ "mpr-23049.ic2",   3 },
	{ "mpr-23050.ic3",   4 },
	{ "mpr-23051.ic4",   5 },
	{ "mpr-23052.ic5",   6 },
	{ "mpr-23053.ic6",   7 },
	{ "mpr-23054.ic7",   8 },
	{ "mpr-23055.ic8",   9 },
	{ "mpr-23056.ic9",   10 },
	{ "mpr-23057.ic10",  11 },
	{ "mpr-23058.ic11",  12 },
	{ "mpr-23059.ic12s", 13 },
	{ "mpr-23060.ic13s", 14 },
	{ "mpr-23061.ic14s", 15 },
	{ NULL, -1 }
};

static const NaomiZipEntry Gram2000ZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23377.ic11",  1 },
	{ "mpr-23357.ic17s", 2 },
	{ "mpr-23358.ic18",  3 },
	{ "mpr-23359.ic19s", 4 },
	{ "mpr-23360.ic20",  5 },
	{ "mpr-23361.ic21s", 6 },
	{ "mpr-23362.ic22",  7 },
	{ "mpr-23363.ic23s", 8 },
	{ "mpr-23364.ic24",  9 },
	{ "mpr-23365.ic25s", 10 },
	{ "mpr-23366.ic26",  11 },
	{ "mpr-23367.ic27s", 12 },
	{ "mpr-23368.ic28",  13 },
	{ "mpr-23369.ic29",  14 },
	{ "mpr-23370.ic30s", 15 },
	{ "mpr-23371.ic31",  16 },
	{ "mpr-23372.ic32s", 17 },
	{ "mpr-23373.ic33",  18 },
	{ "mpr-23374.ic34s", 19 },
	{ "mpr-23375.ic35",  20 },
	{ "mpr-23376.ic36s", 21 },
	{ NULL, -1 }
};

static const NaomiZipEntry CapsnkZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23511c.ic22", 1 },
	{ "mpr-23504.ic1",   2 },
	{ "mpr-23505.ic2",   3 },
	{ "mpr-23506.ic3",   4 },
	{ "mpr-23507.ic4",   5 },
	{ "mpr-23508.ic5",   6 },
	{ "mpr-23509.ic6",   7 },
	{ "mpr-23510.ic7",   8 },
	{ NULL, -1 }
};

static const NaomiZipEntry CapsnkaZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23511a.ic22", 1 },
	{ "mpr-23504.ic1",   2 },
	{ "mpr-23505.ic2",   3 },
	{ "mpr-23506.ic3",   4 },
	{ "mpr-23507.ic4",   5 },
	{ "mpr-23508.ic5",   6 },
	{ "mpr-23509.ic6",   7 },
	{ "mpr-23510.ic7",   8 },
	{ NULL, -1 }
};

static const NaomiZipEntry CapsnkbZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23511.ic22",  1 },
	{ "mpr-23504.ic1",   2 },
	{ "mpr-23505.ic2",   3 },
	{ "mpr-23506.ic3",   4 },
	{ "mpr-23507.ic4",   5 },
	{ "mpr-23508.ic5",   6 },
	{ "mpr-23509.ic6",   7 },
	{ "mpr-23510.ic7",   8 },
	{ NULL, -1 }
};

static const NaomiZipEntry Doa2mZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "doa2verm.ic22",   1 },
	{ "mpr-22100.ic1",   2 },
	{ "mpr-22101.ic2",   3 },
	{ "mpr-22102.ic3",   4 },
	{ "mpr-22103.ic4",   5 },
	{ "mpr-22104.ic5",   6 },
	{ "mpr-22105.ic6",   7 },
	{ "mpr-22106.ic7",   8 },
	{ "mpr-22107.ic8",   9 },
	{ "mpr-22108.ic9",   10 },
	{ "mpr-22109.ic10",  11 },
	{ "mpr-22110.ic11",  12 },
	{ "mpr-22111.ic12s", 13 },
	{ "mpr-22112.ic13s", 14 },
	{ "mpr-22113.ic14s", 15 },
	{ "mpr-22114.ic15s", 16 },
	{ "mpr-22115.ic16s", 17 },
	{ "mpr-22116.ic17s", 18 },
	{ "mpr-22117.ic18s", 19 },
	{ "mpr-22118.ic19s", 20 },
	{ "mpr-22119.ic20s", 21 },
	{ "mpr-22120.ic21s", 22 },
	{ NULL, -1 }
};

static const NaomiZipEntry Doa2ZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22207.ic22",  1 },
	{ "mpr-22100.ic1",   2 },
	{ "mpr-22101.ic2",   3 },
	{ "mpr-22102.ic3",   4 },
	{ "mpr-22103.ic4",   5 },
	{ "mpr-22104.ic5",   6 },
	{ "mpr-22105.ic6",   7 },
	{ "mpr-22106.ic7",   8 },
	{ "mpr-22107.ic8",   9 },
	{ "mpr-22108.ic9",   10 },
	{ "mpr-22109.ic10",  11 },
	{ "mpr-22110.ic11",  12 },
	{ "mpr-22111.ic12s", 13 },
	{ "mpr-22112.ic13s", 14 },
	{ "mpr-22113.ic14s", 15 },
	{ "mpr-22114.ic15s", 16 },
	{ "mpr-22115.ic16s", 17 },
	{ "mpr-22116.ic17s", 18 },
	{ "mpr-22117.ic18s", 19 },
	{ "mpr-22118.ic19s", 20 },
	{ "mpr-22119.ic20s", 21 },
	{ "mpr-22120.ic21s", 22 },
	{ NULL, -1 }
};

static const NaomiZipEntry Doa2aZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22121a.ic22", 1 },
	{ "mpr-22100.ic1",   2 },
	{ "mpr-22101.ic2",   3 },
	{ "mpr-22102.ic3",   4 },
	{ "mpr-22103.ic4",   5 },
	{ "mpr-22104.ic5",   6 },
	{ "mpr-22105.ic6",   7 },
	{ "mpr-22106.ic7",   8 },
	{ "mpr-22107.ic8",   9 },
	{ "mpr-22108.ic9",   10 },
	{ "mpr-22109.ic10",  11 },
	{ "mpr-22110.ic11",  12 },
	{ "mpr-22111.ic12s", 13 },
	{ "mpr-22112.ic13s", 14 },
	{ "mpr-22113.ic14s", 15 },
	{ "mpr-22114.ic15s", 16 },
	{ "mpr-22115.ic16s", 17 },
	{ "mpr-22116.ic17s", 18 },
	{ "mpr-22117.ic18s", 19 },
	{ "mpr-22118.ic19s", 20 },
	{ "mpr-22119.ic20s", 21 },
	{ "mpr-22120.ic21s", 22 },
	{ NULL, -1 }
};

static const NaomiZipEntry PstoneZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-21597.ic22",  1 },
	{ "mpr-21589.ic1",   2 },
	{ "mpr-21590.ic2",   3 },
	{ "mpr-21591.ic3",   4 },
	{ "mpr-21592.ic4",   5 },
	{ "mpr-21593.ic5",   6 },
	{ "mpr-21594.ic6",   7 },
	{ "mpr-21595.ic7",   8 },
	{ "mpr-21596.ic8",   9 },
	{ NULL, -1 }
};

static const NaomiZipEntry Pstone2ZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23127.ic22",  1 },
	{ "mpr-23118.ic1",   2 },
	{ "mpr-23119.ic2",   3 },
	{ "mpr-23120.ic3",   4 },
	{ "mpr-23121.ic4",   5 },
	{ "mpr-23122.ic5",   6 },
	{ "mpr-23123.ic6",   7 },
	{ "mpr-23124.ic7",   8 },
	{ "mpr-23125.ic8",   9 },
	{ "mpr-23126.ic9",   10 },
	{ NULL, -1 }
};

static const NaomiZipEntry Pstone2bZipEntries[] = {
	{ "00.ic1",  0 },
	{ "01.ic2",  1 },
	{ "02.ic3",  2 },
	{ "03.ic4",  3 },
	{ "04.ic5",  4 },
	{ "05.ic6",  5 },
	{ "06.ic7",  6 },
	{ "07.ic8",  7 },
	{ "08.ic9",  8 },
	{ "09.ic10", 9 },
	{ NULL, -1 }
};

static const NaomiZipEntry ToyfightZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22035.ic22",  1 },
	{ "mpr-22025.ic1",   2 },
	{ "mpr-22026.ic2",   3 },
	{ "mpr-22027.ic3",   4 },
	{ "mpr-22028.ic4",   5 },
	{ "mpr-22029.ic5",   6 },
	{ "mpr-22030.ic6",   7 },
	{ "mpr-22031.ic7",   8 },
	{ "mpr-22032.ic8",   9 },
	{ "mpr-22033.ic9",   10 },
	{ "mpr-22034.ic10",  11 },
	{ NULL, -1 }
};

static const NaomiZipEntry GundmctZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23638.ic22",  1 },
	{ "mpr-23628.ic1",   2 },
	{ "mpr-23629.ic2",   3 },
	{ "mpr-23630.ic3",   4 },
	{ "mpr-23631.ic4",   5 },
	{ "mpr-23632.ic5",   6 },
	{ "mpr-23633.ic6",   7 },
	{ "mpr-23634.ic7",   8 },
	{ "mpr-23635.ic8",   9 },
	{ "mpr-23636.ic9",   10 },
	{ "mpr-23637.ic10",  11 },
	{ NULL, -1 }
};

static const NaomiZipEntry PuyodaZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22206.ic22",  1 },
	{ "mpr-22186.ic1",   2 },
	{ "mpr-22187.ic2",   3 },
	{ "mpr-22188.ic3",   4 },
	{ "mpr-22189.ic4",   5 },
	{ "mpr-22190.ic5",   6 },
	{ "mpr-22191.ic6",   7 },
	{ "mpr-22192.ic7",   8 },
	{ "mpr-22193.ic8",   9 },
	{ "mpr-22194.ic9",   10 },
	{ "mpr-22195.ic10",  11 },
	{ "mpr-22196.ic11",  12 },
	{ "mpr-22197.ic12s", 13 },
	{ "mpr-22198.ic13s", 14 },
	{ "mpr-22199.ic14s", 15 },
	{ "mpr-22200.ic15s", 16 },
	{ "mpr-22201.ic16s", 17 },
	{ "mpr-22202.ic17s", 18 },
	{ "mpr-22203.ic18s", 19 },
	{ "mpr-22204.ic19s", 20 },
	{ "mpr-22205.ic20s", 21 },
	{ NULL, -1 }
};

static const NaomiZipEntry VirnbaZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23073.ic22",  1 },
	{ "mpr-22928.ic1",   2 },
	{ "mpr-22929.ic2",   3 },
	{ "mpr-22930.ic3",   4 },
	{ "mpr-22931.ic4",   5 },
	{ "mpr-22932.ic5",   6 },
	{ "mpr-22933.ic6",   7 },
	{ "mpr-22934.ic7",   8 },
	{ "mpr-22935.ic8",   9 },
	{ "mpr-22936.ic9",   10 },
	{ "mpr-22937.ic10",  11 },
	{ "mpr-22938.ic11",  12 },
	{ "mpr-22939.ic12s", 13 },
	{ "mpr-22940.ic13s", 14 },
	{ "mpr-22941.ic14s", 15 },
	{ "mpr-22942.ic15s", 16 },
	{ "mpr-22943.ic16s", 17 },
	{ "mpr-22944.ic17s", 18 },
	{ "mpr-22945.ic18s", 19 },
	{ "mpr-22946.ic19s", 20 },
	{ "mpr-22947.ic20s", 21 },
	{ "mpr-22948.ic21s", 22 },
	{ NULL, -1 }
};

static const NaomiZipEntry VirnbaoZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22949.ic22",  1 },
	{ "mpr-22928.ic1",   2 },
	{ "mpr-22929.ic2",   3 },
	{ "mpr-22930.ic3",   4 },
	{ "mpr-22931.ic4",   5 },
	{ "mpr-22932.ic5",   6 },
	{ "mpr-22933.ic6",   7 },
	{ "mpr-22934.ic7",   8 },
	{ "mpr-22935.ic8",   9 },
	{ "mpr-22936.ic9",   10 },
	{ "mpr-22937.ic10",  11 },
	{ "mpr-22938.ic11",  12 },
	{ "mpr-22939.ic12s", 13 },
	{ "mpr-22940.ic13s", 14 },
	{ "mpr-22941.ic14s", 15 },
	{ "mpr-22942.ic15s", 16 },
	{ "mpr-22943.ic16s", 17 },
	{ "mpr-22944.ic17s", 18 },
	{ "mpr-22945.ic18s", 19 },
	{ "mpr-22946.ic19s", 20 },
	{ "mpr-22947.ic20s", 21 },
	{ "mpr-22948.ic21s", 22 },
	{ NULL, -1 }
};

static const NaomiZipEntry WwfroyalZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22261.ic22",  1 },
	{ "mpr-22262.ic1",   2 },
	{ "mpr-22263.ic2",   3 },
	{ "mpr-22264.ic3",   4 },
	{ "mpr-22265.ic4",   5 },
	{ "mpr-22266.ic5",   6 },
	{ "mpr-22267.ic6",   7 },
	{ "mpr-22268.ic7",   8 },
	{ "mpr-22269.ic8",   9 },
	{ NULL, -1 }
};

static const NaomiZipEntry SpawnZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22977b.ic22", 1 },
	{ "mpr-22967.ic1",   2 },
	{ "mpr-22968.ic2",   3 },
	{ "mpr-22969.ic3",   4 },
	{ "mpr-22970.ic4",   5 },
	{ "mpr-22971.ic5",   6 },
	{ "mpr-22972.ic6",   7 },
	{ "mpr-22973.ic7",   8 },
	{ "mpr-22974.ic8",   9 },
	{ "mpr-22975.ic9",   10 },
	{ "mpr-22976.ic10",  11 },
	{ NULL, -1 }
};

static const NaomiZipEntry VtennisZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-22927.ic22",  1 },
	{ "mpr-22916.ic1",   2 },
	{ "mpr-22917.ic2",   3 },
	{ "mpr-22918.ic3",   4 },
	{ "mpr-22919.ic4",   5 },
	{ "mpr-22920.ic5",   6 },
	{ "mpr-22921.ic6",   7 },
	{ "mpr-22922.ic7",   8 },
	{ "mpr-22923.ic8",   9 },
	{ "mpr-22924.ic9",   10 },
	{ "mpr-22925.ic10",  11 },
	{ "mpr-22926.ic11",  12 },
	{ NULL, -1 }
};

static const NaomiZipEntry Zerogu2ZipEntries[] = {
	{ "epr-21576h.ic27", 0 },
	{ "epr-23689.ic22",  1 },
	{ "mpr-23684.ic1",   2 },
	{ "mpr-23685.ic2",   3 },
	{ "mpr-23686.ic3",   4 },
	{ "mpr-23687.ic4",   5 },
	{ "mpr-23688.ic5",   6 },
	{ NULL, -1 }
};

NAOMI_CONFIG(Cspike, CspikeZipEntries);
NAOMI_CONFIG(Mvsc2, Mvsc2ZipEntries);
NAOMI_CONFIG(Gram2000, Gram2000ZipEntries);
NAOMI_CONFIG(Capsnk, CapsnkZipEntries);
NAOMI_CONFIG(Capsnka, CapsnkaZipEntries);
NAOMI_CONFIG(Capsnkb, CapsnkbZipEntries);
NAOMI_CONFIG(Doa2m, Doa2mZipEntries);
NAOMI_CONFIG(Doa2, Doa2ZipEntries);
NAOMI_CONFIG(Doa2a, Doa2aZipEntries);
NAOMI_CONFIG(Pstone, PstoneZipEntries);
NAOMI_CONFIG(Pstone2, Pstone2ZipEntries);
NAOMI_CONFIG(Pstone2b, Pstone2bZipEntries);
NAOMI_CONFIG(Toyfight, ToyfightZipEntries);
NAOMI_CONFIG(Gundmct, GundmctZipEntries);
NAOMI_CONFIG(Puyoda, PuyodaZipEntries);
NAOMI_CONFIG(Virnba, VirnbaZipEntries);
NAOMI_CONFIG(Virnbao, VirnbaoZipEntries);
NAOMI_CONFIG(Wwfroyal, WwfroyalZipEntries);
NAOMI_CONFIG(Spawn, SpawnZipEntries);
NAOMI_CONFIG(Vtennis, VtennisZipEntries);
NAOMI_CONFIG(Zerogu2, Zerogu2ZipEntries);

DEFINE_NAOMI_INIT(Cspike)
DEFINE_NAOMI_INIT(Mvsc2)
DEFINE_NAOMI_INIT(Gram2000)
DEFINE_NAOMI_INIT(Capsnk)
DEFINE_NAOMI_INIT(Capsnka)
DEFINE_NAOMI_INIT(Capsnkb)
DEFINE_NAOMI_INIT(Doa2m)
DEFINE_NAOMI_INIT(Doa2)
DEFINE_NAOMI_INIT(Doa2a)
DEFINE_NAOMI_INIT(Pstone)
DEFINE_NAOMI_INIT(Pstone2)
DEFINE_NAOMI_INIT(Pstone2b)
DEFINE_NAOMI_INIT(Toyfight)
DEFINE_NAOMI_INIT(Gundmct)
DEFINE_NAOMI_INIT(Puyoda)
DEFINE_NAOMI_INIT(Virnba)
DEFINE_NAOMI_INIT(Virnbao)
DEFINE_NAOMI_INIT(Wwfroyal)
DEFINE_NAOMI_INIT(Spawn)
DEFINE_NAOMI_INIT(Vtennis)
DEFINE_NAOMI_INIT(Zerogu2)

static struct BurnRomInfo cspikeRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23210.ic22",  0x0400000, 0xa15c54b5, BRF_PRG | BRF_ESS },
	{ "mpr-23198.ic1",   0x0800000, 0xce8d3edf, BRF_PRG | BRF_ESS },
	{ "mpr-23199.ic2",   0x0800000, 0x0979392a, BRF_PRG | BRF_ESS },
	{ "mpr-23200.ic3",   0x0800000, 0xe4b2db33, BRF_PRG | BRF_ESS },
	{ "mpr-23201.ic4",   0x0800000, 0xc55ca0fa, BRF_PRG | BRF_ESS },
	{ "mpr-23202.ic5",   0x0800000, 0x983bb21c, BRF_PRG | BRF_ESS },
	{ "mpr-23203.ic6",   0x0800000, 0xf61b8d96, BRF_PRG | BRF_ESS },
	{ "mpr-23204.ic7",   0x0800000, 0x03593ecd, BRF_PRG | BRF_ESS },
	{ "mpr-23205.ic8",   0x0800000, 0xe8c9349b, BRF_PRG | BRF_ESS },
	{ "mpr-23206.ic9",   0x0800000, 0x8089d80f, BRF_PRG | BRF_ESS },
	{ "mpr-23207.ic10",  0x0800000, 0x39f692a1, BRF_PRG | BRF_ESS },
	{ "mpr-23208.ic11",  0x0800000, 0xb9494f4b, BRF_PRG | BRF_ESS },
	{ "mpr-23209.ic12s", 0x0800000, 0x560188c0, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(cspike)
STD_ROM_FN(cspike)

static struct BurnRomInfo mvsc2RomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23062a.ic22", 0x0400000, 0x96038276, BRF_PRG | BRF_ESS },
	{ "mpr-23048.ic1",   0x0800000, 0x93d7a63a, BRF_PRG | BRF_ESS },
	{ "mpr-23049.ic2",   0x0800000, 0x003dcce0, BRF_PRG | BRF_ESS },
	{ "mpr-23050.ic3",   0x0800000, 0x1d6b88a7, BRF_PRG | BRF_ESS },
	{ "mpr-23051.ic4",   0x0800000, 0x01226aaa, BRF_PRG | BRF_ESS },
	{ "mpr-23052.ic5",   0x0800000, 0x74bee120, BRF_PRG | BRF_ESS },
	{ "mpr-23053.ic6",   0x0800000, 0xd92d4401, BRF_PRG | BRF_ESS },
	{ "mpr-23054.ic7",   0x0800000, 0x78ba02e8, BRF_PRG | BRF_ESS },
	{ "mpr-23055.ic8",   0x0800000, 0x84319604, BRF_PRG | BRF_ESS },
	{ "mpr-23056.ic9",   0x0800000, 0xd7386034, BRF_PRG | BRF_ESS },
	{ "mpr-23057.ic10",  0x0800000, 0xa3f087db, BRF_PRG | BRF_ESS },
	{ "mpr-23058.ic11",  0x0800000, 0x61a6cc5d, BRF_PRG | BRF_ESS },
	{ "mpr-23059.ic12s", 0x0800000, 0x64808024, BRF_PRG | BRF_ESS },
	{ "mpr-23060.ic13s", 0x0800000, 0x67519942, BRF_PRG | BRF_ESS },
	{ "mpr-23061.ic14s", 0x0800000, 0xfb1844c4, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(mvsc2)
STD_ROM_FN(mvsc2)

static struct BurnRomInfo gram2000RomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23377.ic11",  0x0400000, 0x4ca3149c, BRF_PRG | BRF_ESS },
	{ "mpr-23357.ic17s", 0x0800000, 0xeaf77487, BRF_PRG | BRF_ESS },
	{ "mpr-23358.ic18",  0x0800000, 0x96819a5b, BRF_PRG | BRF_ESS },
	{ "mpr-23359.ic19s", 0x0800000, 0x757b9e89, BRF_PRG | BRF_ESS },
	{ "mpr-23360.ic20",  0x0800000, 0xb38d28ff, BRF_PRG | BRF_ESS },
	{ "mpr-23361.ic21s", 0x0800000, 0x680d7d77, BRF_PRG | BRF_ESS },
	{ "mpr-23362.ic22",  0x0800000, 0x84b7988d, BRF_PRG | BRF_ESS },
	{ "mpr-23363.ic23s", 0x0800000, 0x17ae2b21, BRF_PRG | BRF_ESS },
	{ "mpr-23364.ic24",  0x0800000, 0x3d422a1c, BRF_PRG | BRF_ESS },
	{ "mpr-23365.ic25s", 0x0800000, 0xe975496c, BRF_PRG | BRF_ESS },
	{ "mpr-23366.ic26",  0x0800000, 0x55e96378, BRF_PRG | BRF_ESS },
	{ "mpr-23367.ic27s", 0x0800000, 0x5d40d017, BRF_PRG | BRF_ESS },
	{ "mpr-23368.ic28",  0x0800000, 0x8fb3be5f, BRF_PRG | BRF_ESS },
	{ "mpr-23369.ic29",  0x0800000, 0xa6a1671d, BRF_PRG | BRF_ESS },
	{ "mpr-23370.ic30s", 0x0800000, 0x29876427, BRF_PRG | BRF_ESS },
	{ "mpr-23371.ic31",  0x0800000, 0x5cad6596, BRF_PRG | BRF_ESS },
	{ "mpr-23372.ic32s", 0x0800000, 0x3d848b16, BRF_PRG | BRF_ESS },
	{ "mpr-23373.ic33",  0x0800000, 0x369227f9, BRF_PRG | BRF_ESS },
	{ "mpr-23374.ic34s", 0x0800000, 0x1f8a2e08, BRF_PRG | BRF_ESS },
	{ "mpr-23375.ic35",  0x0800000, 0x7d4043db, BRF_PRG | BRF_ESS },
	{ "mpr-23376.ic36s", 0x0800000, 0xe09cb473, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(gram2000)
STD_ROM_FN(gram2000)

static struct BurnRomInfo capsnkRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23511c.ic22", 0x0400000, 0x3dbf8eb2, BRF_PRG | BRF_ESS },
	{ "mpr-23504.ic1",   0x1000000, 0xe01a31d2, BRF_PRG | BRF_ESS },
	{ "mpr-23505.ic2",   0x1000000, 0x3a34d5fe, BRF_PRG | BRF_ESS },
	{ "mpr-23506.ic3",   0x1000000, 0x9cbab27d, BRF_PRG | BRF_ESS },
	{ "mpr-23507.ic4",   0x1000000, 0x363c1734, BRF_PRG | BRF_ESS },
	{ "mpr-23508.ic5",   0x1000000, 0x0a3590aa, BRF_PRG | BRF_ESS },
	{ "mpr-23509.ic6",   0x1000000, 0x281d633d, BRF_PRG | BRF_ESS },
	{ "mpr-23510.ic7",   0x1000000, 0xb856fef5, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(capsnk)
STD_ROM_FN(capsnk)

static struct BurnRomInfo capsnkaRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23511a.ic22", 0x0400000, 0xfe00650f, BRF_PRG | BRF_ESS },
	{ "mpr-23504.ic1",   0x1000000, 0xe01a31d2, BRF_PRG | BRF_ESS },
	{ "mpr-23505.ic2",   0x1000000, 0x3a34d5fe, BRF_PRG | BRF_ESS },
	{ "mpr-23506.ic3",   0x1000000, 0x9cbab27d, BRF_PRG | BRF_ESS },
	{ "mpr-23507.ic4",   0x1000000, 0x363c1734, BRF_PRG | BRF_ESS },
	{ "mpr-23508.ic5",   0x1000000, 0x0a3590aa, BRF_PRG | BRF_ESS },
	{ "mpr-23509.ic6",   0x1000000, 0x281d633d, BRF_PRG | BRF_ESS },
	{ "mpr-23510.ic7",   0x1000000, 0xb856fef5, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(capsnka)
STD_ROM_FN(capsnka)

static struct BurnRomInfo capsnkbRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23511.ic22",  0x0400000, 0x8717da61, BRF_PRG | BRF_ESS },
	{ "mpr-23504.ic1",   0x1000000, 0xe01a31d2, BRF_PRG | BRF_ESS },
	{ "mpr-23505.ic2",   0x1000000, 0x3a34d5fe, BRF_PRG | BRF_ESS },
	{ "mpr-23506.ic3",   0x1000000, 0x9cbab27d, BRF_PRG | BRF_ESS },
	{ "mpr-23507.ic4",   0x1000000, 0x363c1734, BRF_PRG | BRF_ESS },
	{ "mpr-23508.ic5",   0x1000000, 0x0a3590aa, BRF_PRG | BRF_ESS },
	{ "mpr-23509.ic6",   0x1000000, 0x281d633d, BRF_PRG | BRF_ESS },
	{ "mpr-23510.ic7",   0x1000000, 0xb856fef5, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(capsnkb)
STD_ROM_FN(capsnkb)

static struct BurnRomInfo doa2mRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "doa2verm.ic22",   0x0400000, 0x94b16f08, BRF_PRG | BRF_ESS },
	{ "mpr-22100.ic1",   0x0800000, 0x92a53e5e, BRF_PRG | BRF_ESS },
	{ "mpr-22101.ic2",   0x0800000, 0x14cd7dce, BRF_PRG | BRF_ESS },
	{ "mpr-22102.ic3",   0x0800000, 0x34e778b1, BRF_PRG | BRF_ESS },
	{ "mpr-22103.ic4",   0x0800000, 0x6f3db8df, BRF_PRG | BRF_ESS },
	{ "mpr-22104.ic5",   0x0800000, 0xfcc2787f, BRF_PRG | BRF_ESS },
	{ "mpr-22105.ic6",   0x0800000, 0x3e2da942, BRF_PRG | BRF_ESS },
	{ "mpr-22106.ic7",   0x0800000, 0x03aceaaf, BRF_PRG | BRF_ESS },
	{ "mpr-22107.ic8",   0x0800000, 0x6f1705e4, BRF_PRG | BRF_ESS },
	{ "mpr-22108.ic9",   0x0800000, 0xd34d3d8a, BRF_PRG | BRF_ESS },
	{ "mpr-22109.ic10",  0x0800000, 0x00ef44dd, BRF_PRG | BRF_ESS },
	{ "mpr-22110.ic11",  0x0800000, 0xa193b577, BRF_PRG | BRF_ESS },
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
};
STD_ROM_PICK(doa2m)
STD_ROM_FN(doa2m)

static struct BurnRomInfo doa2RomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22207.ic22",  0x0400000, 0x313d0e55, BRF_PRG | BRF_ESS },
	{ "mpr-22100.ic1",   0x0800000, 0x92a53e5e, BRF_PRG | BRF_ESS },
	{ "mpr-22101.ic2",   0x0800000, 0x14cd7dce, BRF_PRG | BRF_ESS },
	{ "mpr-22102.ic3",   0x0800000, 0x34e778b1, BRF_PRG | BRF_ESS },
	{ "mpr-22103.ic4",   0x0800000, 0x6f3db8df, BRF_PRG | BRF_ESS },
	{ "mpr-22104.ic5",   0x0800000, 0xfcc2787f, BRF_PRG | BRF_ESS },
	{ "mpr-22105.ic6",   0x0800000, 0x3e2da942, BRF_PRG | BRF_ESS },
	{ "mpr-22106.ic7",   0x0800000, 0x03aceaaf, BRF_PRG | BRF_ESS },
	{ "mpr-22107.ic8",   0x0800000, 0x6f1705e4, BRF_PRG | BRF_ESS },
	{ "mpr-22108.ic9",   0x0800000, 0xd34d3d8a, BRF_PRG | BRF_ESS },
	{ "mpr-22109.ic10",  0x0800000, 0x00ef44dd, BRF_PRG | BRF_ESS },
	{ "mpr-22110.ic11",  0x0800000, 0xa193b577, BRF_PRG | BRF_ESS },
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
};
STD_ROM_PICK(doa2)
STD_ROM_FN(doa2)

static struct BurnRomInfo doa2aRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22121a.ic22", 0x0400000, 0x30f93b5e, BRF_PRG | BRF_ESS },
	{ "mpr-22100.ic1",   0x0800000, 0x92a53e5e, BRF_PRG | BRF_ESS },
	{ "mpr-22101.ic2",   0x0800000, 0x14cd7dce, BRF_PRG | BRF_ESS },
	{ "mpr-22102.ic3",   0x0800000, 0x34e778b1, BRF_PRG | BRF_ESS },
	{ "mpr-22103.ic4",   0x0800000, 0x6f3db8df, BRF_PRG | BRF_ESS },
	{ "mpr-22104.ic5",   0x0800000, 0xfcc2787f, BRF_PRG | BRF_ESS },
	{ "mpr-22105.ic6",   0x0800000, 0x3e2da942, BRF_PRG | BRF_ESS },
	{ "mpr-22106.ic7",   0x0800000, 0x03aceaaf, BRF_PRG | BRF_ESS },
	{ "mpr-22107.ic8",   0x0800000, 0x6f1705e4, BRF_PRG | BRF_ESS },
	{ "mpr-22108.ic9",   0x0800000, 0xd34d3d8a, BRF_PRG | BRF_ESS },
	{ "mpr-22109.ic10",  0x0800000, 0x00ef44dd, BRF_PRG | BRF_ESS },
	{ "mpr-22110.ic11",  0x0800000, 0xa193b577, BRF_PRG | BRF_ESS },
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
};
STD_ROM_PICK(doa2a)
STD_ROM_FN(doa2a)

static struct BurnRomInfo pstoneRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-21597.ic22",  0x0200000, 0x62c7acc0, BRF_PRG | BRF_ESS },
	{ "mpr-21589.ic1",   0x0800000, 0x2fa66608, BRF_PRG | BRF_ESS },
	{ "mpr-21590.ic2",   0x0800000, 0x6341b399, BRF_PRG | BRF_ESS },
	{ "mpr-21591.ic3",   0x0800000, 0x7f2d99aa, BRF_PRG | BRF_ESS },
	{ "mpr-21592.ic4",   0x0800000, 0x6ebe3b25, BRF_PRG | BRF_ESS },
	{ "mpr-21593.ic5",   0x0800000, 0x84366f3e, BRF_PRG | BRF_ESS },
	{ "mpr-21594.ic6",   0x0800000, 0xddfa0467, BRF_PRG | BRF_ESS },
	{ "mpr-21595.ic7",   0x0800000, 0x7ab218f7, BRF_PRG | BRF_ESS },
	{ "mpr-21596.ic8",   0x0800000, 0xf27dbdc5, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(pstone)
STD_ROM_FN(pstone)

static struct BurnRomInfo pstone2RomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23127.ic22",  0x0400000, 0x185761d6, BRF_PRG | BRF_ESS },
	{ "mpr-23118.ic1",   0x0800000, 0xc69f3c3c, BRF_PRG | BRF_ESS },
	{ "mpr-23119.ic2",   0x0800000, 0xa80d444d, BRF_PRG | BRF_ESS },
	{ "mpr-23120.ic3",   0x0800000, 0xc285dd64, BRF_PRG | BRF_ESS },
	{ "mpr-23121.ic4",   0x0800000, 0x1f3f6505, BRF_PRG | BRF_ESS },
	{ "mpr-23122.ic5",   0x0800000, 0x5e403a12, BRF_PRG | BRF_ESS },
	{ "mpr-23123.ic6",   0x0800000, 0x4b71078b, BRF_PRG | BRF_ESS },
	{ "mpr-23124.ic7",   0x0800000, 0x508c0207, BRF_PRG | BRF_ESS },
	{ "mpr-23125.ic8",   0x0800000, 0xb9938bbc, BRF_PRG | BRF_ESS },
	{ "mpr-23126.ic9",   0x0800000, 0xfbb0325b, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(pstone2)
STD_ROM_FN(pstone2)

static struct BurnRomInfo pstone2bRomDesc[] = {
	{ "00.ic1",  0x0800000, 0x3175e60f, BRF_PRG | BRF_ESS },
	{ "01.ic2",  0x0800000, 0xc69f3c3c, BRF_PRG | BRF_ESS },
	{ "02.ic3",  0x0800000, 0xa80d444d, BRF_PRG | BRF_ESS },
	{ "03.ic4",  0x0800000, 0xc285dd64, BRF_PRG | BRF_ESS },
	{ "04.ic5",  0x0800000, 0x1f3f6505, BRF_PRG | BRF_ESS },
	{ "05.ic6",  0x0800000, 0x5e403a12, BRF_PRG | BRF_ESS },
	{ "06.ic7",  0x0800000, 0x4b71078b, BRF_PRG | BRF_ESS },
	{ "07.ic8",  0x0800000, 0x508c0207, BRF_PRG | BRF_ESS },
	{ "08.ic9",  0x0800000, 0xb9938bbc, BRF_PRG | BRF_ESS },
	{ "09.ic10", 0x0800000, 0xfbb0325b, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(pstone2b)
STD_ROM_FN(pstone2b)

static struct BurnRomInfo toyfightRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22035.ic22",  0x0400000, 0xdbc76493, BRF_PRG | BRF_ESS },
	{ "mpr-22025.ic1",   0x0800000, 0x30237202, BRF_PRG | BRF_ESS },
	{ "mpr-22026.ic2",   0x0800000, 0xf28e71ff, BRF_PRG | BRF_ESS },
	{ "mpr-22027.ic3",   0x0800000, 0x1a84632d, BRF_PRG | BRF_ESS },
	{ "mpr-22028.ic4",   0x0800000, 0x2b34ccba, BRF_PRG | BRF_ESS },
	{ "mpr-22029.ic5",   0x0800000, 0x8162953a, BRF_PRG | BRF_ESS },
	{ "mpr-22030.ic6",   0x0800000, 0x5bf5fed6, BRF_PRG | BRF_ESS },
	{ "mpr-22031.ic7",   0x0800000, 0xee7c40cc, BRF_PRG | BRF_ESS },
	{ "mpr-22032.ic8",   0x0800000, 0x3c48c9ba, BRF_PRG | BRF_ESS },
	{ "mpr-22033.ic9",   0x0800000, 0x5fe5586e, BRF_PRG | BRF_ESS },
	{ "mpr-22034.ic10",  0x0800000, 0x3aa5ce5e, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(toyfight)
STD_ROM_FN(toyfight)

static struct BurnRomInfo gundmctRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23638.ic22",  0x0400000, 0x03e8600d, BRF_PRG | BRF_ESS },
	{ "mpr-23628.ic1",   0x1000000, 0x8668ba2f, BRF_PRG | BRF_ESS },
	{ "mpr-23629.ic2",   0x1000000, 0xb60f3048, BRF_PRG | BRF_ESS },
	{ "mpr-23630.ic3",   0x1000000, 0x0b47643f, BRF_PRG | BRF_ESS },
	{ "mpr-23631.ic4",   0x1000000, 0xdbd863c9, BRF_PRG | BRF_ESS },
	{ "mpr-23632.ic5",   0x1000000, 0x8c008562, BRF_PRG | BRF_ESS },
	{ "mpr-23633.ic6",   0x1000000, 0xca104486, BRF_PRG | BRF_ESS },
	{ "mpr-23634.ic7",   0x1000000, 0x32bf6938, BRF_PRG | BRF_ESS },
	{ "mpr-23635.ic8",   0x1000000, 0xf9279277, BRF_PRG | BRF_ESS },
	{ "mpr-23636.ic9",   0x1000000, 0x57199e9f, BRF_PRG | BRF_ESS },
	{ "mpr-23637.ic10",  0x1000000, 0x737b5dff, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(gundmct)
STD_ROM_FN(gundmct)

static struct BurnRomInfo puyodaRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22206.ic22",  0x0040000, 0x3882dd01, BRF_PRG | BRF_ESS },
	{ "mpr-22186.ic1",   0x0800000, 0x30b1a1d6, BRF_PRG | BRF_ESS },
	{ "mpr-22187.ic2",   0x0800000, 0x0eae60e5, BRF_PRG | BRF_ESS },
	{ "mpr-22188.ic3",   0x0800000, 0x2e651f16, BRF_PRG | BRF_ESS },
	{ "mpr-22189.ic4",   0x0800000, 0x69ec44fc, BRF_PRG | BRF_ESS },
	{ "mpr-22190.ic5",   0x0800000, 0xd86bef21, BRF_PRG | BRF_ESS },
	{ "mpr-22191.ic6",   0x0800000, 0xb52364cd, BRF_PRG | BRF_ESS },
	{ "mpr-22192.ic7",   0x0800000, 0x08f09148, BRF_PRG | BRF_ESS },
	{ "mpr-22193.ic8",   0x0800000, 0xbe5f58a8, BRF_PRG | BRF_ESS },
	{ "mpr-22194.ic9",   0x0800000, 0x2d0370fd, BRF_PRG | BRF_ESS },
	{ "mpr-22195.ic10",  0x0800000, 0xccc8bb28, BRF_PRG | BRF_ESS },
	{ "mpr-22196.ic11",  0x0800000, 0xd65aa87b, BRF_PRG | BRF_ESS },
	{ "mpr-22197.ic12s", 0x0800000, 0x2c79608e, BRF_PRG | BRF_ESS },
	{ "mpr-22198.ic13s", 0x0800000, 0xb5373909, BRF_PRG | BRF_ESS },
	{ "mpr-22199.ic14s", 0x0800000, 0x4ba34fd9, BRF_PRG | BRF_ESS },
	{ "mpr-22200.ic15s", 0x0800000, 0xeb3d4a5e, BRF_PRG | BRF_ESS },
	{ "mpr-22201.ic16s", 0x0800000, 0xdce19598, BRF_PRG | BRF_ESS },
	{ "mpr-22202.ic17s", 0x0800000, 0xf3ac92a6, BRF_PRG | BRF_ESS },
	{ "mpr-22203.ic18s", 0x0800000, 0x881d6316, BRF_PRG | BRF_ESS },
	{ "mpr-22204.ic19s", 0x0800000, 0x2c5e5140, BRF_PRG | BRF_ESS },
	{ "mpr-22205.ic20s", 0x0800000, 0x7d523ae5, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(puyoda)
STD_ROM_FN(puyoda)

static struct BurnRomInfo virnbaRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23073.ic22",  0x0400000, 0xce5c3d28, BRF_PRG | BRF_ESS },
	{ "mpr-22928.ic1",   0x0800000, 0x63245c98, BRF_PRG | BRF_ESS },
	{ "mpr-22929.ic2",   0x0800000, 0xeea89d21, BRF_PRG | BRF_ESS },
	{ "mpr-22930.ic3",   0x0800000, 0x2fbefa9a, BRF_PRG | BRF_ESS },
	{ "mpr-22931.ic4",   0x0800000, 0x7332e559, BRF_PRG | BRF_ESS },
	{ "mpr-22932.ic5",   0x0800000, 0xef80e18c, BRF_PRG | BRF_ESS },
	{ "mpr-22933.ic6",   0x0800000, 0x6a374076, BRF_PRG | BRF_ESS },
	{ "mpr-22934.ic7",   0x0800000, 0x72f3ee15, BRF_PRG | BRF_ESS },
	{ "mpr-22935.ic8",   0x0800000, 0x35fda6e9, BRF_PRG | BRF_ESS },
	{ "mpr-22936.ic9",   0x0800000, 0xb26df107, BRF_PRG | BRF_ESS },
	{ "mpr-22937.ic10",  0x0800000, 0x477a374b, BRF_PRG | BRF_ESS },
	{ "mpr-22938.ic11",  0x0800000, 0xd59431a4, BRF_PRG | BRF_ESS },
	{ "mpr-22939.ic12s", 0x0800000, 0xb31d3e6d, BRF_PRG | BRF_ESS },
	{ "mpr-22940.ic13s", 0x0800000, 0x90a81fbf, BRF_PRG | BRF_ESS },
	{ "mpr-22941.ic14s", 0x0800000, 0x8a72a77d, BRF_PRG | BRF_ESS },
	{ "mpr-22942.ic15s", 0x0800000, 0x710f709f, BRF_PRG | BRF_ESS },
	{ "mpr-22943.ic16s", 0x0800000, 0xc544f593, BRF_PRG | BRF_ESS },
	{ "mpr-22944.ic17s", 0x0800000, 0xcb096baa, BRF_PRG | BRF_ESS },
	{ "mpr-22945.ic18s", 0x0800000, 0xf2f914e8, BRF_PRG | BRF_ESS },
	{ "mpr-22946.ic19s", 0x0800000, 0xc79696c5, BRF_PRG | BRF_ESS },
	{ "mpr-22947.ic20s", 0x0800000, 0x5e5eb595, BRF_PRG | BRF_ESS },
	{ "mpr-22948.ic21s", 0x0800000, 0x1b0de917, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(virnba)
STD_ROM_FN(virnba)

static struct BurnRomInfo virnbaoRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22949.ic22",  0x0400000, 0xfd91447e, BRF_PRG | BRF_ESS },
	{ "mpr-22928.ic1",   0x0800000, 0x63245c98, BRF_PRG | BRF_ESS },
	{ "mpr-22929.ic2",   0x0800000, 0xeea89d21, BRF_PRG | BRF_ESS },
	{ "mpr-22930.ic3",   0x0800000, 0x2fbefa9a, BRF_PRG | BRF_ESS },
	{ "mpr-22931.ic4",   0x0800000, 0x7332e559, BRF_PRG | BRF_ESS },
	{ "mpr-22932.ic5",   0x0800000, 0xef80e18c, BRF_PRG | BRF_ESS },
	{ "mpr-22933.ic6",   0x0800000, 0x6a374076, BRF_PRG | BRF_ESS },
	{ "mpr-22934.ic7",   0x0800000, 0x72f3ee15, BRF_PRG | BRF_ESS },
	{ "mpr-22935.ic8",   0x0800000, 0x35fda6e9, BRF_PRG | BRF_ESS },
	{ "mpr-22936.ic9",   0x0800000, 0xb26df107, BRF_PRG | BRF_ESS },
	{ "mpr-22937.ic10",  0x0800000, 0x477a374b, BRF_PRG | BRF_ESS },
	{ "mpr-22938.ic11",  0x0800000, 0xd59431a4, BRF_PRG | BRF_ESS },
	{ "mpr-22939.ic12s", 0x0800000, 0xb31d3e6d, BRF_PRG | BRF_ESS },
	{ "mpr-22940.ic13s", 0x0800000, 0x90a81fbf, BRF_PRG | BRF_ESS },
	{ "mpr-22941.ic14s", 0x0800000, 0x8a72a77d, BRF_PRG | BRF_ESS },
	{ "mpr-22942.ic15s", 0x0800000, 0x710f709f, BRF_PRG | BRF_ESS },
	{ "mpr-22943.ic16s", 0x0800000, 0xc544f593, BRF_PRG | BRF_ESS },
	{ "mpr-22944.ic17s", 0x0800000, 0xcb096baa, BRF_PRG | BRF_ESS },
	{ "mpr-22945.ic18s", 0x0800000, 0xf2f914e8, BRF_PRG | BRF_ESS },
	{ "mpr-22946.ic19s", 0x0800000, 0xc79696c5, BRF_PRG | BRF_ESS },
	{ "mpr-22947.ic20s", 0x0800000, 0x5e5eb595, BRF_PRG | BRF_ESS },
	{ "mpr-22948.ic21s", 0x0800000, 0x1b0de917, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(virnbao)
STD_ROM_FN(virnbao)

static struct BurnRomInfo wwfroyalRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22261.ic22",  0x0400000, 0x60e5a6cd, BRF_PRG | BRF_ESS },
	{ "mpr-22262.ic1",   0x1000000, 0xf18c7920, BRF_PRG | BRF_ESS },
	{ "mpr-22263.ic2",   0x1000000, 0x5a397a54, BRF_PRG | BRF_ESS },
	{ "mpr-22264.ic3",   0x1000000, 0xedca701e, BRF_PRG | BRF_ESS },
	{ "mpr-22265.ic4",   0x1000000, 0x7dfe71a1, BRF_PRG | BRF_ESS },
	{ "mpr-22266.ic5",   0x1000000, 0x3e9ac148, BRF_PRG | BRF_ESS },
	{ "mpr-22267.ic6",   0x1000000, 0x67ec1027, BRF_PRG | BRF_ESS },
	{ "mpr-22268.ic7",   0x1000000, 0x536f5eea, BRF_PRG | BRF_ESS },
	{ "mpr-22269.ic8",   0x1000000, 0x6c0cf740, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(wwfroyal)
STD_ROM_FN(wwfroyal)

static struct BurnRomInfo spawnRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22977b.ic22", 0x0400000, 0x814ff5d1, BRF_PRG | BRF_ESS },
	{ "mpr-22967.ic1",   0x0800000, 0x78c7d914, BRF_PRG | BRF_ESS },
	{ "mpr-22968.ic2",   0x0800000, 0x8c4ae1bb, BRF_PRG | BRF_ESS },
	{ "mpr-22969.ic3",   0x0800000, 0x2928627c, BRF_PRG | BRF_ESS },
	{ "mpr-22970.ic4",   0x0800000, 0x12e27ffd, BRF_PRG | BRF_ESS },
	{ "mpr-22971.ic5",   0x0800000, 0x993d2bce, BRF_PRG | BRF_ESS },
	{ "mpr-22972.ic6",   0x0800000, 0xe0f75067, BRF_PRG | BRF_ESS },
	{ "mpr-22973.ic7",   0x0800000, 0x698498ca, BRF_PRG | BRF_ESS },
	{ "mpr-22974.ic8",   0x0800000, 0x20983c51, BRF_PRG | BRF_ESS },
	{ "mpr-22975.ic9",   0x0800000, 0x0d3c70d1, BRF_PRG | BRF_ESS },
	{ "mpr-22976.ic10",  0x0800000, 0x092d8063, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(spawn)
STD_ROM_FN(spawn)

static struct BurnRomInfo vtennisRomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-22927.ic22",  0x0400000, 0x89781723, BRF_PRG | BRF_ESS },
	{ "mpr-22916.ic1",   0x0800000, 0x903873e5, BRF_PRG | BRF_ESS },
	{ "mpr-22917.ic2",   0x0800000, 0x5f020fa6, BRF_PRG | BRF_ESS },
	{ "mpr-22918.ic3",   0x0800000, 0x3c3bf533, BRF_PRG | BRF_ESS },
	{ "mpr-22919.ic4",   0x0800000, 0x3d8dd003, BRF_PRG | BRF_ESS },
	{ "mpr-22920.ic5",   0x0800000, 0xefd781d4, BRF_PRG | BRF_ESS },
	{ "mpr-22921.ic6",   0x0800000, 0x79e75be1, BRF_PRG | BRF_ESS },
	{ "mpr-22922.ic7",   0x0800000, 0x44bd3883, BRF_PRG | BRF_ESS },
	{ "mpr-22923.ic8",   0x0800000, 0x9ebdf0f8, BRF_PRG | BRF_ESS },
	{ "mpr-22924.ic9",   0x0800000, 0xecde9d57, BRF_PRG | BRF_ESS },
	{ "mpr-22925.ic10",  0x0800000, 0x81057e42, BRF_PRG | BRF_ESS },
	{ "mpr-22926.ic11",  0x0800000, 0x57eec89d, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(vtennis)
STD_ROM_FN(vtennis)

static struct BurnRomInfo zerogu2RomDesc[] = {
	{ "epr-21576h.ic27", 0x0200000, 0xd4895685, BRF_BIOS | BRF_ESS },
	{ "epr-23689.ic22",  0x0400000, 0xba42267c, BRF_PRG | BRF_ESS },
	{ "mpr-23684.ic1",   0x1000000, 0x035aec98, BRF_PRG | BRF_ESS },
	{ "mpr-23685.ic2",   0x1000000, 0xd878ff99, BRF_PRG | BRF_ESS },
	{ "mpr-23686.ic3",   0x1000000, 0xa61b4d49, BRF_PRG | BRF_ESS },
	{ "mpr-23687.ic4",   0x1000000, 0xe125439a, BRF_PRG | BRF_ESS },
	{ "mpr-23688.ic5",   0x1000000, 0x38412edf, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(zerogu2)
STD_ROM_FN(zerogu2)

struct BurnDriver BurnDrvcspike = {
	"cspike", NULL, NULL, NULL, "2000",
	"Cannon Spike / Gun Spike\0", NULL, "Capcom / Psikyo", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_RUNGUN, 0,
	NaomiGetZipName, cspikeRomInfo, cspikeRomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	CspikeInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvmvsc2 = {
	"mvsc2", NULL, NULL, NULL, "2000",
	"Marvel vs. Capcom 2 New Age of Heroes (USA)\0", NULL, "Capcom", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, mvsc2RomInfo, mvsc2RomName, NULL, NULL, NULL, NULL, Naomi6BtnInputInfo, NULL,
	Mvsc2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvgram2000 = {
	"gram2000", NULL, NULL, NULL, "2000",
	"Giant Gram 2000\0", NULL, "Sega", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SPORTSMISC, 0,
	NaomiGetZipName, gram2000RomInfo, gram2000RomName, NULL, NULL, NULL, NULL, NaomiStd4BtnInputInfo, NULL,
	Gram2000Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvcapsnk = {
	"capsnk", NULL, NULL, NULL, "2000",
	"Capcom vs. SNK Millennium Fight 2000 (Rev C)\0", NULL, "Capcom / SNK", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, capsnkRomInfo, capsnkRomName, NULL, NULL, NULL, NULL, NaomiCapcom4BtnInputInfo, NULL,
	CapsnkInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvcapsnka = {
	"capsnka", "capsnk", NULL, NULL, "2000",
	"Capcom vs. SNK Millennium Fight 2000 (Rev A)\0", NULL, "Capcom / SNK", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, capsnkaRomInfo, capsnkaRomName, NULL, NULL, NULL, NULL, NaomiCapcom4BtnInputInfo, NULL,
	CapsnkaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvcapsnkb = {
	"capsnkb", "capsnk", NULL, NULL, "2000",
	"Capcom vs. SNK Millennium Fight 2000\0", NULL, "Capcom / SNK", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, capsnkbRomInfo, capsnkbRomName, NULL, NULL, NULL, NULL, NaomiCapcom4BtnInputInfo, NULL,
	CapsnkbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvdoa2m = {
	"doa2m", NULL, NULL, NULL, "2000",
	"Dead or Alive 2 Millennium\0", NULL, "Tecmo", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, doa2mRomInfo, doa2mRomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	Doa2mInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvdoa2 = {
	"doa2", "doa2m", NULL, NULL, "1999",
	"Dead or Alive 2\0", NULL, "Tecmo", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, doa2RomInfo, doa2RomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	Doa2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvdoa2a = {
	"doa2a", "doa2m", NULL, NULL, "1999",
	"Dead or Alive 2 (Rev A)\0", NULL, "Tecmo", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, doa2aRomInfo, doa2aRomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	Doa2aInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvpstone = {
	"pstone", NULL, NULL, NULL, "1999",
	"Power Stone\0", NULL, "Capcom", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, pstoneRomInfo, pstoneRomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	PstoneInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvpstone2 = {
	"pstone2", NULL, NULL, NULL, "2000",
	"Power Stone 2\0", NULL, "Capcom", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, pstone2RomInfo, pstone2RomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	Pstone2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvpstone2b = {
	"pstone2b", "pstone2", NULL, NULL, "2000",
	"Power Stone 2 (bootleg)\0", NULL, "bootleg", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, pstone2bRomInfo, pstone2bRomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	Pstone2bInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvtoyfight = {
	"toyfight", NULL, NULL, NULL, "1999",
	"Toy Fighter\0", NULL, "Sega", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, toyfightRomInfo, toyfightRomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	ToyfightInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvgundmct = {
	"gundmct", NULL, NULL, NULL, "2000",
	"Mobile Suit Gundam: Federation vs. Zeon\0", NULL, "Capcom / Banpresto", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, gundmctRomInfo, gundmctRomName, NULL, NULL, NULL, NULL, NaomiStd4BtnInputInfo, NULL,
	GundmctInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvpuyoda = {
	"puyoda", NULL, NULL, NULL, "1999",
	"Puyo Puyo Da!\0", NULL, "Compile / Sega", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NaomiGetZipName, puyodaRomInfo, puyodaRomName, NULL, NULL, NULL, NULL, Naomi1BtnInputInfo, NULL,
	PuyodaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvvirnba = {
	"virnba", NULL, NULL, NULL, "2000",
	"Virtua NBA (USA)\0", NULL, "Sega", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SPORTSMISC, 0,
	NaomiGetZipName, virnbaRomInfo, virnbaRomName, NULL, NULL, NULL, NULL, Naomi2BtnInputInfo, NULL,
	VirnbaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvvirnbao = {
	"virnbao", "virnba", NULL, NULL, "1999",
	"Virtua NBA\0", NULL, "Sega", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_SPORTSMISC, 0,
	NaomiGetZipName, virnbaoRomInfo, virnbaoRomName, NULL, NULL, NULL, NULL, Naomi2BtnInputInfo, NULL,
	VirnbaoInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvwwfroyal = {
	"wwfroyal", NULL, NULL, NULL, "2000",
	"WWF Royal Rumble\0", NULL, "Sega", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, wwfroyalRomInfo, wwfroyalRomName, NULL, NULL, NULL, NULL, Naomi3BtnInputInfo, NULL,
	WwfroyalInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvspawn = {
	"spawn", NULL, NULL, NULL, "1999",
	"Spawn: In the Demon's Hand\0", NULL, "Capcom / Todd McFarlane Entertainment", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_ACTION, 0,
	NaomiGetZipName, spawnRomInfo, spawnRomName, NULL, NULL, NULL, NULL, NaomiStd4BtnInputInfo, NULL,
	SpawnInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvvtennis = {
	"vtennis", NULL, NULL, NULL, "1999",
	"Virtua Tennis / Power Smash\0", NULL, "Sega", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SPORTSMISC, 0,
	NaomiGetZipName, vtennisRomInfo, vtennisRomName, NULL, NULL, NULL, NULL, Naomi2BtnInputInfo, NULL,
	VtennisInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvzerogu2 = {
	"zerogu2", NULL, NULL, NULL, "2001",
	"Zero Gunner 2\0", NULL, "Psikyo", "Sega Naomi",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VERSHOOT, 0,
	NaomiGetZipName, zerogu2RomInfo, zerogu2RomName, NULL, NULL, NULL, NULL, Naomi2BtnInputInfo, NULL,
	Zerogu2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

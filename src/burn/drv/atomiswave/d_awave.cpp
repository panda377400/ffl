#include "burnint.h"
#include "awave_core.h"
#include "burn_gun.h"

static UINT8 NaomiInputPort0[16];
static UINT8 NaomiInputPort1[16];
static UINT8 NaomiReset;
static UINT8 NaomiResetLatch;
static UINT8 NaomiTest;
static UINT8 NaomiService;
static INT16 NaomiAnalogPort0;
static INT16 NaomiAnalogPort1;
static INT16 NaomiAnalogPort2;
static INT16 NaomiGun0X;
static INT16 NaomiGun0Y;
static INT16 NaomiGun1X;
static INT16 NaomiGun1Y;
static ClearOpposite<1, UINT32> NaomiClearOppositesP1;
static ClearOpposite<2, UINT32> NaomiClearOppositesP2;
static const NaomiGameConfig* NaomiCurrentGame = NULL;

static const UINT32 NaomiAtomiswaveJoypadMap[] = {
	AWAVE_INPUT_BTN1,   // JOYPAD_B
	AWAVE_INPUT_BTN3,   // JOYPAD_Y
	AWAVE_INPUT_COIN,   // JOYPAD_SELECT
	AWAVE_INPUT_START,  // JOYPAD_START
	AWAVE_INPUT_UP,     // JOYPAD_UP
	AWAVE_INPUT_DOWN,   // JOYPAD_DOWN
	AWAVE_INPUT_LEFT,   // JOYPAD_LEFT
	AWAVE_INPUT_RIGHT,  // JOYPAD_RIGHT
	AWAVE_INPUT_BTN2,   // JOYPAD_A
	AWAVE_INPUT_BTN4,   // JOYPAD_X
	0,                  // JOYPAD_L
	AWAVE_INPUT_BTN5,   // JOYPAD_R
	0,                  // JOYPAD_L2
	0,                  // JOYPAD_R2
	AWAVE_INPUT_TEST,   // JOYPAD_L3
	AWAVE_INPUT_SERVICE // JOYPAD_R3
};

static const UINT32 NaomiAtomiswaveLightgunMap[] = {
	0,                    // deprecated
	0,                    // deprecated
	AWAVE_INPUT_TRIGGER,  // LIGHTGUN_TRIGGER
	AWAVE_INPUT_BTN1,     // LIGHTGUN_AUX_A
	AWAVE_INPUT_BTN2,     // LIGHTGUN_AUX_B
	0,                    // deprecated
	AWAVE_INPUT_START,    // LIGHTGUN_START
	AWAVE_INPUT_COIN,     // LIGHTGUN_SELECT
	AWAVE_INPUT_BTN3,     // LIGHTGUN_AUX_C
	AWAVE_INPUT_UP,       // LIGHTGUN_DPAD_UP
	AWAVE_INPUT_DOWN,     // LIGHTGUN_DPAD_DOWN
	AWAVE_INPUT_LEFT,     // LIGHTGUN_DPAD_LEFT
	AWAVE_INPUT_RIGHT,    // LIGHTGUN_DPAD_RIGHT
};

static struct BurnInputInfo NaomiAwInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Button 3",  BIT_DIGITAL, NaomiInputPort0 + 6,  "p1 fire 3" },
	{ "P1 Button 4",  BIT_DIGITAL, NaomiInputPort0 + 7,  "p1 fire 4" },
	{ "P1 Button 5",  BIT_DIGITAL, NaomiInputPort0 + 8,  "p1 fire 5" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 9,  "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 10, "p1 coin"   },

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Button 2",  BIT_DIGITAL, NaomiInputPort1 + 5,  "p2 fire 2" },
	{ "P2 Button 3",  BIT_DIGITAL, NaomiInputPort1 + 6,  "p2 fire 3" },
	{ "P2 Button 4",  BIT_DIGITAL, NaomiInputPort1 + 7,  "p2 fire 4" },
	{ "P2 Button 5",  BIT_DIGITAL, NaomiInputPort1 + 8,  "p2 fire 5" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 9,  "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 10, "p2 coin"   },

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(NaomiAw)

#define A(a, b, c, d) {a, b, (UINT8*)(c), d}

static struct BurnInputInfo NaomiAwLightgunInputList[] = {
	{ "P1 Up",        BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Down",      BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",      BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",     BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Trigger",   BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 6,  "p1 fire 3" },
	{ "P1 Button 3",  BIT_DIGITAL, NaomiInputPort0 + 7,  "p1 fire 4" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 8,  "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 9,  "p1 coin"   },
	{ "P1 Reload",    BIT_DIGITAL, NaomiInputPort0 + 10, "p1 fire 5" },
	A("P1 Aim X", BIT_ANALOG_REL, &NaomiGun0X, "p1 x-axis"),
	A("P1 Aim Y", BIT_ANALOG_REL, &NaomiGun0Y, "p1 y-axis"),

	{ "P2 Up",        BIT_DIGITAL, NaomiInputPort1 + 0,  "p2 up"     },
	{ "P2 Down",      BIT_DIGITAL, NaomiInputPort1 + 1,  "p2 down"   },
	{ "P2 Left",      BIT_DIGITAL, NaomiInputPort1 + 2,  "p2 left"   },
	{ "P2 Right",     BIT_DIGITAL, NaomiInputPort1 + 3,  "p2 right"  },
	{ "P2 Trigger",   BIT_DIGITAL, NaomiInputPort1 + 4,  "p2 fire 1" },
	{ "P2 Button 1",  BIT_DIGITAL, NaomiInputPort1 + 5,  "p2 fire 2" },
	{ "P2 Button 2",  BIT_DIGITAL, NaomiInputPort1 + 6,  "p2 fire 3" },
	{ "P2 Button 3",  BIT_DIGITAL, NaomiInputPort1 + 7,  "p2 fire 4" },
	{ "P2 Start",     BIT_DIGITAL, NaomiInputPort1 + 8,  "p2 start"  },
	{ "P2 Coin",      BIT_DIGITAL, NaomiInputPort1 + 9,  "p2 coin"   },
	{ "P2 Reload",    BIT_DIGITAL, NaomiInputPort1 + 10, "p2 fire 5" },
	A("P2 Aim X", BIT_ANALOG_REL, &NaomiGun1X, "p2 x-axis"),
	A("P2 Aim Y", BIT_ANALOG_REL, &NaomiGun1Y, "p2 y-axis"),

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(NaomiAwLightgun)

static struct BurnInputInfo NaomiAwRacingInputList[] = {
	{ "P1 High Shift", BIT_DIGITAL, NaomiInputPort0 + 0,  "p1 up"     },
	{ "P1 Low Shift",  BIT_DIGITAL, NaomiInputPort0 + 1,  "p1 down"   },
	{ "P1 Left",       BIT_DIGITAL, NaomiInputPort0 + 2,  "p1 left"   },
	{ "P1 Right",      BIT_DIGITAL, NaomiInputPort0 + 3,  "p1 right"  },
	{ "P1 Boost",      BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Accelerator",BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Brake",      BIT_DIGITAL, NaomiInputPort0 + 6,  "p1 fire 3" },
	{ "P1 Start",      BIT_DIGITAL, NaomiInputPort0 + 9,  "p1 start"  },
	{ "P1 Coin",       BIT_DIGITAL, NaomiInputPort0 + 10, "p1 coin"   },
	A("Steering", BIT_ANALOG_ABS, &NaomiAnalogPort0, "p1 x-axis"),
	A("Accelerator", BIT_ANALOG_ABS, &NaomiAnalogPort1, "p1 y-axis"),
	A("Brake", BIT_ANALOG_ABS, &NaomiAnalogPort2, "p1 z-axis"),

	{ "Reset",         BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",          BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",       BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(NaomiAwRacing)

static struct BurnInputInfo NaomiAwBlockPongInputList[] = {
	{ "P1 Button 1",  BIT_DIGITAL, NaomiInputPort0 + 4,  "p1 fire 1" },
	{ "P1 Button 2",  BIT_DIGITAL, NaomiInputPort0 + 5,  "p1 fire 2" },
	{ "P1 Button 3",  BIT_DIGITAL, NaomiInputPort0 + 6,  "p1 fire 3" },
	{ "P1 Button 4",  BIT_DIGITAL, NaomiInputPort0 + 7,  "p1 fire 4" },
	{ "P1 Button 5",  BIT_DIGITAL, NaomiInputPort0 + 8,  "p1 fire 5" },
	{ "P1 Start",     BIT_DIGITAL, NaomiInputPort0 + 9,  "p1 start"  },
	{ "P1 Coin",      BIT_DIGITAL, NaomiInputPort0 + 10, "p1 coin"   },
	A("P1 Analog X", BIT_ANALOG_ABS, &NaomiAnalogPort0, "p1 x-axis"),
	A("P1 Analog Y", BIT_ANALOG_ABS, &NaomiAnalogPort1, "p1 y-axis"),

	{ "Reset",        BIT_DIGITAL, &NaomiReset,          "reset"     },
	{ "Test",         BIT_DIGITAL, &NaomiTest,           "diag"      },
	{ "Service",      BIT_DIGITAL, &NaomiService,        "service"   },
};

STDINPUTINFO(NaomiAwBlockPong)

#undef A

static INT32 NaomiGetZipName(char** pszName, UINT32 i)
{
	if (pszName == NULL) {
		return 1;
	}

	if (i == 0) {
		*pszName = (char*)((NaomiCurrentGame != NULL && NaomiCurrentGame->biosZipName != NULL)
			? NaomiCurrentGame->biosZipName
			: "awbios");
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

	if (inputs[0])  state |= AWAVE_INPUT_UP;
	if (inputs[1])  state |= AWAVE_INPUT_DOWN;
	if (inputs[2])  state |= AWAVE_INPUT_LEFT;
	if (inputs[3])  state |= AWAVE_INPUT_RIGHT;
	if (inputs[4])  state |= AWAVE_INPUT_BTN1;
	if (inputs[5])  state |= AWAVE_INPUT_BTN2;
	if (inputs[6])  state |= AWAVE_INPUT_BTN3;
	if (inputs[7])  state |= AWAVE_INPUT_BTN4;
	if (inputs[8])  state |= AWAVE_INPUT_BTN5;
	if (inputs[9])  state |= AWAVE_INPUT_START;
	if (inputs[10]) state |= AWAVE_INPUT_COIN;

	clearOpposites.neu(0, state, AWAVE_INPUT_UP, AWAVE_INPUT_DOWN, AWAVE_INPUT_LEFT, AWAVE_INPUT_RIGHT);
	return state;
}

template<int N>
static UINT32 NaomiBuildLightgunStateTyped(UINT8* inputs, ClearOpposite<N, UINT32>& clearOpposites)
{
	UINT32 state = 0;

	if (inputs[0])  state |= AWAVE_INPUT_UP;
	if (inputs[1])  state |= AWAVE_INPUT_DOWN;
	if (inputs[2])  state |= AWAVE_INPUT_LEFT;
	if (inputs[3])  state |= AWAVE_INPUT_RIGHT;
	if (inputs[4])  state |= AWAVE_INPUT_TRIGGER;
	if (inputs[5])  state |= AWAVE_INPUT_BTN1;
	if (inputs[6])  state |= AWAVE_INPUT_BTN2;
	if (inputs[7])  state |= AWAVE_INPUT_BTN3;
	if (inputs[8])  state |= AWAVE_INPUT_START;
	if (inputs[9])  state |= AWAVE_INPUT_COIN;

	clearOpposites.neu(0, state, AWAVE_INPUT_UP, AWAVE_INPUT_DOWN, AWAVE_INPUT_LEFT, AWAVE_INPUT_RIGHT);
	return state;
}

static UINT32 NaomiBuildRacingState(UINT8* inputs)
{
	UINT32 state = 0;

	if (inputs[0])  state |= AWAVE_INPUT_UP;
	if (inputs[1])  state |= AWAVE_INPUT_DOWN;
	if (inputs[4])  state |= AWAVE_INPUT_BTN1;
	if (inputs[9])  state |= AWAVE_INPUT_START;
	if (inputs[10]) state |= AWAVE_INPUT_COIN;

	return state;
}

static INT16 NaomiResolveRacingAxis(INT16 analogValue, UINT8 negativePressed, UINT8 positivePressed)
{
	if (analogValue != 0) {
		return analogValue;
	}

	if (negativePressed && !positivePressed) {
		return -0x7fff;
	}

	if (positivePressed && !negativePressed) {
		return 0x7fff;
	}

	return 0;
}

static INT16 NaomiResolveRacingTrigger(INT16 analogValue, UINT8 pressed)
{
	if (analogValue != 0) {
		return analogValue;
	}

	return pressed ? 0x7fff : 0;
}

static UINT32 NaomiBuildBlockPongState(UINT8* inputs)
{
	UINT32 state = 0;

	if (inputs[4])  state |= AWAVE_INPUT_BTN1;
	if (inputs[5])  state |= AWAVE_INPUT_BTN2;
	if (inputs[6])  state |= AWAVE_INPUT_BTN3;
	if (inputs[7])  state |= AWAVE_INPUT_BTN4;
	if (inputs[8])  state |= AWAVE_INPUT_BTN5;
	if (inputs[9])  state |= AWAVE_INPUT_START;
	if (inputs[10]) state |= AWAVE_INPUT_COIN;

	return state;
}

static INT16 NaomiConvertGunAxis(UINT8 axis)
{
	return (INT16)(((INT32)axis * 257) - 32768);
}

static INT32 DrvInitCommon(const NaomiGameConfig* config)
{
	memset(NaomiInputPort0, 0, sizeof(NaomiInputPort0));
	memset(NaomiInputPort1, 0, sizeof(NaomiInputPort1));
	NaomiReset = 0;
	NaomiResetLatch = 0;
	NaomiTest = 0;
	NaomiService = 0;
	NaomiAnalogPort0 = 0;
	NaomiAnalogPort1 = 0;
	NaomiAnalogPort2 = 0;
	NaomiGun0X = 0;
	NaomiGun0Y = 0;
	NaomiGun1X = 0;
	NaomiGun1Y = 0;
	NaomiClearOppositesP1.reset();
	NaomiClearOppositesP2.reset();
	NaomiCurrentGame = config;

	if (NaomiCoreInit(config)) {
		NaomiCoreExit();
		return 1;
	}

	if (config->inputType == NAOMI_GAME_INPUT_LIGHTGUN) {
		BurnGunInit(2, false);
	}

	return 0;
}

static INT32 DrvExit()
{
	if (NaomiCurrentGame != NULL && NaomiCurrentGame->inputType == NAOMI_GAME_INPUT_LIGHTGUN && BurnGunIsActive()) {
		BurnGunExit();
	}
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
	NaomiAnalogPort0 = 0;
	NaomiAnalogPort1 = 0;
	NaomiAnalogPort2 = 0;
	NaomiGun0X = 0;
	NaomiGun0Y = 0;
	NaomiGun1X = 0;
	NaomiGun1Y = 0;
	NaomiClearOppositesP1.reset();
	NaomiClearOppositesP2.reset();

	NaomiCoreExit();
	if (NaomiCoreInit(NaomiCurrentGame)) {
		return 1;
	}

	if (NaomiCurrentGame->inputType == NAOMI_GAME_INPUT_LIGHTGUN && !BurnGunIsActive()) {
		BurnGunInit(2, false);
	}
	return 0;
}

static INT32 DrvFrame()
{
	UINT32 p1State = 0;
	UINT32 p2State = 0;
	INT32 ret;

	switch (NaomiCurrentGame ? NaomiCurrentGame->inputType : NAOMI_GAME_INPUT_DIGITAL) {
		case NAOMI_GAME_INPUT_LIGHTGUN:
			p1State = NaomiBuildLightgunStateTyped(NaomiInputPort0, NaomiClearOppositesP1);
			p2State = NaomiBuildLightgunStateTyped(NaomiInputPort1, NaomiClearOppositesP2);
			break;

		case NAOMI_GAME_INPUT_RACING:
			p1State = NaomiBuildRacingState(NaomiInputPort0);
			break;

		case NAOMI_GAME_INPUT_BLOCKPONG:
			p1State = NaomiBuildBlockPongState(NaomiInputPort0);
			break;

		default:
			p1State = NaomiBuildPadStateTyped(NaomiInputPort0, NaomiClearOppositesP1);
			p2State = NaomiBuildPadStateTyped(NaomiInputPort1, NaomiClearOppositesP2);
			break;
	}

	if (NaomiTest) {
		p1State |= AWAVE_INPUT_TEST;
	}
	if (NaomiService) {
		p1State |= AWAVE_INPUT_SERVICE;
	}

	NaomiCoreSetPadState(0, p1State);
	NaomiCoreSetPadState(1, p2State);
	NaomiCoreSetPadState(2, 0);
	NaomiCoreSetPadState(3, 0);
	NaomiCoreSetAnalogState(0, 0, 0);
	NaomiCoreSetAnalogState(0, 1, 0);
	NaomiCoreSetAnalogState(0, 2, 0);
	NaomiCoreSetAnalogState(0, 3, 0);
	NaomiCoreSetAnalogState(1, 0, 0);
	NaomiCoreSetAnalogState(1, 1, 0);
	NaomiCoreSetAnalogState(1, 2, 0);
	NaomiCoreSetAnalogState(1, 3, 0);
	NaomiCoreSetAnalogButtonState(0, 0, 0);
	NaomiCoreSetAnalogButtonState(0, 1, 0);
	NaomiCoreSetAnalogButtonState(1, 0, 0);
	NaomiCoreSetAnalogButtonState(1, 1, 0);
	NaomiCoreSetLightgunState(0, 0, 0, 0, 0);
	NaomiCoreSetLightgunState(1, 0, 0, 0, 0);

	switch (NaomiCurrentGame ? NaomiCurrentGame->inputType : NAOMI_GAME_INPUT_DIGITAL) {
		case NAOMI_GAME_INPUT_LIGHTGUN:
			if (BurnGunIsActive()) {
				BurnGunMakeInputs(0, NaomiGun0X, NaomiGun0Y);
				BurnGunMakeInputs(1, NaomiGun1X, NaomiGun1Y);
			}
			NaomiCoreSetLightgunState(0, NaomiConvertGunAxis(BurnGunReturnX(0)), NaomiConvertGunAxis(BurnGunReturnY(0)), NaomiInputPort0[10] ? 1 : 0, NaomiInputPort0[10] ? 1 : 0);
			NaomiCoreSetLightgunState(1, NaomiConvertGunAxis(BurnGunReturnX(1)), NaomiConvertGunAxis(BurnGunReturnY(1)), NaomiInputPort1[10] ? 1 : 0, NaomiInputPort1[10] ? 1 : 0);
			break;

		case NAOMI_GAME_INPUT_RACING:
		{
			INT16 steering = NaomiResolveRacingAxis(NaomiAnalogPort0, NaomiInputPort0[2], NaomiInputPort0[3]);
			INT16 accel = NaomiResolveRacingTrigger(NaomiAnalogPort1, NaomiInputPort0[5]);
			INT16 brake = NaomiResolveRacingTrigger(NaomiAnalogPort2, NaomiInputPort0[6]);

			// Atomiswave racing games (e.g. Maximum Speed) use a second Maple controller
			// on port 2 for analog steering and triggers, while digital buttons stay on port 0.
			NaomiCoreSetAnalogState(2, 0, steering);
			NaomiCoreSetAnalogButtonState(2, 0, brake);
			NaomiCoreSetAnalogButtonState(2, 1, accel);
			break;
		}

		case NAOMI_GAME_INPUT_BLOCKPONG:
			NaomiCoreSetAnalogState(0, 0, NaomiAnalogPort0);
			NaomiCoreSetAnalogState(0, 1, NaomiAnalogPort1);
			break;

		default:
			break;
	}

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

#define AW_CONFIG(_var, _zipEntries) \
static const NaomiGameConfig NaomiGame##_var = { \
	#_var, #_var ".zip", _zipEntries, "awbios", "atomiswave", "awave", \
	NaomiAtomiswaveJoypadMap, sizeof(NaomiAtomiswaveJoypadMap) / sizeof(NaomiAtomiswaveJoypadMap[0]), \
	NULL, 0, NAOMI_GAME_INPUT_DIGITAL, \
}

#define AW_CONFIG_SPECIAL(_var, _zipEntries, _inputType) \
static const NaomiGameConfig NaomiGame##_var = { \
	#_var, #_var ".zip", _zipEntries, "awbios", "atomiswave", "awave", \
	NaomiAtomiswaveJoypadMap, sizeof(NaomiAtomiswaveJoypadMap) / sizeof(NaomiAtomiswaveJoypadMap[0]), \
	NULL, 0, _inputType, \
}

#define AW_CONFIG_LIGHTGUN(_var, _zipEntries) \
static const NaomiGameConfig NaomiGame##_var = { \
	#_var, #_var ".zip", _zipEntries, "awbios", "atomiswave", "awave", \
	NaomiAtomiswaveJoypadMap, sizeof(NaomiAtomiswaveJoypadMap) / sizeof(NaomiAtomiswaveJoypadMap[0]), \
	NaomiAtomiswaveLightgunMap, sizeof(NaomiAtomiswaveLightgunMap) / sizeof(NaomiAtomiswaveLightgunMap[0]), NAOMI_GAME_INPUT_LIGHTGUN, \
}

#define DEFINE_AW_INIT(_var) \
static INT32 _var##Init() { return DrvInitCommon(&NaomiGame##_var); }

static const NaomiZipEntry KofxiZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax3201p01.fmem1", 1 },
	{ "ax3201m01.mrom1", 2 },
	{ "ax3202m01.mrom2", 3 },
	{ "ax3203m01.mrom3", 4 },
	{ "ax3204m01.mrom4", 5 },
	{ "ax3205m01.mrom5", 6 },
	{ "ax3206m01.mrom6", 7 },
	{ "ax3207m01.mrom7", 8 },
	{ NULL, -1 }
};

static const NaomiZipEntry Kov7sprtZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax1301p01.ic18",  1 },
	{ "ax1301m01.ic11",  2 },
	{ "ax1301m02.ic12",  3 },
	{ "ax1301m03.ic13",  4 },
	{ "ax1301m04.ic14",  5 },
	{ "ax1301m05.ic15",  6 },
	{ "ax1301m06.ic16",  7 },
	{ "ax1301m07.ic17",  8 },
	{ NULL, -1 }
};

static const NaomiZipEntry Mslug6ZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax3001p01.fmem1", 1 },
	{ "ax3001m01.mrom1", 2 },
	{ "ax3002m01.mrom2", 3 },
	{ "ax3003m01.mrom3", 4 },
	{ "ax3004m01.mrom4", 5 },
	{ NULL, -1 }
};

static const NaomiZipEntry AnmlbsktZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "vm2001f01.u3",    1 },
	{ "vm2001f01.u4",    2 },
	{ "vm2001f01.u2",    3 },
	{ "vm2001f01.u15",   4 },
	{ NULL, -1 }
};

static const NaomiZipEntry AnmlbsktaZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "u3",              1 },
	{ "u1",              2 },
	{ "u4",              3 },
	{ "u2",              4 },
	{ NULL, -1 }
};

static const NaomiZipEntry BasschalZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "vera.u3",         1 },
	{ "vera.u1",         2 },
	{ "vera.u4",         3 },
	{ "vera.u2",         4 },
	{ "vera.u15",        5 },
	{ "vera.u17",        6 },
	{ "vera.u14",        7 },
	{ "vera.u16",        8 },
	{ NULL, -1 }
};

static const NaomiZipEntry BasschaloZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "610-0811.u3",     1 },
	{ "610-0811.u1",     2 },
	{ "vera.u4",         3 },
	{ "610-0811.u2",     4 },
	{ "610-0811.u15",    5 },
	{ "610-0811.u17",    6 },
	{ "610-0811.u14",    7 },
	{ "vera.u16",        8 },
	{ NULL, -1 }
};

static const NaomiZipEntry DemofistZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax0601p01.ic18",  1 },
	{ "ax0601m01.ic11",  2 },
	{ "ax0602m01.ic12",  3 },
	{ "ax0603m01.ic13",  4 },
	{ "ax0604m01.ic14",  5 },
	{ "ax0605m01.ic15",  6 },
	{ "ax0606m01.ic16",  7 },
	{ "ax0607m01.ic17",  8 },
	{ NULL, -1 }
};

static const NaomiZipEntry DirtypigZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "695-0014.u3",     1 },
	{ "695-0014.u1",     2 },
	{ "695-0014.u4",     3 },
	{ "695-0014.u2",     4 },
	{ "695-0014.u15",    5 },
	{ "695-0014.u17",    6 },
	{ "695-0014.u14",    7 },
	{ "695-0014.u16",    8 },
	{ NULL, -1 }
};

static const NaomiZipEntry DolphinZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax0401p01.ic18",  1 },
	{ "ax0401m01.ic11",  2 },
	{ "ax0402m01.ic12",  3 },
	{ "ax0403m01.ic13",  4 },
	{ "ax0404m01.ic14",  5 },
	{ "ax0405m01.ic15",  6 },
	{ NULL, -1 }
};

static const NaomiZipEntry FotnsZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax1901p01.ic18",  1 },
	{ "ax1901m01.ic11",  2 },
	{ "ax1902m01.ic12",  3 },
	{ "ax1903m01.ic13",  4 },
	{ "ax1904m01.ic14",  5 },
	{ "ax1905m01.ic15",  6 },
	{ "ax1906m01.ic16",  7 },
	{ "ax1907m01.ic17",  8 },
	{ NULL, -1 }
};

static const NaomiZipEntry GgisukaZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax1201p01.ic18",  1 },
	{ "ax1201m01.ic10",  2 },
	{ "ax1202m01.ic11",  3 },
	{ "ax1203m01.ic12",  4 },
	{ "ax1204m01.ic13",  5 },
	{ "ax1205m01.ic14",  6 },
	{ "ax1206m01.ic15",  7 },
	{ "ax1207m01.ic16",  8 },
	{ "ax1208m01.ic17",  9 },
	{ NULL, -1 }
};

static const NaomiZipEntry Ggx15ZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax0801p01.ic18",  1 },
	{ "ax0801m01.ic11",  2 },
	{ "ax0802m01.ic12",  3 },
	{ "ax0803m01.ic13",  4 },
	{ "ax0804m01.ic14",  5 },
	{ "ax0805m01.ic15",  6 },
	{ "ax0806m01.ic16",  7 },
	{ "ax0807m01.ic17",  8 },
	{ NULL, -1 }
};

static const NaomiZipEntry KofnwZipEntries[] = {
	{ "bios0.ic23",         0 },
	{ "ax2201en_p01.ic18",  1 },
	{ "ax2201m01.ic11",     2 },
	{ "ax2202m01.ic12",     3 },
	{ "ax2203m01.ic13",     4 },
	{ "ax2204m01.ic14",     5 },
	{ "ax2205m01.ic15",     6 },
	{ "ax2206m01.ic16",     7 },
	{ NULL, -1 }
};

static const NaomiZipEntry KofnwjZipEntries[] = {
	{ "bios0.ic23",         0 },
	{ "ax2201jp_p01.ic18",  1 },
	{ "ax2201m01.ic11",     2 },
	{ "ax2202m01.ic12",     3 },
	{ "ax2203m01.ic13",     4 },
	{ "ax2204m01.ic14",     5 },
	{ "ax2205m01.ic15",     6 },
	{ "ax2206m01.ic16",     7 },
	{ NULL, -1 }
};

static const NaomiZipEntry NgbcZipEntries[] = {
	{ "bios0.ic23",            0 },
	{ "ax3301en_p01.fmem1",    1 },
	{ "ax3301m01.mrom1",       2 },
	{ "ax3302m01.mrom2",       3 },
	{ "ax3303m01.mrom3",       4 },
	{ "ax3304m01.mrom4",       5 },
	{ "ax3305m01.mrom5",       6 },
	{ "ax3306m01.mrom6",       7 },
	{ "ax3307m01.mrom7",       8 },
	{ NULL, -1 }
};

static const NaomiZipEntry NgbcjZipEntries[] = {
	{ "bios0.ic23",         0 },
	{ "ax3301p01.fmem1",    1 },
	{ "ax3301m01.mrom1",    2 },
	{ "ax3302m01.mrom2",    3 },
	{ "ax3303m01.mrom3",    4 },
	{ "ax3304m01.mrom4",    5 },
	{ "ax3305m01.mrom5",    6 },
	{ "ax3306m01.mrom6",    7 },
	{ "ax3307m01.mrom7",    8 },
	{ NULL, -1 }
};

static const NaomiZipEntry RumblefZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax1801p01.ic18",  1 },
	{ "ax1801m01.ic11",  2 },
	{ "ax1802m01.ic12",  3 },
	{ "ax1803m01.ic13",  4 },
	{ "ax1804m01.ic14",  5 },
	{ "ax1805m01.ic15",  6 },
	{ "ax1806m01.ic16",  7 },
	{ "ax1807m01.ic17",  8 },
	{ NULL, -1 }
};

static const NaomiZipEntry RumblefpZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ic12",            1 },
	{ "ic13",            2 },
	{ "ic14",            3 },
	{ "ic15",            4 },
	{ "ic16",            5 },
	{ "ic17",            6 },
	{ "ic18",            7 },
	{ "ic19",            8 },
	{ "ic20",            9 },
	{ "ic21",            10 },
	{ "ic22",            11 },
	{ "ic23",            12 },
	{ "ic24",            13 },
	{ "ic25",            14 },
	{ "ic26",            15 },
	{ NULL, -1 }
};

static const NaomiZipEntry Rumblef2ZipEntries[] = {
	{ "bios0.ic23",         0 },
	{ "ax3401p01.fmem1",    1 },
	{ "ax3401m01.mrom1",    2 },
	{ "ax3402m01.mrom2",    3 },
	{ "ax3403m01.mrom3",    4 },
	{ "ax3404m01.mrom4",    5 },
	{ "ax3405m01.mrom5",    6 },
	{ NULL, -1 }
};

static const NaomiZipEntry Rumblf2pZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ic12",            1 },
	{ "ic13",            2 },
	{ "ic14",            3 },
	{ "ic15",            4 },
	{ "ic16",            5 },
	{ "ic17",            6 },
	{ "ic18",            7 },
	{ "ic19",            8 },
	{ "ic20",            9 },
	{ "ic21",            10 },
	{ "ic22",            11 },
	{ "ic23",            12 },
	{ "ic24",            13 },
	{ "ic25",            14 },
	{ "ic26",            15 },
	{ NULL, -1 }
};

static const NaomiZipEntry SalmanktZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax1401p01.ic18",  1 },
	{ "ax1401m01.ic11",  2 },
	{ "ax1402m01.ic12",  3 },
	{ "ax1403m01.ic13",  4 },
	{ "ax1404m01.ic14",  5 },
	{ "ax1405m01.ic15",  6 },
	{ "ax1406m01.ic16",  7 },
	{ "ax1407m01.ic17",  8 },
	{ NULL, -1 }
};

static const NaomiZipEntry SamsptkZipEntries[] = {
	{ "bios0.ic23",         0 },
	{ "ax2901p01.fmem1",    1 },
	{ "ax2901m01.mrom1",    2 },
	{ "ax2902m01.mrom2",    3 },
	{ "ax2903m01.mrom3",    4 },
	{ "ax2904m01.mrom4",    5 },
	{ "ax2905m01.mrom5",    6 },
	{ "ax2906m01.mrom6",    7 },
	{ "ax2907m01.mrom7",    8 },
	{ NULL, -1 }
};

static const NaomiZipEntry SushibarZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ic12",            1 },
	{ "ic14",            2 },
	{ "ic15",            3 },
	{ "ic16",            4 },
	{ "ic17",            5 },
	{ "ic18",            6 },
	{ NULL, -1 }
};

static const NaomiZipEntry WaidriveZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "u3",              1 },
	{ "u1",              2 },
	{ NULL, -1 }
};

static const NaomiZipEntry MaxspeedZipEntries[] = {
	{ "bios0.ic23",      0 },
	{ "ax0501p01.ic18",  1 },
	{ "ax0501m01.ic11",  2 },
	{ "ax0502m01.ic12",  3 },
	{ "ax0503m01.ic13",  4 },
	{ "ax0504m01.ic14",  5 },
	{ "ax0505m01.ic15",  6 },
	{ NULL, -1 }
};

AW_CONFIG(Kofxi, KofxiZipEntries);
AW_CONFIG(Kov7sprt, Kov7sprtZipEntries);
AW_CONFIG(Mslug6, Mslug6ZipEntries);
AW_CONFIG(Anmlbskt, AnmlbsktZipEntries);
AW_CONFIG(Anmlbskta, AnmlbsktaZipEntries);
AW_CONFIG(Basschal, BasschalZipEntries);
AW_CONFIG(Basschalo, BasschaloZipEntries);
AW_CONFIG(Demofist, DemofistZipEntries);
AW_CONFIG(Dirtypig, DirtypigZipEntries);
AW_CONFIG(Dolphin, DolphinZipEntries);
AW_CONFIG(Fotns, FotnsZipEntries);
AW_CONFIG(Ggisuka, GgisukaZipEntries);
AW_CONFIG(Ggx15, Ggx15ZipEntries);
AW_CONFIG(Kofnw, KofnwZipEntries);
AW_CONFIG(Kofnwj, KofnwjZipEntries);
AW_CONFIG(Ngbc, NgbcZipEntries);
AW_CONFIG(Ngbcj, NgbcjZipEntries);
AW_CONFIG(Rumblef, RumblefZipEntries);
AW_CONFIG(Rumblefp, RumblefpZipEntries);
AW_CONFIG(Rumblef2, Rumblef2ZipEntries);
AW_CONFIG(Rumblf2p, Rumblf2pZipEntries);
AW_CONFIG(Salmankt, SalmanktZipEntries);
AW_CONFIG(Samsptk, SamsptkZipEntries);
AW_CONFIG(Sushibar, SushibarZipEntries);
AW_CONFIG(Waidrive, WaidriveZipEntries);
AW_CONFIG_SPECIAL(Maxspeed, MaxspeedZipEntries, NAOMI_GAME_INPUT_RACING);

DEFINE_AW_INIT(Kofxi)
DEFINE_AW_INIT(Kov7sprt)
DEFINE_AW_INIT(Mslug6)
DEFINE_AW_INIT(Anmlbskt)
DEFINE_AW_INIT(Anmlbskta)
DEFINE_AW_INIT(Basschal)
DEFINE_AW_INIT(Basschalo)
DEFINE_AW_INIT(Demofist)
DEFINE_AW_INIT(Dirtypig)
DEFINE_AW_INIT(Dolphin)
DEFINE_AW_INIT(Fotns)
DEFINE_AW_INIT(Ggisuka)
DEFINE_AW_INIT(Ggx15)
DEFINE_AW_INIT(Kofnw)
DEFINE_AW_INIT(Kofnwj)
DEFINE_AW_INIT(Ngbc)
DEFINE_AW_INIT(Ngbcj)
DEFINE_AW_INIT(Rumblef)
DEFINE_AW_INIT(Rumblefp)
DEFINE_AW_INIT(Rumblef2)
DEFINE_AW_INIT(Rumblf2p)
DEFINE_AW_INIT(Salmankt)
DEFINE_AW_INIT(Samsptk)
DEFINE_AW_INIT(Sushibar)
DEFINE_AW_INIT(Waidrive)
DEFINE_AW_INIT(Maxspeed)

static struct BurnRomInfo kofxiRomDesc[] = {
	{ "bios0.ic23",      0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax3201p01.fmem1", 0x00800000, 0x6dbdd71b, BRF_PRG | BRF_ESS },
	{ "ax3201m01.mrom1", 0x2000000,  0x7f9d6af9, BRF_PRG | BRF_ESS },
	{ "ax3202m01.mrom2", 0x2000000,  0x1ae40afa, BRF_PRG | BRF_ESS },
	{ "ax3203m01.mrom3", 0x2000000,  0x8c5e3bfd, BRF_PRG | BRF_ESS },
	{ "ax3204m01.mrom4", 0x2000000,  0xba97f80c, BRF_PRG | BRF_ESS },
	{ "ax3205m01.mrom5", 0x2000000,  0x3c747067, BRF_PRG | BRF_ESS },
	{ "ax3206m01.mrom6", 0x2000000,  0xcb81e5f5, BRF_PRG | BRF_ESS },
	{ "ax3207m01.mrom7", 0x2000000,  0x164f6329, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(kofxi)
STD_ROM_FN(kofxi)

static struct BurnRomInfo kov7sprtRomDesc[] = {
	{ "bios0.ic23",      0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax1301p01.ic18",  0x00800000, 0x6833a334, BRF_PRG | BRF_ESS },
	{ "ax1301m01.ic11",  0x1000000,  0x58ae7ca1, BRF_PRG | BRF_ESS },
	{ "ax1301m02.ic12",  0x1000000,  0x871ea03f, BRF_PRG | BRF_ESS },
	{ "ax1301m03.ic13",  0x1000000,  0xabc328bc, BRF_PRG | BRF_ESS },
	{ "ax1301m04.ic14",  0x1000000,  0x25a176d1, BRF_PRG | BRF_ESS },
	{ "ax1301m05.ic15",  0x1000000,  0xe6573a93, BRF_PRG | BRF_ESS },
	{ "ax1301m06.ic16",  0x1000000,  0xcb8cacb4, BRF_PRG | BRF_ESS },
	{ "ax1301m07.ic17",  0x1000000,  0x0ca92213, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(kov7sprt)
STD_ROM_FN(kov7sprt)

static struct BurnRomInfo mslug6RomDesc[] = {
	{ "bios0.ic23",      0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax3001p01.fmem1", 0x00800000, 0xaf67dbce, BRF_PRG | BRF_ESS },
	{ "ax3001m01.mrom1", 0x2000000,  0xe56417ee, BRF_PRG | BRF_ESS },
	{ "ax3002m01.mrom2", 0x2000000,  0x1be3bbc1, BRF_PRG | BRF_ESS },
	{ "ax3003m01.mrom3", 0x2000000,  0x4fe37370, BRF_PRG | BRF_ESS },
	{ "ax3004m01.mrom4", 0x2000000,  0x2f4c4c6f, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(mslug6)
STD_ROM_FN(mslug6)

static struct BurnRomInfo anmlbsktRomDesc[] = {
	{ "bios0.ic23",    0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "vm2001f01.u3",  0x0800000,  0x4fb33380, BRF_PRG | BRF_ESS },
	{ "vm2001f01.u4",  0x0800000,  0x7cb2e7c3, BRF_PRG | BRF_ESS },
	{ "vm2001f01.u2",  0x0800000,  0x386070a1, BRF_PRG | BRF_ESS },
	{ "vm2001f01.u15", 0x0800000,  0x2bb1be28, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(anmlbskt)
STD_ROM_FN(anmlbskt)

static struct BurnRomInfo anmlbsktaRomDesc[] = {
	{ "bios0.ic23", 0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "u3",         0x1000000,  0xcd082af3, BRF_PRG | BRF_ESS },
	{ "u1",         0x1000000,  0x4a2a01d3, BRF_PRG | BRF_ESS },
	{ "u4",         0x1000000,  0x646e9773, BRF_PRG | BRF_ESS },
	{ "u2",         0x1000000,  0xb9162d97, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(anmlbskta)
STD_ROM_FN(anmlbskta)

static struct BurnRomInfo basschalRomDesc[] = {
	{ "bios0.ic23", 0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "vera.u3",    0x1000000,  0x8cbec9d7, BRF_PRG | BRF_ESS },
	{ "vera.u1",    0x1000000,  0xcfef27e5, BRF_PRG | BRF_ESS },
	{ "vera.u4",    0x1000000,  0xbd1f13aa, BRF_PRG | BRF_ESS },
	{ "vera.u2",    0x1000000,  0x0a463c37, BRF_PRG | BRF_ESS },
	{ "vera.u15",   0x1000000,  0xe588afd1, BRF_PRG | BRF_ESS },
	{ "vera.u17",   0x1000000,  0xd78389a4, BRF_PRG | BRF_ESS },
	{ "vera.u14",   0x1000000,  0x35df044f, BRF_PRG | BRF_ESS },
	{ "vera.u16",   0x1000000,  0x3590072d, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(basschal)
STD_ROM_FN(basschal)

static struct BurnRomInfo basschaloRomDesc[] = {
	{ "bios0.ic23",  0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "610-0811.u3", 0x1000000,  0xef31abe7, BRF_PRG | BRF_ESS },
	{ "610-0811.u1", 0x1000000,  0x44c3cf90, BRF_PRG | BRF_ESS },
	{ "vera.u4",     0x1000000,  0xbd1f13aa, BRF_PRG | BRF_ESS },
	{ "610-0811.u2", 0x1000000,  0x1c61ed69, BRF_PRG | BRF_ESS },
	{ "610-0811.u15",0x1000000,  0xe8f02238, BRF_PRG | BRF_ESS },
	{ "610-0811.u17",0x1000000,  0xdb799f5a, BRF_PRG | BRF_ESS },
	{ "610-0811.u14",0x1000000,  0xf2769383, BRF_PRG | BRF_ESS },
	{ "vera.u16",    0x1000000,  0x3590072d, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(basschalo)
STD_ROM_FN(basschalo)

static struct BurnRomInfo demofistRomDesc[] = {
	{ "bios0.ic23",     0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax0601p01.ic18", 0x0800000,  0x0efb38ad, BRF_PRG | BRF_ESS },
	{ "ax0601m01.ic11", 0x1000000,  0x12fda2c7, BRF_PRG | BRF_ESS },
	{ "ax0602m01.ic12", 0x1000000,  0xaea61fdf, BRF_PRG | BRF_ESS },
	{ "ax0603m01.ic13", 0x1000000,  0xd5879d35, BRF_PRG | BRF_ESS },
	{ "ax0604m01.ic14", 0x1000000,  0xa7b09048, BRF_PRG | BRF_ESS },
	{ "ax0605m01.ic15", 0x1000000,  0x18d8437e, BRF_PRG | BRF_ESS },
	{ "ax0606m01.ic16", 0x1000000,  0x42c81617, BRF_PRG | BRF_ESS },
	{ "ax0607m01.ic17", 0x1000000,  0x96e5aa84, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(demofist)
STD_ROM_FN(demofist)

static struct BurnRomInfo dirtypigRomDesc[] = {
	{ "bios0.ic23",  0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "695-0014.u3", 0x1000000,  0x9fdd7d07, BRF_PRG | BRF_ESS },
	{ "695-0014.u1", 0x1000000,  0xa91d2fcb, BRF_PRG | BRF_ESS },
	{ "695-0014.u4", 0x1000000,  0x3342f237, BRF_PRG | BRF_ESS },
	{ "695-0014.u2", 0x1000000,  0x4d82152f, BRF_PRG | BRF_ESS },
	{ "695-0014.u15", 0x1000000, 0xd239a549, BRF_PRG | BRF_ESS },
	{ "695-0014.u17", 0x1000000, 0x16bb5992, BRF_PRG | BRF_ESS },
	{ "695-0014.u14", 0x1000000, 0x55470242, BRF_PRG | BRF_ESS },
	{ "695-0014.u16", 0x1000000, 0x730180a4, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(dirtypig)
STD_ROM_FN(dirtypig)

static struct BurnRomInfo dolphinRomDesc[] = {
	{ "bios0.ic23",     0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax0401p01.ic18", 0x0800000,  0x195d6328, BRF_PRG | BRF_ESS },
	{ "ax0401m01.ic11", 0x1000000,  0x5e5dca57, BRF_PRG | BRF_ESS },
	{ "ax0402m01.ic12", 0x1000000,  0x77dd4771, BRF_PRG | BRF_ESS },
	{ "ax0403m01.ic13", 0x1000000,  0x911d0674, BRF_PRG | BRF_ESS },
	{ "ax0404m01.ic14", 0x1000000,  0xf82a4ca3, BRF_PRG | BRF_ESS },
	{ "ax0405m01.ic15", 0x1000000,  0xb88298d7, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(dolphin)
STD_ROM_FN(dolphin)

static struct BurnRomInfo fotnsRomDesc[] = {
	{ "bios0.ic23",     0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax1901p01.ic18", 0x0800000,  0xa06998b0, BRF_PRG | BRF_ESS },
	{ "ax1901m01.ic11", 0x1000000,  0xff5a1642, BRF_PRG | BRF_ESS },
	{ "ax1902m01.ic12", 0x1000000,  0xd9aae8a9, BRF_PRG | BRF_ESS },
	{ "ax1903m01.ic13", 0x1000000,  0x1711b23d, BRF_PRG | BRF_ESS },
	{ "ax1904m01.ic14", 0x1000000,  0x443bfb26, BRF_PRG | BRF_ESS },
	{ "ax1905m01.ic15", 0x1000000,  0xeb1cada0, BRF_PRG | BRF_ESS },
	{ "ax1906m01.ic16", 0x1000000,  0xfe6da168, BRF_PRG | BRF_ESS },
	{ "ax1907m01.ic17", 0x1000000,  0x9d3a0520, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(fotns)
STD_ROM_FN(fotns)

static struct BurnRomInfo ggisukaRomDesc[] = {
	{ "bios0.ic23",     0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax1201p01.ic18", 0x0800000,  0x0a78d52c, BRF_PRG | BRF_ESS },
	{ "ax1201m01.ic10", 0x1000000,  0xdf96ce30, BRF_PRG | BRF_ESS },
	{ "ax1202m01.ic11", 0x1000000,  0xdfc6fd67, BRF_PRG | BRF_ESS },
	{ "ax1203m01.ic12", 0x1000000,  0xbf623df9, BRF_PRG | BRF_ESS },
	{ "ax1204m01.ic13", 0x1000000,  0xc80c3930, BRF_PRG | BRF_ESS },
	{ "ax1205m01.ic14", 0x1000000,  0xe99a269d, BRF_PRG | BRF_ESS },
	{ "ax1206m01.ic15", 0x1000000,  0x807ab795, BRF_PRG | BRF_ESS },
	{ "ax1207m01.ic16", 0x1000000,  0x6636d1b8, BRF_PRG | BRF_ESS },
	{ "ax1208m01.ic17", 0x1000000,  0x38bda476, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(ggisuka)
STD_ROM_FN(ggisuka)

static struct BurnRomInfo ggx15RomDesc[] = {
	{ "bios0.ic23",     0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax0801p01.ic18", 0x0800000,  0xd920c6bb, BRF_PRG | BRF_ESS },
	{ "ax0801m01.ic11", 0x1000000,  0x61879b2d, BRF_PRG | BRF_ESS },
	{ "ax0802m01.ic12", 0x1000000,  0xc0ff124d, BRF_PRG | BRF_ESS },
	{ "ax0803m01.ic13", 0x1000000,  0x4400c89a, BRF_PRG | BRF_ESS },
	{ "ax0804m01.ic14", 0x1000000,  0x70f58ab4, BRF_PRG | BRF_ESS },
	{ "ax0805m01.ic15", 0x1000000,  0x72740e45, BRF_PRG | BRF_ESS },
	{ "ax0806m01.ic16", 0x1000000,  0x3bf8ecba, BRF_PRG | BRF_ESS },
	{ "ax0807m01.ic17", 0x1000000,  0xe397dd79, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(ggx15)
STD_ROM_FN(ggx15)

static struct BurnRomInfo kofnwRomDesc[] = {
	{ "bios0.ic23",        0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax2201en_p01.ic18", 0x0800000,  0x27aab918, BRF_PRG | BRF_ESS },
	{ "ax2201m01.ic11",    0x1000000,  0x22ea665b, BRF_PRG | BRF_ESS },
	{ "ax2202m01.ic12",    0x1000000,  0x7fad1bea, BRF_PRG | BRF_ESS },
	{ "ax2203m01.ic13",    0x1000000,  0x78986ca4, BRF_PRG | BRF_ESS },
	{ "ax2204m01.ic14",    0x1000000,  0x6ffbeb04, BRF_PRG | BRF_ESS },
	{ "ax2205m01.ic15",    0x1000000,  0x2851b791, BRF_PRG | BRF_ESS },
	{ "ax2206m01.ic16",    0x1000000,  0xe53eb965, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(kofnw)
STD_ROM_FN(kofnw)

static struct BurnRomInfo kofnwjRomDesc[] = {
	{ "bios0.ic23",        0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax2201jp_p01.ic18", 0x0800000,  0xecc4a5c7, BRF_PRG | BRF_ESS },
	{ "ax2201m01.ic11",    0x1000000,  0x22ea665b, BRF_PRG | BRF_ESS },
	{ "ax2202m01.ic12",    0x1000000,  0x7fad1bea, BRF_PRG | BRF_ESS },
	{ "ax2203m01.ic13",    0x1000000,  0x78986ca4, BRF_PRG | BRF_ESS },
	{ "ax2204m01.ic14",    0x1000000,  0x6ffbeb04, BRF_PRG | BRF_ESS },
	{ "ax2205m01.ic15",    0x1000000,  0x2851b791, BRF_PRG | BRF_ESS },
	{ "ax2206m01.ic16",    0x1000000,  0xe53eb965, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(kofnwj)
STD_ROM_FN(kofnwj)

static struct BurnRomInfo ngbcRomDesc[] = {
	{ "bios0.ic23",         0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax3301en_p01.fmem1", 0x0800000,  0xf7e24e67, BRF_PRG | BRF_ESS },
	{ "ax3301m01.mrom1",    0x2000000,  0xe6013de9, BRF_PRG | BRF_ESS },
	{ "ax3302m01.mrom2",    0x2000000,  0xf7cfef6c, BRF_PRG | BRF_ESS },
	{ "ax3303m01.mrom3",    0x2000000,  0x0cdf8647, BRF_PRG | BRF_ESS },
	{ "ax3304m01.mrom4",    0x2000000,  0x2f031db0, BRF_PRG | BRF_ESS },
	{ "ax3305m01.mrom5",    0x2000000,  0xf6668aaa, BRF_PRG | BRF_ESS },
	{ "ax3306m01.mrom6",    0x2000000,  0x5cf32fbd, BRF_PRG | BRF_ESS },
	{ "ax3307m01.mrom7",    0x2000000,  0x26d9da53, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(ngbc)
STD_ROM_FN(ngbc)

static struct BurnRomInfo ngbcjRomDesc[] = {
	{ "bios0.ic23",       0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax3301p01.fmem1",  0x0800000,  0x6dd78275, BRF_PRG | BRF_ESS },
	{ "ax3301m01.mrom1",  0x2000000,  0xe6013de9, BRF_PRG | BRF_ESS },
	{ "ax3302m01.mrom2",  0x2000000,  0xf7cfef6c, BRF_PRG | BRF_ESS },
	{ "ax3303m01.mrom3",  0x2000000,  0x0cdf8647, BRF_PRG | BRF_ESS },
	{ "ax3304m01.mrom4",  0x2000000,  0x2f031db0, BRF_PRG | BRF_ESS },
	{ "ax3305m01.mrom5",  0x2000000,  0xf6668aaa, BRF_PRG | BRF_ESS },
	{ "ax3306m01.mrom6",  0x2000000,  0x5cf32fbd, BRF_PRG | BRF_ESS },
	{ "ax3307m01.mrom7",  0x2000000,  0x26d9da53, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(ngbcj)
STD_ROM_FN(ngbcj)

static struct BurnRomInfo rumblefRomDesc[] = {
	{ "bios0.ic23",      0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax1801p01.ic18",  0x0800000,  0x2f7fb163, BRF_PRG | BRF_ESS },
	{ "ax1801m01.ic11",  0x1000000,  0xc38aa61c, BRF_PRG | BRF_ESS },
	{ "ax1802m01.ic12",  0x1000000,  0x72e0ebc8, BRF_PRG | BRF_ESS },
	{ "ax1803m01.ic13",  0x1000000,  0xd0f59d98, BRF_PRG | BRF_ESS },
	{ "ax1804m01.ic14",  0x1000000,  0x15595cba, BRF_PRG | BRF_ESS },
	{ "ax1805m01.ic15",  0x1000000,  0x3d3f8e0d, BRF_PRG | BRF_ESS },
	{ "ax1806m01.ic16",  0x1000000,  0xac2751bb, BRF_PRG | BRF_ESS },
	{ "ax1807m01.ic17",  0x1000000,  0x3b2fbdb0, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(rumblef)
STD_ROM_FN(rumblef)

static struct BurnRomInfo rumblefpRomDesc[] = {
	{ "bios0.ic23", 0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ic12",       0x0800000,  0x79866072, BRF_PRG | BRF_ESS },
	{ "ic13",       0x0800000,  0x5630bc83, BRF_PRG | BRF_ESS },
	{ "ic14",       0x0800000,  0xbcd49846, BRF_PRG | BRF_ESS },
	{ "ic15",       0x0800000,  0x61257cfb, BRF_PRG | BRF_ESS },
	{ "ic16",       0x0800000,  0xc2eb7c61, BRF_PRG | BRF_ESS },
	{ "ic17",       0x0800000,  0xdcf673d3, BRF_PRG | BRF_ESS },
	{ "ic18",       0x0800000,  0x72c066bb, BRF_PRG | BRF_ESS },
	{ "ic19",       0x0800000,  0xb20bf301, BRF_PRG | BRF_ESS },
	{ "ic20",       0x0800000,  0xd27e7393, BRF_PRG | BRF_ESS },
	{ "ic21",       0x0800000,  0xc2da1ecf, BRF_PRG | BRF_ESS },
	{ "ic22",       0x0800000,  0x730e0e1c, BRF_PRG | BRF_ESS },
	{ "ic23",       0x0800000,  0xd93afcac, BRF_PRG | BRF_ESS },
	{ "ic24",       0x0800000,  0x262d97b9, BRF_PRG | BRF_ESS },
	{ "ic25",       0x0800000,  0xe45cf169, BRF_PRG | BRF_ESS },
	{ "ic26",       0x0800000,  0x6421720d, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(rumblefp)
STD_ROM_FN(rumblefp)

static struct BurnRomInfo rumblef2RomDesc[] = {
	{ "bios0.ic23",      0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax3401p01.fmem1", 0x0800000,  0xa33601cf, BRF_PRG | BRF_ESS },
	{ "ax3401m01.mrom1", 0x2000000,  0x60894d4c, BRF_PRG | BRF_ESS },
	{ "ax3402m01.mrom2", 0x2000000,  0xe4224cc9, BRF_PRG | BRF_ESS },
	{ "ax3403m01.mrom3", 0x2000000,  0x081c0edb, BRF_PRG | BRF_ESS },
	{ "ax3404m01.mrom4", 0x2000000,  0xa426b443, BRF_PRG | BRF_ESS },
	{ "ax3405m01.mrom5", 0x2000000,  0x4766ce56, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(rumblef2)
STD_ROM_FN(rumblef2)

static struct BurnRomInfo rumblf2pRomDesc[] = {
	{ "bios0.ic23", 0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ic12",       0x0800000,  0x1a0e74ab, BRF_PRG | BRF_ESS },
	{ "ic13",       0x0800000,  0x5630bc83, BRF_PRG | BRF_ESS },
	{ "ic14",       0x0800000,  0x7fcfc59c, BRF_PRG | BRF_ESS },
	{ "ic15",       0x0800000,  0xeee00692, BRF_PRG | BRF_ESS },
	{ "ic16",       0x0800000,  0xcd029db9, BRF_PRG | BRF_ESS },
	{ "ic17",       0x0800000,  0x223a5b58, BRF_PRG | BRF_ESS },
	{ "ic18",       0x0800000,  0x5e2d2f67, BRF_PRG | BRF_ESS },
	{ "ic19",       0x0800000,  0x3cfb2adc, BRF_PRG | BRF_ESS },
	{ "ic20",       0x0800000,  0x2c216a05, BRF_PRG | BRF_ESS },
	{ "ic21",       0x0800000,  0x79540865, BRF_PRG | BRF_ESS },
	{ "ic22",       0x0800000,  0xc91d95a0, BRF_PRG | BRF_ESS },
	{ "ic23",       0x0800000,  0x5c39ca18, BRF_PRG | BRF_ESS },
	{ "ic24",       0x0800000,  0x858d2775, BRF_PRG | BRF_ESS },
	{ "ic25",       0x0800000,  0x975d35fb, BRF_PRG | BRF_ESS },
	{ "ic26",       0x0800000,  0xff9a2c4c, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(rumblf2p)
STD_ROM_FN(rumblf2p)

static struct BurnRomInfo salmanktRomDesc[] = {
	{ "bios0.ic23",     0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax1401p01.ic18", 0x0800000,  0x28d779e0, BRF_PRG | BRF_ESS },
	{ "ax1401m01.ic11", 0x1000000,  0xfd7af845, BRF_PRG | BRF_ESS },
	{ "ax1402m01.ic12", 0x1000000,  0xf6006f85, BRF_PRG | BRF_ESS },
	{ "ax1403m01.ic13", 0x1000000,  0x074f7c4b, BRF_PRG | BRF_ESS },
	{ "ax1404m01.ic14", 0x1000000,  0xaf4e3829, BRF_PRG | BRF_ESS },
	{ "ax1405m01.ic15", 0x1000000,  0xb548446f, BRF_PRG | BRF_ESS },
	{ "ax1406m01.ic16", 0x1000000,  0x437673e6, BRF_PRG | BRF_ESS },
	{ "ax1407m01.ic17", 0x1000000,  0x6b6acc0a, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(salmankt)
STD_ROM_FN(salmankt)

static struct BurnRomInfo samsptkRomDesc[] = {
	{ "bios0.ic23",      0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax2901p01.fmem1", 0x0800000,  0x58e0030b, BRF_PRG | BRF_ESS },
	{ "ax2901m01.mrom1", 0x2000000,  0xdbbbd90d, BRF_PRG | BRF_ESS },
	{ "ax2902m01.mrom2", 0x2000000,  0xa3bd7890, BRF_PRG | BRF_ESS },
	{ "ax2903m01.mrom3", 0x2000000,  0x56f50fdd, BRF_PRG | BRF_ESS },
	{ "ax2904m01.mrom4", 0x2000000,  0x8a3ae175, BRF_PRG | BRF_ESS },
	{ "ax2905m01.mrom5", 0x2000000,  0x429877ba, BRF_PRG | BRF_ESS },
	{ "ax2906m01.mrom6", 0x2000000,  0xcb95298d, BRF_PRG | BRF_ESS },
	{ "ax2907m01.mrom7", 0x2000000,  0x48015081, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(samsptk)
STD_ROM_FN(samsptk)

static struct BurnRomInfo sushibarRomDesc[] = {
	{ "bios0.ic23", 0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ic12",       0x0800000,  0x06a2ed58, BRF_PRG | BRF_ESS },
	{ "ic14",       0x0800000,  0x4860f944, BRF_PRG | BRF_ESS },
	{ "ic15",       0x0800000,  0x7113506c, BRF_PRG | BRF_ESS },
	{ "ic16",       0x0800000,  0x77e8e39e, BRF_PRG | BRF_ESS },
	{ "ic17",       0x0800000,  0x0eba54ea, BRF_PRG | BRF_ESS },
	{ "ic18",       0x0800000,  0xb9957c76, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(sushibar)
STD_ROM_FN(sushibar)

static struct BurnRomInfo waidriveRomDesc[] = {
	{ "bios0.ic23", 0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "u3",         0x1000000,  0x7acfb499, BRF_PRG | BRF_ESS },
	{ "u1",         0x1000000,  0xb3c1c3bb, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(waidrive)
STD_ROM_FN(waidrive)

static struct BurnRomInfo maxspeedRomDesc[] = {
	{ "bios0.ic23",      0x00020000, 0x719b2b0b, BRF_BIOS | BRF_ESS },
	{ "ax0501p01.ic18",  0x0800000,  0xe1651867, BRF_PRG | BRF_ESS },
	{ "ax0501m01.ic11",  0x1000000,  0x4a847a59, BRF_PRG | BRF_ESS },
	{ "ax0502m01.ic12",  0x1000000,  0x2580237f, BRF_PRG | BRF_ESS },
	{ "ax0503m01.ic13",  0x1000000,  0xe5a3766b, BRF_PRG | BRF_ESS },
	{ "ax0504m01.ic14",  0x1000000,  0x7955b55a, BRF_PRG | BRF_ESS },
	{ "ax0505m01.ic15",  0x1000000,  0xe8ccc660, BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(maxspeed)
STD_ROM_FN(maxspeed)

struct BurnDriver BurnDrvkofxi = {
	"kofxi", NULL, NULL, NULL, "2005",
	"The King of Fighters XI\0", NULL, "SNK Playmore", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, kofxiRomInfo, kofxiRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	KofxiInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvkov7sprt = {
	"kov7sprt", NULL, NULL, NULL, "2004",
	"Knights of Valour - The Seven Spirits\0", NULL, "IGS", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SCRFIGHT, 0,
	NaomiGetZipName, kov7sprtRomInfo, kov7sprtRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	Kov7sprtInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvmslug6 = {
	"mslug6", NULL, NULL, NULL, "2006",
	"Metal Slug 6\0", NULL, "SNK Playmore", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_RUNGUN, 0,
	NaomiGetZipName, mslug6RomInfo, mslug6RomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	Mslug6Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvanmlbskt = {
	"anmlbskt", NULL, NULL, NULL, "2005",
	"Animal Basket\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NaomiGetZipName, anmlbsktRomInfo, anmlbsktRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	AnmlbsktInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	480, 640, 3, 4
};

struct BurnDriver BurnDrvanmlbskta = {
	"anmlbskta", "anmlbskt", NULL, NULL, "2005",
	"Animal Basket (19 Jan 2005)\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NaomiGetZipName, anmlbsktaRomInfo, anmlbsktaRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	AnmlbsktaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	480, 640, 3, 4
};

struct BurnDriver BurnDrvbasschal = {
	"basschal", NULL, NULL, NULL, "2005",
	"Sega Bass Fishing Challenge (Ver. A)\0", NULL, "Sega", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SPORTSMISC, 0,
	NaomiGetZipName, basschalRomInfo, basschalRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	BasschalInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvbasschalo = {
	"basschalo", "basschal", NULL, NULL, "2004",
	"Sega Bass Fishing Challenge\0", NULL, "Sega", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_SPORTSMISC, 0,
	NaomiGetZipName, basschaloRomInfo, basschaloRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	BasschaloInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvdemofist = {
	"demofist", NULL, NULL, NULL, "2003",
	"Demolish Fist\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SCRFIGHT, 0,
	NaomiGetZipName, demofistRomInfo, demofistRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	DemofistInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvdirtypig = {
	"dirtypig", NULL, NULL, NULL, "2004",
	"Dirty Pigskin Football\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SPORTSMISC, 0,
	NaomiGetZipName, dirtypigRomInfo, dirtypigRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	DirtypigInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvdolphin = {
	"dolphin", NULL, NULL, NULL, "2003",
	"Dolphin Blue\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_RUNGUN, 0,
	NaomiGetZipName, dolphinRomInfo, dolphinRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	DolphinInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvfotns = {
	"fotns", NULL, NULL, NULL, "2005",
	"Fist of the North Star\0", NULL, "Sega / Arc System Works", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, fotnsRomInfo, fotnsRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	FotnsInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvggisuka = {
	"ggisuka", NULL, NULL, NULL, "2004",
	"Guilty Gear Isuka\0", NULL, "Sammy / Arc System Works", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, ggisukaRomInfo, ggisukaRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	GgisukaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvggx15 = {
	"ggx15", NULL, NULL, NULL, "2003",
	"Guilty Gear X ver. 1.5\0", NULL, "Sammy / Arc System Works", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, ggx15RomInfo, ggx15RomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	Ggx15Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvkofnw = {
	"kofnw", NULL, NULL, NULL, "2004",
	"The King of Fighters Neowave\0", NULL, "SNK Playmore", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, kofnwRomInfo, kofnwRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	KofnwInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvkofnwj = {
	"kofnwj", "kofnw", NULL, NULL, "2004",
	"The King of Fighters Neowave (Japan)\0", NULL, "SNK Playmore", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, kofnwjRomInfo, kofnwjRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	KofnwjInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvngbc = {
	"ngbc", NULL, NULL, NULL, "2005",
	"NeoGeo Battle Coliseum\0", NULL, "SNK Playmore", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, ngbcRomInfo, ngbcRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	NgbcInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvngbcj = {
	"ngbcj", "ngbc", NULL, NULL, "2005",
	"NeoGeo Battle Coliseum (Japan)\0", NULL, "SNK Playmore", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, ngbcjRomInfo, ngbcjRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	NgbcjInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvrumblef = {
	"rumblef", NULL, NULL, NULL, "2004",
	"The Rumble Fish\0", NULL, "Sammy / Dimps", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, rumblefRomInfo, rumblefRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	RumblefInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvrumblefp = {
	"rumblefp", "rumblef", NULL, NULL, "2004",
	"The Rumble Fish (prototype)\0", NULL, "Sammy / Dimps", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, rumblefpRomInfo, rumblefpRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	RumblefpInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvrumblef2 = {
	"rumblef2", NULL, NULL, NULL, "2005",
	"The Rumble Fish 2\0", NULL, "Sammy / Dimps", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, rumblef2RomInfo, rumblef2RomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	Rumblef2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvrumblf2p = {
	"rumblf2p", "rumblef2", NULL, NULL, "2005",
	"The Rumble Fish 2 (prototype)\0", NULL, "Sammy / Dimps", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, rumblf2pRomInfo, rumblf2pRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	Rumblf2pInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvsalmankt = {
	"salmankt", NULL, NULL, NULL, "2004",
	"Net Select: Salaryman Kintaro\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
	NaomiGetZipName, salmanktRomInfo, salmanktRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	SalmanktInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvsamsptk = {
	"samsptk", NULL, NULL, NULL, "2005",
	"Samurai Shodown VI\0", NULL, "SNK Playmore", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NaomiGetZipName, samsptkRomInfo, samsptkRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	SamsptkInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvsushibar = {
	"sushibar", NULL, NULL, NULL, "2003",
	"Sushi Bar\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NaomiGetZipName, sushibarRomInfo, sushibarRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	SushibarInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

struct BurnDriver BurnDrvwaidrive = {
	"waidrive", NULL, NULL, NULL, "2004",
	"WaiWai Drive\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
	NaomiGetZipName, waidriveRomInfo, waidriveRomName, NULL, NULL, NULL, NULL, NaomiAwInputInfo, NULL,
	WaidriveInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	480, 640, 3, 4
};

struct BurnDriver BurnDrvmaxspeed = {
	"maxspeed", NULL, NULL, NULL, "2002",
	"Maximum Speed\0", NULL, "Sammy", "Sega Atomiswave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_RACING, 0,
	NaomiGetZipName, maxspeedRomInfo, maxspeedRomName, NULL, NULL, NULL, NULL, NaomiAwRacingInputInfo, NULL,
	MaxspeedInit, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	640, 480, 4, 3
};

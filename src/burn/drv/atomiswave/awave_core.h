#pragma once

#include "burnint.h"

struct NaomiZipEntry {
	const char* filename;
	INT32 burnRomIndex;
};

struct NaomiGameConfig {
	const char* driverName;
	const char* zipName;
	const NaomiZipEntry* zipEntries;
	const char* biosZipName;
	const char* systemName;
	const char* runtimeSubdir;
	const UINT32* joypadMap;
	UINT32 joypadMapCount;
	const UINT32* lightgunMap;
	UINT32 lightgunMapCount;
	UINT32 inputType;
};

enum AwaveInputBits {
	AWAVE_INPUT_BTN3    = (1U << 0),
	AWAVE_INPUT_BTN2    = (1U << 1),
	AWAVE_INPUT_BTN1    = (1U << 2),
	AWAVE_INPUT_START   = (1U << 3),
	AWAVE_INPUT_UP      = (1U << 4),
	AWAVE_INPUT_DOWN    = (1U << 5),
	AWAVE_INPUT_LEFT    = (1U << 6),
	AWAVE_INPUT_RIGHT   = (1U << 7),
	AWAVE_INPUT_BTN5    = (1U << 9),
	AWAVE_INPUT_BTN4    = (1U << 10),
	AWAVE_INPUT_TRIGGER = (1U << 12),
	AWAVE_INPUT_SERVICE = (1U << 13),
	AWAVE_INPUT_TEST    = (1U << 14),
	AWAVE_INPUT_COIN    = (1U << 15),
};

enum NaomiGameInputType {
	NAOMI_GAME_INPUT_DIGITAL = 0,
	NAOMI_GAME_INPUT_LIGHTGUN,
	NAOMI_GAME_INPUT_RACING,
	NAOMI_GAME_INPUT_BLOCKPONG,
};

enum NaomiInputBits {
	NAOMI_INPUT_BTN8    = (1U << 2),
	NAOMI_INPUT_BTN7    = (1U << 3),
	NAOMI_INPUT_BTN6    = (1U << 4),
	NAOMI_INPUT_BTN5    = (1U << 5),
	NAOMI_INPUT_BTN4    = (1U << 6),
	NAOMI_INPUT_BTN3    = (1U << 7),
	NAOMI_INPUT_BTN2    = (1U << 8),
	NAOMI_INPUT_BTN1    = (1U << 9),
	NAOMI_INPUT_RIGHT   = (1U << 10),
	NAOMI_INPUT_LEFT    = (1U << 11),
	NAOMI_INPUT_DOWN    = (1U << 12),
	NAOMI_INPUT_UP      = (1U << 13),
	NAOMI_INPUT_SERVICE = (1U << 14),
	NAOMI_INPUT_START   = (1U << 15),
	NAOMI_INPUT_RELOAD  = (1U << 17),
	NAOMI_INPUT_TEST    = (1U << 18),
	NAOMI_INPUT_COIN    = (1U << 19),
};

INT32 NaomiCoreInit(const NaomiGameConfig* config);
INT32 NaomiCoreExit();
INT32 NaomiCoreReset();
INT32 NaomiCoreFrame();
INT32 NaomiCoreDraw();
INT32 NaomiCoreScan(INT32 nAction, INT32* pnMin);

void NaomiCoreSetPadState(INT32 port, UINT32 state);
void NaomiCoreSetAnalogState(INT32 port, INT32 axis, INT16 value);
void NaomiCoreSetAnalogButtonState(INT32 port, INT32 button, INT16 value);
void NaomiCoreSetLightgunState(INT32 port, INT16 x, INT16 y, UINT8 offscreen, UINT8 reload);

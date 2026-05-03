#pragma once

#include "burnint.h"

struct NaomiZipEntry {
	const char* filename;
	INT32 burnRomIndex;
};

enum AwavePlatform {
	AWAVE_PLATFORM_ATOMISWAVE = 0,
	AWAVE_PLATFORM_NAOMI = 1,
	AWAVE_PLATFORM_NAOMI2 = 2,
	AWAVE_PLATFORM_DREAMCAST = 3,
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
	AwavePlatform platform;
	const char* contentPath;
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
	NAOMI_GAME_INPUT_DREAMCAST_PAD,
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

// Hardware-present bridge for FBNeo-QT/other OpenGL frontends.
// When valid is non-zero, texture is an OpenGL GL_TEXTURE_2D containing
// the latest Flycast-rendered frame. The texture belongs to the active
// OpenGL context used by the Atomiswave/Flycast core. Frontends must either
// provide/share that context via OGLSetContext/OGLSetMakeCurrent/OGLSetDoneCurrent,
// or keep using the CPU readback fallback.
struct NaomiCoreHwVideoInfo {
	UINT32 size;
	UINT32 texture;
	UINT32 framebuffer;
	INT32 width;
	INT32 height;
	INT32 upsideDown;
	INT32 valid;
};

INT32 NaomiCoreGetHwVideoInfo(NaomiCoreHwVideoInfo* info);
INT32 NaomiCoreUsingHwDirectPresent();
INT32 NaomiCoreWantsRedraw();
INT32 NaomiCoreScan(INT32 nAction, INT32* pnMin);

void NaomiCoreSetPadState(INT32 port, UINT32 state);
void NaomiCoreSetAnalogState(INT32 port, INT32 axis, INT16 value);
void NaomiCoreSetAnalogButtonState(INT32 port, INT32 button, INT16 value);
void NaomiCoreSetLightgunState(INT32 port, INT16 x, INT16 y, UINT8 offscreen, UINT8 reload);

// OpenGL context bridge. FBNeo-QT should register its render context before
// NaomiCoreInit() when using FBNEO_AWAVE_VIDEO_MODE=direct_texture.
void OGLSetContext(void* ptr);
void OGLSetMakeCurrent(void (*f)(void*));
void OGLSetDoneCurrent(void (*f)(void*));
void OGLSetSwapBuffers(void (*f)(void*));
void OGLSetUseFallbackContext(bool use);
bool OGLUsingFallbackContext();

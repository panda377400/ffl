#ifndef FBNEO_MGBA_CORE_H
#define FBNEO_MGBA_CORE_H

#include "burnint.h"

enum {
	MGBA_PLATFORM_GBA = 0,
	MGBA_PLATFORM_GB = 1,
};

struct MgbaSystemConfig {
	INT32 expectedPlatform;
	const char* gbModel;
	const char* sgbModel;
	const char* cgbModel;
	const char* cgbHybridModel;
	const char* cgbSgbModel;
	INT32 gbColors;
	INT32 sgbBorders;
	INT32 outputWidth;
	INT32 outputHeight;
};

extern const MgbaSystemConfig MgbaSystemGba;
extern const MgbaSystemConfig MgbaSystemGb;
extern const MgbaSystemConfig MgbaSystemGbc;

INT32 MgbaCoreInit(const MgbaSystemConfig* config);
INT32 MgbaCoreExit();
INT32 MgbaCoreReset();
INT32 MgbaCoreFrame(UINT16 keys);
INT32 MgbaCoreDraw();
INT32 MgbaCoreScan(INT32 nAction, INT32* pnMin);

#endif

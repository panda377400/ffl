#pragma once
#include "burnint.h"
#include "awave_core.h"

enum AwaveVideoMode {
    AWAVE_VIDEO_CPU_FALLBACK = 0,
    AWAVE_VIDEO_HW_TEXTURE = 1,
    AWAVE_VIDEO_SKIP_READBACK = 2,
};

AwaveVideoMode AwaveVideoGetMode();
void AwaveVideoSetCpuFrame(const void* pixels, INT32 width, INT32 height, INT32 pitchBytes);
void AwaveVideoSetHwFrame(UINT32 texture, UINT32 framebuffer, INT32 width, INT32 height, INT32 upsideDown);
INT32 AwaveVideoDrawCpu();
INT32 AwaveVideoGetHwInfo(NaomiCoreHwVideoInfo* info);
INT32 AwaveVideoUsingHwDirectPresent();
INT32 AwaveVideoWantsRedraw();
void AwaveVideoReset();

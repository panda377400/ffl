#include "awave_video.h"
#include <vector>
#include <cstring>
#include <cstdlib>

static std::vector<UINT32> g_frame;
static INT32 g_w = 640;
static INT32 g_h = 480;
static INT32 g_validCpu = 0;
static NaomiCoreHwVideoInfo g_hw;

AwaveVideoMode AwaveVideoGetMode()
{
    const char* env = getenv("FBNEO_AWAVE_VIDEO_MODE");
    if (env && env[0]) {
        if (!strcmp(env, "direct") || !strcmp(env, "direct_texture") || !strcmp(env, "hw")) return AWAVE_VIDEO_HW_TEXTURE;
        if (!strcmp(env, "skip") || !strcmp(env, "skip_readback")) return AWAVE_VIDEO_SKIP_READBACK;
        if (!strcmp(env, "cpu") || !strcmp(env, "readback")) return AWAVE_VIDEO_CPU_FALLBACK;
    }
    return AWAVE_VIDEO_CPU_FALLBACK;
}

void AwaveVideoSetCpuFrame(const void* pixels, INT32 width, INT32 height, INT32 pitchBytes)
{
    if (pixels == NULL || width <= 0 || height <= 0) return;
    g_w = width;
    g_h = height;
    g_frame.resize((size_t)width * height);
    const UINT8* src = (const UINT8*)pixels;
    for (INT32 y = 0; y < height; y++) {
        memcpy(&g_frame[(size_t)y * width], src + y * pitchBytes, (size_t)width * sizeof(UINT32));
    }
    g_validCpu = 1;
}

void AwaveVideoSetHwFrame(UINT32 texture, UINT32 framebuffer, INT32 width, INT32 height, INT32 upsideDown)
{
    memset(&g_hw, 0, sizeof(g_hw));
    g_hw.size = sizeof(g_hw);
    g_hw.texture = texture;
    g_hw.framebuffer = framebuffer;
    g_hw.width = width;
    g_hw.height = height;
    g_hw.upsideDown = upsideDown;
    g_hw.valid = texture != 0 && width > 0 && height > 0;
}

INT32 AwaveVideoDrawCpu()
{
    if (pBurnDraw == NULL || !g_validCpu || g_frame.empty()) return 1;
    const INT32 outW = nBurnBpp == 4 ? g_w : g_w;
    (void)outW;
    if (nBurnBpp == 4) {
        for (INT32 y = 0; y < g_h; y++) {
            memcpy((UINT8*)pBurnDraw + y * nBurnPitch, &g_frame[(size_t)y * g_w], (size_t)g_w * sizeof(UINT32));
        }
        return 0;
    }
    // Most FBNeo builds for modern frontends use 32-bit draw buffers.
    // Keep a clear failure for older formats rather than silently corrupting pixels.
    return 1;
}

INT32 AwaveVideoGetHwInfo(NaomiCoreHwVideoInfo* info)
{
    if (info == NULL) return 1;
    *info = g_hw;
    return 0;
}

INT32 AwaveVideoUsingHwDirectPresent()
{
    return AwaveVideoGetMode() == AWAVE_VIDEO_HW_TEXTURE && g_hw.valid;
}

INT32 AwaveVideoWantsRedraw()
{
    return g_validCpu || g_hw.valid;
}

void AwaveVideoReset()
{
    g_frame.clear();
    g_validCpu = 0;
    memset(&g_hw, 0, sizeof(g_hw));
    g_hw.size = sizeof(g_hw);
}

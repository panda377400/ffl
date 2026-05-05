#include "awave_video.h"

#include <vector>
#include <cstring>
#include <cstdlib>

static std::vector<UINT32> g_frame;
static INT32 g_w = 640;
static INT32 g_h = 480;
static INT32 g_validCpu = 0;
static NaomiCoreHwVideoInfo g_hw;
static UINT32 g_set_cpu_count = 0;
static UINT32 g_draw_cpu_count = 0;

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
	if (pitchBytes < width * (INT32)sizeof(UINT32)) return;

	g_w = width;
	g_h = height;
	g_frame.resize((size_t)width * height);

	const UINT8* src = (const UINT8*)pixels;
	for (INT32 y = 0; y < height; y++) {
		memcpy(&g_frame[(size_t)y * width], src + y * pitchBytes, (size_t)width * sizeof(UINT32));
	}

	g_validCpu = 1;

	if (((++g_set_cpu_count) & 0x3f) == 0) {
		UINT32 x = 0;
		const size_t total = g_frame.size();
		const size_t step = total / 257 + 1;
		for (size_t i = 0; i < total; i += step) x ^= g_frame[i];
		bprintf(0, _T("[AW] AwaveVideoSetCpuFrame #%u %dx%d pitch=%d xor=%08x\n"),
			g_set_cpu_count, width, height, pitchBytes, x);
	}
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
	if (pBurnDraw == NULL || !g_validCpu || g_frame.empty()) {
		if (((++g_draw_cpu_count) & 0x3f) == 0) {
			bprintf(0, _T("[AW] AwaveVideoDrawCpu no frame pBurnDraw=%p valid=%d size=%u\n"),
				pBurnDraw, g_validCpu, (unsigned)g_frame.size());
		}
		return 1;
	}

	if (nBurnBpp == 4) {
		for (INT32 y = 0; y < g_h; y++) {
			memcpy((UINT8*)pBurnDraw + y * nBurnPitch, &g_frame[(size_t)y * g_w], (size_t)g_w * sizeof(UINT32));
		}

		if (((++g_draw_cpu_count) & 0x3f) == 0) {
			bprintf(0, _T("[AW] AwaveVideoDrawCpu #%u copied %dx%d to pBurnDraw=%p pitch=%d\n"),
				g_draw_cpu_count, g_w, g_h, pBurnDraw, nBurnPitch);
		}
		return 0;
	}

	if (((++g_draw_cpu_count) & 0x3f) == 0) {
		bprintf(0, _T("[AW] AwaveVideoDrawCpu unsupported nBurnBpp=%d pitch=%d\n"), nBurnBpp, nBurnPitch);
	}
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
	g_set_cpu_count = 0;
	g_draw_cpu_count = 0;
	memset(&g_hw, 0, sizeof(g_hw));
	g_hw.size = sizeof(g_hw);
}

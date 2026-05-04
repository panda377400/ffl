#include "awave_core.h"

#include "awave_content.h"
#include "awave_audio.h"
#include "awave_video.h"
#include "awave_input.h"
#include "flycast_shim/fbneo_flycast_api.h"

#include <vector>
#include <cstring>
#include <cstdio>
#include <stdarg.h>

static const NaomiGameConfig* g_cfg = NULL;
static AwaveContentPackage g_content;
static bool g_coreInited = false;
static bool g_gameLoaded = false;
static void* g_oglContext = NULL;
static void (*g_makeCurrent)(void*) = NULL;
static void (*g_doneCurrent)(void*) = NULL;
static void (*g_swapBuffers)(void*) = NULL;
static bool g_useFallbackContext = false;

static void AwaveDiagLog(const char* fmt, ...)
{
	FILE* f = fopen("awave_core_debug.log", "ab");
	if (f) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(f, fmt, ap);
		va_end(ap);
		fputc('\n', f);
		fclose(f);
	}
}

static void AwaveTrace(const TCHAR* msg)
{
	bprintf(0, _T("[AW] "));
	bprintf(0, msg);
	bprintf(0, _T("\n"));
}

static int ToFbfcPlatform(AwavePlatform p)
{
	switch (p) {
	case AWAVE_PLATFORM_NAOMI: return FBFC_PLATFORM_NAOMI;
	case AWAVE_PLATFORM_NAOMI2: return FBFC_PLATFORM_NAOMI2;
	case AWAVE_PLATFORM_DREAMCAST: return FBFC_PLATFORM_DREAMCAST;
	case AWAVE_PLATFORM_ATOMISWAVE:
	default: return FBFC_PLATFORM_ATOMISWAVE;
	}
}

static int ToFbfcInput(UINT32 t)
{
	switch (t) {
	case NAOMI_GAME_INPUT_LIGHTGUN: return FBFC_INPUT_LIGHTGUN;
	case NAOMI_GAME_INPUT_RACING: return FBFC_INPUT_RACING;
	case NAOMI_GAME_INPUT_BLOCKPONG: return FBFC_INPUT_BLOCKPONG;
	case NAOMI_GAME_INPUT_DREAMCAST_PAD: return FBFC_INPUT_DREAMCAST_PAD;
	case NAOMI_GAME_INPUT_DIGITAL:
	default: return FBFC_INPUT_DIGITAL;
	}
}

static void AwaveAudioCallback(const INT16* stereo, int frames, void* user)
{
	(void)user;
	AwaveAudioPush(stereo, frames);
}

static void AwaveVideoCallback(const FbneoFlycastVideoFrame* frame, void* user)
{
	(void)user;
	if (frame == NULL) return;

	if (frame->valid_hw) {
		AwaveVideoSetHwFrame(frame->texture, frame->framebuffer, frame->width, frame->height, frame->upside_down);
	}

	if (frame->valid_cpu && frame->pixels != NULL) {
		AwaveVideoSetCpuFrame(frame->pixels, frame->width, frame->height, frame->pitch_bytes);
	}
}

static void FillInputState(FbneoFlycastInputState& out)
{
	memset(&out, 0, sizeof(out));

	for (INT32 p = 0; p < 4; p++) {
		out.pad[p] = AwaveInputGetPad(p);
		for (INT32 a = 0; a < 8; a++) out.analog[p][a] = AwaveInputGetAnalog(p, a);
		for (INT32 b = 0; b < 4; b++) out.analog_button[p][b] = AwaveInputGetAnalogButton(p, b);
		AwaveInputGetLightgun(p, &out.gun_x[p], &out.gun_y[p], &out.gun_offscreen[p], &out.gun_reload[p]);
	}
}

static INT32 BuildFlycastGameInfo(std::vector<FbneoFlycastRomEntry>& roms, FbneoFlycastGameInfo& game)
{
	roms.clear();
	if (g_cfg == NULL) return 1;

	roms.reserve(g_content.entries.size());
	for (size_t i = 0; i < g_content.entries.size(); i++) {
		FbneoFlycastRomEntry r;
		r.filename = g_content.entries[i].filename;
		r.data = g_content.entries[i].data.empty() ? NULL : g_content.entries[i].data.data();
		r.size = g_content.entries[i].data.size();
		roms.push_back(r);
	}

	memset(&game, 0, sizeof(game));
	game.driver_name = g_cfg->driverName;
	game.zip_name = g_cfg->zipName;
	game.bios_zip_name = g_cfg->biosZipName ? g_cfg->biosZipName : "awbios.zip";
	game.system_name = g_cfg->systemName ? g_cfg->systemName : "atomiswave";
	game.save_dir = NULL;
	game.platform = ToFbfcPlatform(g_cfg->platform);
	game.input_type = ToFbfcInput(g_cfg->inputType);
	game.roms = roms.empty() ? NULL : roms.data();
	game.rom_count = (int)roms.size();
	return 0;
}

INT32 NaomiCoreInit(const NaomiGameConfig* config)
{
	AwaveTrace(_T("NaomiCoreInit enter"));
	AwaveDiagLog("NaomiCoreInit enter");

	NaomiCoreExit();
	AwaveDiagLog("after NaomiCoreExit");

	if (config == NULL) {
		AwaveDiagLog("config == NULL");
		return 1;
	}

	g_cfg = config;
	AwaveDiagLog("driver=%s zip=%s bios=%s system=%s platform=%d input=%u",
		config->driverName ? config->driverName : "",
		config->zipName ? config->zipName : "",
		config->biosZipName ? config->biosZipName : "",
		config->systemName ? config->systemName : "",
		(int)config->platform,
		(unsigned)config->inputType);

	AwaveTrace(_T("before AwaveAudioInit"));
	AwaveAudioInit(nBurnSoundRate > 0 ? nBurnSoundRate : 44100, 3);

	AwaveTrace(_T("before AwaveVideoReset"));
	AwaveVideoReset();

	AwaveTrace(_T("before AwaveInputResetRuntime"));
	AwaveInputResetRuntime();

	FbneoFlycastCallbacks cb;
	memset(&cb, 0, sizeof(cb));
	cb.audio = AwaveAudioCallback;
	cb.video = AwaveVideoCallback;
	cb.user = NULL;

	AwaveTrace(_T("before fbfc_init"));
	AwaveDiagLog("before fbfc_init");
	if (!fbfc_init(&cb)) {
		AwaveDiagLog("fbfc_init failed");
		bprintf(0, _T("atomiswave: fbneo_flycast_api init failed. See awave_core_debug.log / fbfc_debug.log.\n"));
		return 1;
	}

	AwaveTrace(_T("after fbfc_init"));
	AwaveDiagLog("after fbfc_init");
	g_coreInited = true;

	AwaveTrace(_T("before AwaveBuildContentPackage"));
	AwaveDiagLog("before AwaveBuildContentPackage");
	if (AwaveBuildContentPackage(config, g_content)) {
		AwaveDiagLog("AwaveBuildContentPackage failed");
		bprintf(0, _T("atomiswave: unable to build ROM package. See awave_core_debug.log.\n"));
		NaomiCoreExit();
		return 1;
	}
	AwaveDiagLog("after AwaveBuildContentPackage entries=%u", (unsigned)g_content.entries.size());

	std::vector<FbneoFlycastRomEntry> roms;
	FbneoFlycastGameInfo game;

	AwaveTrace(_T("before BuildFlycastGameInfo"));
	if (BuildFlycastGameInfo(roms, game)) {
		AwaveDiagLog("BuildFlycastGameInfo failed");
		NaomiCoreExit();
		return 1;
	}
	AwaveDiagLog("after BuildFlycastGameInfo rom_count=%d", game.rom_count);

	AwaveTrace(_T("before fbfc_load_game"));
	AwaveDiagLog("before fbfc_load_game");
	if (!fbfc_load_game(&game)) {
		AwaveDiagLog("fbfc_load_game failed");
		bprintf(0, _T("atomiswave: Flycast failed to load game. See fbneo_flycast_runtime/fbfc_debug.log.\n"));
		NaomiCoreExit();
		return 1;
	}

	AwaveTrace(_T("after fbfc_load_game"));
	AwaveDiagLog("after fbfc_load_game");
	g_gameLoaded = true;
	return 0;
}

INT32 NaomiCoreExit()
{
	AwaveDiagLog("NaomiCoreExit enter gameLoaded=%d coreInited=%d", g_gameLoaded ? 1 : 0, g_coreInited ? 1 : 0);

	if (g_gameLoaded) {
		fbfc_unload_game();
		g_gameLoaded = false;
	}

	if (g_coreInited) {
		fbfc_shutdown();
		g_coreInited = false;
	}

	AwaveFreeContentPackage(g_content);
	AwaveAudioExit();
	AwaveVideoReset();
	AwaveInputResetRuntime();
	g_cfg = NULL;
	return 0;
}

INT32 NaomiCoreReset()
{
	AwaveAudioReset();
	AwaveVideoReset();
	AwaveInputResetRuntime();
	if (g_gameLoaded) fbfc_reset();
	return 0;
}

INT32 NaomiCoreFrame()
{
	if (!g_gameLoaded) return 1;

	if (g_makeCurrent) g_makeCurrent(g_oglContext);

	FbneoFlycastInputState input;
	FillInputState(input);
	const INT32 ret = fbfc_run_frame(&input) ? 0 : 1;

	if (pBurnSoundOut && nBurnSoundLen > 0) {
		AwaveAudioRender(pBurnSoundOut, nBurnSoundLen);
	}

	if (g_doneCurrent) g_doneCurrent(g_oglContext);
	return ret;
}

INT32 NaomiCoreDraw()
{
	if (AwaveVideoUsingHwDirectPresent()) return 0;
	return AwaveVideoDrawCpu();
}

INT32 NaomiCoreScan(INT32 nAction, INT32* pnMin)
{
	if (pnMin) *pnMin = 0x029702;
	(void)nAction;
	return 0;
}

INT32 NaomiCoreGetHwVideoInfo(NaomiCoreHwVideoInfo* info)
{
	return AwaveVideoGetHwInfo(info);
}

INT32 NaomiCoreUsingHwDirectPresent()
{
	return AwaveVideoUsingHwDirectPresent();
}

INT32 NaomiCoreWantsRedraw()
{
	return AwaveVideoWantsRedraw();
}

void NaomiCoreSetPadState(INT32 port, UINT32 state) { AwaveInputSetPad(port, state); }
void NaomiCoreSetAnalogState(INT32 port, INT32 axis, INT16 value) { AwaveInputSetAnalog(port, axis, value); }
void NaomiCoreSetAnalogButtonState(INT32 port, INT32 button, INT16 value) { AwaveInputSetAnalogButton(port, button, value); }
void NaomiCoreSetLightgunState(INT32 port, INT16 x, INT16 y, UINT8 offscreen, UINT8 reload) { AwaveInputSetLightgun(port, x, y, offscreen, reload); }

void OGLSetContext(void* ptr) { g_oglContext = ptr; }
void OGLSetMakeCurrent(void (*f)(void*)) { g_makeCurrent = f; }
void OGLSetDoneCurrent(void (*f)(void*)) { g_doneCurrent = f; }
void OGLSetSwapBuffers(void (*f)(void*)) { g_swapBuffers = f; }
void OGLSetUseFallbackContext(bool use) { g_useFallbackContext = use; }
bool OGLUsingFallbackContext() { return g_useFallbackContext; }

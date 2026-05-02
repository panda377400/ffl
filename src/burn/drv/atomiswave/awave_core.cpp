#include "burnint.h"
#include "awave_wrap_config.h"
#include "awave_core.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <GL/gl.h>
#include <windows.h>
#else
#include <GL/gl.h>
#include <sys/stat.h>
#endif

#include <libretro.h>
#include <zlib.h>

#ifndef RETRO_ENVIRONMENT_SET_SAVE_STATE_IN_BACKGROUND
#define RETRO_ENVIRONMENT_SET_SAVE_STATE_IN_BACKGROUND 0
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif

#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif

#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif

#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#endif

#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif

#ifndef GL_RENDERBUFFER_BINDING
#define GL_RENDERBUFFER_BINDING 0x8CA7
#endif

#ifndef GL_PIXEL_PACK_BUFFER
#define GL_PIXEL_PACK_BUFFER 0x88EB
#endif

#ifndef GL_PIXEL_PACK_BUFFER_BINDING
#define GL_PIXEL_PACK_BUFFER_BINDING 0x88ED
#endif

#ifndef GL_STREAM_READ
#define GL_STREAM_READ 0x88E1
#endif

#ifndef GL_READ_ONLY
#define GL_READ_ONLY 0x88B8
#endif

extern "C" {
	void flycast_retro_set_video_refresh(retro_video_refresh_t cb);
	void flycast_retro_set_audio_sample(retro_audio_sample_t cb);
	void flycast_retro_set_audio_sample_batch(retro_audio_sample_batch_t cb);
	void flycast_retro_set_input_poll(retro_input_poll_t cb);
	void flycast_retro_set_input_state(retro_input_state_t cb);
	void flycast_retro_set_environment(retro_environment_t cb);
	void flycast_retro_set_controller_port_device(unsigned port, unsigned device);
	void flycast_retro_init(void);
	void flycast_retro_deinit(void);
	void flycast_retro_run(void);
	void flycast_retro_reset(void);
	bool flycast_retro_load_game(const struct retro_game_info* game);
	void flycast_retro_unload_game(void);
	void flycast_retro_audio_init(void);
	void flycast_retro_audio_deinit(void);
	void flycast_retro_audio_flush_buffer(void);
	void flycast_retro_audio_upload(void);
	void fbneo_awave_reset_present_filter(void);
	size_t flycast_retro_serialize_size(void);
	bool flycast_retro_serialize(void* data, size_t size);
	bool flycast_retro_unserialize(const void* data, size_t size);
	void flycast_retro_get_system_av_info(struct retro_system_av_info* info);
}

extern int screen_width;
extern int screen_height;
extern void rend_reset_fb_clear_flag();
extern "C" void* OGLGetProcAddress(const char* name);
extern void* OGLGetContext();
extern void OGLMakeCurrentContext();
extern void OGLDoneCurrentContext();
extern bool OGLCreateFallbackContext();
extern void OGLDestroyFallbackContext();
extern void OGLSetUseFallbackContext(bool use);
extern bool OGLUsingFallbackContext();

static const NaomiGameConfig* NaomiGame = NULL;
static bool NaomiLoaded = false;
static UINT32 NaomiPads[4];
static const UINT32* NaomiJoypadMap = NULL;
static UINT32 NaomiJoypadMapCount = 0;
static const UINT32* NaomiLightgunMap = NULL;
static UINT32 NaomiLightgunMapCount = 0;
static INT16 NaomiAnalog[4][4];
static INT16 NaomiAnalogButtons[4][2];
static INT16 NaomiLightgunAxis[4][2];
static UINT8 NaomiLightgunOffscreen[4];
static UINT8 NaomiLightgunReload[4];
static std::vector<UINT32> NaomiVideo;
static std::vector<UINT32> NaomiHardwareVideoScratch;
static std::vector<INT16> NaomiAudio;
static std::vector<UINT8> NaomiState;
static std::mutex NaomiAudioMutex;
static INT32 NaomiVideoWidth = 640;
static INT32 NaomiVideoHeight = 480;
static INT32 NaomiVideoStride = 640;
static bool NaomiVideoFromCallback = false;
static bool NaomiVideoReadPending = false;
static unsigned NaomiPendingVideoWidth = 0;
static unsigned NaomiPendingVideoHeight = 0;
static INT32 NaomiVideoProbeReads = 0;
static INT32 NaomiVideoStartupSkipReads = 0;
static INT32 NaomiAcceptedVideoFrames = 0;
static size_t NaomiAudioReadOffset = 0;
static INT32 NaomiAudioWarmupFrames = 0;
static double NaomiAudioSourcePos = 0.0;
static INT32 NaomiAudioSampleRate = 44100;
static INT16 NaomiAudioLastSample[2] = { 0, 0 };
static INT32 NaomiVideoWarmupReads = 0;
static char NaomiTempRoot[1024];
static char NaomiLegacyTempRoot[1024];
static char NaomiTempContentDir[1024];
static char NaomiTempZipPath[1024];
static char NaomiTempZipWritePath[1024];
static char NaomiTempSaveDir[1024];
static UINT32 NaomiContentGeneration = 0;
static struct retro_hw_render_callback NaomiHwRender;
static bool NaomiHwRenderRequested = false;
static bool NaomiHwRenderReady = false;
static bool NaomiRetroInitialized = false;
static bool NaomiRetroGameLoaded = false;
static const bool NaomiUseFrontendFramebuffer = true;
static GLuint NaomiFrontendFramebuffer = 0;
static GLuint NaomiFrontendColorTexture = 0;
static GLuint NaomiFrontendDepthStencilBuffer = 0;
static INT32 NaomiFrontendFramebufferWidth = 0;
static INT32 NaomiFrontendFramebufferHeight = 0;
static bool NaomiHwDirectFrameValid = false;
static INT32 NaomiHwDirectFrameWidth = 0;
static INT32 NaomiHwDirectFrameHeight = 0;
static bool NaomiHwDirectWarnedNoFrontend = false;

enum NaomiVideoPresentMode {
	NAOMI_VIDEO_PRESENT_READBACK = 0,
	NAOMI_VIDEO_PRESENT_DIRECT_TEXTURE = 1,
	NAOMI_VIDEO_PRESENT_SKIP_READBACK = 2,
};

// Async readback through Pixel Buffer Objects.
// This keeps glReadPixels from forcing a full GPU/CPU sync on the same frame.
// It intentionally adds two frames of video latency, which is much cheaper than
// stalling the emulator for 4-8 ms every frame.
static const INT32 NaomiReadbackPboCount = 3;
static GLuint NaomiReadbackPbos[NaomiReadbackPboCount] = { 0, 0, 0 };
static bool NaomiReadbackPboValid[NaomiReadbackPboCount] = { false, false, false };
static INT32 NaomiReadbackPboIndex = 0;
static INT32 NaomiReadbackPboWidth = 0;
static INT32 NaomiReadbackPboHeight = 0;
static size_t NaomiReadbackPboBytes = 0;
static bool NaomiReadbackPboAvailableLogged = false;

static NaomiVideoPresentMode NaomiGetVideoPresentMode()
{
	const char* env = getenv("FBNEO_AWAVE_VIDEO_MODE");
	if (env != NULL && env[0] != 0) {
		if (!strcmp(env, "direct") || !strcmp(env, "direct_texture") || !strcmp(env, "hw") || !strcmp(env, "texture")) {
			return NAOMI_VIDEO_PRESENT_DIRECT_TEXTURE;
		}
		if (!strcmp(env, "skip") || !strcmp(env, "skip_readback") || !strcmp(env, "no_readback")) {
			return NAOMI_VIDEO_PRESENT_SKIP_READBACK;
		}
		if (!strcmp(env, "readback") || !strcmp(env, "cpu") || !strcmp(env, "cpu_readback")) {
			return NAOMI_VIDEO_PRESENT_READBACK;
		}
	}

	// Safe default for the current standalone atomiswave directory: keep CPU
	// readback unless a frontend explicitly asks for direct texture output.
	return NAOMI_VIDEO_PRESENT_READBACK;
}

static const char* NaomiVideoPresentModeName(NaomiVideoPresentMode mode)
{
	switch (mode) {
		case NAOMI_VIDEO_PRESENT_DIRECT_TEXTURE: return "direct_texture";
		case NAOMI_VIDEO_PRESENT_SKIP_READBACK: return "skip_readback";
		case NAOMI_VIDEO_PRESENT_READBACK:
		default: return "cpu_readback";
	}
}

static bool NaomiWantsDirectTexturePresent()
{
	return NaomiGetVideoPresentMode() == NAOMI_VIDEO_PRESENT_DIRECT_TEXTURE;
}

static bool NaomiWantsSkipReadback()
{
	return NaomiGetVideoPresentMode() == NAOMI_VIDEO_PRESENT_SKIP_READBACK;
}

static bool NaomiCanExposeHwTextureToFrontend()
{
	return NaomiWantsDirectTexturePresent()
		&& NaomiHwRenderReady
		&& NaomiFrontendColorTexture != 0
		&& NaomiFrontendFramebufferWidth > 0
		&& NaomiFrontendFramebufferHeight > 0
		&& !OGLUsingFallbackContext();
}

// Lightweight performance counters for zzBurnDebug.html.
// The logs are intentionally throttled to avoid slowing the emulator down.
static bool NaomiPerfLoggingEnabled()
{
	static INT32 enabled = -1;
	if (enabled < 0) {
		const char* env = getenv("FBNEO_AWAVE_PROFILE");
		enabled = (env != NULL && env[0] != 0 && strcmp(env, "0") != 0) ? 1 : 0;
	}
	return enabled != 0;
}
static INT32 NaomiPerfFrameCounter = 0;
static INT32 NaomiPerfDrawCounter = 0;
static double NaomiPerfContextAccumMs = 0.0;
static double NaomiPerfRunAccumMs = 0.0;
static double NaomiPerfCaptureAccumMs = 0.0;
static double NaomiPerfAudioAccumMs = 0.0;
static double NaomiPerfTotalAccumMs = 0.0;
static double NaomiPerfOuterDeltaAccumMs = 0.0;
static double NaomiPerfOuterMaxDeltaMs = 0.0;
static double NaomiLastFrameEntryMs = 0.0;
static double NaomiPerfReadPixelsAccumMs = 0.0;
static double NaomiPerfReadFlipAccumMs = 0.0;
static double NaomiPerfDrawAccumMs = 0.0;
static INT32 NaomiPerfReadPixelsSamples = 0;
static INT32 NaomiPerfSlowRunCount = 0;
static double NaomiPerfSlowRunMaxMs = 0.0;
static INT32 NaomiPerfInvalidTimingCount = 0;

static bool NaomiActivateHwRender();
static void NaomiResetReadbackPboState();
static INT32 NaomiLoadBurnRom(INT32 index, std::vector<UINT8>& data);

typedef void (APIENTRY *NaomiPFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *NaomiPFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *NaomiPFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint*);
typedef GLenum (APIENTRY *NaomiPFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void (APIENTRY *NaomiPFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (APIENTRY *NaomiPFNGLGENRENDERBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *NaomiPFNGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *NaomiPFNGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *NaomiPFNGLRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *NaomiPFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum, GLenum, GLenum, GLuint);

struct NaomiFramebufferFns {
	NaomiPFNGLGENFRAMEBUFFERSPROC genFramebuffers;
	NaomiPFNGLBINDFRAMEBUFFERPROC bindFramebuffer;
	NaomiPFNGLDELETEFRAMEBUFFERSPROC deleteFramebuffers;
	NaomiPFNGLCHECKFRAMEBUFFERSTATUSPROC checkFramebufferStatus;
	NaomiPFNGLFRAMEBUFFERTEXTURE2DPROC framebufferTexture2D;
	NaomiPFNGLGENRENDERBUFFERSPROC genRenderbuffers;
	NaomiPFNGLBINDRENDERBUFFERPROC bindRenderbuffer;
	NaomiPFNGLDELETERENDERBUFFERSPROC deleteRenderbuffers;
	NaomiPFNGLRENDERBUFFERSTORAGEPROC renderbufferStorage;
	NaomiPFNGLFRAMEBUFFERRENDERBUFFERPROC framebufferRenderbuffer;
};

static NaomiFramebufferFns NaomiGlFramebufferFns = {};

typedef void (APIENTRY *NaomiPFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *NaomiPFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *NaomiPFNGLBUFFERDATAPROC)(GLenum, std::ptrdiff_t, const void*, GLenum);
typedef void* (APIENTRY *NaomiPFNGLMAPBUFFERPROC)(GLenum, GLenum);
typedef GLboolean (APIENTRY *NaomiPFNGLUNMAPBUFFERPROC)(GLenum);
typedef void (APIENTRY *NaomiPFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);

struct NaomiPboFns {
	NaomiPFNGLGENBUFFERSPROC genBuffers;
	NaomiPFNGLBINDBUFFERPROC bindBuffer;
	NaomiPFNGLBUFFERDATAPROC bufferData;
	NaomiPFNGLMAPBUFFERPROC mapBuffer;
	NaomiPFNGLUNMAPBUFFERPROC unmapBuffer;
	NaomiPFNGLDELETEBUFFERSPROC deleteBuffers;
};

static NaomiPboFns NaomiGlPboFns = {};

struct NaomiZipRecord {
	const char* filename;
	UINT32 crc;
	UINT32 size;
	UINT32 offset;
};

struct NaomiDirectArchiveEntry {
	std::string filename;
	UINT32 crc;
	std::vector<UINT8> data;
};

static std::vector<NaomiDirectArchiveEntry> NaomiDirectArchiveEntries;
static const char* NaomiDirectArchivePrefix = "fbneo-awave://";

#ifdef _WIN32
static int NaomiMkdir(const char* path)
{
	return _mkdir(path);
}
#else
static int NaomiMkdir(const char* path)
{
	return mkdir(path, 0755);
}
#endif

static void NaomiCopyString(char* dst, size_t dstLen, const char* src)
{
	if (dstLen == 0) {
		return;
	}

	if (src == NULL) {
		dst[0] = 0;
		return;
	}

	strncpy(dst, src, dstLen - 1);
	dst[dstLen - 1] = 0;
}

static void NaomiEnsureDirectory(const char* path)
{
	if (path == NULL || path[0] == 0) {
		return;
	}

	char temp[1024];
	NaomiCopyString(temp, sizeof(temp), path);

	for (char* p = temp; *p; p++) {
		if (*p == '/' || *p == '\\') {
			char saved = *p;
			*p = 0;
			if (temp[0] != 0) {
				NaomiMkdir(temp);
			}
			*p = saved;
		}
	}

	NaomiMkdir(temp);
}

static bool NaomiFileExists(const char* path)
{
	if (path == NULL || path[0] == 0) {
		return false;
	}

#ifdef _WIN32
	const DWORD attributes = GetFileAttributesA(path);
	return attributes != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st;
	return stat(path, &st) == 0;
#endif
}

static void NaomiGetAppDirectory(char* path, size_t pathLen)
{
	if (pathLen == 0) {
		return;
	}

	path[0] = 0;

#ifdef _WIN32
	DWORD copied = GetModuleFileNameA(NULL, path, (DWORD)pathLen);
	if (copied > 0 && copied < pathLen) {
		char* lastBackslash = strrchr(path, '\\');
		char* lastSlash = strrchr(path, '/');
		char* lastSeparator = lastBackslash;
		if (lastSeparator == NULL || (lastSlash != NULL && lastSlash > lastSeparator)) {
			lastSeparator = lastSlash;
		}
		if (lastSeparator != NULL) {
			lastSeparator[1] = 0;
			return;
		}
	}
#else
	if (getcwd(path, pathLen - 2) != NULL) {
		const size_t used = strlen(path);
		path[used] = '/';
		path[used + 1] = 0;
		return;
	}
#endif

	NaomiCopyString(path, pathLen, ".\\");
}

static void NaomiCopyFileIfMissing(const char* source, const char* destination)
{
	if (!NaomiFileExists(source) || NaomiFileExists(destination)) {
		return;
	}

	FILE* src = fopen(source, "rb");
	if (src == NULL) {
		return;
	}

	FILE* dst = fopen(destination, "wb");
	if (dst == NULL) {
		fclose(src);
		return;
	}

	char buffer[16384];
	while (!feof(src)) {
		const size_t read = fread(buffer, 1, sizeof(buffer), src);
		if (read == 0) {
			break;
		}
		if (fwrite(buffer, 1, read, dst) != read) {
			break;
		}
	}

	fclose(dst);
	fclose(src);
}

static void NaomiBuildLegacyTempPaths(char* root, size_t rootLen, char* saveDir, size_t saveDirLen)
{
	const char* base = getenv("TEMP");
	if (base == NULL || base[0] == 0) {
		base = getenv("TMP");
	}
	if (base == NULL || base[0] == 0) {
		base = ".";
	}

	snprintf(root, rootLen, "%s\\fbneo_flycast\\%s", base, NaomiGame->driverName);
	snprintf(saveDir, saveDirLen, "%s\\save", root);
	root[rootLen - 1] = 0;
	saveDir[saveDirLen - 1] = 0;
}

static void NaomiMigrateLegacySaveData()
{
	char legacySaveDir[1024];
	char legacyFile[1024];
	char runtimeFile[1024];

	NaomiBuildLegacyTempPaths(NaomiLegacyTempRoot, sizeof(NaomiLegacyTempRoot), legacySaveDir, sizeof(legacySaveDir));

	snprintf(legacyFile, sizeof(legacyFile), "%s\\reicast\\%s.eeprom", legacySaveDir, NaomiGame->zipName);
	snprintf(runtimeFile, sizeof(runtimeFile), "%s\\reicast\\%s.eeprom", NaomiTempSaveDir, NaomiGame->zipName);
	NaomiCopyFileIfMissing(legacyFile, runtimeFile);

	snprintf(legacyFile, sizeof(legacyFile), "%s\\reicast\\%s.nvmem", legacySaveDir, NaomiGame->zipName);
	snprintf(runtimeFile, sizeof(runtimeFile), "%s\\reicast\\%s.nvmem", NaomiTempSaveDir, NaomiGame->zipName);
	NaomiCopyFileIfMissing(legacyFile, runtimeFile);

	snprintf(legacyFile, sizeof(legacyFile), "%s\\reicast\\%s.nvmem2", legacySaveDir, NaomiGame->zipName);
	snprintf(runtimeFile, sizeof(runtimeFile), "%s\\reicast\\%s.nvmem2", NaomiTempSaveDir, NaomiGame->zipName);
	NaomiCopyFileIfMissing(legacyFile, runtimeFile);
}

static bool NaomiUseDirectArchive()
{
	const char* env = getenv("FBNEO_AWAVE_USE_DIRECT_ROM");
	return !(env != NULL && (env[0] == '0' || !strcmp(env, "disabled")));
}

static bool NaomiEnvEnabled(const char* name)
{
	const char* env = getenv(name);
	return env != NULL && env[0] != 0 && strcmp(env, "0") != 0 && strcmp(env, "disabled") != 0;
}

static bool NaomiTraceSlowRunEnabled()
{
	static INT32 enabled = -1;
	if (enabled < 0) {
		enabled = NaomiEnvEnabled("FBNEO_AWAVE_TRACE_SLOWRUN") ? 1 : 0;
	}
	return enabled != 0;
}

static bool NaomiValidProfileDeltaMs(double value)
{
	return value == value && value >= 0.0 && value < 1000.0;
}

static double NaomiCleanProfileDeltaMs(double value)
{
	return NaomiValidProfileDeltaMs(value) ? value : 0.0;
}

static void NaomiBuildTempPaths()
{
	char appDir[1024];
	const char* runtimeSubdir = (NaomiGame != NULL && NaomiGame->runtimeSubdir != NULL && NaomiGame->runtimeSubdir[0] != 0)
		? NaomiGame->runtimeSubdir
		: "awave";

	NaomiGetAppDirectory(appDir, sizeof(appDir));

	snprintf(NaomiTempRoot, sizeof(NaomiTempRoot), "%sconfig\\games\\%s\\%s", appDir, runtimeSubdir, NaomiGame->driverName);
	snprintf(NaomiTempContentDir, sizeof(NaomiTempContentDir), "%s\\content", NaomiTempRoot);
	if (NaomiUseDirectArchive()) {
		snprintf(NaomiTempZipPath, sizeof(NaomiTempZipPath), "%s%s", NaomiDirectArchivePrefix, NaomiGame->zipName);
		NaomiTempZipWritePath[0] = 0;
	} else {
		snprintf(NaomiTempZipPath, sizeof(NaomiTempZipPath), "%s\\%s", NaomiTempContentDir, NaomiGame->zipName);
		snprintf(NaomiTempZipWritePath, sizeof(NaomiTempZipWritePath), "%s\\%s.tmp", NaomiTempContentDir, NaomiGame->zipName);
	}
	snprintf(NaomiTempSaveDir, sizeof(NaomiTempSaveDir), "%s\\save", NaomiTempRoot);

	NaomiTempRoot[sizeof(NaomiTempRoot) - 1] = 0;
	NaomiTempContentDir[sizeof(NaomiTempContentDir) - 1] = 0;
	NaomiTempZipPath[sizeof(NaomiTempZipPath) - 1] = 0;
	NaomiTempZipWritePath[sizeof(NaomiTempZipWritePath) - 1] = 0;
	NaomiTempSaveDir[sizeof(NaomiTempSaveDir) - 1] = 0;

	NaomiEnsureDirectory(NaomiTempRoot);
	NaomiEnsureDirectory(NaomiTempContentDir);
	NaomiEnsureDirectory(NaomiTempSaveDir);
	NaomiEnsureDirectory((std::string(NaomiTempSaveDir) + "\\reicast").c_str());
	NaomiMigrateLegacySaveData();
}

static void NaomiDirectArchiveClear()
{
	NaomiDirectArchiveEntries.clear();
}

static void NaomiDirectArchiveBuildPath()
{
	snprintf(NaomiTempZipPath, sizeof(NaomiTempZipPath), "%s%s", NaomiDirectArchivePrefix, NaomiGame->zipName);
	NaomiTempZipPath[sizeof(NaomiTempZipPath) - 1] = 0;
}

static INT32 NaomiBuildDirectContentArchive()
{
	NaomiDirectArchiveClear();
	NaomiDirectArchiveBuildPath();

	for (INT32 i = 0; NaomiGame->zipEntries[i].filename != NULL; i++) {
		const char* filename = NaomiGame->zipEntries[i].filename;
		std::vector<UINT8> data;

		if (NaomiLoadBurnRom(NaomiGame->zipEntries[i].burnRomIndex, data)) {
			bprintf(0, _T("naomi: failed to load Burn ROM for in-memory archive: %S\n"), filename);
			NaomiDirectArchiveClear();
			return 1;
		}

		NaomiDirectArchiveEntry entry;
		entry.filename = filename;
		entry.crc = (UINT32)crc32(0L, data.data(), (uInt)data.size());
		entry.data.swap(data);
		NaomiDirectArchiveEntries.push_back(entry);
	}

	bprintf(0, _T("AWAVE profile: direct FBNeo ROM archive enabled (%d files, no temp zip copy)\n"), (INT32)NaomiDirectArchiveEntries.size());
	return 0;
}

extern "C" int NaomiDirectArchiveMatch(const char* path)
{
	return path != NULL && !strncmp(path, NaomiDirectArchivePrefix, strlen(NaomiDirectArchivePrefix));
}

extern "C" int NaomiDirectArchiveEntryCount()
{
	return (int)NaomiDirectArchiveEntries.size();
}

extern "C" const char* NaomiDirectArchiveEntryName(int index)
{
	if (index < 0 || index >= (int)NaomiDirectArchiveEntries.size()) {
		return NULL;
	}
	return NaomiDirectArchiveEntries[index].filename.c_str();
}

extern "C" unsigned int NaomiDirectArchiveEntryCrc(int index)
{
	if (index < 0 || index >= (int)NaomiDirectArchiveEntries.size()) {
		return 0;
	}
	return NaomiDirectArchiveEntries[index].crc;
}

extern "C" unsigned int NaomiDirectArchiveRead(int index, unsigned int offset, void* buffer, unsigned int length)
{
	if (buffer == NULL || index < 0 || index >= (int)NaomiDirectArchiveEntries.size()) {
		return 0;
	}
	const std::vector<UINT8>& data = NaomiDirectArchiveEntries[index].data;
	if (offset >= data.size()) {
		return 0;
	}
	const unsigned int available = (unsigned int)std::min<size_t>((size_t)length, data.size() - offset);
	memcpy(buffer, data.data() + offset, available);
	return available;
}

static void NaomiClearRuntimeState()
{
	NaomiVideo.clear();
	NaomiHardwareVideoScratch.clear();
	NaomiDirectArchiveClear();
	{
		std::lock_guard<std::mutex> lock(NaomiAudioMutex);
		NaomiAudio.clear();
		NaomiAudioReadOffset = 0; NaomiAudioSourcePos = 0.0; NaomiAudioLastSample[0] = NaomiAudioLastSample[1] = 0;
	}
	NaomiVideoWidth = 640;
	NaomiVideoHeight = 480;
	NaomiVideoStride = 640;
	NaomiVideoFromCallback = false;
	NaomiVideoReadPending = false;
	NaomiPendingVideoWidth = 0;
	NaomiPendingVideoHeight = 0;
	NaomiVideoProbeReads = 0;
	NaomiVideoStartupSkipReads = 0;
	NaomiAcceptedVideoFrames = 0;
	NaomiAudioWarmupFrames = 0;
	NaomiVideoWarmupReads = 0;
	NaomiHwDirectFrameValid = false;
	NaomiHwDirectFrameWidth = 0;
	NaomiHwDirectFrameHeight = 0;
	memset(NaomiPads, 0, sizeof(NaomiPads));
	memset(NaomiAnalog, 0, sizeof(NaomiAnalog));
	memset(NaomiAnalogButtons, 0, sizeof(NaomiAnalogButtons));
	memset(NaomiLightgunAxis, 0, sizeof(NaomiLightgunAxis));
	memset(NaomiLightgunOffscreen, 0, sizeof(NaomiLightgunOffscreen));
	memset(NaomiLightgunReload, 0, sizeof(NaomiLightgunReload));
	fbneo_awave_reset_present_filter();
	rend_reset_fb_clear_flag();
	NaomiResetReadbackPboState();
}

static void NaomiResetFrameState()
{
	NaomiClearRuntimeState();
	NaomiState.clear();
}

template<typename T>
static T NaomiGetGlProc(const char* coreName, const char* extName = NULL)
{
	T proc = reinterpret_cast<T>(OGLGetProcAddress(coreName));
	if (proc == NULL && extName != NULL) {
		proc = reinterpret_cast<T>(OGLGetProcAddress(extName));
	}
	return proc;
}

static bool NaomiLoadFramebufferFns()
{
	if (NaomiGlFramebufferFns.bindFramebuffer != NULL) {
		return true;
	}

	NaomiGlFramebufferFns.genFramebuffers = NaomiGetGlProc<NaomiPFNGLGENFRAMEBUFFERSPROC>("glGenFramebuffers", "glGenFramebuffersEXT");
	NaomiGlFramebufferFns.bindFramebuffer = NaomiGetGlProc<NaomiPFNGLBINDFRAMEBUFFERPROC>("glBindFramebuffer", "glBindFramebufferEXT");
	NaomiGlFramebufferFns.deleteFramebuffers = NaomiGetGlProc<NaomiPFNGLDELETEFRAMEBUFFERSPROC>("glDeleteFramebuffers", "glDeleteFramebuffersEXT");
	NaomiGlFramebufferFns.checkFramebufferStatus = NaomiGetGlProc<NaomiPFNGLCHECKFRAMEBUFFERSTATUSPROC>("glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
	NaomiGlFramebufferFns.framebufferTexture2D = NaomiGetGlProc<NaomiPFNGLFRAMEBUFFERTEXTURE2DPROC>("glFramebufferTexture2D", "glFramebufferTexture2DEXT");
	NaomiGlFramebufferFns.genRenderbuffers = NaomiGetGlProc<NaomiPFNGLGENRENDERBUFFERSPROC>("glGenRenderbuffers", "glGenRenderbuffersEXT");
	NaomiGlFramebufferFns.bindRenderbuffer = NaomiGetGlProc<NaomiPFNGLBINDRENDERBUFFERPROC>("glBindRenderbuffer", "glBindRenderbufferEXT");
	NaomiGlFramebufferFns.deleteRenderbuffers = NaomiGetGlProc<NaomiPFNGLDELETERENDERBUFFERSPROC>("glDeleteRenderbuffers", "glDeleteRenderbuffersEXT");
	NaomiGlFramebufferFns.renderbufferStorage = NaomiGetGlProc<NaomiPFNGLRENDERBUFFERSTORAGEPROC>("glRenderbufferStorage", "glRenderbufferStorageEXT");
	NaomiGlFramebufferFns.framebufferRenderbuffer = NaomiGetGlProc<NaomiPFNGLFRAMEBUFFERRENDERBUFFERPROC>("glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT");

	return NaomiGlFramebufferFns.genFramebuffers != NULL
		&& NaomiGlFramebufferFns.bindFramebuffer != NULL
		&& NaomiGlFramebufferFns.deleteFramebuffers != NULL
		&& NaomiGlFramebufferFns.checkFramebufferStatus != NULL
		&& NaomiGlFramebufferFns.framebufferTexture2D != NULL
		&& NaomiGlFramebufferFns.genRenderbuffers != NULL
		&& NaomiGlFramebufferFns.bindRenderbuffer != NULL
		&& NaomiGlFramebufferFns.deleteRenderbuffers != NULL
		&& NaomiGlFramebufferFns.renderbufferStorage != NULL
		&& NaomiGlFramebufferFns.framebufferRenderbuffer != NULL;
}


static bool NaomiLoadPboFns()
{
	if (NaomiGlPboFns.bindBuffer != NULL) {
		return true;
	}

	NaomiGlPboFns.genBuffers = NaomiGetGlProc<NaomiPFNGLGENBUFFERSPROC>("glGenBuffers", "glGenBuffersARB");
	NaomiGlPboFns.bindBuffer = NaomiGetGlProc<NaomiPFNGLBINDBUFFERPROC>("glBindBuffer", "glBindBufferARB");
	NaomiGlPboFns.bufferData = NaomiGetGlProc<NaomiPFNGLBUFFERDATAPROC>("glBufferData", "glBufferDataARB");
	NaomiGlPboFns.mapBuffer = NaomiGetGlProc<NaomiPFNGLMAPBUFFERPROC>("glMapBuffer", "glMapBufferARB");
	NaomiGlPboFns.unmapBuffer = NaomiGetGlProc<NaomiPFNGLUNMAPBUFFERPROC>("glUnmapBuffer", "glUnmapBufferARB");
	NaomiGlPboFns.deleteBuffers = NaomiGetGlProc<NaomiPFNGLDELETEBUFFERSPROC>("glDeleteBuffers", "glDeleteBuffersARB");

	return NaomiGlPboFns.genBuffers != NULL
		&& NaomiGlPboFns.bindBuffer != NULL
		&& NaomiGlPboFns.bufferData != NULL
		&& NaomiGlPboFns.mapBuffer != NULL
		&& NaomiGlPboFns.unmapBuffer != NULL
		&& NaomiGlPboFns.deleteBuffers != NULL;
}

static void NaomiResetReadbackPboState()
{
	for (INT32 i = 0; i < NaomiReadbackPboCount; i++) {
		NaomiReadbackPboValid[i] = false;
	}
	NaomiReadbackPboIndex = 0;
}

static void NaomiDestroyReadbackPbos()
{
	if (NaomiGlPboFns.deleteBuffers != NULL) {
		NaomiGlPboFns.deleteBuffers(NaomiReadbackPboCount, NaomiReadbackPbos);
	}

	for (INT32 i = 0; i < NaomiReadbackPboCount; i++) {
		NaomiReadbackPbos[i] = 0;
		NaomiReadbackPboValid[i] = false;
	}

	NaomiReadbackPboIndex = 0;
	NaomiReadbackPboWidth = 0;
	NaomiReadbackPboHeight = 0;
	NaomiReadbackPboBytes = 0;
}

static bool NaomiEnsureReadbackPbos(unsigned width, unsigned height)
{
	if (width == 0 || height == 0 || !NaomiLoadPboFns()) {
		return false;
	}

	const size_t bytes = (size_t)width * (size_t)height * sizeof(UINT32);
	if (NaomiReadbackPbos[0] != 0
		&& NaomiReadbackPboWidth == (INT32)width
		&& NaomiReadbackPboHeight == (INT32)height
		&& NaomiReadbackPboBytes == bytes) {
		return true;
	}

	GLint previousPackBuffer = 0;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);

	NaomiDestroyReadbackPbos();

	NaomiGlPboFns.genBuffers(NaomiReadbackPboCount, NaomiReadbackPbos);
	for (INT32 i = 0; i < NaomiReadbackPboCount; i++) {
		if (NaomiReadbackPbos[i] == 0) {
			NaomiGlPboFns.bindBuffer(GL_PIXEL_PACK_BUFFER, (GLuint)previousPackBuffer);
			NaomiDestroyReadbackPbos();
			return false;
		}
		NaomiGlPboFns.bindBuffer(GL_PIXEL_PACK_BUFFER, NaomiReadbackPbos[i]);
		NaomiGlPboFns.bufferData(GL_PIXEL_PACK_BUFFER, (std::ptrdiff_t)bytes, NULL, GL_STREAM_READ);
		NaomiReadbackPboValid[i] = false;
	}

	NaomiGlPboFns.bindBuffer(GL_PIXEL_PACK_BUFFER, (GLuint)previousPackBuffer);

	NaomiReadbackPboIndex = 0;
	NaomiReadbackPboWidth = (INT32)width;
	NaomiReadbackPboHeight = (INT32)height;
	NaomiReadbackPboBytes = bytes;

	if (!NaomiReadbackPboAvailableLogged) {
		NaomiReadbackPboAvailableLogged = true;
		bprintf(0, _T("AWAVE profile: async PBO readback enabled (%d buffers, %dx%d)\n"),
			NaomiReadbackPboCount,
			(INT32)width,
			(INT32)height);
	}

	return true;
}


static void NaomiDestroyFrontendFramebuffer()
{
	NaomiDestroyReadbackPbos();

	if (!NaomiUseFrontendFramebuffer) {
		NaomiFrontendFramebuffer = 0;
		NaomiFrontendColorTexture = 0;
		NaomiFrontendDepthStencilBuffer = 0;
		NaomiFrontendFramebufferWidth = 0;
		NaomiFrontendFramebufferHeight = 0;
		return;
	}

	if (NaomiGlFramebufferFns.bindFramebuffer == NULL && !NaomiLoadFramebufferFns()) {
		NaomiFrontendFramebuffer = 0;
		NaomiFrontendColorTexture = 0;
		NaomiFrontendDepthStencilBuffer = 0;
		NaomiFrontendFramebufferWidth = 0;
		NaomiFrontendFramebufferHeight = 0;
		return;
	}

	if (NaomiFrontendFramebuffer != 0) {
		NaomiGlFramebufferFns.deleteFramebuffers(1, &NaomiFrontendFramebuffer);
		NaomiFrontendFramebuffer = 0;
	}
	if (NaomiFrontendDepthStencilBuffer != 0) {
		NaomiGlFramebufferFns.deleteRenderbuffers(1, &NaomiFrontendDepthStencilBuffer);
		NaomiFrontendDepthStencilBuffer = 0;
	}
	if (NaomiFrontendColorTexture != 0) {
		glDeleteTextures(1, &NaomiFrontendColorTexture);
		NaomiFrontendColorTexture = 0;
	}

	NaomiFrontendFramebufferWidth = 0;
	NaomiFrontendFramebufferHeight = 0;
}

static bool NaomiEnsureFrontendFramebuffer(INT32 width, INT32 height)
{
	if (!NaomiUseFrontendFramebuffer) {
		if (OGLGetContext() == NULL) {
			return false;
		}

		NaomiFrontendFramebufferWidth = std::max<INT32>(640, width);
		NaomiFrontendFramebufferHeight = std::max<INT32>(480, height);
		return true;
	}

	if (OGLGetContext() == NULL || !NaomiLoadFramebufferFns()) {
		return false;
	}

	width = std::max<INT32>(640, width);
	height = std::max<INT32>(480, height);

	if (NaomiFrontendFramebuffer != 0 && NaomiFrontendFramebufferWidth == width && NaomiFrontendFramebufferHeight == height) { return true; }

	GLint previousFramebuffer = 0;
	GLint previousTexture = 0;
	GLint previousRenderbuffer = 0;
	GLfloat previousClearColor[4] = { 0.f, 0.f, 0.f, 0.f };
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
	glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);

	NaomiDestroyFrontendFramebuffer();

	glGenTextures(1, &NaomiFrontendColorTexture);
	glBindTexture(GL_TEXTURE_2D, NaomiFrontendColorTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	NaomiGlFramebufferFns.genRenderbuffers(1, &NaomiFrontendDepthStencilBuffer);
	NaomiGlFramebufferFns.bindRenderbuffer(GL_RENDERBUFFER, NaomiFrontendDepthStencilBuffer);
	NaomiGlFramebufferFns.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	NaomiGlFramebufferFns.genFramebuffers(1, &NaomiFrontendFramebuffer);
	NaomiGlFramebufferFns.bindFramebuffer(GL_FRAMEBUFFER, NaomiFrontendFramebuffer);
	NaomiGlFramebufferFns.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, NaomiFrontendColorTexture, 0);
	NaomiGlFramebufferFns.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, NaomiFrontendDepthStencilBuffer);

	const GLenum status = NaomiGlFramebufferFns.checkFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE) {
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
	}

	NaomiGlFramebufferFns.bindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFramebuffer);
	NaomiGlFramebufferFns.bindRenderbuffer(GL_RENDERBUFFER, (GLuint)previousRenderbuffer);
	glBindTexture(GL_TEXTURE_2D, (GLuint)previousTexture);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		NaomiDestroyFrontendFramebuffer();
		return false;
	}

	NaomiFrontendFramebufferWidth = width;
	NaomiFrontendFramebufferHeight = height;
	return true;
}

static bool NaomiIsStartupBlankFrame(const UINT32* pixels, size_t count)
{
	if (pixels == NULL || count == 0) {
		return true;
	}

	UINT32 minR = 255, minG = 255, minB = 255;
	UINT32 maxR = 0, maxG = 0, maxB = 0;
	UINT32 sumR = 0, sumG = 0, sumB = 0;
	const size_t sampleStep = std::max<size_t>(1, count / 4096);
	size_t samples = 0;

	for (size_t i = 0; i < count; i += sampleStep) {
		const UINT32 pixel = pixels[i];
		const UINT32 r = (pixel >> 16) & 0xff;
		const UINT32 g = (pixel >> 8) & 0xff;
		const UINT32 b = pixel & 0xff;

		minR = std::min(minR, r); maxR = std::max(maxR, r);
		minG = std::min(minG, g); maxG = std::max(maxG, g);
		minB = std::min(minB, b); maxB = std::max(maxB, b);
		sumR += r; sumG += g; sumB += b;
		samples++;
	}

	if (samples == 0) {
		return true;
	}

	const UINT32 range = std::max(maxR - minR, std::max(maxG - minG, maxB - minB));
	const UINT32 avgR = sumR / (UINT32)samples;
	const UINT32 avgG = sumG / (UINT32)samples;
	const UINT32 avgB = sumB / (UINT32)samples;
	const UINT32 chroma = std::max(avgR, std::max(avgG, avgB)) - std::min(avgR, std::min(avgG, avgB));

	return range <= 3 && chroma <= 3 && avgR < 128;
}

static INT32 NaomiLoadBurnRom(INT32 index, std::vector<UINT8>& data)
{
	struct BurnRomInfo ri;

	if (BurnDrvGetRomInfo(&ri, index)) {
		return 1;
	}

	data.resize(ri.nLen);
	if (data.empty()) {
		return 1;
	}

	if (BurnLoadRom(&data[0], index, 1)) {
		data.clear();
		return 1;
	}

	return 0;
}

static INT32 NaomiBuildContentZip()
{
	if (NaomiFileExists(NaomiTempZipPath) && !NaomiEnvEnabled("FBNEO_AWAVE_REBUILD_CONTENT_ZIP")) {
		bprintf(0, _T("AWAVE profile: reusing cached content zip %S\n"), NaomiTempZipPath);
		return 0;
	}

	FILE* fp = NULL;
	std::vector<NaomiZipRecord> records;

	auto write16 = [&](UINT16 value) -> bool {
		return fwrite(&value, sizeof(value), 1, fp) == 1;
	};
	auto write32 = [&](UINT32 value) -> bool {
		return fwrite(&value, sizeof(value), 1, fp) == 1;
	};
	auto fail = [&](const char* reason, const char* name) -> INT32 {
		if (fp) {
			fclose(fp);
			fp = NULL;
		}
		remove(NaomiTempZipWritePath);
		if (name) {
			bprintf(0, _T("naomi: %S for %S\n"), reason, name);
		} else {
			bprintf(0, _T("naomi: %S\n"), reason);
		}
		return 1;
	};

	remove(NaomiTempZipWritePath);

	fp = fopen(NaomiTempZipWritePath, "wb");
	if (fp == NULL) {
		bprintf(0, _T("naomi: failed to create %S\n"), NaomiTempZipWritePath);
		return 1;
	}

	for (INT32 i = 0; NaomiGame->zipEntries[i].filename != NULL; i++) {
		const char* filename = NaomiGame->zipEntries[i].filename;
		const size_t nameLen = strlen(filename);
		std::vector<UINT8> data;
		NaomiZipRecord record;
		long offset;

		if (nameLen > 0xffff) {
			return fail("zip entry name too long", filename);
		}

		if (NaomiLoadBurnRom(NaomiGame->zipEntries[i].burnRomIndex, data)) {
			return fail("failed to load Burn ROM", filename);
		}

		offset = ftell(fp);
		if (offset < 0) {
			return fail("ftell failed", filename);
		}

		record.filename = filename;
		record.crc = (UINT32)crc32(0L, data.data(), (uInt)data.size());
		record.size = (UINT32)data.size();
		record.offset = (UINT32)offset;

		if (!write32(0x04034b50) ||
			!write16(20) ||
			!write16(0) ||
			!write16(0) ||
			!write16(0) ||
			!write16(0) ||
			!write32(record.crc) ||
			!write32(record.size) ||
			!write32(record.size) ||
			!write16((UINT16)nameLen) ||
			!write16(0) ||
			fwrite(filename, 1, nameLen, fp) != nameLen ||
			fwrite(data.data(), 1, data.size(), fp) != data.size()) {
			return fail("failed to write zip local entry", filename);
		}

		records.push_back(record);
	}

	long centralDirOffset = ftell(fp);
	if (centralDirOffset < 0) {
		return fail("ftell failed", NULL);
	}

	for (size_t i = 0; i < records.size(); i++) {
		const NaomiZipRecord& record = records[i];
		const size_t nameLen = strlen(record.filename);

		if (!write32(0x02014b50) ||
			!write16(20) ||
			!write16(20) ||
			!write16(0) ||
			!write16(0) ||
			!write16(0) ||
			!write16(0) ||
			!write32(record.crc) ||
			!write32(record.size) ||
			!write32(record.size) ||
			!write16((UINT16)nameLen) ||
			!write16(0) ||
			!write16(0) ||
			!write16(0) ||
			!write16(0) ||
			!write32(0) ||
			!write32(record.offset) ||
			fwrite(record.filename, 1, nameLen, fp) != nameLen) {
			return fail("failed to write zip central directory", record.filename);
		}
	}

	long centralDirEnd = ftell(fp);
	if (centralDirEnd < 0) {
		return fail("ftell failed", NULL);
	}

	UINT32 centralDirSize = (UINT32)(centralDirEnd - centralDirOffset);
	UINT16 entryCount = (UINT16)records.size();

	if (!write32(0x06054b50) ||
		!write16(0) ||
		!write16(0) ||
		!write16(entryCount) ||
		!write16(entryCount) ||
		!write32(centralDirSize) ||
		!write32((UINT32)centralDirOffset) ||
		!write16(0)) {
		return fail("failed to finalize zip archive", NULL);
	}

	fclose(fp);
	fp = NULL;

	remove(NaomiTempZipPath);
	if (rename(NaomiTempZipWritePath, NaomiTempZipPath) != 0) {
		remove(NaomiTempZipWritePath);
		bprintf(0, _T("naomi: failed to publish %S\n"), NaomiTempZipPath);
		return 1;
	}

	return 0;
}


static bool NaomiEnvTruthy(const char* value)
{
	return value != NULL && value[0] != 0 && strcmp(value, "0") != 0 && strcmp(value, "disabled") != 0 && strcmp(value, "false") != 0;
}

static bool NaomiIsHeavyRendererGame()
{
	if (NaomiGame == NULL || NaomiGame->driverName == NULL) {
		return false;
	}

	// These Atomiswave/NAOMI titles most clearly expose the wrapper's slow
	// visual defaults. Use a Flycast-like render preset unless the user opts out.
	return !strcmp(NaomiGame->driverName, "samsptk") ||
		!strcmp(NaomiGame->driverName, "kov7sprt") ||
		!strcmp(NaomiGame->driverName, "ngbc");
}

static const char* NaomiGetRenderPresetName()
{
	const char* preset = getenv("FBNEO_AWAVE_RENDER_PRESET");
	if (preset != NULL && preset[0] != 0) {
		if (!strcmp(preset, "accurate") || !strcmp(preset, "safe") || !strcmp(preset, "0")) {
			return "accurate";
		}
		if (!strcmp(preset, "flycast") || !strcmp(preset, "fast") || !strcmp(preset, "default")) {
			return "flycast";
		}
		if (!strcmp(preset, "balanced") || !strcmp(preset, "compat") || !strcmp(preset, "fbneo")) {
			return "balanced";
		}
		if (!strcmp(preset, "speed") || !strcmp(preset, "fastest")) {
			return "speed";
		}
		return NaomiEnvTruthy(preset) ? "balanced" : "accurate";
	}

	const char* fastVisuals = getenv("FBNEO_AWAVE_FAST_VISUALS");
	if (fastVisuals != NULL && fastVisuals[0] != 0) {
		return NaomiEnvTruthy(fastVisuals) ? "balanced" : "accurate";
	}

	// Default for known heavy Atomiswave games: keep correctness-critical RTTB and
	// single-threaded readback, but relax alpha sorting from per-pixel to
	// per-triangle. This avoids the V6 Flycast preset's missing-layer/race issues.
	return NaomiIsHeavyRendererGame() ? "balanced" : "accurate";
}

static bool NaomiUseFlycastRenderPreset()
{
	return !strcmp(NaomiGetRenderPresetName(), "flycast");
}

static bool NaomiUseBalancedRenderPreset()
{
	return !strcmp(NaomiGetRenderPresetName(), "balanced");
}

static bool NaomiUseSpeedRenderPreset()
{
	const char* preset = NaomiGetRenderPresetName();
	return !strcmp(preset, "speed") || !strcmp(preset, "fastest");
}

static INT32 NaomiAudioLatencyFrames()
{
	const char* env = getenv("FBNEO_AWAVE_AUDIO_LATENCY_FRAMES");
	INT32 frames = NaomiUseSpeedRenderPreset() ? 1 : 2;
	if (env != NULL && env[0] != 0) {
		frames = atoi(env);
	}
	if (frames < 1) { frames = 1; }
	if (frames > 8) { frames = 8; }
	return frames;
}

static bool NaomiUseSpeedHacks()
{
	const char* env = getenv("FBNEO_AWAVE_SPEED_HACKS");
	if (env != NULL && env[0] != 0) {
		return NaomiEnvTruthy(env);
	}
	return NaomiUseSpeedRenderPreset();
}

static const char* NaomiGetAlphaSortingValue()
{
	const char* env = getenv("FBNEO_AWAVE_ALPHA_SORTING");
	if (env != NULL && env[0] != 0) {
		if (!strcmp(env, "strip") || !strcmp(env, "per-strip") || !strcmp(env, "fast")) {
			return "per-strip (fast, least accurate)";
		}
		if (!strcmp(env, "triangle") || !strcmp(env, "per-triangle") || !strcmp(env, "normal")) {
			return "per-triangle (normal)";
		}
		if (!strcmp(env, "pixel") || !strcmp(env, "per-pixel") || !strcmp(env, "accurate")) {
			return "per-pixel (accurate)";
		}
	}

	if (NaomiUseSpeedRenderPreset()) {
		return "per-strip (fast, least accurate)";
	}
	return (NaomiUseFlycastRenderPreset() || NaomiUseBalancedRenderPreset()) ? "per-triangle (normal)" : "per-pixel (accurate)";
}

static const char* NaomiGetRttbValue()
{
	const char* env = getenv("FBNEO_AWAVE_RTTB");
	if (env != NULL && env[0] != 0) {
		return NaomiEnvTruthy(env) ? "enabled" : "disabled";
	}
	return NaomiUseFlycastRenderPreset() ? "disabled" : "enabled";
}

static const char* NaomiGetThreadedRenderingValue()
{
	const char* env = getenv("FBNEO_AWAVE_THREADED_RENDERING");
	if (env != NULL && env[0] != 0) {
		return NaomiEnvTruthy(env) ? "enabled" : "disabled";
	}
	return NaomiUseFlycastRenderPreset() ? "enabled" : "disabled";
}

static const char* NaomiGetOptionValue(const char* key)
{
	if (key == NULL) {
		return NULL;
	}

	// Visual-safe baseline:
	// Keep the async PBO readback optimization, but stop using global low-end
	// visual shortcuts. The previous low-end test improved speed but can cause
	// missing backgrounds, black quads and texture/ordering artifacts on many
	// NAOMI/Atomiswave titles.
	//
	// Important choices:
	// - RTTB stays enabled for every game.
	// - alpha sorting goes back to per-pixel accurate.
	// - threaded rendering is disabled because this wrapper reads back the
	//   rendered framebuffer into FBNeo. With threaded rendering enabled, the
	//   readback can race the render thread and capture incomplete/intermediate
	//   frames, which looks like missing layers or black/garbled textures.
	// - video quality options such as mipmapping, pvr2_filtering,
	//   emulate_framebuffer, delay_frame_swapping, etc. are left to Flycast's
	//   own defaults by returning NULL.
	if (!strcmp(key, "reicast_system")) return (NaomiGame && NaomiGame->systemName) ? NaomiGame->systemName : "atomiswave";
	if (!strcmp(key, "flycast_system")) return (NaomiGame && NaomiGame->systemName) ? NaomiGame->systemName : "atomiswave";

	if (!strcmp(key, "reicast_internal_resolution")) return "640x480";
	if (!strcmp(key, "flycast_internal_resolution")) return "640x480";

	if (!strcmp(key, "reicast_boot_to_bios")) return "disabled";
	if (!strcmp(key, "flycast_boot_to_bios")) return "disabled";

	if (!strcmp(key, "reicast_hle_bios") || !strcmp(key, "flycast_hle_bios")) {
		const char* hle = getenv("FBNEO_AWAVE_HLE_BIOS");
		return (hle != NULL && (hle[0] == '0' || !strcmp(hle, "disabled"))) ? "disabled" : "enabled";
	}

	if (!strcmp(key, "reicast_gdrom_fast_loading")) return "disabled";
	if (!strcmp(key, "flycast_gdrom_fast_loading")) return "disabled";

	if (!strcmp(key, "reicast_cable_type")) return "TV (Composite)";
	if (!strcmp(key, "flycast_cable_type")) return "TV (Composite)";

	if (!strcmp(key, "reicast_broadcast")) return "Default";
	if (!strcmp(key, "flycast_broadcast")) return "Default";

	if (!strcmp(key, "reicast_framerate")) return "fullspeed";
	if (!strcmp(key, "flycast_framerate")) return "fullspeed";

	if (!strcmp(key, "reicast_region")) return "Default";
	if (!strcmp(key, "flycast_region")) return "Default";

	if (!strcmp(key, "reicast_language")) return "Default";
	if (!strcmp(key, "flycast_language")) return "Default";

	if (!strcmp(key, "reicast_div_matching") || !strcmp(key, "flycast_div_matching")) {
		return NaomiUseSpeedHacks() ? "disabled" : "auto";
	}

	if (!strcmp(key, "reicast_force_wince")) return "disabled";
	if (!strcmp(key, "flycast_force_wince")) return "disabled";

	if (!strcmp(key, "reicast_anisotropic_filtering")) return "disabled";
	if (!strcmp(key, "flycast_anisotropic_filtering")) return "disabled";

	if (!strcmp(key, "reicast_enable_purupuru")) return "disabled";
	if (!strcmp(key, "flycast_enable_purupuru")) return "disabled";

	if (!strcmp(key, "reicast_analog_stick_deadzone")) return "15";
	if (!strcmp(key, "flycast_analog_stick_deadzone")) return "15";

	if (!strcmp(key, "reicast_trigger_deadzone")) return "0";
	if (!strcmp(key, "flycast_trigger_deadzone")) return "0";

	if (!strcmp(key, "reicast_digital_triggers")) return "disabled";
	if (!strcmp(key, "flycast_digital_triggers")) return "disabled";

	// Keep dynarec explicitly requested, but do not touch visual quality paths.
	if (!strcmp(key, "reicast_cpu_mode")) return "dynamic_recompiler";
	if (!strcmp(key, "flycast_cpu_mode")) return "dynamic_recompiler";

	if (!strcmp(key, "reicast_enable_rttb")) return NaomiGetRttbValue();
	if (!strcmp(key, "flycast_enable_rttb")) return NaomiGetRttbValue();

	if (!strcmp(key, "reicast_render_to_texture_upscaling")) return "1x";
	if (!strcmp(key, "flycast_render_to_texture_upscaling")) return "1x";

	if (!strcmp(key, "reicast_alpha_sorting")) return NaomiGetAlphaSortingValue();
	if (!strcmp(key, "flycast_alpha_sorting")) return NaomiGetAlphaSortingValue();

	if (!strcmp(key, "reicast_threaded_rendering")) return NaomiGetThreadedRenderingValue();
	if (!strcmp(key, "flycast_threaded_rendering")) return NaomiGetThreadedRenderingValue();

	if (!strcmp(key, "reicast_synchronous_rendering")) return "disabled";
	if (!strcmp(key, "flycast_synchronous_rendering")) return "disabled";

	if (!strcmp(key, "reicast_enable_dsp") || !strcmp(key, "flycast_enable_dsp")) {
		return NaomiUseSpeedHacks() ? "disabled" : NULL;
	}
	if (!strcmp(key, "reicast_mipmapping") || !strcmp(key, "flycast_mipmapping")) {
		return NaomiUseSpeedHacks() ? "disabled" : NULL;
	}
	if (!strcmp(key, "reicast_fog") || !strcmp(key, "flycast_fog")) {
		return NaomiUseSpeedHacks() ? "disabled" : NULL;
	}
	if (!strcmp(key, "reicast_pvr2_filtering") || !strcmp(key, "flycast_pvr2_filtering")) {
		return NaomiUseSpeedHacks() ? "disabled" : NULL;
	}
	if (!strcmp(key, "reicast_delay_frame_swapping") || !strcmp(key, "flycast_delay_frame_swapping")) {
		return "disabled";
	}
	if (!strcmp(key, "reicast_frame_skipping") || !strcmp(key, "flycast_frame_skipping")) {
		return "disabled";
	}

	if (!strcmp(key, "reicast_allow_service_buttons")) return "enabled";
	if (!strcmp(key, "flycast_allow_service_buttons")) return "enabled";

	if (!strcmp(key, "reicast_screen_rotation")) return "horizontal";
	if (!strcmp(key, "flycast_screen_rotation")) return "horizontal";

	return NULL;
}

static void NaomiLogPrintf(enum retro_log_level level, const char* fmt, ...)
{
	if (level < RETRO_LOG_WARN) {
		return;
	}

	char buffer[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	buffer[sizeof(buffer) - 1] = 0;
	bprintf(0, _T("%S"), buffer);
}

static double NaomiNowMs()
{
#ifdef _WIN32
	static LARGE_INTEGER freq = [](){ LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
	static LARGE_INTEGER start = [](){ LARGE_INTEGER v; QueryPerformanceCounter(&v); return v; }();
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return ((double)(now.QuadPart - start.QuadPart) * 1000.0) / (double)freq.QuadPart;
#else
	using Clock = std::chrono::steady_clock;
	static const Clock::time_point start = Clock::now();
	return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
#endif
}

static double NaomiSafeProfileMs(double value)
{
	// MSVC prints non-finite doubles as 1.#IO / -1.#IO in zzBurnDebug.html.
	// Keep profiler output useful even if an early accumulator/divisor is invalid.
	if (value != value || value < -1000000.0 || value > 1000000.0) {
		return 0.0;
	}
	return value;
}


static void NaomiLogRuntimePacingOnce()
{
	static bool logged = false;
	if (logged) return;
	logged = true;
	bprintf(0, _T("AWAVE pacing: nBurnSoundRate=%d nBurnSoundLen=%d nBurnBpp=%d nBurnPitch=%d pBurnDraw=%p pBurnSoundOut=%p\n"),
		nBurnSoundRate, nBurnSoundLen, nBurnBpp, nBurnPitch, pBurnDraw, pBurnSoundOut);
}

static void NaomiLogBuildConfigOnce()
{
	static bool logged = false;
	if (logged) {
		return;
	}
	logged = true;

	bprintf(0, _T("AWAVE profile: build/runtime diagnostics enabled\n"));
#ifdef _WIN32
	bprintf(0, _T("AWAVE profile: timer=qpc\n"));
#else
	bprintf(0, _T("AWAVE profile: timer=steady_clock\n"));
#endif

#if defined(TARGET_WIN64)
	bprintf(0, _T("AWAVE profile: TARGET_WIN64=1\n"));
#elif defined(TARGET_WIN86)
	bprintf(0, _T("AWAVE profile: TARGET_WIN86=1\n"));
#else
	bprintf(0, _T("AWAVE profile WARNING: no TARGET_WINxx macro is defined\n"));
#endif

#if defined(TARGET_NO_THREADS)
	bprintf(0, _T("AWAVE profile WARNING: TARGET_NO_THREADS=1, threaded rendering option cannot help\n"));
#else
	bprintf(0, _T("AWAVE profile: TARGET_NO_THREADS=0\n"));
#endif

#if defined(TARGET_NO_REC)
	bprintf(0, _T("AWAVE profile WARNING: TARGET_NO_REC=1, dynarec is disabled\n"));
#else
	bprintf(0, _T("AWAVE profile: TARGET_NO_REC=0\n"));
#endif

#if defined(HAVE_OPENGL)
	bprintf(0, _T("AWAVE profile: HAVE_OPENGL=1\n"));
#else
	bprintf(0, _T("AWAVE profile WARNING: HAVE_OPENGL is not defined\n"));
#endif

#if defined(HAVE_GL3)
	bprintf(0, _T("AWAVE profile: HAVE_GL3=1\n"));
#endif

#if defined(HAVE_GL4)
	bprintf(0, _T("AWAVE profile: HAVE_GL4=1\n"));
#endif

#if defined(HAVE_OIT)
	bprintf(0, _T("AWAVE profile: HAVE_OIT=1\n"));
#endif
}

static void NaomiLogGlInfoOnce(const char* stage)
{
	static bool logged = false;
	if (logged) {
		return;
	}
	logged = true;

	const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

	bprintf(0, _T("AWAVE profile: GL stage=%S vendor=%S renderer=%S version=%S\n"),
		stage ? stage : "unknown",
		vendor ? vendor : "null",
		renderer ? renderer : "null",
		version ? version : "null");
}


static bool NaomiShouldLogOptionOnce(const char* key)
{
	if (key == NULL) {
		return false;
	}

	static std::vector<std::string> loggedKeys;
	for (size_t i = 0; i < loggedKeys.size(); i++) {
		if (loggedKeys[i] == key) {
			return false;
		}
	}

	loggedKeys.push_back(key);
	return true;
}

static bool NaomiClearThreadWaitsCallback(unsigned, void*)
{
	return true;
}

static bool NaomiCanUseHwRender()
{
	return OGLGetContext() != NULL;
}

static retro_proc_address_t NaomiGetProcAddress(const char* name)
{
	if (name == NULL || !NaomiCanUseHwRender()) {
		return NULL;
	}

	return reinterpret_cast<retro_proc_address_t>(OGLGetProcAddress(name));
}

static uintptr_t NaomiGetCurrentFramebuffer()
{
	return NaomiUseFrontendFramebuffer ? NaomiFrontendFramebuffer : 0;
}

static void NaomiDeactivateHwRender()
{
	if (NaomiHwRenderReady && NaomiHwRender.context_destroy) {
		OGLMakeCurrentContext();
		NaomiHwRender.context_destroy();
		OGLDoneCurrentContext();
	}

	memset(&NaomiHwRender, 0, sizeof(NaomiHwRender));
	NaomiHwRenderRequested = false;
	NaomiHwRenderReady = false;
}

static INT32 NaomiLoadRetroGame()
{
	struct retro_game_info gameInfo;
	struct retro_system_av_info avInfo;
	const bool useHwContext = NaomiHwRenderRequested || NaomiHwRenderReady;

	memset(&gameInfo, 0, sizeof(gameInfo));
	gameInfo.path = NaomiTempZipPath;

	flycast_retro_audio_flush_buffer();

	if (useHwContext) {
		OGLMakeCurrentContext();
	}
	if (!flycast_retro_load_game(&gameInfo)) {
		if (useHwContext) {
			OGLDoneCurrentContext();
		}
		bprintf(0, _T("naomi: flycast_retro_load_game failed\n"));
		NaomiRetroGameLoaded = false;
		return 1;
	}
	if (useHwContext) {
		OGLDoneCurrentContext();
	}
	NaomiRetroGameLoaded = true;
	// The direct ROM archive is only needed while flycast_retro_load_game()
	// reads the NAOMI/Atomiswave blobs. Clear it immediately so large games
	// such as samsptk do not keep a second full ROM copy in memory.
	NaomiDirectArchiveClear();

	if (NaomiHwRenderRequested && !NaomiActivateHwRender()) {
		bprintf(0, _T("naomi: failed to activate hw render context\n"));
		return 1;
	}

	memset(&avInfo, 0, sizeof(avInfo));
	flycast_retro_get_system_av_info(&avInfo);
	if (avInfo.timing.sample_rate > 1000.0) {
		NaomiAudioSampleRate = (INT32)(avInfo.timing.sample_rate + 0.5);
	} else {
		NaomiAudioSampleRate = 44100;
	}
	NaomiAudioSourcePos = 0.0;
	if (avInfo.geometry.base_width > 0) {
		NaomiVideoWidth = (INT32)avInfo.geometry.base_width;
	}
	if (avInfo.geometry.base_height > 0) {
		NaomiVideoHeight = (INT32)avInfo.geometry.base_height;
	}
	NaomiState.resize(flycast_retro_serialize_size());
	NaomiAudioWarmupFrames = 2;
	NaomiVideoWarmupReads = 2;
	NaomiVideoStartupSkipReads = 12;
	NaomiVideoProbeReads = 60;
	return 0;
}

static void NaomiShutdownRetroCore()
{
	const bool useHwContext = NaomiHwRenderRequested || NaomiHwRenderReady;

	if (NaomiRetroGameLoaded) {
		flycast_retro_audio_flush_buffer();
		if (useHwContext) {
			OGLMakeCurrentContext();
		}
		flycast_retro_unload_game();
		if (useHwContext) {
			OGLDoneCurrentContext();
		}
		NaomiRetroGameLoaded = false;
	}

	NaomiDeactivateHwRender();
	NaomiDestroyFrontendFramebuffer();

	if (NaomiRetroInitialized) {
		flycast_retro_audio_deinit();
		flycast_retro_deinit();
		NaomiRetroInitialized = false;
	}
}

static bool NaomiActivateHwRender()
{
	if (!NaomiHwRenderRequested || NaomiHwRenderReady || NaomiHwRender.context_reset == NULL) {
		return NaomiHwRenderReady;
	}

	OGLMakeCurrentContext();
	if (!NaomiEnsureFrontendFramebuffer(std::max<INT32>(1, NaomiVideoWidth), std::max<INT32>(1, NaomiVideoHeight))) {
		OGLDoneCurrentContext();
		return false;
	}
	if (NaomiUseFrontendFramebuffer && NaomiFrontendFramebuffer != 0 && NaomiGlFramebufferFns.bindFramebuffer != NULL) {
		GLint previousFramebuffer = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
		NaomiGlFramebufferFns.bindFramebuffer(GL_FRAMEBUFFER, NaomiFrontendFramebuffer);
		NaomiHwRender.context_reset();
		NaomiGlFramebufferFns.bindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFramebuffer);
	} else {
		NaomiHwRender.context_reset();
	}
	OGLDoneCurrentContext();
	NaomiHwRenderReady = true;
	return true;
}

static void NaomiTrimAudioQueueLocked(size_t maxFrames)
{
	if (maxFrames == 0) {
		NaomiAudio.clear();
		NaomiAudioReadOffset = 0;
		return;
	}

	if (NaomiAudioReadOffset > NaomiAudio.size()) {
		NaomiAudioReadOffset = NaomiAudio.size();
	}

	const size_t bufferedSamples = NaomiAudio.size() - NaomiAudioReadOffset;
	const size_t bufferedFrames = bufferedSamples / 2;
	if (bufferedFrames <= maxFrames) {
		return;
	}

	const size_t dropFrames = bufferedFrames - maxFrames;
	const size_t dropSamples = dropFrames * 2;
	NaomiAudioReadOffset += dropSamples;

	if (NaomiAudioReadOffset >= NaomiAudio.size()) {
		NaomiAudio.clear();
		NaomiAudioReadOffset = 0;
		return;
	}

	const size_t remainingSamples = NaomiAudio.size() - NaomiAudioReadOffset;
	memmove(NaomiAudio.data(), NaomiAudio.data() + NaomiAudioReadOffset, remainingSamples * sizeof(INT16));
	NaomiAudio.resize(remainingSamples);
	NaomiAudioReadOffset = 0;
}

static bool NaomiPublishHardwareTextureFrame(unsigned width, unsigned height)
{
	if (!NaomiHwRenderReady || width == 0 || height == 0) {
		NaomiHwDirectFrameValid = false;
		return false;
	}

	if (!NaomiEnsureFrontendFramebuffer((INT32)width, (INT32)height)) {
		NaomiHwDirectFrameValid = false;
		return false;
	}

	NaomiVideoWidth = (INT32)width;
	NaomiVideoHeight = (INT32)height;
	NaomiVideoStride = (INT32)width;
	NaomiHwDirectFrameWidth = (INT32)width;
	NaomiHwDirectFrameHeight = (INT32)height;

	if (NaomiWantsDirectTexturePresent()) {
		if (OGLUsingFallbackContext()) {
			if (!NaomiHwDirectWarnedNoFrontend) {
				NaomiHwDirectWarnedNoFrontend = true;
				bprintf(0, _T("AWAVE profile WARNING: direct_texture requested but Atomiswave is using fallback hidden GL context; frontend cannot see this texture. Use CPU readback or wire FBNeo-QT GL context through OGLSetContext.\n"));
			}
			NaomiHwDirectFrameValid = false;
			return false;
		}

		NaomiHwDirectFrameValid = (NaomiFrontendColorTexture != 0);
		if (NaomiHwDirectFrameValid) {
			NaomiAcceptedVideoFrames++;
		}
		return NaomiHwDirectFrameValid;
	}

	if (NaomiWantsSkipReadback()) {
		// Diagnostic mode: run Flycast without paying the GPU->CPU readback cost.
		// The normal FBNeo framebuffer will be stale/blank, but audio/core speed can
		// be measured to prove whether readback/display is the bottleneck.
		NaomiHwDirectFrameValid = false;
		NaomiAcceptedVideoFrames++;
		return true;
	}

	return false;
}

static bool NaomiReadHardwareVideo(unsigned width, unsigned height)
{
	if (!NaomiHwRenderReady || width == 0 || height == 0) {
		return false;
	}

	if (!NaomiEnsureFrontendFramebuffer((INT32)width, (INT32)height)) {
		return false;
	}

	NaomiVideoWidth = (INT32)width;
	NaomiVideoHeight = (INT32)height;
	NaomiVideoStride = (INT32)width;
	const size_t framePixels = (size_t)width * (size_t)height;
	const size_t frameBytes = framePixels * sizeof(UINT32);
	if (NaomiHardwareVideoScratch.size() != framePixels) {
		NaomiHardwareVideoScratch.resize(framePixels);
	}

	GLint previousFramebuffer = 0;
	GLint previousReadBuffer = GL_BACK;
	GLint previousPackBuffer = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
	glGetIntegerv(GL_READ_BUFFER, &previousReadBuffer);
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);

	if (NaomiUseFrontendFramebuffer && NaomiFrontendFramebuffer != 0 && NaomiGlFramebufferFns.bindFramebuffer != NULL) {
		NaomiGlFramebufferFns.bindFramebuffer(GL_FRAMEBUFFER, NaomiFrontendFramebuffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
	} else {
		if (NaomiGlFramebufferFns.bindFramebuffer == NULL && !NaomiLoadFramebufferFns()) {
			return false;
		}
		NaomiGlFramebufferFns.bindFramebuffer(GL_FRAMEBUFFER, 0);
		glReadBuffer(GL_BACK);
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	while (glGetError() != GL_NO_ERROR) {
	}

	bool gotFrame = false;
	GLenum readError = GL_NO_ERROR;
	const bool usePboReadback = NaomiEnsureReadbackPbos(width, height);

	const double readPixelsStartMs = NaomiNowMs();
	if (usePboReadback) {
		const INT32 issueIndex = NaomiReadbackPboIndex;
		const INT32 mapIndex = (issueIndex + 1) % NaomiReadbackPboCount; // two frames older with a 3-buffer ring

		// Orphan the PBO before issuing a new read. This is the key part that
		// avoids waiting for the GPU to finish using the same buffer.
		NaomiGlPboFns.bindBuffer(GL_PIXEL_PACK_BUFFER, NaomiReadbackPbos[issueIndex]);
		NaomiGlPboFns.bufferData(GL_PIXEL_PACK_BUFFER, (std::ptrdiff_t)frameBytes, NULL, GL_STREAM_READ);
		glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
		readError = glGetError();
		NaomiReadbackPboValid[issueIndex] = (readError == GL_NO_ERROR);

		// Read back an older PBO instead of the one we just filled.
		// This intentionally trades two frames of latency for much lower stalls.
		NaomiGlPboFns.bindBuffer(GL_PIXEL_PACK_BUFFER, NaomiReadbackPbos[mapIndex]);
		if (NaomiReadbackPboValid[mapIndex]) {
			void* mapped = NaomiGlPboFns.mapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
			if (mapped != NULL) {
				memcpy(NaomiHardwareVideoScratch.data(), mapped, frameBytes);
				gotFrame = (NaomiGlPboFns.unmapBuffer(GL_PIXEL_PACK_BUFFER) == GL_TRUE);
			}
		}

		NaomiGlPboFns.bindBuffer(GL_PIXEL_PACK_BUFFER, (GLuint)previousPackBuffer);
		NaomiReadbackPboIndex = (issueIndex + 1) % NaomiReadbackPboCount;
	} else {
		glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_BGRA, GL_UNSIGNED_BYTE, NaomiHardwareVideoScratch.data());
		readError = glGetError();
		gotFrame = (readError == GL_NO_ERROR);
	}
	const double readPixelsEndMs = NaomiNowMs();

	if (NaomiPerfLoggingEnabled()) {
		NaomiPerfReadPixelsAccumMs += (readPixelsEndMs - readPixelsStartMs);
		NaomiPerfReadPixelsSamples++;
	}

	if (NaomiGlFramebufferFns.bindFramebuffer != NULL) {
		NaomiGlFramebufferFns.bindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFramebuffer);
	} else {
		return false;
	}
	glReadBuffer(previousReadBuffer);
	if (NaomiGlPboFns.bindBuffer != NULL) {
		NaomiGlPboFns.bindBuffer(GL_PIXEL_PACK_BUFFER, (GLuint)previousPackBuffer);
	}

	if (readError != GL_NO_ERROR || !gotFrame) {
		return false;
	}

	if (NaomiVideoWarmupReads > 0) {
		NaomiVideoWarmupReads--;
		return false;
	}

	if (NaomiAcceptedVideoFrames == 0 && NaomiVideoStartupSkipReads > 0) {
		NaomiVideoStartupSkipReads--;
		return false;
	}

	if (NaomiAcceptedVideoFrames == 0 && NaomiVideoProbeReads > 0) {
		NaomiVideoProbeReads--;
		if (NaomiIsStartupBlankFrame(NaomiHardwareVideoScratch.data(), framePixels)) {
			return false;
		}
	}

	if (NaomiVideo.size() != framePixels) {
		NaomiVideo.resize(framePixels);
	}

	const double flipStartMs = NaomiNowMs();
	for (unsigned y = 0; y < height; y++) {
		const UINT32* srcRow = NaomiHardwareVideoScratch.data() + ((size_t)(height - 1 - y) * (size_t)width);
		UINT32* dstRow = NaomiVideo.data() + ((size_t)y * (size_t)width);
		memcpy(dstRow, srcRow, (size_t)width * sizeof(UINT32));
	}
	if (NaomiPerfLoggingEnabled()) {
		NaomiPerfReadFlipAccumMs += (NaomiNowMs() - flipStartMs);
	}

	NaomiAcceptedVideoFrames++;
	return true;
}



static bool NaomiEnvironmentCallback(unsigned cmd, void* data)
{
	switch (cmd) {
		case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
			struct retro_log_callback* cb = (struct retro_log_callback*)data;
			if (cb == NULL) return false;
			cb->log = NaomiLogPrintf;
			return true;
		}

		case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
		case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
		case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
		case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
		case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
		case RETRO_ENVIRONMENT_SET_ROTATION:
		case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:
		case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE:
		case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE:
		case RETRO_ENVIRONMENT_SET_SAVE_STATE_IN_BACKGROUND:
		case (4 | 0x800000): // RETRO_ENVIRONMENT_POLL_TYPE_OVERRIDE
			return true;

		case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
			*(const char**)data = NaomiTempRoot;
			return true;

		case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
			*(const char**)data = NaomiTempSaveDir;
			return true;

		case RETRO_ENVIRONMENT_GET_VARIABLE: {
			struct retro_variable* var = (struct retro_variable*)data;
			if (var == NULL) return false;
			var->value = NaomiGetOptionValue(var->key);
			if (NaomiShouldLogOptionOnce(var->key)) {
				bprintf(0, _T("AWAVE option: %S = %S\n"),
					var->key ? var->key : "null",
					var->value ? var->value : "<frontend/default>");
			}
			return (var->value != NULL);
		}

		case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
			*(bool*)data = false;
			return true;

		case RETRO_ENVIRONMENT_SET_HW_RENDER: {
			struct retro_hw_render_callback* cb = (struct retro_hw_render_callback*)data;
			if (cb == NULL) {
				return false;
			}

			if (!OGLCreateFallbackContext()) {
				return false;
			}
			OGLSetUseFallbackContext(true);

			NaomiHwRender = *cb;
			NaomiHwRender.get_proc_address = NaomiGetProcAddress;
			NaomiHwRender.get_current_framebuffer = NaomiGetCurrentFramebuffer;
			NaomiHwRenderRequested = true;
			NaomiHwRenderReady = false;
			if (NaomiUseFrontendFramebuffer) {
				OGLMakeCurrentContext();
				NaomiEnsureFrontendFramebuffer(std::max<INT32>(1, NaomiVideoWidth), std::max<INT32>(1, NaomiVideoHeight));
				OGLDoneCurrentContext();
			}
			*cb = NaomiHwRender;
			if (!NaomiActivateHwRender()) {
				memset(&NaomiHwRender, 0, sizeof(NaomiHwRender));
				NaomiHwRenderRequested = false;
				NaomiHwRenderReady = false;
				return false;
			}
			return true;
		}

		case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
			*(unsigned*)data = NaomiCanUseHwRender() ? RETRO_HW_CONTEXT_OPENGL : RETRO_HW_CONTEXT_DUMMY;
			return true;

		case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
			*(bool*)data = false;
			return true;

		case (3 | 0x800000): { // RETRO_ENVIRONMENT_GET_CLEAR_ALL_THREAD_WAITS_CB
			retro_environment_t* cb = (retro_environment_t*)data;
			if (cb == NULL) return false;
			*cb = NaomiClearThreadWaitsCallback;
			return true;
		}

		case RETRO_ENVIRONMENT_SET_MESSAGE: {
			const struct retro_message* msg = (const struct retro_message*)data;
			if (msg && msg->msg) {
				bprintf(0, _T("naomi: %S\n"), msg->msg);
			}
			return true;
		}

		default:
			return false;
	}
}

static void NaomiCopyVideoFrame(const void* data, unsigned width, unsigned height, size_t pitch)
{
	if (data == RETRO_HW_FRAME_BUFFER_VALID) {
		NaomiPendingVideoWidth = width;
		NaomiPendingVideoHeight = height;
		NaomiVideoReadPending = true;
		return;
	}

	if (NaomiHwRenderRequested || NaomiHwRenderReady) {
		return;
	}

	if (data == NULL || width == 0 || height == 0) {
		return;
	}

	const INT32 stride = (pitch > 0) ? (INT32)(pitch / sizeof(UINT32)) : (INT32)width;
	if (stride < (INT32)width) {
		return;
	}

	NaomiVideoWidth = (INT32)width;
	NaomiVideoHeight = (INT32)height;
	NaomiVideoStride = stride;
	const size_t framePixels = (size_t)stride * (size_t)height;
	if (NaomiVideo.size() != framePixels) {
		NaomiVideo.resize(framePixels);
	}
	memcpy(NaomiVideo.data(), data, framePixels * sizeof(UINT32));

	NaomiVideoFromCallback = true;
}

static void NaomiVideoCallback(const void* data, unsigned width, unsigned height, size_t pitch)
{
	NaomiCopyVideoFrame(data, width, height, pitch);
}

static void NaomiAudioSampleCallback(int16_t, int16_t)
{
}

static size_t NaomiAudioBatchCallback(const int16_t* data, size_t frames)
{
	if (data == NULL || frames == 0) {
		return 0;
	}

	std::lock_guard<std::mutex> lock(NaomiAudioMutex);
	if (NaomiAudioReadOffset > 0) {
		const size_t remainingSamples = (NaomiAudioReadOffset < NaomiAudio.size()) ? (NaomiAudio.size() - NaomiAudioReadOffset) : 0;
		if (remainingSamples == 0) {
			NaomiAudio.clear();
			NaomiAudioReadOffset = 0;
			NaomiAudioSourcePos = 0.0;
		} else if (NaomiAudioReadOffset >= 8192 || NaomiAudioReadOffset >= remainingSamples) {
			memmove(NaomiAudio.data(), NaomiAudio.data() + NaomiAudioReadOffset, remainingSamples * sizeof(INT16));
			NaomiAudio.resize(remainingSamples);
			NaomiAudioReadOffset = 0;
		}
	}

	const size_t oldSize = NaomiAudio.size();
	NaomiAudio.resize(oldSize + frames * 2);
	memcpy(&NaomiAudio[oldSize], data, frames * 2 * sizeof(int16_t));

	const INT32 sinkRate = (nBurnSoundRate > 1000) ? nBurnSoundRate : 44100;
	const double sourcePerSink = (NaomiAudioSampleRate > 1000) ? ((double)NaomiAudioSampleRate / (double)sinkRate) : 1.0;
	const INT32 latencyFrames = NaomiAudioLatencyFrames();
	const size_t targetFrames = (nBurnSoundLen > 0)
		? (size_t)((double)nBurnSoundLen * sourcePerSink * (double)latencyFrames)
		: 2048;
	NaomiTrimAudioQueueLocked(std::max(targetFrames, frames + (size_t)64));
	return frames;
}

static void NaomiInputPollCallback(void)
{
}

static int16_t NaomiInputStateCallback(unsigned port, unsigned device, unsigned index, unsigned id)
{
	if (port >= 4) {
		return 0;
	}

	if (device == RETRO_DEVICE_JOYPAD && NaomiJoypadMap != NULL && id < NaomiJoypadMapCount) {
		const UINT32 mask = NaomiJoypadMap[id];
		return (mask != 0 && (NaomiPads[port] & mask) != 0) ? 1 : 0;
	}

	if (device == RETRO_DEVICE_LIGHTGUN && NaomiLightgunMap != NULL) {
		if (id < NaomiLightgunMapCount) {
			const UINT32 mask = NaomiLightgunMap[id];
			return (mask != 0 && (NaomiPads[port] & mask) != 0) ? 1 : 0;
		}

		switch (id) {
			case RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X:
				return NaomiLightgunAxis[port][0];
			case RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y:
				return NaomiLightgunAxis[port][1];
			case RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN:
				return NaomiLightgunOffscreen[port] ? 1 : 0;
			case RETRO_DEVICE_ID_LIGHTGUN_RELOAD:
				return NaomiLightgunReload[port] ? 1 : 0;
			default:
				break;
		}
	}

	if (device == RETRO_DEVICE_POINTER) {
		switch (id) {
			case RETRO_DEVICE_ID_POINTER_X:
				return NaomiLightgunAxis[port][0];
			case RETRO_DEVICE_ID_POINTER_Y:
				return NaomiLightgunAxis[port][1];
			case RETRO_DEVICE_ID_POINTER_PRESSED:
				return (NaomiPads[port] & (AWAVE_INPUT_TRIGGER | AWAVE_INPUT_BTN1)) ? 1 : 0;
			case RETRO_DEVICE_ID_POINTER_COUNT:
				return 1;
			default:
				break;
		}
	}

	if (device == RETRO_DEVICE_ANALOG) {
		if (id == RETRO_DEVICE_ID_ANALOG_X || id == RETRO_DEVICE_ID_ANALOG_Y) {
			const unsigned axisBase =
				(index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) ? 2U :
				(index == RETRO_DEVICE_INDEX_ANALOG_LEFT)  ? 0U :
				4U;
			if (axisBase < 4U) {
				return NaomiAnalog[port][axisBase + id];
			}
		}

		if (index == RETRO_DEVICE_INDEX_ANALOG_BUTTON) {
			switch (id) {
				case RETRO_DEVICE_ID_JOYPAD_L2:
					return NaomiAnalogButtons[port][0];
				case RETRO_DEVICE_ID_JOYPAD_R2:
					return NaomiAnalogButtons[port][1];
				default:
					break;
			}
		}
	}

	return 0;
}

static void NaomiCaptureVideo()
{
	const bool useHwRender = NaomiHwRenderRequested || NaomiHwRenderReady;

	if (!useHwRender && NaomiVideoFromCallback) {
		return;
	}

	if (NaomiVideoReadPending) {
		const unsigned width = (NaomiPendingVideoWidth > 0) ? NaomiPendingVideoWidth : (unsigned)std::max<INT32>(1, screen_width);
		const unsigned height = (NaomiPendingVideoHeight > 0) ? NaomiPendingVideoHeight : (unsigned)std::max<INT32>(1, screen_height);

		if (NaomiWantsDirectTexturePresent()) {
			if (NaomiPublishHardwareTextureFrame(width, height)) {
				NaomiVideoFromCallback = true;
			} else if (NaomiReadHardwareVideo(width, height)) {
				// Safety fallback when the frontend has not yet supplied/shared a GL context.
				NaomiVideoFromCallback = true;
			}
		} else if (NaomiWantsSkipReadback()) {
			if (NaomiPublishHardwareTextureFrame(width, height)) {
				NaomiVideoFromCallback = true;
			}
		} else if (NaomiReadHardwareVideo(width, height)) {
			NaomiVideoFromCallback = true;
		}
		NaomiVideoReadPending = false;
		return;
	}
}

static void NaomiDrainAudio()
{
	if (pBurnSoundOut == NULL || nBurnSoundLen <= 0) {
		std::lock_guard<std::mutex> lock(NaomiAudioMutex);
		NaomiAudio.clear();
		NaomiAudioReadOffset = 0;
		NaomiAudioSourcePos = 0.0;
		return;
	}

	const INT32 sinkRate = (nBurnSoundRate > 1000) ? nBurnSoundRate : 44100;
	const double step = (NaomiAudioSampleRate > 1000) ? ((double)NaomiAudioSampleRate / (double)sinkRate) : 1.0;
	bool underrun = false;

	std::lock_guard<std::mutex> lock(NaomiAudioMutex);
	if (NaomiAudioReadOffset > NaomiAudio.size()) {
		NaomiAudioReadOffset = NaomiAudio.size();
	}

	// Keep the bridge as a low-latency frame synchronizer, not a growing FIFO.
	// Logs showed samsptk sitting around ~2205 queued frames. That makes
	// input/audio feel late and can hide pacing problems.
	if (nBurnSoundLen > 0 && NaomiAudioReadOffset < NaomiAudio.size()) {
		const size_t queuedBeforeDrain = (NaomiAudio.size() - NaomiAudioReadOffset) / 2;
		const size_t wantedBeforeDrain = (size_t)nBurnSoundLen * (size_t)NaomiAudioLatencyFrames();
		if (queuedBeforeDrain > wantedBeforeDrain + (size_t)nBurnSoundLen) {
			const size_t dropFrames = queuedBeforeDrain - wantedBeforeDrain;
			NaomiAudioReadOffset += dropFrames * 2;
			NaomiAudioSourcePos = 0.0;
		}
	}

	const size_t availableSamples = NaomiAudio.size() - NaomiAudioReadOffset;
	const size_t queuedFrames = availableSamples / 2;
	const INT16* src = queuedFrames > 0 ? (NaomiAudio.data() + NaomiAudioReadOffset) : NULL;

	for (INT32 i = 0; i < nBurnSoundLen; i++) {
		const size_t frame0 = (size_t)NaomiAudioSourcePos;
		const double frac = NaomiAudioSourcePos - (double)frame0;
		INT32 left = NaomiAudioLastSample[0];
		INT32 right = NaomiAudioLastSample[1];
		if (src != NULL && frame0 + 1 < queuedFrames) {
			const INT16 l0 = src[frame0 * 2 + 0];
			const INT16 r0 = src[frame0 * 2 + 1];
			const INT16 l1 = src[(frame0 + 1) * 2 + 0];
			const INT16 r1 = src[(frame0 + 1) * 2 + 1];
			left = (INT32)((double)l0 + ((double)l1 - (double)l0) * frac);
			right = (INT32)((double)r0 + ((double)r1 - (double)r0) * frac);
		} else if (src != NULL && frame0 < queuedFrames) {
			left = src[frame0 * 2 + 0];
			right = src[frame0 * 2 + 1];
			underrun = true;
		} else {
			underrun = true;
		}
		pBurnSoundOut[i * 2 + 0] = (INT16)left;
		pBurnSoundOut[i * 2 + 1] = (INT16)right;
		NaomiAudioLastSample[0] = (INT16)left;
		NaomiAudioLastSample[1] = (INT16)right;
		NaomiAudioSourcePos += step;
	}
	if (underrun) {
		size_t consumedFrames = (size_t)NaomiAudioSourcePos;
		if (consumedFrames > queuedFrames) {
			consumedFrames = queuedFrames;
		}
		NaomiAudioSourcePos -= (double)consumedFrames;
		NaomiAudioReadOffset += consumedFrames * 2;
		if (NaomiAudioReadOffset >= NaomiAudio.size()) {
			NaomiAudio.clear();
			NaomiAudioReadOffset = 0;
			NaomiAudioSourcePos = 0.0;
		}
		return;
	}
	size_t consumedFrames = (size_t)NaomiAudioSourcePos;
	if (consumedFrames > queuedFrames) { consumedFrames = queuedFrames; }
	NaomiAudioSourcePos -= (double)consumedFrames;
	NaomiAudioReadOffset += consumedFrames * 2;
	if (NaomiAudioReadOffset >= NaomiAudio.size()) {
		NaomiAudio.clear();
		NaomiAudioReadOffset = 0;
		return;
	}
	const size_t remainingSamples = NaomiAudio.size() - NaomiAudioReadOffset;
	if (NaomiAudioReadOffset >= 8192 || NaomiAudioReadOffset >= remainingSamples) {
		memmove(NaomiAudio.data(), NaomiAudio.data() + NaomiAudioReadOffset, remainingSamples * sizeof(INT16));
		NaomiAudio.resize(remainingSamples);
		NaomiAudioReadOffset = 0;
	}
	const size_t maxQueuedSourceFrames = (size_t)((double)nBurnSoundLen * step * (double)NaomiAudioLatencyFrames()) + 64;
	NaomiTrimAudioQueueLocked(std::max(maxQueuedSourceFrames, (size_t)nBurnSoundLen + 64));
}

INT32 NaomiCoreInit(const NaomiGameConfig* config)
{
	NaomiCoreExit();

	if (config == NULL) {
		return 1;
	}

	NaomiGame = config;
	NaomiJoypadMap = config->joypadMap;
	NaomiJoypadMapCount = config->joypadMapCount;
	NaomiLightgunMap = config->lightgunMap;
	NaomiLightgunMapCount = config->lightgunMapCount;
	NaomiResetFrameState();
	bprintf(0, _T("AWAVE profile: visual mode=%S; render_preset=%S, RTTB=%S, alpha=%S, threaded_rendering=%S, direct_rom=%S, speed_hacks=%S, audio_latency_frames=%d\n"),
		NaomiVideoPresentModeName(NaomiGetVideoPresentMode()),
		NaomiGetRenderPresetName(),
		NaomiGetRttbValue(),
		NaomiGetAlphaSortingValue(),
		NaomiGetThreadedRenderingValue(),
		NaomiUseDirectArchive() ? "enabled" : "disabled",
		NaomiUseSpeedHacks() ? "enabled" : "disabled",
		NaomiAudioLatencyFrames());

	const bool externalContextAvailable = (OGLGetContext() != NULL);
	if (NaomiWantsDirectTexturePresent() && externalContextAvailable) {
		OGLSetUseFallbackContext(false);
		bprintf(0, _T("AWAVE profile: using frontend/shared OpenGL context for direct texture present\n"));
	} else {
		if (NaomiWantsDirectTexturePresent() && !externalContextAvailable) {
			bprintf(0, _T("AWAVE profile WARNING: direct_texture requested but no frontend GL context is registered; creating fallback context and falling back to CPU visibility unless frontend patch is applied\n"));
		}
		if (!OGLCreateFallbackContext()) {
			bprintf(0, _T("naomi: failed to create fallback GL context\n"));
			return 1;
		}
		OGLSetUseFallbackContext(true);
	}
	NaomiLogBuildConfigOnce();
	NaomiLogRuntimePacingOnce();
	OGLMakeCurrentContext();
	NaomiLogGlInfoOnce("fallback-init");
	OGLDoneCurrentContext();

	NaomiBuildTempPaths();
	if (NaomiUseDirectArchive()) {
		if (NaomiBuildDirectContentArchive()) {
			bprintf(0, _T("naomi: direct content archive build failed\n"));
			return 1;
		}
	} else {
		if (NaomiBuildContentZip()) {
			bprintf(0, _T("naomi: content zip build failed\n"));
			return 1;
		}
	}

	flycast_retro_set_environment(NaomiEnvironmentCallback);
	flycast_retro_set_video_refresh(NaomiVideoCallback);
	flycast_retro_set_audio_sample(NaomiAudioSampleCallback);
	flycast_retro_set_audio_sample_batch(NaomiAudioBatchCallback);
	flycast_retro_set_input_poll(NaomiInputPollCallback);
	flycast_retro_set_input_state(NaomiInputStateCallback);
	flycast_retro_init();
	NaomiRetroInitialized = true;
	flycast_retro_audio_init();
	flycast_retro_audio_flush_buffer();

	const unsigned primaryDevice = (NaomiLightgunMap != NULL && NaomiLightgunMapCount > 0) ? RETRO_DEVICE_LIGHTGUN : RETRO_DEVICE_JOYPAD;
	for (unsigned i = 0; i < 4; i++) {
		flycast_retro_set_controller_port_device(i, (i < 2) ? primaryDevice : RETRO_DEVICE_JOYPAD);
	}

	if (NaomiLoadRetroGame()) {
		NaomiCoreExit();
		return 1;
	}
	NaomiLoaded = true;
	return 0;
}

INT32 NaomiCoreExit()
{
	NaomiShutdownRetroCore();

	NaomiLoaded = false;
	NaomiGame = NULL;
	NaomiJoypadMap = NULL;
	NaomiJoypadMapCount = 0;
	NaomiLightgunMap = NULL;
	NaomiLightgunMapCount = 0;
	NaomiResetFrameState();
	OGLSetUseFallbackContext(false);
	OGLDestroyFallbackContext();
	NaomiTempRoot[0] = 0;
	NaomiTempContentDir[0] = 0;
	NaomiTempZipPath[0] = 0;
	NaomiTempSaveDir[0] = 0;

	return 0;
}

INT32 NaomiCoreReset()
{
	if (!NaomiLoaded) {
		return 1;
	}

	const bool useHwContext = NaomiHwRenderRequested || NaomiHwRenderReady;

	if (NaomiGame == NULL || !NaomiRetroInitialized || !NaomiRetroGameLoaded) {
		return 1;
	}

	flycast_retro_audio_flush_buffer();

	if (useHwContext) {
		OGLMakeCurrentContext();
	}
	flycast_retro_reset();
	if (useHwContext) {
		OGLDoneCurrentContext();
	}

	flycast_retro_audio_flush_buffer();
	NaomiClearRuntimeState();
	NaomiAudioWarmupFrames = 2;
	NaomiVideoWarmupReads = 2;
	NaomiVideoStartupSkipReads = 12;
	NaomiVideoProbeReads = 60;
	return 0;
}

INT32 NaomiCoreFrame()
{
	if (!NaomiLoaded) {
		return 1;
	}

	const bool useHwContext = NaomiHwRenderRequested || NaomiHwRenderReady;
	const double frameStartMs = NaomiNowMs();
	double outerDeltaMs = 0.0;
	if (NaomiLastFrameEntryMs > 0.0) {
		outerDeltaMs = frameStartMs - NaomiLastFrameEntryMs;
	}
	NaomiLastFrameEntryMs = frameStartMs;

	NaomiVideoFromCallback = false;
	NaomiVideoReadPending = false;
	// NOTE: Removed glClear of frontend FBO at frame start.
	// Clearing the FBO every frame causes flickering when Flycast produces
	// RTT/intermediate frames or when video read timing is earlier than final render.
	// Now we keep the previous frame content until a valid new frame is rendered.
	double contextMs = 0.0;
	if (useHwContext) {
		const double contextStartMs = NaomiNowMs();
		OGLMakeCurrentContext();
		contextMs += NaomiNowMs() - contextStartMs;
		// No longer clearing the frontend FBO - let Flycast render on top of previous frame
	}

	const double runStartMs = NaomiNowMs();
	flycast_retro_run();
	const double runMsRaw = NaomiNowMs() - runStartMs;
	const double runMs = NaomiCleanProfileDeltaMs(runMsRaw);
	if (NaomiPerfLoggingEnabled()) {
		if (!NaomiValidProfileDeltaMs(runMsRaw)) {
			NaomiPerfInvalidTimingCount++;
		} else if (runMsRaw > 10.0) {
			NaomiPerfSlowRunCount++;
			if (runMsRaw > NaomiPerfSlowRunMaxMs) { NaomiPerfSlowRunMaxMs = runMsRaw; }
			if (NaomiTraceSlowRunEnabled()) {
				size_t queuedFramesNow = 0;
				{
					std::lock_guard<std::mutex> lock(NaomiAudioMutex);
					if (NaomiAudioReadOffset < NaomiAudio.size()) {
						queuedFramesNow = (NaomiAudio.size() - NaomiAudioReadOffset) / 2;
					}
				}
				bprintf(0, _T("AWAVE slow-run frame=%d run=%0.3f ms audioQueued=%d preset=%S speed_hacks=%d\n"),
					NaomiPerfFrameCounter + 1, NaomiSafeProfileMs(runMsRaw), (INT32)queuedFramesNow, NaomiGetRenderPresetName(), NaomiUseSpeedHacks() ? 1 : 0);
			}
		}
	}

	const double captureStartMs = NaomiNowMs();
	NaomiCaptureVideo();
	const double captureMs = NaomiNowMs() - captureStartMs;

	if (useHwContext) {
		const double contextStartMs = NaomiNowMs();
		OGLDoneCurrentContext();
		contextMs += NaomiNowMs() - contextStartMs;
	}

	const double audioStartMs = NaomiNowMs();
	flycast_retro_audio_upload();
	NaomiDrainAudio();
	const double audioMs = NaomiNowMs() - audioStartMs;

	if (NaomiAudioWarmupFrames > 0) {
		NaomiAudioWarmupFrames--;
	}

	if (NaomiPerfLoggingEnabled()) {
		NaomiPerfFrameCounter++;
		NaomiPerfContextAccumMs += NaomiCleanProfileDeltaMs(contextMs);
		NaomiPerfRunAccumMs += runMs;
		NaomiPerfCaptureAccumMs += NaomiCleanProfileDeltaMs(captureMs);
		NaomiPerfAudioAccumMs += NaomiCleanProfileDeltaMs(audioMs);
		NaomiPerfTotalAccumMs += NaomiCleanProfileDeltaMs(NaomiNowMs() - frameStartMs);
		if (NaomiValidProfileDeltaMs(outerDeltaMs)) {
			NaomiPerfOuterDeltaAccumMs += outerDeltaMs;
			if (outerDeltaMs > NaomiPerfOuterMaxDeltaMs) { NaomiPerfOuterMaxDeltaMs = outerDeltaMs; }
		} else if (outerDeltaMs != 0.0) {
			NaomiPerfInvalidTimingCount++;
		}

		if ((NaomiPerfFrameCounter % 120) == 0) {
			size_t queuedFrames = 0;
			{
				std::lock_guard<std::mutex> lock(NaomiAudioMutex);
				if (NaomiAudioReadOffset < NaomiAudio.size()) {
					queuedFrames = (NaomiAudio.size() - NaomiAudioReadOffset) / 2;
				}
			}

			const double divisor = 120.0;
			const double readDivisor = (NaomiPerfReadPixelsSamples > 0) ? (double)NaomiPerfReadPixelsSamples : 1.0;
			bprintf(0, _T("AWAVE profile frame=%d hwReq=%d hwReady=%d pbo=%d video=%dx%d accepted=%d audioQueued=%d avg: outer=%0.3f outerMax=%0.3f total=%0.3f ctx=%0.3f run=%0.3f capture=%0.3f readpix=%0.3f flip=%0.3f audio=%0.3f ms slow=%d slowMax=%0.3f invalidTime=%d soundLen=%d soundRate=%d\n"),
				NaomiPerfFrameCounter,
				NaomiHwRenderRequested ? 1 : 0,
				NaomiHwRenderReady ? 1 : 0,
				(NaomiReadbackPbos[0] != 0) ? 1 : 0,
				NaomiVideoWidth,
				NaomiVideoHeight,
				NaomiAcceptedVideoFrames,
				(INT32)queuedFrames,
				NaomiSafeProfileMs(NaomiPerfOuterDeltaAccumMs / divisor),
				NaomiSafeProfileMs(NaomiPerfOuterMaxDeltaMs),
				NaomiSafeProfileMs(NaomiPerfTotalAccumMs / divisor),
				NaomiSafeProfileMs(NaomiPerfContextAccumMs / divisor),
				NaomiSafeProfileMs(NaomiPerfRunAccumMs / divisor),
				NaomiSafeProfileMs(NaomiPerfCaptureAccumMs / divisor),
				NaomiSafeProfileMs(NaomiPerfReadPixelsAccumMs / readDivisor),
				NaomiSafeProfileMs(NaomiPerfReadFlipAccumMs / readDivisor),
				NaomiSafeProfileMs(NaomiPerfAudioAccumMs / divisor),
				NaomiPerfSlowRunCount,
				NaomiSafeProfileMs(NaomiPerfSlowRunMaxMs),
				NaomiPerfInvalidTimingCount,
				nBurnSoundLen,
				nBurnSoundRate);

			NaomiPerfContextAccumMs = 0.0;
			NaomiPerfRunAccumMs = 0.0;
			NaomiPerfCaptureAccumMs = 0.0;
			NaomiPerfAudioAccumMs = 0.0;
			NaomiPerfTotalAccumMs = 0.0;
			NaomiPerfOuterDeltaAccumMs = 0.0;
			NaomiPerfOuterMaxDeltaMs = 0.0;
			NaomiPerfReadPixelsAccumMs = 0.0;
			NaomiPerfReadFlipAccumMs = 0.0;
			NaomiPerfReadPixelsSamples = 0;
			NaomiPerfSlowRunCount = 0;
			NaomiPerfSlowRunMaxMs = 0.0;
			NaomiPerfInvalidTimingCount = 0;
		}
	}

	return 0;
}

INT32 NaomiCoreGetHwVideoInfo(NaomiCoreHwVideoInfo* info)
{
	if (info == NULL) {
		return 1;
	}

	memset(info, 0, sizeof(*info));
	info->size = sizeof(*info);
	info->texture = (UINT32)NaomiFrontendColorTexture;
	info->framebuffer = (UINT32)NaomiFrontendFramebuffer;
	info->width = NaomiHwDirectFrameWidth > 0 ? NaomiHwDirectFrameWidth : NaomiFrontendFramebufferWidth;
	info->height = NaomiHwDirectFrameHeight > 0 ? NaomiHwDirectFrameHeight : NaomiFrontendFramebufferHeight;
	info->upsideDown = 1;
	info->valid = NaomiCanExposeHwTextureToFrontend() && NaomiHwDirectFrameValid ? 1 : 0;
	return info->valid ? 0 : 1;
}

INT32 NaomiCoreUsingHwDirectPresent()
{
	return NaomiCanExposeHwTextureToFrontend() && NaomiHwDirectFrameValid ? 1 : 0;
}

INT32 NaomiCoreWantsRedraw()
{
	if (!NaomiLoaded) {
		return 0;
	}

	// skip_readback intentionally does not publish a CPU framebuffer.
	// Avoid BurnDrvRedraw()/SoftFX entirely so the test measures Flycast core
	// and audio pacing rather than FBNeo's black-frame redraw path.
	if (NaomiWantsSkipReadback()) {
		return 0;
	}

	// In final direct_texture mode the frontend presents the GL texture.
	// Do not also ask FBNeo to redraw stale/readback CPU data.
	if (NaomiCoreUsingHwDirectPresent()) {
		return 0;
	}

	return 1;
}

INT32 NaomiCoreDraw()
{
	if (!NaomiLoaded || pBurnDraw == NULL) {
		return 0;
	}

	const double drawStartMs = NaomiNowMs();

	if (NaomiCoreUsingHwDirectPresent()) {
		// Final intended path: the frontend presents NaomiFrontendColorTexture
		// directly. Do not overwrite pBurnDraw with stale CPU readback data.
		return 0;
	}

	if (NaomiVideo.empty()) {
		memset(pBurnDraw, 0, (size_t)nBurnPitch * 480U);
		return 0;
	}

	const INT32 dstWidth = (nBurnBpp > 0) ? (nBurnPitch / nBurnBpp) : 0;
	const INT32 drawWidth = std::max<INT32>(0, std::min(NaomiVideoWidth, dstWidth));
	const INT32 drawHeight = std::max<INT32>(0, std::min(NaomiVideoHeight, 480));

	if (drawWidth < dstWidth || drawHeight < 480) {
		memset(pBurnDraw, 0, (size_t)nBurnPitch * 480U);
	}

	const bool directRgb32 = (nBurnBpp >= 4)
		&& BurnHighCol(0x12, 0x34, 0x56, 0) == 0x00123456
		&& BurnHighCol(0xaa, 0x55, 0x11, 0) == 0x00aa5511;
	const bool fastRgb565 = (nBurnBpp == 2);

	for (INT32 y = 0; y < drawHeight; y++) {
		UINT8* dst = pBurnDraw + (y * nBurnPitch);
		const UINT32* src = &NaomiVideo[(y * NaomiVideoStride)];

		if (directRgb32) {
			memcpy(dst, src, (size_t)drawWidth * sizeof(UINT32));
			continue;
		}

		if (fastRgb565) {
			UINT16* dst16 = (UINT16*)dst;
			for (INT32 x = 0; x < drawWidth; x++) {
				UINT32 pixel = src[x];
				dst16[x] = (UINT16)(((pixel >> 8) & 0xf800) | ((pixel >> 5) & 0x07e0) | ((pixel >> 3) & 0x001f));
			}
			continue;
		}

		for (INT32 x = 0; x < drawWidth; x++) {
			UINT32 pixel = src[x];
			INT32 r = (pixel >> 16) & 0xff;
			INT32 g = (pixel >> 8) & 0xff;
			INT32 b = pixel & 0xff;
			PutPix(dst + (x * nBurnBpp), BurnHighCol(r, g, b, 0));
		}
	}

	if (NaomiPerfLoggingEnabled()) {
		NaomiPerfDrawCounter++;
		NaomiPerfDrawAccumMs += (NaomiNowMs() - drawStartMs);
		if ((NaomiPerfDrawCounter % 120) == 0) {
			bprintf(0, _T("AWAVE profile draw=%d avg: draw=%0.3f ms bpp=%d pitch=%d dstWidth=%d draw=%dx%d direct32=%d rgb565=%d\n"),
				NaomiPerfDrawCounter,
				NaomiSafeProfileMs(NaomiPerfDrawAccumMs / 120.0),
				nBurnBpp,
				nBurnPitch,
				dstWidth,
				drawWidth,
				drawHeight,
				directRgb32 ? 1 : 0,
				fastRgb565 ? 1 : 0);
			NaomiPerfDrawAccumMs = 0.0;
		}
	}

	return 0;
}

INT32 NaomiCoreScan(INT32 nAction, INT32* pnMin)
{
	if (pnMin) {
		*pnMin = 0x029900;
	}

	if (!NaomiLoaded || NaomiState.empty()) {
		return 0;
	}

	if (nAction & ACB_VOLATILE) {
		if (nAction & ACB_WRITE) {
			ScanVar(NaomiState.data(), (INT32)NaomiState.size(), (char*)"naomi_state");
			flycast_retro_unserialize(NaomiState.data(), NaomiState.size());
		} else {
			flycast_retro_serialize(NaomiState.data(), NaomiState.size());
			ScanVar(NaomiState.data(), (INT32)NaomiState.size(), (char*)"naomi_state");
		}
	}

	return 0;
}

void NaomiCoreSetPadState(INT32 port, UINT32 state)
{
	if (port < 0 || port >= 4) {
		return;
	}

	NaomiPads[port] = state;
}

void NaomiCoreSetAnalogState(INT32 port, INT32 axis, INT16 value)
{
	if (port < 0 || port >= 4 || axis < 0 || axis >= 4) {
		return;
	}

	NaomiAnalog[port][axis] = value;
}

void NaomiCoreSetAnalogButtonState(INT32 port, INT32 button, INT16 value)
{
	if (port < 0 || port >= 4 || button < 0 || button >= 2) {
		return;
	}

	NaomiAnalogButtons[port][button] = value;
}

void NaomiCoreSetLightgunState(INT32 port, INT16 x, INT16 y, UINT8 offscreen, UINT8 reload)
{
	if (port < 0 || port >= 4) {
		return;
	}

	NaomiLightgunAxis[port][0] = x;
	NaomiLightgunAxis[port][1] = y;
	NaomiLightgunOffscreen[port] = offscreen ? 1 : 0;
	NaomiLightgunReload[port] = reload ? 1 : 0;
}
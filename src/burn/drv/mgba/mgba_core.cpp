#include "burnint.h"
#include "mgba_core.h"
#include "mgba_wrap_config.h"

#ifdef MAP_READ
#undef MAP_READ
#endif

#ifdef MAP_WRITE
#undef MAP_WRITE
#endif

#include <mgba/core/blip_buf.h>
#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/serialize.h>
#include <mgba-util/vfs.h>

const MgbaSystemConfig MgbaSystemGba = {
	MGBA_PLATFORM_GBA,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	-1,
	-1,
	240,
	160,
};

const MgbaSystemConfig MgbaSystemGb = {
	MGBA_PLATFORM_GB,
	"CGB",
	"SGB",
	"CGB",
	"CGB",
	"CGB",
	3,
	0,
	256,
	224,
};

const MgbaSystemConfig MgbaSystemGbc = {
	MGBA_PLATFORM_GB,
	"CGB",
	"CGB",
	"CGB",
	"CGB",
	"CGB",
	-1,
	0,
	160,
	144,
};

static const MgbaSystemConfig* MgbaSystem = &MgbaSystemGba;
static struct mCore* MgbaCore = NULL;

static UINT8* MgbaRom = NULL;
static INT32 MgbaRomLen = 0;
static UINT8* MgbaBios = NULL;
static INT32 MgbaBiosLen = 0;

static UINT8* MgbaState = NULL;
static INT32 MgbaStateLen = 0;
static UINT8* MgbaNvram = NULL;
static INT32 MgbaNvramLen = 0;

static color_t* MgbaVideo = NULL;
static INT16* MgbaAudioScratch = NULL;
static INT32 MgbaAudioScratchCapacity = 0;
static INT32 MgbaAudioScratchUsed = 0;
static struct mAVStream MgbaStream;
static INT16 MgbaAudioCallbackBuffer[512 * 2];

static INT32 MgbaVideoWidth = 240;
static INT32 MgbaVideoHeight = 160;
static INT32 MgbaVideoStride = 256;
static INT32 MgbaSampleRate = 32768;
static INT32 MgbaAudioFrames = 2048;
static INT32 MgbaAudioBufferFrames = 2048;

static INT32 MgbaGetOutputWidth()
{
	if (MgbaSystem != NULL && MgbaSystem->outputWidth > 0) {
		return MgbaSystem->outputWidth;
	}

	return 240;
}

static INT32 MgbaGetOutputHeight()
{
	if (MgbaSystem != NULL && MgbaSystem->outputHeight > 0) {
		return MgbaSystem->outputHeight;
	}

	return 160;
}

static bool MgbaUseGbAudioStream()
{
	return MgbaSystem != NULL && MgbaSystem->expectedPlatform == MGBA_PLATFORM_GB;
}

static void MgbaStreamAudioRateChanged(struct mAVStream*, unsigned)
{
}

static void MgbaQueueAudioFrames(const INT16* src, INT32 frames)
{
	if (MgbaAudioScratch == NULL || src == NULL || frames <= 0 || MgbaAudioScratchCapacity <= 0) {
		return;
	}

	if (frames > MgbaAudioScratchCapacity) {
		src += (frames - MgbaAudioScratchCapacity) * 2;
		frames = MgbaAudioScratchCapacity;
	}

	INT32 room = MgbaAudioScratchCapacity - MgbaAudioScratchUsed;
	if (frames > room) {
		INT32 drop = frames - room;
		INT32 keep = MgbaAudioScratchUsed - drop;
		if (keep > 0) {
			memmove(MgbaAudioScratch, MgbaAudioScratch + (drop * 2), keep * 2 * sizeof(INT16));
			MgbaAudioScratchUsed = keep;
		} else {
			MgbaAudioScratchUsed = 0;
		}
	}

	memcpy(MgbaAudioScratch + (MgbaAudioScratchUsed * 2), src, frames * 2 * sizeof(INT16));
	MgbaAudioScratchUsed += frames;
}

static void MgbaStreamPostAudioBuffer(struct mAVStream*, struct blip_t* left, struct blip_t* right)
{
	if (MgbaAudioScratch == NULL || MgbaAudioScratchCapacity <= 0) {
		return;
	}

	INT32 produced = blip_read_samples(left, MgbaAudioCallbackBuffer, 512, 1);
	if (produced > 0) {
		blip_read_samples(right, MgbaAudioCallbackBuffer + 1, 512, 1);
		MgbaQueueAudioFrames(MgbaAudioCallbackBuffer, produced);
	}
}

static void MgbaPullGbAudioFromBlips()
{
	if (MgbaCore == NULL || MgbaAudioScratch == NULL) {
		return;
	}

	struct blip_t* left = MgbaCore->getAudioChannel(MgbaCore, 0);
	struct blip_t* right = MgbaCore->getAudioChannel(MgbaCore, 1);
	if (left == NULL || right == NULL) {
		return;
	}

	INT32 availLeft = blip_samples_avail(left);
	INT32 availRight = blip_samples_avail(right);
	INT32 avail = d_min(availLeft, availRight);

	while (avail > 0) {
		INT32 chunk = d_min(avail, 512);
		INT32 produced = blip_read_samples(left, MgbaAudioCallbackBuffer, chunk, 1);
		if (produced <= 0) {
			break;
		}

		blip_read_samples(right, MgbaAudioCallbackBuffer + 1, produced, 1);
		MgbaQueueAudioFrames(MgbaAudioCallbackBuffer, produced);
		availLeft = blip_samples_avail(left);
		availRight = blip_samples_avail(right);
		avail = d_min(availLeft, availRight);
	}
}

static void MgbaDrainQueuedAudio(INT16* dst, INT32 dstFrames)
{
	if (dst == NULL || dstFrames <= 0) {
		return;
	}

	memset(dst, 0, dstFrames * 2 * sizeof(INT16));

	if (MgbaAudioScratch == NULL || MgbaAudioScratchUsed <= 0) {
		return;
	}

	INT32 copyFrames = d_min(MgbaAudioScratchUsed, dstFrames);
	memcpy(dst, MgbaAudioScratch, copyFrames * 2 * sizeof(INT16));

	INT32 keep = MgbaAudioScratchUsed - copyFrames;
	if (keep > 0) {
		memmove(MgbaAudioScratch, MgbaAudioScratch + (copyFrames * 2), keep * 2 * sizeof(INT16));
	}
	MgbaAudioScratchUsed = keep;
}

static INT32 MgbaLoadRequiredRom(UINT8** buffer, INT32* length, INT32 index)
{
	struct BurnRomInfo ri;
	if (BurnDrvGetRomInfo(&ri, index)) {
		return 1;
	}

	*length = ri.nLen;
	*buffer = (UINT8*)BurnMalloc(*length);
	if (*buffer == NULL) {
		return 1;
	}

	if (BurnLoadRom(*buffer, index, 1)) {
		BurnFree(*buffer);
		*buffer = NULL;
		*length = 0;
		return 1;
	}

	return 0;
}

static void MgbaTryLoadOptionalRom(UINT8** buffer, INT32* length, INT32 index)
{
	struct BurnRomInfo ri;
	if (BurnDrvGetRomInfo(&ri, index)) {
		return;
	}

	*length = ri.nLen;
	*buffer = (UINT8*)BurnMalloc(*length);
	if (*buffer == NULL) {
		*length = 0;
		return;
	}

	if (BurnLoadRom(*buffer, index, 1)) {
		BurnFree(*buffer);
		*buffer = NULL;
		*length = 0;
	}
}

static void MgbaApplySystemOverrides()
{
	if (MgbaCore == NULL || MgbaSystem == NULL) {
		return;
	}

	if (MgbaSystem->gbModel) {
		mCoreConfigSetOverrideValue(&MgbaCore->config, "gb.model", MgbaSystem->gbModel);
	}
	if (MgbaSystem->sgbModel) {
		mCoreConfigSetOverrideValue(&MgbaCore->config, "sgb.model", MgbaSystem->sgbModel);
	}
	if (MgbaSystem->cgbModel) {
		mCoreConfigSetOverrideValue(&MgbaCore->config, "cgb.model", MgbaSystem->cgbModel);
	}
	if (MgbaSystem->cgbHybridModel) {
		mCoreConfigSetOverrideValue(&MgbaCore->config, "cgb.hybridModel", MgbaSystem->cgbHybridModel);
	}
	if (MgbaSystem->cgbSgbModel) {
		mCoreConfigSetOverrideValue(&MgbaCore->config, "cgb.sgbModel", MgbaSystem->cgbSgbModel);
	}
	if (MgbaSystem->gbColors >= 0) {
		mCoreConfigSetDefaultIntValue(&MgbaCore->config, "gb.colors", MgbaSystem->gbColors);
	}
	if (MgbaSystem->sgbBorders >= 0) {
		mCoreConfigSetDefaultIntValue(&MgbaCore->config, "sgb.borders", MgbaSystem->sgbBorders);
	}
}

static INT32 MgbaEnsureStateBuffer()
{
	if (MgbaCore == NULL) {
		return 1;
	}

	struct VFile* vf = VFileMemChunk(NULL, 0);
	if (vf == NULL) {
		return 1;
	}

	if (!mCoreSaveStateNamed(MgbaCore, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC)) {
		vf->close(vf);
		return 1;
	}

	INT32 size = (INT32)vf->size(vf);
	if (size <= 0) {
		vf->close(vf);
		return 1;
	}

	if (MgbaStateLen != size) {
		BurnFree(MgbaState);
		MgbaState = (UINT8*)BurnMalloc(size);
		MgbaStateLen = (MgbaState != NULL) ? size : 0;
	}

	if (MgbaState == NULL) {
		vf->close(vf);
		return 1;
	}

	void* stateView = vf->map(vf, size, MAP_READ);
	if (stateView) {
		memcpy(MgbaState, stateView, size);
		vf->unmap(vf, stateView, size);
	}
	vf->close(vf);

	return 0;
}

static INT32 MgbaRefreshNvramFromCore()
{
	if (MgbaCore == NULL || MgbaCore->savedataClone == NULL) {
		return 1;
	}

	void* clone = NULL;
	INT32 size = (INT32)MgbaCore->savedataClone(MgbaCore, &clone);
	if (size <= 0 || clone == NULL) {
		free(clone);
		return 1;
	}

	if (MgbaNvramLen != size) {
		BurnFree(MgbaNvram);
		MgbaNvram = (UINT8*)BurnMalloc(size);
		MgbaNvramLen = (MgbaNvram != NULL) ? size : 0;
	}

	if (MgbaNvram == NULL) {
		free(clone);
		return 1;
	}

	memcpy(MgbaNvram, clone, size);
	free(clone);
	return 0;
}

static INT32 MgbaRestoreNvramToCore()
{
	if (MgbaCore == NULL || MgbaCore->savedataRestore == NULL || MgbaNvram == NULL || MgbaNvramLen <= 0) {
		return 1;
	}

	return MgbaCore->savedataRestore(MgbaCore, MgbaNvram, MgbaNvramLen, true) ? 0 : 1;
}

INT32 MgbaCoreInit(const MgbaSystemConfig* config)
{
	unsigned width = 0;
	unsigned height = 0;
	double refreshRate = 59.72750057;
	INT32 frameCycles = 0;
	INT32 frequency = 0;
	struct VFile* rom = NULL;
	INT32 allocWidth = 0;
	INT32 allocHeight = 0;

	MgbaSystem = config ? config : &MgbaSystemGba;

	if (MgbaLoadRequiredRom(&MgbaRom, &MgbaRomLen, 0)) {
		return 1;
	}

	MgbaTryLoadOptionalRom(&MgbaBios, &MgbaBiosLen, 1);

	rom = VFileFromMemory(MgbaRom, MgbaRomLen);
	if (rom == NULL) {
		MgbaCoreExit();
		return 1;
	}

	MgbaCore = mCoreFindVF(rom);
	if (MgbaCore == NULL) {
		rom->close(rom);
		MgbaCoreExit();
		return 1;
	}

	mCoreInitConfig(MgbaCore, NULL);
	if (!MgbaCore->init(MgbaCore)) {
		rom->close(rom);
		MgbaCore = NULL;
		MgbaCoreExit();
		return 1;
	}

	if (MgbaCore->platform && MgbaCore->platform(MgbaCore) != MgbaSystem->expectedPlatform) {
		rom->close(rom);
		MgbaCoreExit();
		return 1;
	}

	allocWidth = MgbaGetOutputWidth();
	allocHeight = MgbaGetOutputHeight();
	MgbaVideoWidth = allocWidth;
	MgbaVideoHeight = allocHeight;
	MgbaVideoStride = (MgbaVideoWidth + 31) & ~31;
	if (MgbaVideoStride < 256) {
		MgbaVideoStride = 256;
	}

	MgbaVideo = (color_t*)BurnMalloc(MgbaVideoStride * d_max(allocHeight, 256) * sizeof(color_t));
	if (MgbaVideo == NULL) {
		rom->close(rom);
		MgbaCoreExit();
		return 1;
	}

	MgbaSampleRate = nBurnSoundRate ? nBurnSoundRate : 32768;
	MgbaAudioFrames = nBurnSoundLen ? d_max(nBurnSoundLen * 2, 1024) : 2048;
	MgbaAudioBufferFrames = MgbaAudioFrames;
	if (MgbaSystem->expectedPlatform == MGBA_PLATFORM_GB) {
		// Match mGBA's libretro frontend for GB/GBC: use a fixed 512-sample internal
		// queue and collect completed output-rate chunks through postAudioBuffer.
		MgbaAudioBufferFrames = 512;
	}
	MgbaAudioScratchCapacity = MgbaUseGbAudioStream() ? d_max(MgbaAudioFrames * 8, 8192) : MgbaAudioFrames;
	MgbaAudioScratch = (INT16*)BurnMalloc(MgbaAudioScratchCapacity * 2 * sizeof(INT16));
	if (MgbaAudioScratch == NULL) {
		rom->close(rom);
		MgbaCoreExit();
		return 1;
	}
	MgbaAudioScratchUsed = 0;
	memset(&MgbaStream, 0, sizeof(MgbaStream));

	MgbaCore->setVideoBuffer(MgbaCore, MgbaVideo, MgbaVideoStride);
	MgbaCore->setAudioBufferSize(MgbaCore, MgbaAudioBufferFrames);

	struct mCoreOptions opts;
	memset(&opts, 0, sizeof(opts));
	opts.useBios = (MgbaBios != NULL && MgbaBiosLen > 0);
	opts.skipBios = !opts.useBios;
	opts.sampleRate = MgbaSampleRate;
	opts.volume = 0x100;
	opts.audioBuffers = MgbaAudioBufferFrames;

	mCoreConfigLoadDefaults(&MgbaCore->config, &opts);
	MgbaApplySystemOverrides();
	mCoreLoadConfig(MgbaCore);

	if (MgbaBios != NULL && MgbaBiosLen > 0) {
		struct VFile* bios = VFileFromMemory(MgbaBios, MgbaBiosLen);
		if (bios != NULL) {
			if (!MgbaCore->loadBIOS(MgbaCore, bios, 0)) {
				bios->close(bios);
			}
		}
	}

	if (!MgbaCore->loadROM(MgbaCore, rom)) {
		MgbaCoreExit();
		return 1;
	}

	MgbaCore->desiredVideoDimensions(MgbaCore, &width, &height);
	MgbaVideoWidth = width ? (INT32)width : allocWidth;
	MgbaVideoHeight = height ? (INT32)height : allocHeight;

	if (MgbaCore->frequency) {
		frequency = MgbaCore->frequency(MgbaCore);
	}
	if (MgbaCore->frameCycles) {
		frameCycles = MgbaCore->frameCycles(MgbaCore);
	}

	if (frequency > 0) {
		blip_set_rates(MgbaCore->getAudioChannel(MgbaCore, 0), frequency, MgbaSampleRate);
		blip_set_rates(MgbaCore->getAudioChannel(MgbaCore, 1), frequency, MgbaSampleRate);
	}

	if (MgbaUseGbAudioStream()) {
		MgbaStream.audioRateChanged = MgbaStreamAudioRateChanged;
		MgbaStream.postAudioFrame = NULL;
		MgbaStream.postAudioBuffer = MgbaStreamPostAudioBuffer;
		MgbaCore->setAVStream(MgbaCore, &MgbaStream);
	}

	if (frequency > 0 && frameCycles > 0) {
		refreshRate = (double)frequency / (double)frameCycles;
	}
	BurnSetRefreshRate(refreshRate);

	MgbaCoreReset();
	MgbaEnsureStateBuffer();
	MgbaRefreshNvramFromCore();

	char title[17];
	memset(title, 0, sizeof(title));
	MgbaCore->getGameTitle(MgbaCore, title);
	bprintf(0, _T("mGBA: loaded \"%hs\" (%dx%d)\n"), title, MgbaVideoWidth, MgbaVideoHeight);

	return 0;
}

INT32 MgbaCoreExit()
{
	if (MgbaCore) {
		if (MgbaUseGbAudioStream() && MgbaCore->setAVStream) {
			MgbaCore->setAVStream(MgbaCore, NULL);
		}
		mCoreConfigDeinit(&MgbaCore->config);
		MgbaCore->deinit(MgbaCore);
		MgbaCore = NULL;
	}

	MgbaSystem = &MgbaSystemGba;

	BurnFree(MgbaAudioScratch);
	MgbaAudioScratch = NULL;
	MgbaAudioScratchCapacity = 0;
	MgbaAudioScratchUsed = 0;
	MgbaAudioBufferFrames = 2048;
	memset(&MgbaStream, 0, sizeof(MgbaStream));

	BurnFree(MgbaVideo);
	MgbaVideo = NULL;

	BurnFree(MgbaState);
	MgbaState = NULL;
	MgbaStateLen = 0;

	BurnFree(MgbaNvram);
	MgbaNvram = NULL;
	MgbaNvramLen = 0;

	BurnFree(MgbaRom);
	MgbaRom = NULL;
	MgbaRomLen = 0;

	BurnFree(MgbaBios);
	MgbaBios = NULL;
	MgbaBiosLen = 0;

	return 0;
}

INT32 MgbaCoreReset()
{
	if (MgbaCore == NULL) {
		return 1;
	}

	MgbaCore->reset(MgbaCore);
	MgbaAudioScratchUsed = 0;

	if (MgbaCore->getAudioChannel) {
		blip_clear(MgbaCore->getAudioChannel(MgbaCore, 0));
		blip_clear(MgbaCore->getAudioChannel(MgbaCore, 1));
	}

	return 0;
}

INT32 MgbaCoreDraw()
{
	if (MgbaCore == NULL || pBurnDraw == NULL || MgbaVideo == NULL) {
		return 1;
	}

	INT32 canvasWidth = 0;
	INT32 canvasHeight = 0;
	BurnDrvGetVisibleSize(&canvasWidth, &canvasHeight);
	if (canvasWidth <= 0 || canvasHeight <= 0) {
		canvasWidth = MgbaGetOutputWidth();
		canvasHeight = MgbaGetOutputHeight();
	}

	memset(pBurnDraw, 0, canvasHeight * nBurnPitch);

	INT32 drawWidth = d_min(MgbaVideoWidth, canvasWidth);
	INT32 drawHeight = d_min(MgbaVideoHeight, canvasHeight);
	INT32 xOffset = (canvasWidth - drawWidth) / 2;
	INT32 yOffset = (canvasHeight - drawHeight) / 2;
	if (xOffset < 0) xOffset = 0;
	if (yOffset < 0) yOffset = 0;

	for (INT32 y = 0; y < drawHeight; y++) {
		UINT8* dst = pBurnDraw + ((y + yOffset) * nBurnPitch) + (xOffset * nBurnBpp);
		color_t* src = MgbaVideo + (y * MgbaVideoStride);

		for (INT32 x = 0; x < drawWidth; x++) {
			UINT16 pixel = src[x];
			INT32 r = ((pixel >> 11) & 0x1f) * 255 / 31;
			INT32 g = ((pixel >> 5) & 0x3f) * 255 / 63;
			INT32 b = ((pixel >> 0) & 0x1f) * 255 / 31;
			PutPix(dst + (x * nBurnBpp), BurnHighCol(r, g, b, 0));
		}
	}

	return 0;
}

INT32 MgbaCoreFrame(UINT16 keys)
{
	if (MgbaCore == NULL) {
		return 1;
	}

	MgbaCore->setKeys(MgbaCore, keys);
	MgbaCore->runFrame(MgbaCore);

	if (pBurnSoundOut && nBurnSoundLen > 0) {
		if (MgbaUseGbAudioStream()) {
			MgbaPullGbAudioFromBlips();
			MgbaDrainQueuedAudio(pBurnSoundOut, nBurnSoundLen);
		} else {
			memset(pBurnSoundOut, 0, nBurnSoundLen * 2 * sizeof(INT16));
			INT32 availLeft = blip_samples_avail(MgbaCore->getAudioChannel(MgbaCore, 0));
			INT32 availRight = blip_samples_avail(MgbaCore->getAudioChannel(MgbaCore, 1));
			INT32 samples = d_min(d_min(availLeft, availRight), nBurnSoundLen);

			if (samples > 0) {
				blip_read_samples(MgbaCore->getAudioChannel(MgbaCore, 0), pBurnSoundOut + 0, samples, 1);
				blip_read_samples(MgbaCore->getAudioChannel(MgbaCore, 1), pBurnSoundOut + 1, samples, 1);
			}
		}
	}

	if (pBurnDraw) {
		MgbaCoreDraw();
	}

	return 0;
}

INT32 MgbaCoreScan(INT32 nAction, INT32* pnMin)
{
	if (pnMin) {
		*pnMin = 0x029900;
	}

	if (MgbaCore == NULL) {
		return 0;
	}

	if (nAction & ACB_VOLATILE) {
		if (nAction & ACB_WRITE) {
			if (MgbaState && MgbaStateLen > 0) {
				ScanVar(MgbaState, MgbaStateLen, (char*)"mgba_state");
				struct VFile* vf = VFileFromConstMemory(MgbaState, MgbaStateLen);
				if (vf) {
					mCoreLoadStateNamed(MgbaCore, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC);
					vf->close(vf);
				}
			}
		} else {
			if (!MgbaEnsureStateBuffer() && MgbaState && MgbaStateLen > 0) {
				ScanVar(MgbaState, MgbaStateLen, (char*)"mgba_state");
			}
		}
	}

	if (nAction & ACB_NVRAM) {
		if (nAction & ACB_WRITE) {
			if (MgbaNvram == NULL || MgbaNvramLen <= 0) {
				MgbaRefreshNvramFromCore();
			}
			if (MgbaNvram && MgbaNvramLen > 0) {
				ScanVar(MgbaNvram, MgbaNvramLen, (char*)"mgba_nvram");
				MgbaRestoreNvramToCore();
			}
		} else {
			if (!MgbaRefreshNvramFromCore() && MgbaNvram && MgbaNvramLen > 0) {
				ScanVar(MgbaNvram, MgbaNvramLen, (char*)"mgba_nvram");
			}
		}
	}

	return 0;
}

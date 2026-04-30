/* Minimal standalone bring-up for the embedded mGBA port. */

#include <mgba/core/blip_buf.h>
#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/interface.h>
#include <mgba-util/vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MGBA_STANDALONE_WIDTH 256
#define MGBA_STANDALONE_HEIGHT 256
#define MGBA_STANDALONE_SAMPLE_RATE 32768
#define MGBA_STANDALONE_AUDIO_BUFFER 2048

static void mgba_standalone_log(struct mLogger* logger, int category, enum mLogLevel level, const char* format, va_list args)
{
	UNUSED(logger);
	UNUSED(level);

	fprintf(stderr, "[mGBA:%s] ", mLogCategoryName(category));
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <rom.gba> [frames]\n", argv[0]);
		return 2;
	}

	int frames = 1;
	if (argc >= 3) {
		frames = atoi(argv[2]);
		if (frames <= 0) {
			frames = 1;
		}
	}

	struct VFile* rom = VFileOpen(argv[1], O_RDONLY);
	if (!rom) {
		fprintf(stderr, "failed to open rom: %s\n", argv[1]);
		return 3;
	}

	struct mCore* core = mCoreFindVF(rom);
	if (!core) {
		rom->close(rom);
		fprintf(stderr, "failed to identify rom core: %s\n", argv[1]);
		return 4;
	}

	struct mLogger logger = { 0 };
	logger.log = mgba_standalone_log;
	mLogSetDefaultLogger(&logger);

	mCoreInitConfig(core, NULL);
	if (!core->init(core)) {
		rom->close(rom);
		fprintf(stderr, "core init failed\n");
		return 5;
	}

	unsigned width = 0;
	unsigned height = 0;
	core->desiredVideoDimensions(core, &width, &height);

	color_t* video = calloc(MGBA_STANDALONE_WIDTH * MGBA_STANDALONE_HEIGHT, sizeof(color_t));
	if (!video) {
		rom->close(rom);
		core->deinit(core);
		fprintf(stderr, "video buffer allocation failed\n");
		return 6;
	}

	core->setVideoBuffer(core, video, MGBA_STANDALONE_WIDTH);
	core->setAudioBufferSize(core, MGBA_STANDALONE_AUDIO_BUFFER);

	struct mCoreOptions opts = { 0 };
	opts.useBios = true;
	opts.volume = 0x100;
	opts.sampleRate = MGBA_STANDALONE_SAMPLE_RATE;
	mCoreConfigLoadDefaults(&core->config, &opts);
	mCoreLoadConfig(core);

	if (!core->loadROM(core, rom)) {
		rom->close(rom);
		core->deinit(core);
		free(video);
		fprintf(stderr, "loadROM failed\n");
		return 7;
	}

	blip_set_rates(core->getAudioChannel(core, 0), core->frequency(core), MGBA_STANDALONE_SAMPLE_RATE);
	blip_set_rates(core->getAudioChannel(core, 1), core->frequency(core), MGBA_STANDALONE_SAMPLE_RATE);

	core->reset(core);

	for (int frame = 0; frame < frames; frame++) {
		core->setKeys(core, 0);
		core->runFrame(core);
	}

	int samples = blip_samples_avail(core->getAudioChannel(core, 0));
	char title[17];
	memset(title, 0, sizeof(title));
	core->getGameTitle(core, title);

	printf("OK title=\"%s\" video=%ux%u frames=%d audio_samples=%d\n",
		title, width, height, frames, samples);

	mCoreConfigDeinit(&core->config);
	core->deinit(core);
	free(video);

	return 0;
}

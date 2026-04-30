// FBNeo-local replacement for the old embedded Flycast audiostream wrapper.
// This follows the newer Flycast libretro audio lifecycle closely enough to
// support explicit init/deinit/flush/upload from the FBNeo wrapper layer.

#include "awave_wrap_config.h"
#include "types.h"

#include <libretro.h>

#include <cstdlib>
#include <limits>
#include <cstring>
#include <mutex>
#include <vector>

extern retro_audio_sample_batch_t audio_batch_cb;

static std::mutex g_audioMutex;
static std::vector<int16_t> g_audioBuffer;
static size_t g_audioBufferIdx = 0;
static size_t g_audioBatchFramesMax = 0;
static bool g_audioDropSamples = true;
static int16_t* g_audioOutBuffer = nullptr;

extern "C" void retro_audio_init(void)
{
	const std::lock_guard<std::mutex> lock(g_audioMutex);

	const size_t audioBufferSize = (44100 / 25) * 2 * 10;
	g_audioBuffer.resize(audioBufferSize);
	g_audioBufferIdx = 0;
	g_audioBatchFramesMax = std::numeric_limits<size_t>::max();

	if (g_audioOutBuffer != nullptr) {
		free(g_audioOutBuffer);
	}
	g_audioOutBuffer = (int16_t*)malloc(audioBufferSize * sizeof(int16_t));

	g_audioDropSamples = false;
}

extern "C" void retro_audio_deinit(void)
{
	const std::lock_guard<std::mutex> lock(g_audioMutex);

	g_audioBuffer.clear();
	g_audioBufferIdx = 0;
	g_audioBatchFramesMax = 0;

	if (g_audioOutBuffer != nullptr) {
		free(g_audioOutBuffer);
		g_audioOutBuffer = nullptr;
	}

	g_audioDropSamples = true;
}

extern "C" void retro_audio_flush_buffer(void)
{
	const std::lock_guard<std::mutex> lock(g_audioMutex);
	g_audioBufferIdx = 0;
	g_audioDropSamples = false;
}

extern "C" void retro_audio_upload(void)
{
	size_t numFrames = 0;
	size_t batchFramesMax = 0;

	{
		const std::lock_guard<std::mutex> lock(g_audioMutex);

		if (g_audioBufferIdx == 0 || audio_batch_cb == nullptr || g_audioOutBuffer == nullptr) {
			g_audioBufferIdx = 0;
			g_audioDropSamples = false;
			return;
		}

		memcpy(g_audioOutBuffer, g_audioBuffer.data(), g_audioBufferIdx * sizeof(int16_t));
		numFrames = g_audioBufferIdx >> 1;
		g_audioBufferIdx = 0;
		g_audioDropSamples = false;
		batchFramesMax = g_audioBatchFramesMax;
	}

	int16_t* audioOutBufferPtr = g_audioOutBuffer;
	while (numFrames > 0) {
		const size_t framesToWrite = (numFrames > batchFramesMax) ? batchFramesMax : numFrames;
		const size_t framesWritten = audio_batch_cb(audioOutBufferPtr, framesToWrite);

		if (framesWritten < framesToWrite && framesWritten > 0) {
			const std::lock_guard<std::mutex> lock(g_audioMutex);
			g_audioBatchFramesMax = framesWritten;
			batchFramesMax = framesWritten;
		}

		if (framesToWrite == 0) {
			break;
		}

		numFrames -= framesToWrite;
		audioOutBufferPtr += framesToWrite << 1;
	}
}

void WriteSample(s16 r, s16 l)
{
	const std::lock_guard<std::mutex> lock(g_audioMutex);

	if (g_audioDropSamples) {
		return;
	}

	if (g_audioBuffer.size() < g_audioBufferIdx + 2) {
		g_audioBufferIdx = 0;
		g_audioDropSamples = true;
		return;
	}

	g_audioBuffer[g_audioBufferIdx++] = l;
	g_audioBuffer[g_audioBufferIdx++] = r;
}

void InitAudio()
{
}

void TermAudio()
{
}

void StartAudioRecording(bool)
{
}

u32 RecordAudio(void*, u32)
{
	return 0;
}

void StopAudioRecording()
{
}

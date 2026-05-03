#include "fbneo_flycast_api.h"
#include <string.h>

static FbneoFlycastCallbacks g_callbacks;
static int g_loaded = 0;
static int g_width = 640;
static int g_height = 480;
static int g_sample_rate = 44100;
static double g_fps = 60.0;

int fbfc_init(const FbneoFlycastCallbacks* callbacks)
{
	memset(&g_callbacks, 0, sizeof(g_callbacks));
	if (callbacks) {
		g_callbacks = *callbacks;
	}

	/*
	 * awave_core.cpp uses the ABI convention: non-zero == success, zero == failure.
	 * Bridge initialization itself succeeds.  Game loading below still returns
	 * failure until this placeholder is replaced by a real Flycast adapter.
	 */
	return 1;
}

void fbfc_shutdown(void)
{
	g_loaded = 0;
	memset(&g_callbacks, 0, sizeof(g_callbacks));
}

int fbfc_load_game(const FbneoFlycastGameInfo* game)
{
	/*
	 * Compile/bring-up placeholder.
	 *
	 * This file does not execute Flycast, render frames, or consume the ROM
	 * package.  It deliberately fails at load time so the frontend does not
	 * enter a fake-success path with no emulation running.
	 */
	if (game == NULL || game->roms == NULL || game->rom_count <= 0) {
		return 0;
	}

	g_loaded = 0;
	return 0;
}

void fbfc_unload_game(void)
{
	g_loaded = 0;
}

void fbfc_reset(void)
{
}

int fbfc_run_frame(const FbneoFlycastInputState* input)
{
	(void)input;
	return 0;
}

int fbfc_serialize_size(void)
{
	return 0;
}

int fbfc_serialize(void* data, int size)
{
	(void)data;
	(void)size;
	return 0;
}

int fbfc_unserialize(const void* data, int size)
{
	(void)data;
	(void)size;
	return 0;
}

int fbfc_get_av_info(int* width, int* height, int* sample_rate, double* fps)
{
	if (width) *width = g_width;
	if (height) *height = g_height;
	if (sample_rate) *sample_rate = g_sample_rate;
	if (fps) *fps = g_fps;
	return 1;
}

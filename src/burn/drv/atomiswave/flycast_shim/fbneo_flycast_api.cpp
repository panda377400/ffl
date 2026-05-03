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
    if (callbacks) g_callbacks = *callbacks;
    return 0;
}

void fbfc_shutdown(void)
{
    g_loaded = 0;
    memset(&g_callbacks, 0, sizeof(g_callbacks));
}

int fbfc_load_game(const FbneoFlycastGameInfo* game)
{
    (void)game;
    /*
       Compile/bring-up placeholder.

       This file exists so make can build fbneo_flycast_api.o.
       Replace this implementation with the static vendor/flycast adapter once
       Flycast is added under vendor/flycast and its libretro entry symbols are
       renamed to flycast_retro_*.
    */
    g_loaded = 1;
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

    if (!g_loaded)
        return 1;

    /* No video/audio is produced by this placeholder. */
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

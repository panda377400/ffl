#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum FbneoFlycastPlatform {
    FBFC_PLATFORM_ATOMISWAVE = 0,
    FBFC_PLATFORM_NAOMI      = 1,
    FBFC_PLATFORM_NAOMI2     = 2,
    FBFC_PLATFORM_DREAMCAST  = 3,
};

enum FbneoFlycastInputType {
    FBFC_INPUT_DIGITAL = 0,
    FBFC_INPUT_LIGHTGUN,
    FBFC_INPUT_RACING,
    FBFC_INPUT_BLOCKPONG,
    FBFC_INPUT_DREAMCAST_PAD,
};

struct FbneoFlycastRomEntry {
    const char* filename;
    const uint8_t* data;
    size_t size;
};

struct FbneoFlycastGameInfo {
    const char* driver_name;
    const char* zip_name;
    const char* bios_zip_name;
    const char* system_name;
    const char* save_dir;
    int platform;
    int input_type;
    const FbneoFlycastRomEntry* roms;
    int rom_count;
};

struct FbneoFlycastVideoFrame {
    const void* pixels;
    int width;
    int height;
    int pitch_bytes;
    uint32_t texture;
    uint32_t framebuffer;
    int upside_down;
    int valid_hw;
    int valid_cpu;
};

struct FbneoFlycastInputState {
    uint32_t pad[4];
    int16_t analog[4][8];
    int16_t analog_button[4][4];
    int16_t gun_x[4];
    int16_t gun_y[4];
    uint8_t gun_offscreen[4];
    uint8_t gun_reload[4];
};

typedef void (*FbneoFlycastAudioCallback)(const int16_t* stereo, int frames, void* user);
typedef void (*FbneoFlycastVideoCallback)(const FbneoFlycastVideoFrame* frame, void* user);

struct FbneoFlycastCallbacks {
    FbneoFlycastAudioCallback audio;
    FbneoFlycastVideoCallback video;
    void* user;
};

int fbfc_init(const FbneoFlycastCallbacks* callbacks);
void fbfc_shutdown(void);
int fbfc_load_game(const FbneoFlycastGameInfo* game);
void fbfc_unload_game(void);
void fbfc_reset(void);
int fbfc_run_frame(const FbneoFlycastInputState* input);
int fbfc_serialize_size(void);
int fbfc_serialize(void* data, int size);
int fbfc_unserialize(const void* data, int size);
int fbfc_get_av_info(int* width, int* height, int* sample_rate, double* fps);

#ifdef __cplusplus
}
#endif

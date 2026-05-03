#include "fbneo_flycast_api.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <string>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#define FBFC_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define FBFC_MKDIR(path) mkdir(path, 0755)
#endif

#if defined(__has_include)
#  if __has_include("../../../../../vendor/flycast/core/deps/libretro-common/include/libretro.h")
#    include "../../../../../vendor/flycast/core/deps/libretro-common/include/libretro.h"
#  elif __has_include("../../../../../vendor/flycast/core/libretro-common/include/libretro.h")
#    include "../../../../../vendor/flycast/core/libretro-common/include/libretro.h"
#  elif __has_include(<libretro.h>)
#    include <libretro.h>
#  else
#    error "libretro.h not found. Add vendor/flycast/core/deps/libretro-common/include to the include path."
#  endif
#else
#  include <libretro.h>
#endif

extern "C" {
void flycast_retro_set_environment(retro_environment_t cb);
void flycast_retro_set_video_refresh(retro_video_refresh_t cb);
void flycast_retro_set_audio_sample(retro_audio_sample_t cb);
void flycast_retro_set_audio_sample_batch(retro_audio_sample_batch_t cb);
void flycast_retro_set_input_poll(retro_input_poll_t cb);
void flycast_retro_set_input_state(retro_input_state_t cb);
void flycast_retro_init(void);
void flycast_retro_deinit(void);
unsigned flycast_retro_api_version(void);
void flycast_retro_get_system_info(struct retro_system_info* info);
void flycast_retro_get_system_av_info(struct retro_system_av_info* info);
void flycast_retro_set_controller_port_device(unsigned port, unsigned device);
void flycast_retro_reset(void);
void flycast_retro_run(void);
size_t flycast_retro_serialize_size(void);
bool flycast_retro_serialize(void* data, size_t size);
bool flycast_retro_unserialize(const void* data, size_t size);
bool flycast_retro_load_game(const struct retro_game_info* game);
void flycast_retro_unload_game(void);
}

static FbneoFlycastCallbacks g_callbacks;
static FbneoFlycastInputState g_input;
static int g_inited = 0;
static int g_loaded = 0;
static int g_width = 640;
static int g_height = 480;
static int g_sample_rate = 44100;
static double g_fps = 60.0;
static std::string g_runtime_dir = "fbneo_flycast_runtime";
static std::string g_system_dir = "fbneo_flycast_runtime/system";
static std::string g_save_dir = "fbneo_flycast_runtime/save";
static std::string g_content_dir = "fbneo_flycast_runtime/content";
static std::string g_game_zip_path;

static void fbfc_log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "atomiswave/flycast: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static void make_dir(const std::string& path)
{
    FBFC_MKDIR(path.c_str());
}

static void ensure_runtime_dirs(void)
{
    make_dir(g_runtime_dir);
    make_dir(g_system_dir);
    make_dir(g_system_dir + "/dc");
    make_dir(g_save_dir);
    make_dir(g_content_dir);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t size)
{
    crc = ~crc;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            const uint32_t mask = -(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static void wr16(FILE* f, uint16_t v)
{
    fputc((int)(v & 0xff), f);
    fputc((int)((v >> 8) & 0xff), f);
}

static void wr32(FILE* f, uint32_t v)
{
    wr16(f, (uint16_t)(v & 0xffff));
    wr16(f, (uint16_t)((v >> 16) & 0xffff));
}

struct ZipCentralEntry {
    std::string name;
    uint32_t crc;
    uint32_t size;
    uint32_t local_offset;
};

static bool write_store_zip(const char* path, const FbneoFlycastRomEntry* roms, int rom_count)
{
    FILE* f = fopen(path, "wb");
    if (!f) {
        fbfc_log("failed to create temporary content zip: %s", path);
        return false;
    }

    std::vector<ZipCentralEntry> central;
    central.reserve((size_t)rom_count);

    for (int i = 0; i < rom_count; i++) {
        const FbneoFlycastRomEntry& r = roms[i];
        if (!r.filename || !r.data || r.size == 0)
            continue;

        ZipCentralEntry ze;
        ze.name = r.filename;
        ze.crc = crc32_update(0, r.data, r.size);
        ze.size = (uint32_t)r.size;
        ze.local_offset = (uint32_t)ftell(f);

        wr32(f, 0x04034b50u);          // local file header
        wr16(f, 20);                   // version needed
        wr16(f, 0);                    // flags
        wr16(f, 0);                    // method: store
        wr16(f, 0);                    // mod time
        wr16(f, 0);                    // mod date
        wr32(f, ze.crc);
        wr32(f, ze.size);
        wr32(f, ze.size);
        wr16(f, (uint16_t)ze.name.size());
        wr16(f, 0);                    // extra len
        fwrite(ze.name.data(), 1, ze.name.size(), f);
        fwrite(r.data, 1, r.size, f);
        central.push_back(ze);
    }

    const uint32_t central_offset = (uint32_t)ftell(f);
    for (size_t i = 0; i < central.size(); i++) {
        const ZipCentralEntry& ze = central[i];
        wr32(f, 0x02014b50u);          // central directory header
        wr16(f, 20);                   // version made by
        wr16(f, 20);                   // version needed
        wr16(f, 0);                    // flags
        wr16(f, 0);                    // method: store
        wr16(f, 0);                    // mod time
        wr16(f, 0);                    // mod date
        wr32(f, ze.crc);
        wr32(f, ze.size);
        wr32(f, ze.size);
        wr16(f, (uint16_t)ze.name.size());
        wr16(f, 0);                    // extra len
        wr16(f, 0);                    // comment len
        wr16(f, 0);                    // disk start
        wr16(f, 0);                    // internal attr
        wr32(f, 0);                    // external attr
        wr32(f, ze.local_offset);
        fwrite(ze.name.data(), 1, ze.name.size(), f);
    }

    const uint32_t central_size = (uint32_t)ftell(f) - central_offset;
    wr32(f, 0x06054b50u);              // end central dir
    wr16(f, 0);                        // disk
    wr16(f, 0);                        // central disk
    wr16(f, (uint16_t)central.size());
    wr16(f, (uint16_t)central.size());
    wr32(f, central_size);
    wr32(f, central_offset);
    wr16(f, 0);                        // comment len

    fclose(f);
    return !central.empty();
}

static bool copy_file_if_exists(const char* src, const char* dst)
{
    FILE* in = fopen(src, "rb");
    if (!in)
        return false;
    FILE* out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    return true;
}

static void stage_bios_files(void)
{
    ensure_runtime_dirs();
    copy_file_if_exists("roms/awbios.zip", "fbneo_flycast_runtime/system/dc/awbios.zip");
    copy_file_if_exists("roms/naomi.zip", "fbneo_flycast_runtime/system/dc/naomi.zip");
    copy_file_if_exists("roms/dc/awbios.zip", "fbneo_flycast_runtime/system/dc/awbios.zip");
    copy_file_if_exists("roms/dc/naomi.zip", "fbneo_flycast_runtime/system/dc/naomi.zip");
}

static void lr_video_cb(const void* data, unsigned width, unsigned height, size_t pitch)
{
#ifdef RETRO_HW_FRAME_BUFFER_VALID
    if (!g_callbacks.video || !data || data == RETRO_HW_FRAME_BUFFER_VALID)
        return;
#else
    if (!g_callbacks.video || !data)
        return;
#endif

    FbneoFlycastVideoFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.pixels = data;
    frame.width = (int)width;
    frame.height = (int)height;
    frame.pitch_bytes = (int)pitch;
    frame.valid_cpu = 1;
    frame.valid_hw = 0;
    g_callbacks.video(&frame, g_callbacks.user);
}

static void lr_audio_cb(int16_t left, int16_t right)
{
    int16_t sample[2] = { left, right };
    if (g_callbacks.audio)
        g_callbacks.audio(sample, 1, g_callbacks.user);
}

static size_t lr_audio_batch_cb(const int16_t* data, size_t frames)
{
    if (g_callbacks.audio && data && frames > 0)
        g_callbacks.audio(data, (int)frames, g_callbacks.user);
    return frames;
}

static void lr_input_poll_cb(void)
{
}

static int16_t joypad_state(unsigned port, unsigned id)
{
    if (port >= 4 || id >= 32)
        return 0;
    return (g_input.pad[port] & (1u << id)) ? 1 : 0;
}

static int16_t lr_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)index;
    if (port >= 4)
        return 0;

    if (device == RETRO_DEVICE_JOYPAD)
        return joypad_state(port, id);

    if (device == RETRO_DEVICE_ANALOG) {
        if (id < 8)
            return g_input.analog[port][id];
        return 0;
    }

    if (device == RETRO_DEVICE_LIGHTGUN) {
        if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X) return g_input.gun_x[port];
        if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y) return g_input.gun_y[port];
        if (id == RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) return g_input.gun_offscreen[port] ? 1 : 0;
        if (id == RETRO_DEVICE_ID_LIGHTGUN_RELOAD) return g_input.gun_reload[port] ? 1 : 0;
        return joypad_state(port, id);
    }

    return 0;
}

static void lr_log_cb(enum retro_log_level level, const char* fmt, ...)
{
    (void)level;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "flycast: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static bool lr_environment_cb(unsigned cmd, void* data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *(const char**)data = g_system_dir.c_str();
        return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char**)data = g_save_dir.c_str();
        return true;
#ifdef RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY
    case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
        *(const char**)data = g_runtime_dir.c_str();
        return true;
#endif
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return true;
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    case RETRO_ENVIRONMENT_SET_MESSAGE:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
        return true;
#ifdef RETRO_ENVIRONMENT_SET_CORE_OPTIONS
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
        return true;
#endif
#ifdef RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
        return true;
#endif
#ifdef RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
        return true;
#endif
#ifdef RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
        return true;
#endif
#ifdef RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK:
        return true;
#endif
#ifdef RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *(unsigned*)data = 2;
        return true;
#endif
    case RETRO_ENVIRONMENT_GET_VARIABLE:
        if (data) {
            struct retro_variable* var = (struct retro_variable*)data;
            var->value = NULL;
        }
        return false;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool*)data = false;
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool*)data = true;
        return true;
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        if (data) {
            struct retro_log_callback* log = (struct retro_log_callback*)data;
            log->log = lr_log_cb;
            return true;
        }
        return false;
    default:
        return false;
    }
}

int fbfc_init(const FbneoFlycastCallbacks* callbacks)
{
    memset(&g_callbacks, 0, sizeof(g_callbacks));
    memset(&g_input, 0, sizeof(g_input));
    if (callbacks)
        g_callbacks = *callbacks;

    ensure_runtime_dirs();

    flycast_retro_set_environment(lr_environment_cb);
    flycast_retro_set_video_refresh(lr_video_cb);
    flycast_retro_set_audio_sample(lr_audio_cb);
    flycast_retro_set_audio_sample_batch(lr_audio_batch_cb);
    flycast_retro_set_input_poll(lr_input_poll_cb);
    flycast_retro_set_input_state(lr_input_state_cb);
    flycast_retro_init();

    g_inited = 1;
    g_loaded = 0;
    return 1;
}

void fbfc_shutdown(void)
{
    if (g_loaded)
        flycast_retro_unload_game();
    if (g_inited)
        flycast_retro_deinit();
    g_loaded = 0;
    g_inited = 0;
    memset(&g_callbacks, 0, sizeof(g_callbacks));
}

int fbfc_load_game(const FbneoFlycastGameInfo* game)
{
    if (!g_inited || !game || !game->roms || game->rom_count <= 0)
        return 0;

    stage_bios_files();

    std::string zip_name;
    if (game->zip_name && game->zip_name[0])
        zip_name = game->zip_name;
    if (zip_name.empty()) {
        zip_name = (game->driver_name && game->driver_name[0]) ? game->driver_name : "fbneo_flycast_game";
        if (zip_name.find('.') == std::string::npos)
            zip_name += ".zip";
    }
    g_game_zip_path = g_content_dir + "/" + zip_name;

    if (!write_store_zip(g_game_zip_path.c_str(), game->roms, game->rom_count)) {
        fbfc_log("failed to build content zip for %s", game->driver_name ? game->driver_name : "<unknown>");
        return 0;
    }

    struct retro_game_info gi;
    memset(&gi, 0, sizeof(gi));
    gi.path = g_game_zip_path.c_str();
    gi.data = NULL;
    gi.size = 0;
    gi.meta = NULL;

    if (!flycast_retro_load_game(&gi)) {
        fbfc_log("flycast_retro_load_game failed: %s", g_game_zip_path.c_str());
        g_loaded = 0;
        return 0;
    }

    struct retro_system_av_info av;
    memset(&av, 0, sizeof(av));
    flycast_retro_get_system_av_info(&av);
    if (av.geometry.base_width) g_width = (int)av.geometry.base_width;
    if (av.geometry.base_height) g_height = (int)av.geometry.base_height;
    if (av.timing.sample_rate) g_sample_rate = (int)av.timing.sample_rate;
    if (av.timing.fps) g_fps = av.timing.fps;

    g_loaded = 1;
    return 1;
}

void fbfc_unload_game(void)
{
    if (g_loaded)
        flycast_retro_unload_game();
    g_loaded = 0;
}

void fbfc_reset(void)
{
    if (g_loaded)
        flycast_retro_reset();
}

int fbfc_run_frame(const FbneoFlycastInputState* input)
{
    if (!g_loaded)
        return 0;
    if (input)
        g_input = *input;
    flycast_retro_run();
    return 1;
}

int fbfc_serialize_size(void)
{
    return g_loaded ? (int)flycast_retro_serialize_size() : 0;
}

int fbfc_serialize(void* data, int size)
{
    if (!g_loaded || !data || size <= 0)
        return 0;
    return flycast_retro_serialize(data, (size_t)size) ? 1 : 0;
}

int fbfc_unserialize(const void* data, int size)
{
    if (!g_loaded || !data || size <= 0)
        return 0;
    return flycast_retro_unserialize(data, (size_t)size) ? 1 : 0;
}

int fbfc_get_av_info(int* width, int* height, int* sample_rate, double* fps)
{
    if (width) *width = g_width;
    if (height) *height = g_height;
    if (sample_rate) *sample_rate = g_sample_rate;
    if (fps) *fps = g_fps;
    return 1;
}

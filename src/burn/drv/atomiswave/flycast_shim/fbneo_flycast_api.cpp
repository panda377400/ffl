#include "fbneo_flycast_api.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#define FBFC_MKDIR(path) _mkdir(path)
#if defined(__MINGW32__) && !defined(FBNEO_FLYCAST_MINGW_STAT64I32_COMPAT)
#define FBNEO_FLYCAST_MINGW_STAT64I32_COMPAT 1
#include <sys/stat.h>
extern "C" int stat64i32(const char* path, struct _stat64i32* st)
{
    return _stat64i32(path, st);
}
#endif
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#define FBFC_MKDIR(path) mkdir(path, 0755)
#endif

#if defined(__has_include)
# if __has_include("../../../../../vendor/flycast/core/deps/libretro-common/include/libretro.h")
#  include "../../../../../vendor/flycast/core/deps/libretro-common/include/libretro.h"
# elif __has_include("../../../../../vendor/flycast/core/libretro-common/include/libretro.h")
#  include "../../../../../vendor/flycast/core/libretro-common/include/libretro.h"
# elif __has_include(<libretro.h>)
#  include <libretro.h>
# else
#  error "libretro.h not found. Add vendor/flycast/core/deps/libretro-common/include to the include path."
# endif
#else
# include <libretro.h>
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
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
static retro_keyboard_event_t g_keyboard_cb = NULL;
static struct retro_disk_control_callback g_disk_cb;
#ifdef RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE
static struct retro_disk_control_ext_callback g_disk_ext_cb;
#endif

static void make_dir_quiet(const std::string& path)
{
    if (path.empty()) return;
    FBFC_MKDIR(path.c_str());
}

static void ensure_runtime_dirs(void)
{
    make_dir_quiet(g_runtime_dir);
    make_dir_quiet(g_system_dir);
    make_dir_quiet(g_system_dir + "/dc");
    make_dir_quiet(g_save_dir);
    make_dir_quiet(g_content_dir);
}

static void reset_debug_log(void)
{
    ensure_runtime_dirs();
    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "wb");
    if (f) {
        fputs("fbneo/flycast debug log\n", f);
        fclose(f);
    }
}

static void fbfc_log(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "atomiswave/flycast: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);

    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "ab");
    if (!f) f = fopen("fbfc_debug.log", "ab");
    if (f) {
        fprintf(f, "atomiswave/flycast: ");
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fputc('\n', f);
        fclose(f);
    }
}

#if defined(_WIN32)
/*
 * v14: normal Flycast VMEM/fault-handler path restored.
 * The old GNU ld --wrap test functions were removed because v12 removes the
 * matching --wrap linker flags; keeping calls to __real_* without --wrap causes
 * unresolved references at link time.
 *
 * The watchdog/context dump below remains active. If retro_init() hangs after
 * the VMEM BASE line, the adapter logs RIP/RSP and leaves the stuck core
 * thread alive so gdb can attach and collect a real backtrace. Do not click
 * the FBNeo error dialog before running gdb_v14_attach_latest.bat.
 */

static PVOID g_fbfc_veh = NULL;
static LONG WINAPI fbfc_vectored_exception_handler(PEXCEPTION_POINTERS p)
{
    if (p && p->ExceptionRecord) {
        ULONG_PTR access_type = 0;
        ULONG_PTR access_addr = 0;
        if (p->ExceptionRecord->NumberParameters > 0) access_type = p->ExceptionRecord->ExceptionInformation[0];
        if (p->ExceptionRecord->NumberParameters > 1) access_addr = p->ExceptionRecord->ExceptionInformation[1];
#if defined(_M_X64) || defined(__x86_64__)
        fbfc_log("WINDOWS EXCEPTION code=%08lx address=%p access_type=%llu access_addr=%p rip=%p rsp=%p tid=%lu",
            (unsigned long)p->ExceptionRecord->ExceptionCode,
            p->ExceptionRecord->ExceptionAddress,
            (unsigned long long)access_type,
            (void*)access_addr,
            p->ContextRecord ? (void*)p->ContextRecord->Rip : NULL,
            p->ContextRecord ? (void*)p->ContextRecord->Rsp : NULL,
            (unsigned long)GetCurrentThreadId());
#else
        fbfc_log("WINDOWS EXCEPTION code=%08lx address=%p access_type=%llu access_addr=%p tid=%lu",
            (unsigned long)p->ExceptionRecord->ExceptionCode,
            p->ExceptionRecord->ExceptionAddress,
            (unsigned long long)access_type,
            (void*)access_addr,
            (unsigned long)GetCurrentThreadId());
#endif
    } else {
        fbfc_log("WINDOWS EXCEPTION tid=%lu", (unsigned long)GetCurrentThreadId());
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

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
        fbfc_log("failed to create temporary content zip: %s errno=%d", path ? path : "", errno);
        return false;
    }

    std::vector<ZipCentralEntry> central;
    central.reserve((size_t)rom_count);

    for (int i = 0; i < rom_count; i++) {
        const FbneoFlycastRomEntry& r = roms[i];
        fbfc_log("rom[%d] name=%s size=%u data=%p", i, r.filename ? r.filename : "", (unsigned)r.size, (const void*)r.data);
        if (!r.filename || !r.filename[0] || !r.data || r.size == 0) {
            fbfc_log("skip invalid rom[%d]", i);
            continue;
        }
        if (r.size > 0xffffffffu) {
            fbfc_log("rom too large for simple zip writer: %s", r.filename);
            fclose(f);
            return false;
        }

        ZipCentralEntry ze;
        ze.name = r.filename;
        ze.crc = crc32_update(0, r.data, r.size);
        ze.size = (uint32_t)r.size;
        ze.local_offset = (uint32_t)ftell(f);

        wr32(f, 0x04034b50u);
        wr16(f, 20);
        wr16(f, 0);
        wr16(f, 0);
        wr16(f, 0);
        wr16(f, 0);
        wr32(f, ze.crc);
        wr32(f, ze.size);
        wr32(f, ze.size);
        wr16(f, (uint16_t)ze.name.size());
        wr16(f, 0);
        fwrite(ze.name.data(), 1, ze.name.size(), f);
        fwrite(r.data, 1, r.size, f);
        central.push_back(ze);
    }

    if (central.empty()) {
        fbfc_log("content zip would be empty");
        fclose(f);
        return false;
    }

    const uint32_t central_offset = (uint32_t)ftell(f);
    for (size_t i = 0; i < central.size(); i++) {
        const ZipCentralEntry& ze = central[i];
        wr32(f, 0x02014b50u);
        wr16(f, 20);
        wr16(f, 20);
        wr16(f, 0);
        wr16(f, 0);
        wr16(f, 0);
        wr16(f, 0);
        wr32(f, ze.crc);
        wr32(f, ze.size);
        wr32(f, ze.size);
        wr16(f, (uint16_t)ze.name.size());
        wr16(f, 0);
        wr16(f, 0);
        wr16(f, 0);
        wr16(f, 0);
        wr32(f, 0);
        wr32(f, ze.local_offset);
        fwrite(ze.name.data(), 1, ze.name.size(), f);
    }

    const uint32_t central_size = (uint32_t)ftell(f) - central_offset;
    wr32(f, 0x06054b50u);
    wr16(f, 0);
    wr16(f, 0);
    wr16(f, (uint16_t)central.size());
    wr16(f, (uint16_t)central.size());
    wr32(f, central_size);
    wr32(f, central_offset);
    wr16(f, 0);

    const int close_ret = fclose(f);
    fbfc_log("wrote content zip: %s files=%u close=%d", path ? path : "", (unsigned)central.size(), close_ret);
    return close_ret == 0;
}

static bool copy_file_if_exists(const char* src, const char* dst)
{
    FILE* in = fopen(src, "rb");
    if (!in) return false;

    FILE* out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        fwrite(buf, 1, n, out);
    }
    fclose(in);
    fclose(out);
    fbfc_log("staged bios: %s -> %s", src, dst);
    return true;
}

static void stage_bios_files(const FbneoFlycastGameInfo* game)
{
    ensure_runtime_dirs();
    const char* bios = (game && game->bios_zip_name && game->bios_zip_name[0]) ? game->bios_zip_name : "awbios.zip";

    std::string src1 = std::string("roms/") + bios;
    std::string src2 = std::string("roms/dc/") + bios;
    std::string dst1 = g_system_dir + "/" + bios;
    std::string dst2 = g_system_dir + "/dc/" + bios;

    copy_file_if_exists(src1.c_str(), dst1.c_str());
    copy_file_if_exists(src1.c_str(), dst2.c_str());
    copy_file_if_exists(src2.c_str(), dst1.c_str());
    copy_file_if_exists(src2.c_str(), dst2.c_str());
    copy_file_if_exists("roms/awbios.zip", "fbneo_flycast_runtime/system/awbios.zip");
    copy_file_if_exists("roms/awbios.zip", "fbneo_flycast_runtime/system/dc/awbios.zip");
    copy_file_if_exists("roms/naomi.zip", "fbneo_flycast_runtime/system/naomi.zip");
    copy_file_if_exists("roms/naomi.zip", "fbneo_flycast_runtime/system/dc/naomi.zip");
    copy_file_if_exists("roms/dc/awbios.zip", "fbneo_flycast_runtime/system/awbios.zip");
    copy_file_if_exists("roms/dc/awbios.zip", "fbneo_flycast_runtime/system/dc/awbios.zip");
    copy_file_if_exists("roms/dc/naomi.zip", "fbneo_flycast_runtime/system/naomi.zip");
    copy_file_if_exists("roms/dc/naomi.zip", "fbneo_flycast_runtime/system/dc/naomi.zip");
}

static void lr_video_cb(const void* data, unsigned width, unsigned height, size_t pitch)
{
#ifdef RETRO_HW_FRAME_BUFFER_VALID
    if (!g_callbacks.video || !data || data == RETRO_HW_FRAME_BUFFER_VALID) return;
#else
    if (!g_callbacks.video || !data) return;
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
    if (g_callbacks.audio) g_callbacks.audio(sample, 1, g_callbacks.user);
}

static size_t lr_audio_batch_cb(const int16_t* data, size_t frames)
{
    if (g_callbacks.audio && data && frames > 0) {
        g_callbacks.audio(data, (int)frames, g_callbacks.user);
    }
    return frames;
}

static void lr_input_poll_cb(void)
{
}

static int16_t joypad_state(unsigned port, unsigned id)
{
    if (port >= 4) return 0;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t)(g_input.pad[port] & 0xffffu);
    if (id >= 32) return 0;
    return (g_input.pad[port] & (1u << id)) ? 1 : 0;
}

static int16_t lr_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port >= 4) return 0;

    if (device == RETRO_DEVICE_JOYPAD) return joypad_state(port, id);

    if (device == RETRO_DEVICE_ANALOG) {
        unsigned axis = index * 2 + id;
        if (axis < 8) return g_input.analog[port][axis];
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

    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "ab");
    if (!f) f = fopen("fbfc_debug.log", "ab");
    if (f) {
        fprintf(f, "flycast: ");
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fclose(f);
    }

    fprintf(stderr, "flycast: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
}

static void update_av_info(const struct retro_system_av_info* av)
{
    if (!av) return;
    if (av->geometry.base_width) g_width = (int)av->geometry.base_width;
    if (av->geometry.base_height) g_height = (int)av->geometry.base_height;
    if (av->timing.sample_rate) g_sample_rate = (int)av->timing.sample_rate;
    if (av->timing.fps) g_fps = av->timing.fps;
    fbfc_log("av info: %dx%d sample_rate=%d fps=%.6f", g_width, g_height, g_sample_rate, g_fps);
}

static retro_perf_tick_t lr_perf_get_counter(void)
{
#if defined(_WIN32)
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (retro_perf_tick_t)t.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (retro_perf_tick_t)tv.tv_sec * 1000000ull + (retro_perf_tick_t)tv.tv_usec;
#endif
}

static retro_time_t lr_perf_get_time_usec(void)
{
#if defined(_WIN32)
    LARGE_INTEGER t;
    LARGE_INTEGER f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    if (f.QuadPart == 0) return 0;
    return (retro_time_t)((t.QuadPart * 1000000ll) / f.QuadPart);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (retro_time_t)tv.tv_sec * 1000000ll + (retro_time_t)tv.tv_usec;
#endif
}

static uint64_t lr_get_cpu_features(void)
{
    uint64_t r = 0;
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#ifdef RETRO_SIMD_MMX
    r |= RETRO_SIMD_MMX;
#endif
#ifdef RETRO_SIMD_CMOV
    r |= RETRO_SIMD_CMOV;
#endif
#ifdef RETRO_SIMD_SSE
    r |= RETRO_SIMD_SSE;
#endif
#ifdef RETRO_SIMD_SSE2
    r |= RETRO_SIMD_SSE2;
#endif
#if defined(__SSE3__) && defined(RETRO_SIMD_SSE3)
    r |= RETRO_SIMD_SSE3;
#endif
#if defined(__SSSE3__) && defined(RETRO_SIMD_SSSE3)
    r |= RETRO_SIMD_SSSE3;
#endif
#if defined(__SSE4_1__) && defined(RETRO_SIMD_SSE4)
    r |= RETRO_SIMD_SSE4;
#endif
#if defined(__SSE4_2__) && defined(RETRO_SIMD_SSE42)
    r |= RETRO_SIMD_SSE42;
#endif
#if defined(__AVX__) && defined(RETRO_SIMD_AVX)
    r |= RETRO_SIMD_AVX;
#endif
#if defined(__AVX2__) && defined(RETRO_SIMD_AVX2)
    r |= RETRO_SIMD_AVX2;
#endif
#endif
    return r;
}

static void lr_perf_register(struct retro_perf_counter* counter)
{
    if (counter) counter->registered = true;
}

static void lr_perf_start(struct retro_perf_counter* counter)
{
    if (counter) counter->start = lr_perf_get_counter();
}

static void lr_perf_stop(struct retro_perf_counter* counter)
{
    if (!counter) return;
    retro_perf_tick_t now = lr_perf_get_counter();
    if (now >= counter->start) counter->total += now - counter->start;
    counter->call_cnt++;
}

static void lr_perf_log(void)
{
}

static void lr_keyboard_dummy(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
    (void)down;
    (void)keycode;
    (void)character;
    (void)key_modifiers;
}

static bool lr_environment_cb(unsigned cmd, void* data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        if (data) *(const char**)data = g_system_dir.c_str();
        return true;

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        if (data) *(const char**)data = g_save_dir.c_str();
        return true;

#ifdef RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY
    case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:
        if (data) *(const char**)data = g_content_dir.c_str();
        return true;
#elif defined(RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY)
    case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
        if (data) *(const char**)data = g_content_dir.c_str();
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_GET_LIBRETRO_PATH
    case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
        if (data) *(const char**)data = "fbneo_flycast_static";
        return true;
#endif

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        if (data) {
            g_pixel_format = *(enum retro_pixel_format*)data;
            fbfc_log("pixel format requested: %d", (int)g_pixel_format);
        }
        return true;

    case RETRO_ENVIRONMENT_SET_GEOMETRY:
        if (data) {
            const struct retro_game_geometry* geo = (const struct retro_game_geometry*)data;
            if (geo->base_width) g_width = (int)geo->base_width;
            if (geo->base_height) g_height = (int)geo->base_height;
            fbfc_log("geometry set: %dx%d", g_width, g_height);
        }
        return true;

    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
        update_av_info((const struct retro_system_av_info*)data);
        return true;

    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    case RETRO_ENVIRONMENT_SET_MESSAGE:
        return true;

#ifdef RETRO_ENVIRONMENT_SET_ROTATION
    case RETRO_ENVIRONMENT_SET_ROTATION:
        return true;
#endif

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
#ifdef RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
        return true;
#endif
#ifdef RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        if (data) *(unsigned*)data = 2;
        return true;
#endif

    case RETRO_ENVIRONMENT_GET_VARIABLE:
        if (data) {
            struct retro_variable* var = (struct retro_variable*)data;
            fbfc_log("get variable: %s", var->key ? var->key : "");
            var->value = NULL;
        }
        return false;

    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        if (data) *(bool*)data = false;
        return true;

    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        if (data) *(bool*)data = true;
        return true;

    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        if (data) {
            struct retro_log_callback* log = (struct retro_log_callback*)data;
            log->log = lr_log_cb;
            return true;
        }
        return false;

    case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
        if (data) {
            struct retro_perf_callback* perf = (struct retro_perf_callback*)data;
            memset(perf, 0, sizeof(*perf));
            perf->get_time_usec = lr_perf_get_time_usec;
            perf->get_cpu_features = lr_get_cpu_features;
            perf->get_perf_counter = lr_perf_get_counter;
            perf->perf_register = lr_perf_register;
            perf->perf_start = lr_perf_start;
            perf->perf_stop = lr_perf_stop;
            perf->perf_log = lr_perf_log;
            fbfc_log("perf interface supplied");
            return true;
        }
        return false;

    case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
        if (data) {
            const struct retro_keyboard_callback* kb = (const struct retro_keyboard_callback*)data;
            g_keyboard_cb = kb->callback ? kb->callback : lr_keyboard_dummy;
            fbfc_log("keyboard callback registered");
        } else {
            g_keyboard_cb = NULL;
        }
        return true;

    case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:
        memset(&g_disk_cb, 0, sizeof(g_disk_cb));
        if (data) memcpy(&g_disk_cb, data, sizeof(g_disk_cb));
        fbfc_log("disk control interface registered");
        return true;

#ifdef RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION
    case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION:
        if (data) *(unsigned*)data = 0;
        fbfc_log("disk control interface version requested: 0");
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE
    case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE:
        memset(&g_disk_ext_cb, 0, sizeof(g_disk_ext_cb));
        if (data) memcpy(&g_disk_ext_cb, data, sizeof(g_disk_ext_cb));
        fbfc_log("disk control ext interface registered");
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_GET_INPUT_BITMASKS
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        fbfc_log("input bitmasks supported");
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_GET_OVERSCAN
    case RETRO_ENVIRONMENT_GET_OVERSCAN:
        if (data) *(bool*)data = false;
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_SET_HW_RENDER
    case RETRO_ENVIRONMENT_SET_HW_RENDER:
        fbfc_log("core requested HW render; this adapter does not expose libretro HW render yet");
        return false;
#endif

#ifdef RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER
    case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
        return false;
#endif

#ifdef RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK
    case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK:
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK
    case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK:
        return false;
#endif

#ifdef RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL
    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS
    case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE
    case RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE:
        if (data) *(float*)data = 60.0f;
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE
    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE:
        if (data) *(int*)data = 3;
        return true;
#endif

#ifdef RETRO_ENVIRONMENT_GET_LANGUAGE
    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        if (data) {
#ifdef RETRO_LANGUAGE_ENGLISH
            *(unsigned*)data = RETRO_LANGUAGE_ENGLISH;
#else
            *(unsigned*)data = 0;
#endif
        }
        return true;
#endif

    default:
        fbfc_log("unhandled environment cmd: %u", cmd);
        return false;
    }
}

#if defined(_WIN32)
enum FbfcCoreOp {
    FBFC_CORE_NONE = 0,
    FBFC_CORE_INIT,
    FBFC_CORE_DEINIT,
    FBFC_CORE_LOAD_GAME,
    FBFC_CORE_UNLOAD_GAME,
    FBFC_CORE_RESET,
    FBFC_CORE_RUN,
    FBFC_CORE_SERIALIZE_SIZE,
    FBFC_CORE_SERIALIZE,
    FBFC_CORE_UNSERIALIZE,
    FBFC_CORE_STOP
};

struct FbfcCoreRequest {
    int op;
    const struct retro_game_info* game;
    void* data;
    const void* cdata;
    size_t size;
    int iret;
    size_t sret;
};

static HANDLE g_core_thread = NULL;
static HANDLE g_core_req_event = NULL;
static HANDLE g_core_done_event = NULL;
static CRITICAL_SECTION g_core_cs;
static int g_core_cs_ready = 0;
static DWORD g_core_tid = 0;
static int g_core_stuck = 0;
static int g_core_timeout_op = FBFC_CORE_NONE;
static FbfcCoreRequest g_core_req;


static DWORD fbfc_core_timeout_ms(int op)
{
    switch (op) {
    case FBFC_CORE_INIT:
        return 15000; /* retro_init should return quickly; current bug hangs here */
    case FBFC_CORE_LOAD_GAME:
        return 60000;
    case FBFC_CORE_RUN:
        return 5000;
    case FBFC_CORE_STOP:
        return 3000;
    default:
        return 15000;
    }
}

static void fbfc_dump_core_thread_context(const char* reason)
{
    if (!g_core_thread) {
        fbfc_log("corethread: context dump skipped, no thread (%s)", reason ? reason : "");
        return;
    }

    DWORD suspend_ret = SuspendThread(g_core_thread);
    if (suspend_ret == (DWORD)-1) {
        fbfc_log("corethread: SuspendThread failed for context dump (%s) err=%lu", reason ? reason : "", (unsigned long)GetLastError());
        return;
    }

#if defined(_M_X64) || defined(__x86_64__)
    CONTEXT ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(g_core_thread, &ctx)) {
        fbfc_log("corethread: TIMEOUT_CONTEXT reason=%s tid=%lu rip=%p rsp=%p rbp=%p",
            reason ? reason : "",
            (unsigned long)g_core_tid,
            (void*)ctx.Rip,
            (void*)ctx.Rsp,
            (void*)ctx.Rbp);
    } else {
        fbfc_log("corethread: GetThreadContext failed (%s) err=%lu", reason ? reason : "", (unsigned long)GetLastError());
    }
#elif defined(_M_IX86) || defined(__i386__)
    CONTEXT ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(g_core_thread, &ctx)) {
        fbfc_log("corethread: TIMEOUT_CONTEXT reason=%s tid=%lu eip=%p esp=%p ebp=%p",
            reason ? reason : "",
            (unsigned long)g_core_tid,
            (void*)ctx.Eip,
            (void*)ctx.Esp,
            (void*)ctx.Ebp);
    } else {
        fbfc_log("corethread: GetThreadContext failed (%s) err=%lu", reason ? reason : "", (unsigned long)GetLastError());
    }
#else
    fbfc_log("corethread: context dump unsupported on this arch (%s)", reason ? reason : "");
#endif

    ResumeThread(g_core_thread);
}

static DWORD WINAPI fbfc_core_thread_proc(void*)
{
    fbfc_log("corethread: started tid=%lu stack_reserve=%lu", (unsigned long)GetCurrentThreadId(), 268435456ul);

    for (;;) {
        WaitForSingleObject(g_core_req_event, INFINITE);

        switch (g_core_req.op) {
        case FBFC_CORE_INIT:
            fbfc_log("corethread: begin flycast_retro_init");
            flycast_retro_init();
            fbfc_log("corethread: end flycast_retro_init");
            g_core_req.iret = 1;
            break;

        case FBFC_CORE_DEINIT:
            fbfc_log("corethread: begin flycast_retro_deinit");
            flycast_retro_deinit();
            fbfc_log("corethread: end flycast_retro_deinit");
            g_core_req.iret = 1;
            break;

        case FBFC_CORE_LOAD_GAME:
            fbfc_log("corethread: begin flycast_retro_load_game");
            g_core_req.iret = flycast_retro_load_game(g_core_req.game) ? 1 : 0;
            fbfc_log("corethread: end flycast_retro_load_game ret=%d", g_core_req.iret);
            break;

        case FBFC_CORE_UNLOAD_GAME:
            fbfc_log("corethread: begin flycast_retro_unload_game");
            flycast_retro_unload_game();
            fbfc_log("corethread: end flycast_retro_unload_game");
            g_core_req.iret = 1;
            break;

        case FBFC_CORE_RESET:
            flycast_retro_reset();
            g_core_req.iret = 1;
            break;

        case FBFC_CORE_RUN:
            flycast_retro_run();
            g_core_req.iret = 1;
            break;

        case FBFC_CORE_SERIALIZE_SIZE:
            g_core_req.sret = flycast_retro_serialize_size();
            g_core_req.iret = 1;
            break;

        case FBFC_CORE_SERIALIZE:
            g_core_req.iret = flycast_retro_serialize(g_core_req.data, g_core_req.size) ? 1 : 0;
            break;

        case FBFC_CORE_UNSERIALIZE:
            g_core_req.iret = flycast_retro_unserialize(g_core_req.cdata, g_core_req.size) ? 1 : 0;
            break;

        case FBFC_CORE_STOP:
            g_core_req.iret = 1;
            SetEvent(g_core_done_event);
            fbfc_log("corethread: stopping");
            return 0;

        default:
            g_core_req.iret = 0;
            break;
        }

        SetEvent(g_core_done_event);
    }
}

static int fbfc_start_core_thread(void)
{
    if (g_core_thread) return 1;

    g_core_req_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    g_core_done_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!g_core_req_event || !g_core_done_event) {
        fbfc_log("corethread: failed to create events err=%lu", (unsigned long)GetLastError());
        return 0;
    }

    InitializeCriticalSection(&g_core_cs);
    g_core_cs_ready = 1;
    memset(&g_core_req, 0, sizeof(g_core_req));
    g_core_stuck = 0;
    g_core_timeout_op = FBFC_CORE_NONE;

    g_core_thread = CreateThread(NULL,
        268435456,
        fbfc_core_thread_proc,
        NULL,
        STACK_SIZE_PARAM_IS_A_RESERVATION,
        &g_core_tid);

    if (!g_core_thread) {
        fbfc_log("corethread: CreateThread failed err=%lu", (unsigned long)GetLastError());
        if (g_core_cs_ready) {
            DeleteCriticalSection(&g_core_cs);
            g_core_cs_ready = 0;
        }
        CloseHandle(g_core_req_event);
        CloseHandle(g_core_done_event);
        g_core_req_event = NULL;
        g_core_done_event = NULL;
        return 0;
    }

    fbfc_log("corethread: created tid=%lu stack_reserve=%lu", (unsigned long)g_core_tid, 268435456ul);
    return 1;
}

static int fbfc_core_call(int op,
    const struct retro_game_info* game = NULL,
    void* data = NULL,
    const void* cdata = NULL,
    size_t size = 0,
    size_t* sret = NULL)
{
    if (!g_core_thread) return 0;

    EnterCriticalSection(&g_core_cs);
    memset(&g_core_req, 0, sizeof(g_core_req));
    g_core_req.op = op;
    g_core_req.game = game;
    g_core_req.data = data;
    g_core_req.cdata = cdata;
    g_core_req.size = size;

    SetEvent(g_core_req_event);
    DWORD timeout_ms = fbfc_core_timeout_ms(op);
    DWORD wr = WaitForSingleObject(g_core_done_event, timeout_ms);
    int ret = 0;
    if (wr == WAIT_OBJECT_0) {
        ret = g_core_req.iret;
        if (sret) *sret = g_core_req.sret;
    } else {
        fbfc_log("corethread: operation timeout op=%d wait_ms=%lu tid=%lu", op, (unsigned long)timeout_ms, (unsigned long)g_core_tid);
        g_core_stuck = 1;
        g_core_timeout_op = op;
        fbfc_dump_core_thread_context("core op timeout");
        if (op == FBFC_CORE_INIT) {
            fbfc_log("corethread: DIAG_MODE leaving stuck init thread alive for gdb attach");
            fbfc_log("corethread: keep the FBNeo error dialog open and run gdb_v14_attach_latest.bat now");
        }
        ret = 0;
        if (sret) *sret = 0;
    }
    LeaveCriticalSection(&g_core_cs);
    return ret;
}

static void fbfc_stop_core_thread(void)
{
    if (g_core_thread) {
        if (g_core_stuck) {
            if (g_core_timeout_op == FBFC_CORE_INIT) {
                fbfc_log("corethread: DIAG_MODE not terminating stuck init thread tid=%lu", (unsigned long)g_core_tid);
                fbfc_log("corethread: attach gdb before closing the FBNeo error dialog; close FBNeo manually after collecting gdb_fbneo_v14_bt.txt");
                return;
            }
            fbfc_log("corethread: terminating stuck core thread tid=%lu", (unsigned long)g_core_tid);
            TerminateThread(g_core_thread, 1);
            WaitForSingleObject(g_core_thread, 3000);
        } else {
            fbfc_core_call(FBFC_CORE_STOP);
            WaitForSingleObject(g_core_thread, INFINITE);
        }
        CloseHandle(g_core_thread);
        g_core_thread = NULL;
        g_core_tid = 0;
        g_core_stuck = 0;
        g_core_timeout_op = FBFC_CORE_NONE;
    }

    if (g_core_req_event) {
        CloseHandle(g_core_req_event);
        g_core_req_event = NULL;
    }
    if (g_core_done_event) {
        CloseHandle(g_core_done_event);
        g_core_done_event = NULL;
    }
    if (g_core_cs_ready) {
        DeleteCriticalSection(&g_core_cs);
        g_core_cs_ready = 0;
    }
}
#else
static int fbfc_start_core_thread(void) { return 1; }
static void fbfc_stop_core_thread(void) { }
#endif

int fbfc_init(const FbneoFlycastCallbacks* callbacks)
{
    if (g_inited) fbfc_shutdown();

    reset_debug_log();
    fbfc_log("fbfc_init enter");

    memset(&g_callbacks, 0, sizeof(g_callbacks));
    memset(&g_input, 0, sizeof(g_input));
    memset(&g_disk_cb, 0, sizeof(g_disk_cb));
#ifdef RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE
    memset(&g_disk_ext_cb, 0, sizeof(g_disk_ext_cb));
#endif
    g_keyboard_cb = NULL;

    if (callbacks) g_callbacks = *callbacks;
    ensure_runtime_dirs();

    fbfc_log("setting libretro callbacks");
    flycast_retro_set_environment(lr_environment_cb);
    flycast_retro_set_video_refresh(lr_video_cb);
    flycast_retro_set_audio_sample(lr_audio_cb);
    flycast_retro_set_audio_sample_batch(lr_audio_batch_cb);
    flycast_retro_set_input_poll(lr_input_poll_cb);
    flycast_retro_set_input_state(lr_input_state_cb);

    fbfc_log("before flycast_retro_init api=%u", flycast_retro_api_version());

#if defined(_WIN32)
    if (!g_fbfc_veh) {
        g_fbfc_veh = AddVectoredExceptionHandler(1, fbfc_vectored_exception_handler);
        fbfc_log("vectored exception handler installed=%p", g_fbfc_veh);
    }
#endif

    if (!fbfc_start_core_thread()) {
        fbfc_log("failed to start Flycast core thread");
        return 0;
    }

#if defined(_WIN32)
    fbfc_log("calling flycast_retro_init on dedicated core thread");
    if (!fbfc_core_call(FBFC_CORE_INIT)) {
        fbfc_log("flycast_retro_init failed or core thread did not return");
        fbfc_stop_core_thread();
        return 0;
    }
#else
    flycast_retro_init();
#endif

    fbfc_log("after flycast_retro_init");

    struct retro_system_info info;
    memset(&info, 0, sizeof(info));
    flycast_retro_get_system_info(&info);
    fbfc_log("core info library=%s version=%s valid_ext=%s need_fullpath=%d block_extract=%d",
        info.library_name ? info.library_name : "",
        info.library_version ? info.library_version : "",
        info.valid_extensions ? info.valid_extensions : "",
        info.need_fullpath ? 1 : 0,
        info.block_extract ? 1 : 0);

    g_inited = 1;
    g_loaded = 0;
    return 1;
}

void fbfc_shutdown(void)
{
    fbfc_log("fbfc_shutdown enter loaded=%d inited=%d", g_loaded, g_inited);

#if defined(_WIN32)
    if (g_loaded) fbfc_core_call(FBFC_CORE_UNLOAD_GAME);
    if (g_inited) fbfc_core_call(FBFC_CORE_DEINIT);
    g_loaded = 0;
    g_inited = 0;
    fbfc_stop_core_thread();
    if (g_fbfc_veh) {
        RemoveVectoredExceptionHandler(g_fbfc_veh);
        g_fbfc_veh = NULL;
        fbfc_log("vectored exception handler removed");
    }
#else
    if (g_loaded) flycast_retro_unload_game();
    if (g_inited) flycast_retro_deinit();
    g_loaded = 0;
    g_inited = 0;
#endif

    memset(&g_callbacks, 0, sizeof(g_callbacks));
}

int fbfc_load_game(const FbneoFlycastGameInfo* game)
{
    fbfc_log("fbfc_load_game enter inited=%d game=%p", g_inited, (const void*)game);
    if (!g_inited) return 0;
    if (!game) return 0;
    if (!game->roms || game->rom_count <= 0) {
        fbfc_log("no rom entries supplied");
        return 0;
    }

    fbfc_log("game driver=%s zip=%s bios=%s system=%s platform=%d input=%d rom_count=%d",
        game->driver_name ? game->driver_name : "",
        game->zip_name ? game->zip_name : "",
        game->bios_zip_name ? game->bios_zip_name : "",
        game->system_name ? game->system_name : "",
        game->platform,
        game->input_type,
        game->rom_count);

    stage_bios_files(game);

    std::string zip_name;
    if (game->zip_name && game->zip_name[0]) zip_name = game->zip_name;
    if (zip_name.empty()) {
        zip_name = (game->driver_name && game->driver_name[0]) ? game->driver_name : "fbneo_flycast_game";
        if (zip_name.find('.') == std::string::npos) zip_name += ".zip";
    }

    g_game_zip_path = g_content_dir + "/" + zip_name;
    if (!write_store_zip(g_game_zip_path.c_str(), game->roms, game->rom_count)) {
        fbfc_log("failed to build content zip for %s", game->driver_name ? game->driver_name : "");
        return 0;
    }

    struct retro_game_info gi;
    memset(&gi, 0, sizeof(gi));
    gi.path = g_game_zip_path.c_str();
    gi.data = NULL;
    gi.size = 0;
    gi.meta = NULL;

    fbfc_log("before flycast_retro_load_game path=%s", gi.path ? gi.path : "");

#if defined(_WIN32)
    if (!fbfc_core_call(FBFC_CORE_LOAD_GAME, &gi)) {
#else
    if (!flycast_retro_load_game(&gi)) {
#endif
        fbfc_log("flycast_retro_load_game returned false: %s", g_game_zip_path.c_str());
        g_loaded = 0;
        return 0;
    }

    fbfc_log("after flycast_retro_load_game");

    struct retro_system_av_info av;
    memset(&av, 0, sizeof(av));
    flycast_retro_get_system_av_info(&av);
    update_av_info(&av);

    g_loaded = 1;
    return 1;
}

void fbfc_unload_game(void)
{
    fbfc_log("fbfc_unload_game loaded=%d", g_loaded);
    if (g_loaded) {
#if defined(_WIN32)
        fbfc_core_call(FBFC_CORE_UNLOAD_GAME);
#else
        flycast_retro_unload_game();
#endif
    }
    g_loaded = 0;
}

void fbfc_reset(void)
{
    if (!g_loaded) return;
#if defined(_WIN32)
    fbfc_core_call(FBFC_CORE_RESET);
#else
    flycast_retro_reset();
#endif
}

int fbfc_run_frame(const FbneoFlycastInputState* input)
{
    if (!g_loaded) return 0;
    if (input) g_input = *input;
#if defined(_WIN32)
    return fbfc_core_call(FBFC_CORE_RUN);
#else
    flycast_retro_run();
    return 1;
#endif
}

int fbfc_serialize_size(void)
{
    if (!g_loaded) return 0;
#if defined(_WIN32)
    size_t s = 0;
    if (!fbfc_core_call(FBFC_CORE_SERIALIZE_SIZE, NULL, NULL, NULL, 0, &s)) return 0;
    return (int)s;
#else
    return (int)flycast_retro_serialize_size();
#endif
}

int fbfc_serialize(void* data, int size)
{
    if (!g_loaded || !data || size <= 0) return 0;
#if defined(_WIN32)
    return fbfc_core_call(FBFC_CORE_SERIALIZE, NULL, data, NULL, (size_t)size);
#else
    return flycast_retro_serialize(data, (size_t)size) ? 1 : 0;
#endif
}

int fbfc_unserialize(const void* data, int size)
{
    if (!g_loaded || !data || size <= 0) return 0;
#if defined(_WIN32)
    return fbfc_core_call(FBFC_CORE_UNSERIALIZE, NULL, NULL, data, (size_t)size);
#else
    return flycast_retro_unserialize(data, (size_t)size) ? 1 : 0;
#endif
}

int fbfc_get_av_info(int* width, int* height, int* sample_rate, double* fps)
{
    if (width) *width = g_width;
    if (height) *height = g_height;
    if (sample_rate) *sample_rate = g_sample_rate;
    if (fps) *fps = g_fps;
    return 1;
}

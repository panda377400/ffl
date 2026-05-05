// Clean replacement for panda377400/ffl
// Path: src/burn/drv/atomiswave/flycast_shim/fbneo_flycast_api.cpp
// Goal: keep the in-process Flycast/libretro bridge, remove helper-process assumptions,
// supply log/perf callbacks, allow dynarec, and keep deterministic unload/reload cleanup.

#include "fbneo_flycast_api.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#include <GL/gl.h>
#define FBFC_MKDIR(path) _mkdir(path)
#if defined(__MINGW32__) && !defined(FBNEO_FLYCAST_MINGW_STAT64I32_COMPAT)
#define FBNEO_FLYCAST_MINGW_STAT64I32_COMPAT 1
#include <sys/stat.h>
extern "C" int stat64i32(const char* path, struct _stat64i32* st) { return _stat64i32(path, st); }
#endif
#else
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
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

static void fbfc_log(const char* fmt, ...);
static void fbfc_vlog_to_file(const char* prefix, const char* fmt, va_list ap);

#if defined(_WIN32)
static PVOID g_fbfc_veh = NULL;
static HWND g_hw_hwnd = NULL;
static HDC g_hw_hdc = NULL;
static HGLRC g_hw_hglrc = NULL;
static int g_hw_ready = 0;
static int g_hw_reset_called = 0;
static struct retro_hw_render_callback g_hw_cb;
static std::vector<uint32_t> g_hw_readback;

static LRESULT CALLBACK fbfc_hw_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool fbfc_hw_make_current(void)
{
    if (!g_hw_hdc || !g_hw_hglrc) return false;
    return wglMakeCurrent(g_hw_hdc, g_hw_hglrc) ? true : false;
}

static bool fbfc_hw_init_window(void)
{
    if (g_hw_ready) return fbfc_hw_make_current();

    HINSTANCE inst = GetModuleHandleA(NULL);
    static ATOM cls = 0;
    if (!cls) {
        WNDCLASSA wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = fbfc_hw_wndproc;
        wc.hInstance = inst;
        wc.lpszClassName = "FBNeoFlycastHiddenWGL";
        cls = RegisterClassA(&wc);
        if (!cls) {
            fbfc_log("WGL: RegisterClass failed err=%lu", (unsigned long)GetLastError());
            return false;
        }
    }

    g_hw_hwnd = CreateWindowExA(0, "FBNeoFlycastHiddenWGL", "FBNeo Flycast WGL",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, inst, NULL);
    if (!g_hw_hwnd) {
        fbfc_log("WGL: CreateWindow failed err=%lu", (unsigned long)GetLastError());
        return false;
    }

    g_hw_hdc = GetDC(g_hw_hwnd);
    if (!g_hw_hdc) {
        fbfc_log("WGL: GetDC failed err=%lu", (unsigned long)GetLastError());
        DestroyWindow(g_hw_hwnd);
        g_hw_hwnd = NULL;
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(g_hw_hdc, &pfd);
    if (!pf || !SetPixelFormat(g_hw_hdc, pf, &pfd)) {
        fbfc_log("WGL: Choose/SetPixelFormat failed pf=%d err=%lu", pf, (unsigned long)GetLastError());
        ReleaseDC(g_hw_hwnd, g_hw_hdc);
        DestroyWindow(g_hw_hwnd);
        g_hw_hdc = NULL;
        g_hw_hwnd = NULL;
        return false;
    }

    g_hw_hglrc = wglCreateContext(g_hw_hdc);
    if (!g_hw_hglrc) {
        fbfc_log("WGL: wglCreateContext failed err=%lu", (unsigned long)GetLastError());
        ReleaseDC(g_hw_hwnd, g_hw_hdc);
        DestroyWindow(g_hw_hwnd);
        g_hw_hdc = NULL;
        g_hw_hwnd = NULL;
        return false;
    }

    if (!fbfc_hw_make_current()) {
        fbfc_log("WGL: wglMakeCurrent failed err=%lu", (unsigned long)GetLastError());
        wglDeleteContext(g_hw_hglrc);
        ReleaseDC(g_hw_hwnd, g_hw_hdc);
        DestroyWindow(g_hw_hwnd);
        g_hw_hglrc = NULL;
        g_hw_hdc = NULL;
        g_hw_hwnd = NULL;
        return false;
    }

    g_hw_ready = 1;
    fbfc_log("WGL: hidden OpenGL context created vendor=%s renderer=%s version=%s",
        glGetString(GL_VENDOR) ? (const char*)glGetString(GL_VENDOR) : "",
        glGetString(GL_RENDERER) ? (const char*)glGetString(GL_RENDERER) : "",
        glGetString(GL_VERSION) ? (const char*)glGetString(GL_VERSION) : "");
    return true;
}

static uintptr_t RETRO_CALLCONV fbfc_hw_get_current_framebuffer(void)
{
    return 0;
}

static retro_proc_address_t RETRO_CALLCONV fbfc_hw_get_proc_address(const char* sym)
{
    if (!sym || !sym[0]) return NULL;
    PROC p = wglGetProcAddress(sym);
    if (!p || p == (PROC)1 || p == (PROC)2 || p == (PROC)3 || p == (PROC)-1) {
        HMODULE gl = GetModuleHandleA("opengl32.dll");
        if (!gl) gl = LoadLibraryA("opengl32.dll");
        if (gl) p = GetProcAddress(gl, sym);
    }
    return (retro_proc_address_t)p;
}

static void fbfc_hw_call_context_reset_if_needed(void)
{
    if (!g_hw_ready || g_hw_reset_called) return;
    if (!fbfc_hw_make_current()) return;
    if (g_hw_cb.context_reset) {
        fbfc_log("WGL: calling core context_reset");
        g_hw_cb.context_reset();
    }
    g_hw_reset_called = 1;
}

static void fbfc_hw_destroy(void)
{
    if (g_hw_ready && g_hw_cb.context_destroy) {
        fbfc_hw_make_current();
        fbfc_log("WGL: calling core context_destroy");
        g_hw_cb.context_destroy();
    }
    g_hw_reset_called = 0;
    memset(&g_hw_cb, 0, sizeof(g_hw_cb));
    g_hw_readback.clear();
    if (g_hw_hglrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_hw_hglrc);
        g_hw_hglrc = NULL;
    }
    if (g_hw_hwnd && g_hw_hdc) {
        ReleaseDC(g_hw_hwnd, g_hw_hdc);
        g_hw_hdc = NULL;
    }
    if (g_hw_hwnd) {
        DestroyWindow(g_hw_hwnd);
        g_hw_hwnd = NULL;
    }
    if (g_hw_ready) fbfc_log("WGL: hidden OpenGL context destroyed");
    g_hw_ready = 0;
}
#else
static bool fbfc_hw_make_current(void) { return true; }
static void fbfc_hw_call_context_reset_if_needed(void) {}
static void fbfc_hw_destroy(void) {}
#endif

static void make_dir_quiet(const std::string& path)
{
    if (!path.empty()) FBFC_MKDIR(path.c_str());
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
    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "ab");
    if (!f) f = fopen("fbfc_debug.log", "ab");
    if (f) {
        long pos = 0;
        if (fseek(f, 0, SEEK_END) == 0) pos = ftell(f);
        if (pos <= 0) {
            fputs("fbneo/flycast debug log\n", f);
            fputs("adapter: clean-v10 in-process dynarec path; log/perf callbacks supplied; helper disabled\n", f);
        }
        fputs("\n---- fbfc session begin ----\n", f);
        fclose(f);
    }
}

static void fbfc_vlog_to_file(const char* prefix, const char* fmt, va_list ap)
{
    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "ab");
    if (!f) f = fopen("fbfc_debug.log", "ab");
    if (!f) return;
    if (prefix) fputs(prefix, f);
    vfprintf(f, fmt, ap);
    fputc('\n', f);
    fclose(f);
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
    va_start(ap, fmt);
    fbfc_vlog_to_file("atomiswave/flycast: ", fmt, ap);
    va_end(ap);
}

#if defined(_WIN32)
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
    central.reserve((size_t)((rom_count > 0) ? rom_count : 0));

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
        wr16(f, 20); wr16(f, 0); wr16(f, 0); wr16(f, 0); wr16(f, 0);
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
        wr16(f, 20); wr16(f, 20); wr16(f, 0); wr16(f, 0); wr16(f, 0); wr16(f, 0);
        wr32(f, ze.crc); wr32(f, ze.size); wr32(f, ze.size);
        wr16(f, (uint16_t)ze.name.size()); wr16(f, 0); wr16(f, 0); wr16(f, 0); wr16(f, 0);
        wr32(f, 0); wr32(f, ze.local_offset);
        fwrite(ze.name.data(), 1, ze.name.size(), f);
    }
    const uint32_t central_size = (uint32_t)ftell(f) - central_offset;
    wr32(f, 0x06054b50u);
    wr16(f, 0); wr16(f, 0); wr16(f, (uint16_t)central.size()); wr16(f, (uint16_t)central.size());
    wr32(f, central_size); wr32(f, central_offset); wr16(f, 0);
    const int close_ret = fclose(f);
    fbfc_log("wrote content zip: %s files=%u close=%d", path ? path : "", (unsigned)central.size(), close_ret);
    return close_ret == 0;
}

static bool copy_file_if_exists(const char* src, const char* dst)
{
    FILE* in = fopen(src, "rb");
    if (!in) return false;
    FILE* out = fopen(dst, "wb");
    if (!out) { fclose(in); return false; }
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
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
    copy_file_if_exists("roms/naomi2.zip", "fbneo_flycast_runtime/system/naomi2.zip");
    copy_file_if_exists("roms/naomi2.zip", "fbneo_flycast_runtime/system/dc/naomi2.zip");
}

static void lr_video_cb(const void* data, unsigned width, unsigned height, size_t pitch)
{
#ifdef RETRO_HW_FRAME_BUFFER_VALID
    if (data == RETRO_HW_FRAME_BUFFER_VALID) {
#if defined(_WIN32)
        if (!g_callbacks.video) return;
        if (!fbfc_hw_make_current()) {
            fbfc_log("WGL: video_cb HW frame but context is not current");
            return;
        }
        int w = width ? (int)width : g_width;
        int h = height ? (int)height : g_height;
        if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return;
        std::vector<unsigned char> rgba((size_t)w * (size_t)h * 4u);
        g_hw_readback.resize((size_t)w * (size_t)h);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            fbfc_log("WGL: glReadPixels GL_RGBA failed err=0x%04x", (unsigned)err);
            return;
        }
        for (int y = 0; y < h; y++) {
            const unsigned char* src = &rgba[(size_t)(h - 1 - y) * (size_t)w * 4u];
            uint32_t* dst = &g_hw_readback[(size_t)y * (size_t)w];
            for (int x = 0; x < w; x++) {
                unsigned r = src[x * 4 + 0];
                unsigned g = src[x * 4 + 1];
                unsigned b = src[x * 4 + 2];
                dst[x] = 0xff000000u | (r << 16) | (g << 8) | b;
            }
        }
        FbneoFlycastVideoFrame frame;
        memset(&frame, 0, sizeof(frame));
        frame.pixels = g_hw_readback.data();
        frame.width = w;
        frame.height = h;
        frame.pitch_bytes = w * 4;
        frame.valid_cpu = 1;
        frame.valid_hw = 0;
        g_callbacks.video(&frame, g_callbacks.user);
        SwapBuffers(g_hw_hdc);
#endif
        return;
    }
#endif
    if (!g_callbacks.video || !data) return;
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
    if (g_callbacks.audio && data && frames > 0) g_callbacks.audio(data, (int)frames, g_callbacks.user);
    return frames;
}

static void lr_input_poll_cb(void) {}

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
    fprintf(stderr, "flycast: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
    va_start(ap, fmt);
    fbfc_vlog_to_file("flycast: ", fmt, ap);
    va_end(ap);
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
    LARGE_INTEGER t, f;
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
#ifdef RETRO_SIMD_SSE3
    r |= RETRO_SIMD_SSE3;
#endif
#ifdef RETRO_SIMD_SSSE3
    r |= RETRO_SIMD_SSSE3;
#endif
#ifdef RETRO_SIMD_SSE4
    r |= RETRO_SIMD_SSE4;
#endif
#ifdef RETRO_SIMD_SSE42
    r |= RETRO_SIMD_SSE42;
#endif
#ifdef RETRO_SIMD_AVX
    r |= RETRO_SIMD_AVX;
#endif
#ifdef RETRO_SIMD_AVX2
    r |= RETRO_SIMD_AVX2;
#endif
    return r;
}

static void lr_perf_register(struct retro_perf_counter* counter) { if (counter) counter->registered = true; }
static void lr_perf_start(struct retro_perf_counter* counter) { if (counter) counter->start = lr_perf_get_counter(); }
static void lr_perf_stop(struct retro_perf_counter* counter)
{
    if (!counter) return;
    retro_perf_tick_t now = lr_perf_get_counter();
    if (now >= counter->start) counter->total += now - counter->start;
    counter->call_cnt++;
}
static void lr_perf_log(void) {}

static struct retro_log_callback g_log_cb = { lr_log_cb };
static struct retro_perf_callback g_perf_cb;

static void fbfc_init_perf_callback(void)
{
    memset(&g_perf_cb, 0, sizeof(g_perf_cb));
    g_perf_cb.get_time_usec = lr_perf_get_time_usec;
    g_perf_cb.get_cpu_features = lr_get_cpu_features;
    g_perf_cb.get_perf_counter = lr_perf_get_counter;
    g_perf_cb.perf_register = lr_perf_register;
    g_perf_cb.perf_start = lr_perf_start;
    g_perf_cb.perf_stop = lr_perf_stop;
    g_perf_cb.perf_log = lr_perf_log;
}

static void lr_keyboard_dummy(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
    (void)down; (void)keycode; (void)character; (void)key_modifiers;
}

struct FbfcCoreOptionValue { const char* key; const char* value; };
static const FbfcCoreOptionValue g_core_option_values[] = {
    { "flycast_dynarec", "enabled" },
    { "reicast_dynarec", "enabled" },
    { "flycast_cpu_mode", "dynamic_recompiler" },
    { "reicast_cpu_mode", "dynamic_recompiler" },
    { "flycast_threaded_rendering", "disabled" },
    { "reicast_threaded_rendering", "disabled" },
    { "flycast_delay_frame_swapping", "disabled" },
    { "flycast_skip_frame", "disabled" },
    { "flycast_frame_skipping", "disabled" },
    { "reicast_frame_skipping", "disabled" },
    { "reicast_auto_skip_frame", "disabled" },
    { "flycast_enable_dsp", "disabled" },
    { "reicast_enable_dsp", "disabled" },
    { "flycast_hle_bios", "disabled" },
    { "flycast_boot_to_bios", "disabled" },
    { "flycast_region", "Default" },
    { "reicast_region", "Default" },
    { "flycast_language", "Default" },
    { "reicast_language", "Default" },
    { "flycast_internal_resolution", "640x480" },
    { "flycast_cable_type", "TV (Composite)" },
    { "flycast_alpha_sorting", "Per-Strip (fast, least accurate)" },
    { "flycast_widescreen_cheats", "Off" },
    { "flycast_widescreen_hack", "Off" },
    { "flycast_gdrom_fast_loading", "Off" },
    { "flycast_custom_textures", "Off" },
    { "flycast_dump_textures", "Off" },
    { "flycast_per_content_vmus", "disabled" },
    { NULL, NULL }
};

static const char* fbfc_lookup_core_option(const char* key)
{
    if (!key || !key[0]) return NULL;
    for (int i = 0; g_core_option_values[i].key; i++) {
        if (strcmp(key, g_core_option_values[i].key) == 0) return g_core_option_values[i].value;
    }
    return NULL;
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
            var->value = fbfc_lookup_core_option(var->key);
            fbfc_log("get variable: %s -> %s", var->key ? var->key : "", var->value ? var->value : "<null>");
            return var->value != NULL;
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
            *(struct retro_log_callback*)data = g_log_cb;
            fbfc_log("log interface requested: supplied");
            return true;
        }
        return false;
    case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
        if (data) {
            fbfc_init_perf_callback();
            *(struct retro_perf_callback*)data = g_perf_cb;
            fbfc_log("perf interface requested: supplied cpu_features=%016llx", (unsigned long long)lr_get_cpu_features());
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
#ifdef RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE
    case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        fbfc_log("rumble interface requested: not supplied");
        return false;
#endif
#ifdef RETRO_ENVIRONMENT_SET_HW_RENDER
    case RETRO_ENVIRONMENT_SET_HW_RENDER:
#if defined(_WIN32)
        if (!data) return false;
        {
            struct retro_hw_render_callback* cb = (struct retro_hw_render_callback*)data;
            fbfc_log("core requested HW render context_type=%d version=%u.%u depth=%d stencil=%d",
                (int)cb->context_type, cb->version_major, cb->version_minor, cb->depth ? 1 : 0, cb->stencil ? 1 : 0);
            if (cb->context_type != RETRO_HW_CONTEXT_OPENGL) {
                fbfc_log("HW render: rejecting non-OpenGL context_type=%d", (int)cb->context_type);
                return false;
            }
            if (!fbfc_hw_init_window()) {
                fbfc_log("HW render: failed to create hidden WGL context");
                return false;
            }
            cb->get_current_framebuffer = fbfc_hw_get_current_framebuffer;
            cb->get_proc_address = fbfc_hw_get_proc_address;
            g_hw_cb = *cb;
            g_hw_reset_called = 0;
            fbfc_log("HW render: accepted OpenGL through hidden WGL + CPU readback (clean-v10; direct-FBO not wired yet)");
            return true;
        }
#else
        fbfc_log("core requested HW render but this shim only implements Win32 WGL currently");
        return false;
#endif
#endif
#ifdef RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER
    case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
        if (data) *(unsigned*)data = RETRO_HW_CONTEXT_OPENGL;
        fbfc_log("preferred HW render requested: returning OpenGL");
        return true;
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
#ifdef RETRO_ENVIRONMENT_GET_FASTFORWARDING
    case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
        if (data) *(bool*)data = false;
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

static const char* fbfc_basename_or_default(const char* value, const char* fallback)
{
    if (!value || !value[0]) return fallback;
    return value;
}

extern "C" int fbfc_init(const FbneoFlycastCallbacks* callbacks)
{
    memset(&g_callbacks, 0, sizeof(g_callbacks));
    memset(&g_input, 0, sizeof(g_input));
    if (callbacks) g_callbacks = *callbacks;
    reset_debug_log();

    if (g_inited) {
        fbfc_log("fbfc_init enter: core already initialized; refreshing callbacks only");
        flycast_retro_set_environment(lr_environment_cb);
        flycast_retro_set_video_refresh(lr_video_cb);
        flycast_retro_set_audio_sample(lr_audio_cb);
        flycast_retro_set_audio_sample_batch(lr_audio_batch_cb);
        flycast_retro_set_input_poll(lr_input_poll_cb);
        flycast_retro_set_input_state(lr_input_state_cb);
        fbfc_log("fbfc_init leave ok (callbacks refreshed)");
        return 1;
    }

    fbfc_log("fbfc_init enter clean-v10");
    flycast_retro_set_environment(lr_environment_cb);
    flycast_retro_set_video_refresh(lr_video_cb);
    flycast_retro_set_audio_sample(lr_audio_cb);
    flycast_retro_set_audio_sample_batch(lr_audio_batch_cb);
    flycast_retro_set_input_poll(lr_input_poll_cb);
    flycast_retro_set_input_state(lr_input_state_cb);

#if defined(_WIN32)
    if (!g_fbfc_veh) {
        g_fbfc_veh = AddVectoredExceptionHandler(0, fbfc_vectored_exception_handler);
        fbfc_log("adapter vectored exception handler installed after clean reset: %p", g_fbfc_veh);
    }
#endif

    fbfc_log("before flycast_retro_init api=%u", flycast_retro_api_version());
    flycast_retro_init();
    fbfc_log("after flycast_retro_init");

    struct retro_system_info sys;
    memset(&sys, 0, sizeof(sys));
    flycast_retro_get_system_info(&sys);
    fbfc_log("system info: library=%s version=%s need_fullpath=%d block_extract=%d valid_ext=%s",
        sys.library_name ? sys.library_name : "",
        sys.library_version ? sys.library_version : "",
        sys.need_fullpath ? 1 : 0,
        sys.block_extract ? 1 : 0,
        sys.valid_extensions ? sys.valid_extensions : "");

    struct retro_system_av_info av;
    memset(&av, 0, sizeof(av));
    flycast_retro_get_system_av_info(&av);
    update_av_info(&av);

    g_inited = 1;
    fbfc_log("fbfc_init leave ok");
    return 1;
}

extern "C" void fbfc_shutdown(void)
{
    fbfc_log("fbfc_shutdown enter");
    if (g_loaded) {
        fbfc_log("before flycast_retro_unload_game");
        flycast_retro_unload_game();
        fbfc_log("after flycast_retro_unload_game");
        g_loaded = 0;
    }
    fbfc_hw_destroy();
    if (g_inited) {
        fbfc_log("before flycast_retro_deinit");
        flycast_retro_deinit();
        fbfc_log("after flycast_retro_deinit");
        g_inited = 0;
    }
#if defined(_WIN32)
    if (g_fbfc_veh) {
        RemoveVectoredExceptionHandler(g_fbfc_veh);
        g_fbfc_veh = NULL;
    }
#endif
    g_game_zip_path.clear();
    fbfc_log("fbfc_shutdown leave");
}

extern "C" int fbfc_load_game(const FbneoFlycastGameInfo* game)
{
    if (!g_inited) {
        fbfc_log("fbfc_load_game called before fbfc_init");
        return 0;
    }
    if (!game || !game->roms || game->rom_count <= 0) {
        fbfc_log("fbfc_load_game invalid game package");
        return 0;
    }
    if (g_loaded) fbfc_unload_game();

    ensure_runtime_dirs();
    stage_bios_files(game);

    const char* driver = fbfc_basename_or_default(game->driver_name, "fbneo_awave_game");
    g_game_zip_path = g_content_dir + "/" + driver + ".zip";
    fbfc_log("building Flycast content zip: driver=%s zip=%s bios=%s system=%s platform=%d input=%d path=%s",
        game->driver_name ? game->driver_name : "",
        game->zip_name ? game->zip_name : "",
        game->bios_zip_name ? game->bios_zip_name : "",
        game->system_name ? game->system_name : "",
        game->platform,
        game->input_type,
        g_game_zip_path.c_str());

    if (!write_store_zip(g_game_zip_path.c_str(), game->roms, game->rom_count)) {
        fbfc_log("failed to build Flycast content zip");
        return 0;
    }

    struct retro_game_info lr_game;
    memset(&lr_game, 0, sizeof(lr_game));
    lr_game.path = g_game_zip_path.c_str();
    lr_game.data = NULL;
    lr_game.size = 0;
    lr_game.meta = NULL;

    fbfc_log("before flycast_retro_load_game path=%s", lr_game.path ? lr_game.path : "");
    bool ok = flycast_retro_load_game(&lr_game);
    fbfc_log("after flycast_retro_load_game ret=%d", ok ? 1 : 0);
    if (!ok) {
        fbfc_log("flycast_retro_load_game failed; cleaning up core before returning failure");
        flycast_retro_unload_game();
        fbfc_hw_destroy();
        flycast_retro_deinit();
        g_loaded = 0;
        g_inited = 0;
        return 0;
    }

    fbfc_hw_call_context_reset_if_needed();

    unsigned device = RETRO_DEVICE_JOYPAD;
    if (game->input_type == FBFC_INPUT_LIGHTGUN) device = RETRO_DEVICE_LIGHTGUN;
    for (unsigned port = 0; port < 4; port++) flycast_retro_set_controller_port_device(port, device);

    struct retro_system_av_info av;
    memset(&av, 0, sizeof(av));
    flycast_retro_get_system_av_info(&av);
    update_av_info(&av);

    g_loaded = 1;
    fbfc_log("fbfc_load_game leave ok");
    return 1;
}

extern "C" void fbfc_unload_game(void)
{
    if (!g_inited || !g_loaded) return;
    fbfc_log("fbfc_unload_game enter");
    flycast_retro_unload_game();
    fbfc_hw_destroy();
    g_loaded = 0;
    fbfc_log("fbfc_unload_game leave");
}

extern "C" void fbfc_reset(void)
{
    if (!g_inited || !g_loaded) return;
    fbfc_log("fbfc_reset");
    flycast_retro_reset();
}

extern "C" int fbfc_run_frame(const FbneoFlycastInputState* input)
{
    if (!g_inited || !g_loaded) return 0;
    if (input) g_input = *input;
    fbfc_hw_make_current();
    flycast_retro_run();
    return 1;
}

extern "C" int fbfc_serialize_size(void)
{
    if (!g_inited || !g_loaded) return 0;
    size_t size = flycast_retro_serialize_size();
    if (size > (size_t)0x7fffffff) return 0;
    return (int)size;
}

extern "C" int fbfc_serialize(void* data, int size)
{
    if (!g_inited || !g_loaded || !data || size <= 0) return 0;
    return flycast_retro_serialize(data, (size_t)size) ? 1 : 0;
}

extern "C" int fbfc_unserialize(const void* data, int size)
{
    if (!g_inited || !g_loaded || !data || size <= 0) return 0;
    return flycast_retro_unserialize(data, (size_t)size) ? 1 : 0;
}

extern "C" int fbfc_get_av_info(int* width, int* height, int* sample_rate, double* fps)
{
    if (width) *width = g_width;
    if (height) *height = g_height;
    if (sample_rate) *sample_rate = g_sample_rate;
    if (fps) *fps = g_fps;
    return 1;
}

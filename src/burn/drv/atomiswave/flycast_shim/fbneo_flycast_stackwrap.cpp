// FBNeo/Flycast static-libretro stack trampoline for MinGW/Windows.
//
// The normal FBNeo/Qt emulation path can call Flycast's libretro entry points
// from a thread with too little stack for Flycast VMEM/JIT initialization.
// Enlarging the PE header stack is not always enough when the caller is an
// existing framework thread, so we use Windows fibers to run the heavy libretro
// calls on a large stack while staying on the same OS thread.
//
// This file is activated by GNU ld --wrap flags in makefile.mingw.

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#else
#include <stddef.h>
#include <stdint.h>
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
void   __real_flycast_retro_init(void);
void   __real_flycast_retro_deinit(void);
void   __real_flycast_retro_get_system_info(struct retro_system_info* info);
void   __real_flycast_retro_get_system_av_info(struct retro_system_av_info* info);
bool   __real_flycast_retro_load_game(const struct retro_game_info* game);
void   __real_flycast_retro_unload_game(void);
void   __real_flycast_retro_run(void);
void   __real_flycast_retro_reset(void);
size_t __real_flycast_retro_serialize_size(void);
bool   __real_flycast_retro_serialize(void* data, size_t size);
bool   __real_flycast_retro_unserialize(const void* data, size_t size);
}

#if defined(_WIN32)
#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif
#ifndef ERROR_ALREADY_FIBER
#define ERROR_ALREADY_FIBER 1280L
#endif

static const SIZE_T FBFC_FLYCAST_FIBER_STACK_RESERVE = 128u * 1024u * 1024u;

typedef void (*FbfcFiberFn)(void*);

struct FbfcFiberCall {
    FbfcFiberFn fn;
    void* arg;
    void* caller_fiber;
};

static void fbfc_stackwrap_log(const char* fmt, ...)
{
    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "ab");
    if (!f) f = fopen("fbfc_debug.log", "ab");
    if (!f) return;

    va_list ap;
    va_start(ap, fmt);
    fprintf(f, "atomiswave/flycast-stackwrap: ");
    vfprintf(f, fmt, ap);
    fprintf(f, "\n");
    va_end(ap);
    fclose(f);
}

static VOID WINAPI fbfc_fiber_entry(void* param)
{
    FbfcFiberCall* call = (FbfcFiberCall*)param;
    if (call && call->fn)
        call->fn(call->arg);

    SwitchToFiber(call->caller_fiber);
}

static bool fbfc_run_on_large_fiber(FbfcFiberFn fn, void* arg, const char* name)
{
    if (!fn)
        return false;

    void* caller = ConvertThreadToFiber(NULL);
    if (!caller) {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_FIBER) {
            caller = GetCurrentFiber();
        } else {
            fbfc_stackwrap_log("ConvertThreadToFiber failed for %s err=%lu; using current stack", name ? name : "?", (unsigned long)err);
            fn(arg);
            return false;
        }
    }

    FbfcFiberCall call;
    call.fn = fn;
    call.arg = arg;
    call.caller_fiber = caller;

    void* fiber = CreateFiberEx(0, FBFC_FLYCAST_FIBER_STACK_RESERVE,
                                STACK_SIZE_PARAM_IS_A_RESERVATION,
                                fbfc_fiber_entry, &call);
    if (!fiber) {
        DWORD err = GetLastError();
        fbfc_stackwrap_log("CreateFiberEx failed for %s err=%lu reserve=%lu; using current stack",
                           name ? name : "?", (unsigned long)err,
                           (unsigned long)FBFC_FLYCAST_FIBER_STACK_RESERVE);
        fn(arg);
        return false;
    }

    fbfc_stackwrap_log("%s on large fiber stack reserve=%lu", name ? name : "flycast call",
                       (unsigned long)FBFC_FLYCAST_FIBER_STACK_RESERVE);
    SwitchToFiber(fiber);
    DeleteFiber(fiber);
    return true;
}
#else

typedef void (*FbfcFiberFn)(void*);
static bool fbfc_run_on_large_fiber(FbfcFiberFn fn, void* arg, const char*)
{
    if (fn) fn(arg);
    return false;
}
#endif

static void task_init(void*) { __real_flycast_retro_init(); }
static void task_deinit(void*) { __real_flycast_retro_deinit(); }
static void task_unload_game(void*) { __real_flycast_retro_unload_game(); }
static void task_run(void*) { __real_flycast_retro_run(); }
static void task_reset(void*) { __real_flycast_retro_reset(); }

struct InfoArg { struct retro_system_info* info; };
static void task_get_system_info(void* p)
{
    InfoArg* a = (InfoArg*)p;
    __real_flycast_retro_get_system_info(a ? a->info : 0);
}

struct AvInfoArg { struct retro_system_av_info* info; };
static void task_get_system_av_info(void* p)
{
    AvInfoArg* a = (AvInfoArg*)p;
    __real_flycast_retro_get_system_av_info(a ? a->info : 0);
}

struct LoadGameArg { const struct retro_game_info* game; bool ret; };
static void task_load_game(void* p)
{
    LoadGameArg* a = (LoadGameArg*)p;
    a->ret = __real_flycast_retro_load_game(a->game);
}

struct SerializeSizeArg { size_t ret; };
static void task_serialize_size(void* p)
{
    SerializeSizeArg* a = (SerializeSizeArg*)p;
    a->ret = __real_flycast_retro_serialize_size();
}

struct SerializeArg { void* data; size_t size; bool ret; };
static void task_serialize(void* p)
{
    SerializeArg* a = (SerializeArg*)p;
    a->ret = __real_flycast_retro_serialize(a->data, a->size);
}

struct UnserializeArg { const void* data; size_t size; bool ret; };
static void task_unserialize(void* p)
{
    UnserializeArg* a = (UnserializeArg*)p;
    a->ret = __real_flycast_retro_unserialize(a->data, a->size);
}

extern "C" {

void __wrap_flycast_retro_init(void)
{
    fbfc_run_on_large_fiber(task_init, NULL, "flycast_retro_init");
}

void __wrap_flycast_retro_deinit(void)
{
    fbfc_run_on_large_fiber(task_deinit, NULL, "flycast_retro_deinit");
}

void __wrap_flycast_retro_get_system_info(struct retro_system_info* info)
{
    InfoArg a = { info };
    fbfc_run_on_large_fiber(task_get_system_info, &a, "flycast_retro_get_system_info");
}

void __wrap_flycast_retro_get_system_av_info(struct retro_system_av_info* info)
{
    AvInfoArg a = { info };
    fbfc_run_on_large_fiber(task_get_system_av_info, &a, "flycast_retro_get_system_av_info");
}

bool __wrap_flycast_retro_load_game(const struct retro_game_info* game)
{
    LoadGameArg a = { game, false };
    fbfc_run_on_large_fiber(task_load_game, &a, "flycast_retro_load_game");
    return a.ret;
}

void __wrap_flycast_retro_unload_game(void)
{
    fbfc_run_on_large_fiber(task_unload_game, NULL, "flycast_retro_unload_game");
}

void __wrap_flycast_retro_run(void)
{
    fbfc_run_on_large_fiber(task_run, NULL, "flycast_retro_run");
}

void __wrap_flycast_retro_reset(void)
{
    fbfc_run_on_large_fiber(task_reset, NULL, "flycast_retro_reset");
}

size_t __wrap_flycast_retro_serialize_size(void)
{
    SerializeSizeArg a = { 0 };
    fbfc_run_on_large_fiber(task_serialize_size, &a, "flycast_retro_serialize_size");
    return a.ret;
}

bool __wrap_flycast_retro_serialize(void* data, size_t size)
{
    SerializeArg a = { data, size, false };
    fbfc_run_on_large_fiber(task_serialize, &a, "flycast_retro_serialize");
    return a.ret;
}

bool __wrap_flycast_retro_unserialize(const void* data, size_t size)
{
    UnserializeArg a = { data, size, false };
    fbfc_run_on_large_fiber(task_unserialize, &a, "flycast_retro_unserialize");
    return a.ret;
}

}

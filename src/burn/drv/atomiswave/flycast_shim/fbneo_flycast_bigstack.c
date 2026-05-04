/*
 * FBNeo/Flycast static-libretro big-stack trampoline for MinGW/Windows.
 *
 * This file intentionally uses C, not C++, because makefile.mingw force-includes
 * fbneo_flycast_bigstack_redirect.h only into C++ sources. That lets this file
 * call the real flycast_retro_init symbol without the macro redirect.
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#else
#include <stdio.h>
#include <stdarg.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
void flycast_retro_init(void);
#ifdef __cplusplus
}
#endif

#if defined(_WIN32)
#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif
#ifndef ERROR_ALREADY_FIBER
#define ERROR_ALREADY_FIBER 1280L
#endif

static const SIZE_T FBFC_FLYCAST_INIT_STACK_COMMIT  = 64u * 1024u;
static const SIZE_T FBFC_FLYCAST_INIT_STACK_RESERVE = 256u * 1024u * 1024u;

typedef void (*FbfcVoidFn)(void);

struct FbfcBigStackCall {
    FbfcVoidFn fn;
    void* caller_fiber;
};

static void fbfc_bigstack_log(const char* fmt, ...)
{
    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "ab");
    if (!f)
        f = fopen("fbfc_debug.log", "ab");
    if (!f)
        return;

    va_list ap;
    va_start(ap, fmt);
    fprintf(f, "atomiswave/flycast-bigstack: ");
    vfprintf(f, fmt, ap);
    fprintf(f, "\n");
    va_end(ap);
    fclose(f);
}

static VOID WINAPI fbfc_bigstack_fiber_entry(void* param)
{
    struct FbfcBigStackCall* call = (struct FbfcBigStackCall*)param;
    if (call && call->fn)
        call->fn();
    SwitchToFiber(call->caller_fiber);
}

static void fbfc_call_on_large_stack(FbfcVoidFn fn, const char* name)
{
    void* caller;
    void* fiber;
    DWORD err;
    struct FbfcBigStackCall call;

    if (!fn)
        return;

    caller = ConvertThreadToFiber(NULL);
    if (!caller) {
        err = GetLastError();
        if (err == ERROR_ALREADY_FIBER) {
            caller = GetCurrentFiber();
        } else {
            fbfc_bigstack_log("ConvertThreadToFiber failed for %s err=%lu; calling on current stack",
                              name ? name : "?", (unsigned long)err);
            fn();
            return;
        }
    }

    call.fn = fn;
    call.caller_fiber = caller;

    fiber = CreateFiberEx(FBFC_FLYCAST_INIT_STACK_COMMIT,
                          FBFC_FLYCAST_INIT_STACK_RESERVE,
                          STACK_SIZE_PARAM_IS_A_RESERVATION,
                          fbfc_bigstack_fiber_entry,
                          &call);
    if (!fiber) {
        err = GetLastError();
        fbfc_bigstack_log("CreateFiberEx failed for %s err=%lu reserve=%lu; calling on current stack",
                          name ? name : "?", (unsigned long)err,
                          (unsigned long)FBFC_FLYCAST_INIT_STACK_RESERVE);
        fn();
        return;
    }

    fbfc_bigstack_log("%s on large fiber stack commit=%lu reserve=%lu",
                      name ? name : "flycast call",
                      (unsigned long)FBFC_FLYCAST_INIT_STACK_COMMIT,
                      (unsigned long)FBFC_FLYCAST_INIT_STACK_RESERVE);
    SwitchToFiber(fiber);
    DeleteFiber(fiber);
}
#else
static void fbfc_call_on_large_stack(void (*fn)(void), const char* name)
{
    (void)name;
    if (fn)
        fn();
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

void fbfc_flycast_retro_init_bigstack(void)
{
    fbfc_call_on_large_stack(flycast_retro_init, "flycast_retro_init");
}

#ifdef __cplusplus
}
#endif

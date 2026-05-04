#ifndef FBNEO_FLYCAST_BIGSTACK_H
#define FBNEO_FLYCAST_BIGSTACK_H

/*
 * FBNeo <-> Flycast bridge helper.
 *
 * Flycast's libretro init can use a deep stack on MinGW/Win64 during VMEM/JIT
 * setup. FBNeo's UI/emulation thread may not have enough stack even if the PE
 * default stack was enlarged, so run retro_init on a temporary Fiber with a
 * large reserved stack while staying on the same OS thread.
 */

#if defined(_WIN32)
#include <windows.h>
#include <stdio.h>

#ifndef ERROR_ALREADY_FIBER
#define ERROR_ALREADY_FIBER 1280L
#endif

#ifndef FBFC_BIGSTACK_COMMIT_BYTES
#define FBFC_BIGSTACK_COMMIT_BYTES 1048576u      /* 1 MiB committed up front */
#endif

#ifndef FBFC_BIGSTACK_RESERVE_BYTES
#define FBFC_BIGSTACK_RESERVE_BYTES 268435456u   /* 256 MiB reserved */
#endif

typedef void (*fbfc_bigstack_void_fn)(void);

struct FbfcBigstackJob {
    fbfc_bigstack_void_fn fn;
    void* caller_fiber;
};

static void fbfc_bigstack_log(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "atomiswave/flycast-bigstack: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);

    FILE* f = fopen("fbneo_flycast_runtime/fbfc_debug.log", "ab");
    if (!f) f = fopen("fbfc_debug.log", "ab");
    if (f) {
        fprintf(f, "atomiswave/flycast-bigstack: ");
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fputc('\n', f);
        fclose(f);
    }
}

static VOID CALLBACK fbfc_bigstack_fiber_proc(void* param)
{
    struct FbfcBigstackJob* job = (struct FbfcBigstackJob*)param;
    if (job && job->fn) {
        job->fn();
    }
    SwitchToFiber(job->caller_fiber);
}

static void fbfc_call_flycast_retro_init_bigstack(fbfc_bigstack_void_fn fn)
{
    if (!fn) return;

    BOOL converted_thread = FALSE;
    void* caller_fiber = ConvertThreadToFiber(NULL);

    if (caller_fiber) {
        converted_thread = TRUE;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_FIBER) {
            caller_fiber = GetCurrentFiber();
        } else {
            fbfc_bigstack_log("ConvertThreadToFiber failed err=%lu; falling back to current stack", (unsigned long)err);
            fn();
            return;
        }
    }

    struct FbfcBigstackJob job;
    job.fn = fn;
    job.caller_fiber = caller_fiber;

    void* worker_fiber = CreateFiberEx(
        (SIZE_T)FBFC_BIGSTACK_COMMIT_BYTES,
        (SIZE_T)FBFC_BIGSTACK_RESERVE_BYTES,
        0,
        fbfc_bigstack_fiber_proc,
        &job);

    if (!worker_fiber) {
        DWORD err = GetLastError();
        fbfc_bigstack_log("CreateFiberEx failed err=%lu; falling back to current stack", (unsigned long)err);
        fn();
        if (converted_thread) ConvertFiberToThread();
        return;
    }

    fbfc_bigstack_log("flycast_retro_init on large fiber stack commit=%u reserve=%u",
        (unsigned)FBFC_BIGSTACK_COMMIT_BYTES,
        (unsigned)FBFC_BIGSTACK_RESERVE_BYTES);

    SwitchToFiber(worker_fiber);
    DeleteFiber(worker_fiber);

    if (converted_thread) {
        ConvertFiberToThread();
    }

    fbfc_bigstack_log("flycast_retro_init returned from large fiber stack");
}

#else

typedef void (*fbfc_bigstack_void_fn)(void);
static void fbfc_call_flycast_retro_init_bigstack(fbfc_bigstack_void_fn fn)
{
    if (fn) fn();
}

#endif /* _WIN32 */

#endif /* FBNEO_FLYCAST_BIGSTACK_H */

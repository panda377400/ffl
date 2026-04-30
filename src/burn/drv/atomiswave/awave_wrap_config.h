#ifndef FBNEO_AWAVE_WRAP_CONFIG_H
#define FBNEO_AWAVE_WRAP_CONFIG_H

#ifndef FLYCAST_FBNEO_EMBED
#define FLYCAST_FBNEO_EMBED 1
#endif

#ifdef _WIN32
#include <sal.h>
#endif

#define perf_cb flycast_perf_cb
#define perf_get_cpu_features_cb flycast_perf_get_cpu_features_cb
#define log_cb flycast_log_cb
#define video_cb flycast_video_cb
#define poll_cb flycast_poll_cb
#define input_cb flycast_input_cb
#define audio_batch_cb flycast_audio_batch_cb
#define environ_cb flycast_environ_cb
#define frontend_clear_thread_waits_cb flycast_frontend_clear_thread_waits_cb
#define retro_audio_init flycast_retro_audio_init
#define retro_audio_deinit flycast_retro_audio_deinit
#define retro_audio_flush_buffer flycast_retro_audio_flush_buffer
#define retro_audio_upload flycast_retro_audio_upload

#ifndef __LIBRETRO__
#define __LIBRETRO__ 1
#endif

#ifndef HAVE_GLSYM_PRIVATE
#define HAVE_GLSYM_PRIVATE 1
#endif

#ifndef HAVE_OPENGL
#define HAVE_OPENGL 1
#endif

#ifndef HAVE_GL3
#define HAVE_GL3 1
#endif

#ifndef HAVE_GL4
#define HAVE_GL4 1
#endif

#ifndef HAVE_OIT
#define HAVE_OIT 1
#endif

#ifndef NO_VERIFY
#define NO_VERIFY 1
#endif

#ifndef XBYAK_NO_OP_NAMES
#define XBYAK_NO_OP_NAMES 1
#endif

#ifdef MMX
#undef MMX
#endif

#ifdef __in
#undef __in
#endif

#ifdef __out
#undef __out
#endif

#ifdef __inout
#undef __inout
#endif

#ifdef interface
#undef interface
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _In_opt_
#define _In_opt_
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#if defined(BUILD_X64_EXE)
#ifndef TARGET_WIN64
#define TARGET_WIN64 1
#endif
#else
#ifndef TARGET_WIN86
#define TARGET_WIN86 1
#endif
#endif

#endif

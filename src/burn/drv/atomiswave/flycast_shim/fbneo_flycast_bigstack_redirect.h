#pragma once

/*
 * Compile-time redirect for the FBNeo/Flycast static libretro bridge.
 *
 * The crash log stops inside flycast_retro_init()/retro_init() with
 * Windows exception c00000fd (stack overflow). GNU ld --wrap did not
 * catch the call after FBNeo's drivers.o partial link, so this header is
 * force-included for C++ sources and rewrites only the shim's call site:
 *
 *     flycast_retro_init() -> fbfc_flycast_retro_init_bigstack()
 *
 * The real flycast_retro_init symbol is still called by the C trampoline
 * in fbneo_flycast_bigstack.c, which is not compiled with this header.
 */

#ifdef __cplusplus
extern "C" {
#endif

void fbfc_flycast_retro_init_bigstack(void);

#ifdef __cplusplus
}
#endif

#define flycast_retro_init fbfc_flycast_retro_init_bigstack

#pragma once
/*
 * fbneo_libretro_prefix.h
 *
 * Source-level prefixing for the statically linked Flycast libretro core.
 * Include this BEFORE <libretro.h> in vendor/flycast/shell/libretro/libretro.cpp.
 * It turns the public libretro entry points into flycast_retro_* symbols so
 * FBNeo can link to this core directly without loading flycast_libretro.dll.
 */
#ifndef FBNEO_FLYCAST_NO_LIBRETRO_PREFIX

#define retro_set_environment              flycast_retro_set_environment
#define retro_set_video_refresh            flycast_retro_set_video_refresh
#define retro_set_audio_sample             flycast_retro_set_audio_sample
#define retro_set_audio_sample_batch       flycast_retro_set_audio_sample_batch
#define retro_set_input_poll               flycast_retro_set_input_poll
#define retro_set_input_state              flycast_retro_set_input_state
#define retro_init                         flycast_retro_init
#define retro_deinit                       flycast_retro_deinit
#define retro_api_version                  flycast_retro_api_version
#define retro_get_system_info              flycast_retro_get_system_info
#define retro_get_system_av_info           flycast_retro_get_system_av_info
#define retro_set_controller_port_device   flycast_retro_set_controller_port_device
#define retro_reset                        flycast_retro_reset
#define retro_run                          flycast_retro_run
#define retro_serialize_size               flycast_retro_serialize_size
#define retro_serialize                    flycast_retro_serialize
#define retro_unserialize                  flycast_retro_unserialize
#define retro_load_game                    flycast_retro_load_game
#define retro_unload_game                  flycast_retro_unload_game

/* Standard libretro exports that some Flycast revisions also provide. */
#define retro_get_region                   flycast_retro_get_region
#define retro_get_memory_data              flycast_retro_get_memory_data
#define retro_get_memory_size              flycast_retro_get_memory_size
#define retro_load_game_special            flycast_retro_load_game_special
#define retro_cheat_reset                  flycast_retro_cheat_reset
#define retro_cheat_set                    flycast_retro_cheat_set

#endif /* FBNEO_FLYCAST_NO_LIBRETRO_PREFIX */

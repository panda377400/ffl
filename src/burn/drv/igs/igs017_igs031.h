#ifndef IGS017_IGS031_H
#define IGS017_IGS031_H

#include "burnint.h"

typedef UINT8 (*igs017_port_read_cb)();
typedef UINT16 (*igs017_palette_scramble_cb)(UINT16 bgr);

INT32 igs017_igs031_init(UINT8 *tile_rom, INT32 tile_len, UINT8 *sprite_rom, INT32 sprite_len);
void igs017_igs031_exit();
void igs017_igs031_reset();
INT32 igs017_igs031_scan(INT32 nAction, INT32 *pnMin);

void igs017_igs031_set_inputs(igs017_port_read_cb port_a, igs017_port_read_cb port_b, igs017_port_read_cb port_c);
void igs017_igs031_set_text_reverse_bits(INT32 reverse_bits);
void igs017_igs031_set_palette_scramble(igs017_palette_scramble_cb cb);

UINT8 igs017_igs031_read(UINT32 address);
void igs017_igs031_write(UINT32 address, UINT8 data);

INT32 igs017_igs031_draw();
void igs017_igs031_recalc_palette();
UINT32 *igs017_igs031_palette();

INT32 igs017_igs031_irq_enable();
INT32 igs017_igs031_nmi_enable();
INT32 igs017_igs031_video_disable();

void igs017_igs031_lhzb2_decrypt_tiles(UINT8 *rom, INT32 len);
void igs017_igs031_mgcs_decrypt_tiles(UINT8 *rom, INT32 len);
void igs017_igs031_tarzan_decrypt_tiles(UINT8 *rom, INT32 len, INT32 address_xor);
void igs017_igs031_slqz2_decrypt_tiles(UINT8 *rom, INT32 len);
void igs017_igs031_sdwx_gfx_decrypt(UINT8 *rom, INT32 len);

void igs017_igs031_mgcs_flip_sprites(UINT8 *rom, INT32 len, INT32 max_size);
void igs017_igs031_lhzb2_decrypt_sprites(UINT8 *rom, INT32 len);
void igs017_igs031_tarzan_decrypt_sprites(UINT8 *rom, INT32 len, INT32 max_size, INT32 flip_size);
void igs017_igs031_spkrform_decrypt_sprites(UINT8 *rom, INT32 len);
void igs017_igs031_starzan_decrypt_sprites(UINT8 *rom, INT32 len, INT32 max_size, INT32 flip_size);
void igs017_igs031_tjsb_decrypt_sprites(UINT8 *rom, INT32 len);
void igs017_igs031_jking302us_decrypt_sprites(UINT8 *rom, INT32 len);

#endif

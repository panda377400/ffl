#include "tiles_generic.h"
#include "bitswap.h"
#include "igs017_igs031.h"

static UINT8 *IgsTileROM;
static UINT8 *IgsTileROMExp;
static UINT8 *IgsSpriteROM;
static UINT8 *IgsSpriteExp;

static UINT8 *m_videoram;
static UINT8 *m_spriteram;
static UINT8 *m_palram;
static UINT8 *m_fg_videoram;
static UINT8 *m_bg_videoram;
static UINT8 *m_unkram;

static UINT32 *m_palette;

static INT32 IgsTileLen;
static INT32 IgsSpriteLen;
static INT32 IgsSpriteExpLen;
static INT32 IgsReverseBits;
static INT32 m_video_disable;
static INT32 m_nmi_enable;
static INT32 m_irq_enable;

static igs017_port_read_cb IgsPortReadA;
static igs017_port_read_cb IgsPortReadB;
static igs017_port_read_cb IgsPortReadC;
static igs017_palette_scramble_cb IgsPaletteScramble;

static inline UINT16 read_le16(const UINT8 *src)
{
	return src[0] | (src[1] << 8);
}

static inline void write_le16(UINT8 *dst, UINT16 data)
{
	dst[0] = data & 0xff;
	dst[1] = data >> 8;
}

static inline UINT32 igs_pal5bit(UINT32 data)
{
	return (data << 3) | (data >> 2);
}

static inline UINT32 igs_calc_color(UINT16 bgr)
{
	UINT32 r = igs_pal5bit((bgr >> 0) & 0x1f);
	UINT32 g = igs_pal5bit((bgr >> 5) & 0x1f);
	UINT32 b = igs_pal5bit((bgr >> 10) & 0x1f);

	return BurnHighCol(r, g, b, 0);
}

static inline const UINT32 COLOR(UINT16 x) { return (x >> 2) & 7; }

static tilemap_callback(igs_fg)
{
	UINT16 code = m_fg_videoram[(offs * 4) + 0] | (m_fg_videoram[(offs * 4) + 1] << 8);
	UINT16 attr = m_fg_videoram[(offs * 4) + 2] | (m_fg_videoram[(offs * 4) + 3] << 8);

	TILE_SET_INFO(0, code, COLOR(attr), TILE_FLIPXY(attr >> 5));
}

static tilemap_callback(igs_bg)
{
	UINT16 code = m_bg_videoram[(offs * 4) + 0] | (m_bg_videoram[(offs * 4) + 1] << 8);
	UINT16 attr = m_bg_videoram[(offs * 4) + 2] | (m_bg_videoram[(offs * 4) + 3] << 8);

	TILE_SET_INFO(0, code, COLOR(attr)+8, TILE_FLIPXY(attr >> 5));
}

static void igs_palette_write(INT32 offset)
{
	offset &= ~1;

	UINT16 bgr = read_le16(m_palram + offset);
	if (IgsPaletteScramble) {
		bgr = IgsPaletteScramble(bgr);
	}

	m_palette[offset >> 1] = igs_calc_color(bgr);
}

static void igs_decode_tiles()
{
	INT32 plane[4] = { 24, 16, 8, 0 };
	INT32 xoffs[8] = { STEP8(0, 1) };
	INT32 yoffs[8] = { STEP8(0, 32) };
	INT32 num = IgsTileLen / 0x20;

	if (IgsTileROMExp == NULL) {
		IgsTileROMExp = (UINT8*)BurnMalloc(num * 0x40);
	}

	if (IgsTileROMExp == NULL) {
		return;
	}

	if (IgsReverseBits) {
		for (INT32 i = 0; i < IgsTileLen; i++) {
			IgsTileROM[i] = BITSWAP08(IgsTileROM[i], 0, 1, 2, 3, 4, 5, 6, 7);
		}
	}

	memset(IgsTileROMExp, 0, num * 0x40);
	GfxDecode(num, 4, 8, 8, plane, xoffs, yoffs, 0x100, IgsTileROM, IgsTileROMExp);
}

static void igs_expand_sprites()
{
	if (IgsSpriteExp == NULL) {
		IgsSpriteExpLen = (IgsSpriteLen / 2) * 3;
		IgsSpriteExp = (UINT8*)BurnMalloc(IgsSpriteExpLen);
	}

	if (IgsSpriteExp == NULL) {
		return;
	}

	for (INT32 i = 0; i < IgsSpriteLen / 2; i++) {
		UINT16 pens = read_le16(IgsSpriteROM + (i * 2));
		IgsSpriteExp[(i * 3) + 0] = (pens >> 0) & 0x1f;
		IgsSpriteExp[(i * 3) + 1] = (pens >> 5) & 0x1f;
		IgsSpriteExp[(i * 3) + 2] = (pens >> 10) & 0x1f;
	}
}

static void igs_draw_sprite(INT32 sx, INT32 sy, INT32 dimx, INT32 dimy, INT32 flipx, INT32 color, UINT32 addr)
{
	if (addr + (dimx * dimy) >= (UINT32)IgsSpriteExpLen) {
		return;
	}

	const UINT8 *srcbase = IgsSpriteExp + addr;
	const UINT16 pal = 0x100 + (color << 5);

	INT32 xinc = flipx ? -1 : 1;
	INT32 xbase = flipx ? (dimx - 1) : 0;
	INT32 minx = 0;
	INT32 maxx = nScreenWidth - 1;
	INT32 miny = 0;
	INT32 maxy = nScreenHeight - 1;

	for (INT32 y = 0; y < dimy; y++) {
		INT32 dy = sy + y;
		if (dy < miny || dy > maxy) {
			continue;
		}

		const UINT8 *src = srcbase + (y * dimx);
		UINT16 *dst = pTransDraw + (dy * nScreenWidth);
		INT32 xidx = xbase;

		for (INT32 x = 0; x < dimx; x++) {
			INT32 dx = sx + x;
			UINT8 px = src[xidx];

			if (dx >= minx && dx <= maxx && px != 0x1f) {
				dst[dx] = pal + px;
			}

			xidx += xinc;
		}
	}
}

static void igs_draw_sprites()
{
	for (INT32 offs = 0; offs < 0x800; offs += 8) {
		UINT8 *s = m_spriteram + offs;

		INT32 y = s[0] | (s[1] << 8);
		INT32 x = s[2] | (s[3] << 8);
		UINT32 addr = ((s[4] >> 6) | (s[5] << 2) | (s[6] << 10) | ((s[7] & 0x0f) << 18)) * 3;
		INT32 flipx = s[7] & 0x10;
		INT32 dimx = ((((s[4] & 0x3f) << 2) | ((s[3] & 0xc0) >> 6)) + 1) * 3;
		INT32 dimy = ((y >> 10) | ((x & 0x03) << 6)) + 1;
		INT32 sx = ((x >> 3) & 0x1ff) - ((x >> 3) & 0x200);
		INT32 sy = (y & 0x1ff) - (y & 0x200);
		INT32 color = (s[7] & 0xe0) >> 5;

		if (sy == -0x200) {
			break;
		}

		igs_draw_sprite(sx, sy, dimx, dimy, flipx, color, addr);
	}
}

void igs017_igs031_set_inputs(igs017_port_read_cb port_a, igs017_port_read_cb port_b, igs017_port_read_cb port_c)
{
	IgsPortReadA = port_a;
	IgsPortReadB = port_b;
	IgsPortReadC = port_c;
}

void igs017_igs031_set_text_reverse_bits(INT32 reverse_bits)
{
	IgsReverseBits = reverse_bits;
}

void igs017_igs031_set_palette_scramble(igs017_palette_scramble_cb cb)
{
	IgsPaletteScramble = cb;
}

INT32 igs017_igs031_init(UINT8 *tile_rom, INT32 tile_len, UINT8 *sprite_rom, INT32 sprite_len)
{
	GenericTilesInit();
	IgsTileROM = tile_rom;
	IgsSpriteROM = sprite_rom;
	IgsTileLen = tile_len;
	IgsSpriteLen = sprite_len;

	m_videoram = (UINT8*)BurnMalloc(0x5000);
	m_palette = (UINT32*)BurnMalloc(0x200 * sizeof(UINT32));

	if (m_videoram == NULL || m_palette == NULL) {
		return 1;
	}

	memset(m_videoram, 0, 0x5000);
	memset(m_palette, 0, 0x200 * sizeof(UINT32));

	m_spriteram = m_videoram + 0x0000;
	m_palram = m_videoram + 0x0800;
	m_unkram = m_videoram + 0x0c00;
	m_fg_videoram = m_videoram + 0x1000;
	m_bg_videoram = m_videoram + 0x3000;

	igs_decode_tiles();
	igs_expand_sprites();

	GenericTilemapInit(0, TILEMAP_SCAN_ROWS, igs_fg_map_callback, 8, 8, 64, 32);
	GenericTilemapInit(1, TILEMAP_SCAN_ROWS, igs_bg_map_callback, 8, 8, 64, 32);
	GenericTilemapSetGfx(0, IgsTileROMExp, 4, 8, 8, (IgsTileLen / 0x20) * 0x40, 0, 0x0f);
	GenericTilemapSetTransparent(0, 0x0f);
	GenericTilemapSetTransparent(1, 0x0f);

	m_video_disable = 0;
	m_nmi_enable = 0;
	m_irq_enable = 0;

	return 0;
}

void igs017_igs031_exit()
{
	BurnFree(IgsTileROMExp);
	BurnFree(IgsSpriteExp);
	BurnFree(m_videoram);
	BurnFree(m_palette);

	IgsTileROM = NULL;
	IgsTileROMExp = NULL;
	IgsSpriteROM = NULL;
	IgsSpriteExp = NULL;
	m_videoram = NULL;
	m_palette = NULL;
	m_spriteram = NULL;
	m_palram = NULL;
	m_fg_videoram = NULL;
	m_bg_videoram = NULL;
	m_unkram = NULL;
	IgsTileLen = 0;
	IgsSpriteLen = 0;
	IgsSpriteExpLen = 0;
	IgsReverseBits = 0;
	IgsPortReadA = NULL;
	IgsPortReadB = NULL;
	IgsPortReadC = NULL;
	IgsPaletteScramble = NULL;
	GenericTilesExit();
}

void igs017_igs031_reset()
{
	m_video_disable = 0;
	m_nmi_enable = 0;
	m_irq_enable = 0;
}

void igs017_igs031_recalc_palette()
{
	for (INT32 i = 0; i < 0x400; i += 2) {
		igs_palette_write(i);
	}
}

UINT32 *igs017_igs031_palette()
{
	return m_palette;
}

INT32 igs017_igs031_irq_enable()
{
	return m_irq_enable;
}

INT32 igs017_igs031_nmi_enable()
{
	return m_nmi_enable;
}

INT32 igs017_igs031_video_disable()
{
	return m_video_disable;
}

UINT8 igs017_igs031_read(UINT32 address)
{
	address &= 0x7fff;

	if (address >= 0x1000 && address <= 0x17ff) return m_spriteram[address - 0x1000];
	if (address >= 0x1800 && address <= 0x1bff) return m_palram[address - 0x1800];
	if (address >= 0x1c00 && address <= 0x1fff) return m_unkram[address - 0x1c00];
	if (address >= 0x2010 && address <= 0x2012) {
		switch (address - 0x2010) {
			case 0: return IgsPortReadA ? IgsPortReadA() : 0xff;
			case 1: return IgsPortReadB ? IgsPortReadB() : 0xff;
			case 2: return IgsPortReadC ? IgsPortReadC() : 0xff;
		}
	}
	if (address >= 0x4000 && address <= 0x5fff) return m_fg_videoram[address - 0x4000];
	if (address >= 0x6000 && address <= 0x7fff) return m_bg_videoram[address - 0x6000];

	return 0xff;
}

void igs017_igs031_write(UINT32 address, UINT8 data)
{
	address &= 0x7fff;

	if (address >= 0x1000 && address <= 0x17ff) {
		m_spriteram[address - 0x1000] = data;
		return;
	}

	if (address >= 0x1800 && address <= 0x1bff) {
		m_palram[address - 0x1800] = data;
		igs_palette_write(address - 0x1800);
		return;
	}

	if (address >= 0x1c00 && address <= 0x1fff) {
		m_unkram[address - 0x1c00] = data;
		return;
	}

	switch (address) {
		case 0x2012:
			m_video_disable = data & 1;
		return;

		case 0x2014:
			m_nmi_enable = data & 1;
		return;

		case 0x2015:
			m_irq_enable = data & 1;
		return;
	}

	if (address >= 0x4000 && address <= 0x5fff) {
		INT32 offset = address - 0x4000;
		if (m_fg_videoram[offset] != data) {
			m_fg_videoram[offset] = data;
		}
		return;
	}

	if (address >= 0x6000 && address <= 0x7fff) {
		INT32 offset = address - 0x6000;
		if (m_bg_videoram[offset] != data) {
			m_bg_videoram[offset] = data;
		}
		return;
	}
}

INT32 igs017_igs031_draw()
{
	BurnTransferClear();

	if (m_video_disable)
		return 0;

	if (nBurnLayer & 1) GenericTilemapDraw(1, pTransDraw, TMAP_FORCEOPAQUE);
	if (nSpriteEnable & 2) igs_draw_sprites();
	if (nBurnLayer & 2) GenericTilemapDraw(0, pTransDraw, 0);

	return 0;
}

INT32 igs017_igs031_scan(INT32 nAction, INT32 *pnMin)
{
	if (pnMin) {
		*pnMin = 0x029743;
	}

	if (nAction & ACB_MEMORY_RAM) {
		struct BurnArea ba;
		memset(&ba, 0, sizeof(ba));
		ba.Data = m_videoram;
		ba.nLen = 0x5000;
		ba.szName = "IGS017/031 RAM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(m_video_disable);
		SCAN_VAR(m_nmi_enable);
		SCAN_VAR(m_irq_enable);
	}

	return 0;
}

void igs017_igs031_lhzb2_decrypt_tiles(UINT8 *rom, INT32 len)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		INT32 addr = (i & ~0xffffff) | BITSWAP24(i, 23,22,21,20,19,18,17,1,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,0);
		rom[i] = tmp[addr];
	}

	BurnFree(tmp);
}

void igs017_igs031_mgcs_decrypt_tiles(UINT8 *rom, INT32 len)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		INT32 addr = (i & ~0xffff) | BITSWAP16(i, 15,14,13,12,11,10,6,7,8,9,5,4,3,2,1,0);
		rom[i ^ 1] = BITSWAP08(tmp[addr], 0,1,2,3,4,5,6,7);
	}

	BurnFree(tmp);
}

void igs017_igs031_tarzan_decrypt_tiles(UINT8 *rom, INT32 len, INT32 address_xor)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		INT32 addr = (i & ~0xffff) | (BITSWAP16(i, 15,14,13,12,11,7,8,6,10,9,5,4,3,2,1,0) ^ address_xor);
		rom[i] = BITSWAP08(tmp[addr], 0,1,2,3,4,5,6,7);
	}

	BurnFree(tmp);
}

void igs017_igs031_slqz2_decrypt_tiles(UINT8 *rom, INT32 len)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		INT32 addr = (i & ~0xff) | BITSWAP08(i, 7,4,5,6,3,2,1,0);
		rom[i] = tmp[addr];
	}

	BurnFree(tmp);
}

void igs017_igs031_sdwx_gfx_decrypt(UINT8 *rom, INT32 len)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		rom[i] = tmp[BITSWAP24(i, 23,22,21,20,19,18,17,16,15,14,13,12,11,7,8,6,10,9,5,4,3,2,1,0)];
	}

	BurnFree(tmp);
}

void igs017_igs031_mgcs_flip_sprites(UINT8 *rom, INT32 len, INT32 max_size)
{
	INT32 rom_size = max_size ? max_size : len;
	if (rom_size > len) rom_size = len;

	for (INT32 i = 0; i < rom_size; i += 2) {
		UINT16 pixels = read_le16(rom + i);
		pixels = BITSWAP16(pixels, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
		pixels = BITSWAP16(pixels, 15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14);
		write_le16(rom + i, pixels);
	}
}

void igs017_igs031_lhzb2_decrypt_sprites(UINT8 *rom, INT32 len)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		INT32 addr = (i & ~0xffff) | BITSWAP16(i, 15,14,13,6,7,10,9,8,11,12,5,4,3,2,1,0);
		rom[i] = tmp[addr];
	}

	for (INT32 i = 0; i < len; i += 2) {
		UINT16 data = read_le16(rom + i);
		data = BITSWAP16(data, 15,7,6,5,4,3,2,1,0,14,13,12,11,10,9,8);
		write_le16(rom + i, data);
	}

	BurnFree(tmp);
}

void igs017_igs031_tarzan_decrypt_sprites(UINT8 *rom, INT32 len, INT32 max_size, INT32 flip_size)
{
	igs017_igs031_mgcs_flip_sprites(rom, len, flip_size);

	INT32 rom_size = max_size ? max_size : len;
	if (rom_size > len) rom_size = len;

	UINT8 *tmp = (UINT8*)BurnMalloc(rom_size);
	if (tmp == NULL) return;

	memcpy(tmp, rom, rom_size);
	for (INT32 i = 0; i < rom_size; i++) {
		INT32 addr = (i & ~0xffff) | BITSWAP16(i, 15,14,13,9,10,11,12,5,6,7,8,4,3,2,1,0);
		rom[i] = tmp[addr];
	}

	BurnFree(tmp);
}

void igs017_igs031_spkrform_decrypt_sprites(UINT8 *rom, INT32 len)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		INT32 addr;
		if (i & 0x80000) {
			addr = (i & ~0xff) | BITSWAP08(i, 7,6,3,4,5,2,1,0);
		} else {
			addr = (i & ~0xffff) | BITSWAP16(i, 15,14,13,12,11,10,4,8,7,6,5,9,3,2,1,0);
		}
		rom[i] = tmp[addr];
	}

	BurnFree(tmp);
}

void igs017_igs031_starzan_decrypt_sprites(UINT8 *rom, INT32 len, INT32 max_size, INT32 flip_size)
{
	igs017_igs031_tarzan_decrypt_sprites(rom, len, max_size, flip_size);

	if (max_size <= 0 || max_size >= len) {
		return;
	}

	INT32 overlay_len = len - max_size;
	UINT8 *tmp = (UINT8*)BurnMalloc(overlay_len);
	if (tmp == NULL) return;

	memcpy(tmp, rom + max_size, overlay_len);
	for (INT32 i = 0; i < overlay_len; i++) {
		INT32 addr = (i & ~0xffff) | BITSWAP16(i, 15,14,13,12,11,10,9,6,5,8,7,1,2,3,4,0);
		rom[max_size + i] = tmp[addr];
	}

	BurnFree(tmp);
}

void igs017_igs031_tjsb_decrypt_sprites(UINT8 *rom, INT32 len)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	memcpy(tmp, rom, len);
	for (INT32 i = 0; i < len; i++) {
		INT32 addr = (i & ~0xff) | BITSWAP08(i, 7,6,5,2,1,4,3,0);
		rom[i] = tmp[addr];
	}

	for (INT32 i = 0; i < len; i += 2) {
		UINT16 data = read_le16(rom + i);
		data = BITSWAP16(data, 15,14,13,12,11,10,9,1,7,6,5,4,3,2,8,0);
		write_le16(rom + i, data);
	}

	BurnFree(tmp);
}

void igs017_igs031_jking302us_decrypt_sprites(UINT8 *rom, INT32 len)
{
	for (INT32 i = 0; i < len; i += 2) {
		UINT16 data = read_le16(rom + i);
		data = BITSWAP16(data, 0,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1);
		write_le16(rom + i, data);
	}
}

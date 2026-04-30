#include "tiles_generic.h"
#include "arm7_intf.h"
#include "ics2115.h"
#include "bitswap.h"
#include "burn_pal.h"
#include "timer.h"

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;
static UINT8 *DrvArmROM;
static UINT8 *DrvArmPrgROM;
static UINT8 *DrvArmPrgMap;
static UINT8 *DrvTextROM;
static UINT8 *DrvTxGfx;
static UINT8 *DrvBgGfx;
static UINT16 *DrvSprColROM;
static UINT16 *DrvSprMaskROM;
static UINT8 *DrvSndROM;
static UINT8 *DrvNVRAM;
static UINT8 *DrvArmRAM;
static UINT8 *DrvMainRAM;
static UINT8 *DrvBgRAM;
static UINT8 *DrvTxRAM;
static UINT8 *DrvRowRAM;
static UINT8 *DrvPalRAM;
static UINT8 *DrvVidRegs;
static UINT8 *DrvSprBuf;

static UINT32 *DrvPalette;
static UINT8 DrvRecalc;
static UINT8 DrvReset;
static UINT8 DrvDips[3] = { 0xff, 0xff, 0xff };
static UINT8 DrvJoy1[16];
static UINT16 DrvInputs[3];
static UINT8 DrvMahjong[5][6];

static UINT8 gpio_out;
static UINT8 kbd_sel;
static UINT8 irq_source;
static UINT8 cpu_fiq_enable;
static UINT8 cpu_irq_enable;
static UINT8 cpu_irq_pending;
static UINT8 cpu_timer_rate[2];
static INT32 cpu_timer_period[2];
static INT32 cpu_timer_count[2];
static UINT8 ics_irq_state;
static UINT8 ext_fiq_state;
static UINT8 clear_mem_previous;
static UINT8 clear_mem_requested;
static UINT8 DrvHopperMotor;
static UINT8 DrvCoin1Pulse;
static UINT8 DrvCoin1Prev;
static UINT16 DrvOutLatch[2];
static UINT32 xor_table[0x100];

static const INT32 tx_gfx_len = 0x100000;
static const INT32 bg_tile_count = 0x333;
static const INT32 bg_gfx_len = bg_tile_count * (32 * 32);

struct sprite_t
{
	INT32 x, y;
	INT32 width, height;
	INT32 color, flip, pri;
	INT32 xgrow, ygrow;
	UINT32 xzoom, yzoom;
	UINT32 offs;
};

static sprite_t spritelist[0xa00 / 2 / 5];
static sprite_t *sprite_ptr_pre;
static UINT32 sprite_aoffset;
static UINT32 sprite_boffset;
static UINT8 sprite_abit;

static UINT8 m027_023vid_read_byte(UINT32 address);
static UINT16 m027_023vid_read_word(UINT32 address);
static UINT32 m027_023vid_read_long(UINT32 address);
static void m027_023vid_write_byte(UINT32 address, UINT8 data);
static void m027_023vid_write_word(UINT32 address, UINT16 data);
static void m027_023vid_write_long(UINT32 address, UINT32 data);

static struct BurnInputInfo MxsqyInputList[] = {
	{"Coin 1",            BIT_DIGITAL, DrvJoy1 +  8, "p1 coin"   },
	{"Payout",            BIT_DIGITAL, DrvJoy1 + 13, "service1"  },
	{"Key In",            BIT_DIGITAL, DrvJoy1 +  9, "service2"  },
	{"Key Out",           BIT_DIGITAL, DrvJoy1 + 10, "service3"  },
	{"Book",              BIT_DIGITAL, DrvJoy1 + 11, "diag"      },
	{"Service",           BIT_DIGITAL, DrvJoy1 + 12, "service"   },
	{"Start",             BIT_DIGITAL, DrvJoy1 +  0, "p1 start"  },
	{"Up",                BIT_DIGITAL, DrvJoy1 +  1, "p1 up"     },
	{"Down",              BIT_DIGITAL, DrvJoy1 +  2, "p1 down"   },
	{"Left",              BIT_DIGITAL, DrvJoy1 +  3, "p1 left"   },
	{"Right",             BIT_DIGITAL, DrvJoy1 +  4, "p1 right"  },
	{"Button 1",          BIT_DIGITAL, DrvJoy1 +  5, "p1 fire 1" },
	{"Button 2",          BIT_DIGITAL, DrvJoy1 +  6, "p1 fire 2" },
	{"Button 3",          BIT_DIGITAL, DrvJoy1 +  7, "p1 fire 3" },
	{"Clear NVRAM",       BIT_DIGITAL, DrvJoy1 + 14, "service4"  },
	{"Mahjong A",         BIT_DIGITAL, DrvMahjong[0] + 0, "mah a"     },
	{"Mahjong E",         BIT_DIGITAL, DrvMahjong[0] + 1, "mah e"     },
	{"Mahjong I",         BIT_DIGITAL, DrvMahjong[0] + 2, "mah i"     },
	{"Mahjong M",         BIT_DIGITAL, DrvMahjong[0] + 3, "mah m"     },
	{"Mahjong Kan",       BIT_DIGITAL, DrvMahjong[0] + 4, "mah kan"   },
	{"Mahjong Start",     BIT_DIGITAL, DrvMahjong[0] + 5, "p1 start"  },
	{"Mahjong B",         BIT_DIGITAL, DrvMahjong[1] + 0, "mah b"     },
	{"Mahjong F",         BIT_DIGITAL, DrvMahjong[1] + 1, "mah f"     },
	{"Mahjong J",         BIT_DIGITAL, DrvMahjong[1] + 2, "mah j"     },
	{"Mahjong N",         BIT_DIGITAL, DrvMahjong[1] + 3, "mah n"     },
	{"Mahjong Reach",     BIT_DIGITAL, DrvMahjong[1] + 4, "mah reach" },
	{"Mahjong Bet",       BIT_DIGITAL, DrvMahjong[1] + 5, "mah bet"   },
	{"Mahjong C",         BIT_DIGITAL, DrvMahjong[2] + 0, "mah c"     },
	{"Mahjong G",         BIT_DIGITAL, DrvMahjong[2] + 1, "mah g"     },
	{"Mahjong K",         BIT_DIGITAL, DrvMahjong[2] + 2, "mah k"     },
	{"Mahjong Chi",       BIT_DIGITAL, DrvMahjong[2] + 3, "mah chi"   },
	{"Mahjong Ron",       BIT_DIGITAL, DrvMahjong[2] + 4, "mah ron"   },
	{"Mahjong D",         BIT_DIGITAL, DrvMahjong[3] + 0, "mah d"     },
	{"Mahjong H",         BIT_DIGITAL, DrvMahjong[3] + 1, "mah h"     },
	{"Mahjong L",         BIT_DIGITAL, DrvMahjong[3] + 2, "mah l"     },
	{"Mahjong Pon",       BIT_DIGITAL, DrvMahjong[3] + 3, "mah pon"   },
	{"Mahjong Last",      BIT_DIGITAL, DrvMahjong[4] + 0, "mah lc"    },
	{"Mahjong Score",     BIT_DIGITAL, DrvMahjong[4] + 1, "mah score" },
	{"Mahjong Double Up", BIT_DIGITAL, DrvMahjong[4] + 2, "mah ff"    },
	{"Mahjong Big",       BIT_DIGITAL, DrvMahjong[4] + 4, "mah big"   },
	{"Mahjong Small",     BIT_DIGITAL, DrvMahjong[4] + 5, "mah small" },
	{"Reset",             BIT_DIGITAL, &DrvReset,    "reset"     },
	{"Dip A",             BIT_DIPSWITCH, DrvDips + 0, "dip"      },
	{"Dip B",             BIT_DIPSWITCH, DrvDips + 1, "dip"      },
	{"Dip C",             BIT_DIPSWITCH, DrvDips + 2, "dip"      },
};

STDINPUTINFO(Mxsqy)

static struct BurnDIPInfo MxsqyDIPList[] = {
	{0x10, 0xff, 0xff, 0xff, NULL                     },
	{0x11, 0xff, 0xff, 0xff, NULL                     },
	{0x12, 0xff, 0xff, 0xff, NULL                     },

	{0   , 0xfe, 0   ,    2, "Controls"              },
	{0x10, 0x01, 0x01, 0x01, "Joystick"              },
	{0x10, 0x01, 0x01, 0x00, "Mahjong"               },
};

STDDIPINFO(Mxsqy)

static inline UINT16 read_le16(UINT8 *src, INT32 offset)
{
	return src[offset + 0] | (src[offset + 1] << 8);
}

static inline void write_le16(UINT8 *src, INT32 offset, UINT16 data)
{
	src[offset + 0] = data & 0xff;
	src[offset + 1] = data >> 8;
}

static inline UINT32 read_le32(UINT8 *src, INT32 offset)
{
	return src[offset + 0] | (src[offset + 1] << 8) | (src[offset + 2] << 16) | (src[offset + 3] << 24);
}

static inline void write_le32(UINT8 *src, INT32 offset, UINT32 data)
{
	src[offset + 0] = data & 0xff;
	src[offset + 1] = (data >> 8) & 0xff;
	src[offset + 2] = (data >> 16) & 0xff;
	src[offset + 3] = (data >> 24) & 0xff;
}

static inline UINT32 gpio_r()
{
	return 0xffffc | (irq_source ? 0x00002 : 0x00000) | 0x00001;
}

static inline UINT32 igs027a_in_port_value()
{
	return 0xff800000 | ((gpio_r() & 0x000fffff) << 3) | 0x00000007;
}

static inline UINT16 dsw_r()
{
	UINT8 result = 0xff;

	for (INT32 i = 0; i < 3; i++) {
		if (!BIT(gpio_out, i)) result &= DrvDips[i];
	}

	return 0xff00 | result;
}

static UINT16 kbd_r()
{
	UINT16 result = 0x003f;

	for (INT32 i = 0; i < 5; i++) {
		if (!BIT(kbd_sel, i)) {
			for (INT32 b = 0; b < 6; b++) {
				if (DrvMahjong[i][b]) result &= ~(1 << b);
			}
		}
	}

	return result;
}

static inline INT32 controls_joystick()
{
	return (DrvDips[0] & 0x01) == 0x01;
}

static inline UINT8 coin1_active()
{
	return DrvCoin1Pulse != 0;
}

static inline UINT8 hopper_line()
{
	return (DrvHopperMotor && ((GetCurrentFrame() % 10) == 0)) ? 0 : 1;
}

static void output_w(INT32 which, UINT16 data)
{
	DrvOutLatch[which] = data;

	if (controls_joystick()) {
		if (which == 1) DrvHopperMotor = (data >> 14) & 1;
	} else {
		if (which == 0) DrvHopperMotor = data & 1;
	}
}

static UINT16 in0_r()
{
	if (controls_joystick()) return 0xffff;

	UINT16 data = 0xffff;
	data = (data & ~0x003f) | kbd_r();
	if (hopper_line() == 0) data &= ~0x0040;
	if (DrvJoy1[12]) data &= ~0x0080;
	if (coin1_active()) data &= ~0x0100;
	if (DrvJoy1[10]) data &= ~0x0200;
	if (DrvJoy1[11]) data &= ~0x0400;
	return data;
}

static UINT16 in1_r()
{
	UINT16 data = 0xffff;

	if (!controls_joystick()) {
		if (DrvJoy1[9]) data &= ~0x0080;
		return data;
	}

	if (DrvJoy1[11]) data &= ~0x0008;
	if (DrvJoy1[12]) data &= ~0x0010;
	if (coin1_active()) data &= ~0x0040;
	if (DrvJoy1[9]) data &= ~0x0080;
	if (DrvJoy1[0]) data &= ~0x0100;
	if (DrvJoy1[1]) data &= ~0x0200;
	if (DrvJoy1[2]) data &= ~0x0400;
	if (DrvJoy1[3]) data &= ~0x0800;
	if (DrvJoy1[4]) data &= ~0x1000;
	if (DrvJoy1[5]) data &= ~0x2000;
	if (DrvJoy1[6]) data &= ~0x4000;
	if (DrvJoy1[7]) data &= ~0x8000;
	return data;
}

static UINT16 in2_r()
{
	UINT16 data = 0xffff;

	if (DrvJoy1[13]) data &= ~0x0001;

	if (controls_joystick()) {
		if (DrvJoy1[10]) data &= ~0x0002;
		if (hopper_line() == 0) data &= ~0x0004;
	}

	return data;
}

static void rebuild_arm_program_map()
{
	memcpy(DrvArmPrgMap, DrvArmPrgROM, 0x80000);

	for (INT32 i = 0; i < 0x80000; i += 4) {
		write_le32(DrvArmPrgMap, i, read_le32(DrvArmPrgROM, i) ^ xor_table[(i >> 2) & 0xff]);
	}
}

static inline UINT32 external_rom_read_long(UINT32 address)
{
	UINT32 offset = (address - 0x08000000) & 0x7fffc;
	UINT32 data = read_le32(DrvArmPrgROM, offset) ^ xor_table[(offset >> 2) & 0xff];

	return data;
}

static void mxsqy_decrypt()
{
	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = read_le16(DrvArmPrgROM, i * 2);

		if ((i & 0x040080) != 0x000080) x ^= 0x0001;
		if ((i & 0x104008) == 0x104008) x ^= 0x0002;
		if ((i & 0x080030) == 0x080010) x ^= 0x0004;
		if ((i & 0x000042) != 0x000042) x ^= 0x0008;
		if ((i & 0x048100) == 0x048000) x ^= 0x0010;
		if ((i & 0x022004) != 0x000004) x ^= 0x0020;
		if ((i & 0x011800) != 0x010000) x ^= 0x0040;
		if ((i & 0x000820) == 0x000820) x ^= 0x0080;

		write_le16(DrvArmPrgROM, i * 2, x);
	}
}

static void gpio_w(UINT8 data)
{
	gpio_out = data & 0x1f;
}

static void kbd_w(UINT16 data)
{
	kbd_sel = data & 0x1f;
}

static void igs027a_update_irq_line()
{
	UINT8 active = (((~cpu_irq_pending) & (~cpu_irq_enable)) & 0xff) != 0;
	Arm7SetIRQLine(ARM7_IRQ_LINE, active ? CPU_IRQSTATUS_ACK : CPU_IRQSTATUS_NONE);
}

static void igs027a_irq_raise(INT32 bit)
{
	if (!BIT(cpu_irq_enable, bit))
	{
		cpu_irq_pending &= ~(1 << bit);
		igs027a_update_irq_line();
	}
}

static void igs027a_external_irq()
{
	igs027a_irq_raise(3);
}

static void igs027a_timer_rate_w(INT32 timer, UINT8 data)
{
	cpu_timer_rate[timer] = data;
	cpu_timer_period[timer] = data ? (4263 * (data + 1)) : 0;
	cpu_timer_count[timer] = 0;
}

static void igs027a_irq_enable_w(UINT8 data)
{
	cpu_irq_enable = data;
	igs027a_update_irq_line();
}

static UINT8 igs027a_irq_pending_r()
{
	UINT8 result = cpu_irq_pending;

	cpu_irq_pending = 0xff;
	igs027a_update_irq_line();

	return result;
}

static void ics2115_irq(INT32 state)
{
	if (state != CPU_IRQSTATUS_NONE && !ics_irq_state)
	{
		irq_source = 1;
		igs027a_external_irq();
	}

	ics_irq_state = (state != CPU_IRQSTATUS_NONE);
}

static void igs027a_external_firq(INT32 state)
{
	ext_fiq_state = (state != 0);

	if (ext_fiq_state && BIT(cpu_fiq_enable, 0)) {
		Arm7SetIRQLine(ARM7_FIRQ_LINE, CPU_IRQSTATUS_AUTO);
	}
}

static void build_inputs()
{
	DrvInputs[0] = 0xffff;
	DrvInputs[1] = 0xffff;
	DrvInputs[2] = 0xffff;

	if (DrvJoy1[11]) DrvInputs[1] &= ~0x0008;
	if (DrvJoy1[12]) DrvInputs[1] &= ~0x0010;
	if (DrvJoy1[ 8]) DrvInputs[1] &= ~0x0040;
	if (DrvJoy1[ 9]) DrvInputs[1] &= ~0x0080;
	if (DrvJoy1[ 0]) DrvInputs[1] &= ~0x0100;
	if (DrvJoy1[ 1]) DrvInputs[1] &= ~0x0200;
	if (DrvJoy1[ 2]) DrvInputs[1] &= ~0x0400;
	if (DrvJoy1[ 3]) DrvInputs[1] &= ~0x0800;
	if (DrvJoy1[ 4]) DrvInputs[1] &= ~0x1000;
	if (DrvJoy1[ 5]) DrvInputs[1] &= ~0x2000;
	if (DrvJoy1[ 6]) DrvInputs[1] &= ~0x4000;
	if (DrvJoy1[ 7]) DrvInputs[1] &= ~0x8000;

	if (DrvJoy1[13]) DrvInputs[2] &= ~0x0001;
	if (DrvJoy1[10]) DrvInputs[2] &= ~0x0002;

	// Keep the existing start key usable if the game is switched to Mahjong mode.
	if (DrvJoy1[0]) DrvMahjong[0][5] = 1;

	if (DrvJoy1[14] && !clear_mem_previous) {
		clear_mem_requested = 1;
	}

	clear_mem_previous = DrvJoy1[14];
}

static void palram_write(INT32 offset, UINT8 data)
{
	DrvPalRAM[offset] = data;

	INT32 base = offset & ~1;
	UINT16 bgr = read_le16(DrvPalRAM, base);

	UINT8 r = (bgr >> 10) & 0x1f;
	UINT8 g = (bgr >>  5) & 0x1f;
	UINT8 b = (bgr >>  0) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	DrvPalette[base >> 1] = BurnHighCol(r, g, b, 0);
}

static tilemap_callback( mxsqy_bg )
{
	UINT16 tileno = read_le16(DrvBgRAM, (offs * 4) + 0);
	UINT16 attr   = read_le16(DrvBgRAM, (offs * 4) + 2);

	TILE_SET_INFO(0, tileno, (attr & 0x3e) >> 1, TILE_FLIPYX(attr >> 6));
}

static tilemap_callback( mxsqy_tx )
{
	UINT16 tileno = read_le16(DrvTxRAM, (offs * 4) + 0);
	UINT16 attr   = read_le16(DrvTxRAM, (offs * 4) + 2);

	TILE_SET_INFO(1, tileno, (attr & 0x3e) >> 1, TILE_FLIPYX(attr >> 6));
}

static INT32 DrvGfxDecode()
{
	INT32 Plane8[4] = { STEP4(0, 1) };
	INT32 XOffs8[8] = { 4, 0, 12, 8, 20, 16, 28, 24 };
	INT32 YOffs8[8] = { STEP8(0, 32) };
	INT32 Plane32[5] = { 4, 3, 2, 1, 0 };
	INT32 XOffs32[32] = { STEP32(0, 5) };
	INT32 YOffs32[32] = { STEP32(0, 32 * 5) };

	UINT8 *tmp = (UINT8*)BurnMalloc(0x80000);
	if (tmp == NULL) return 1;

	memcpy(tmp, DrvTextROM, 0x80000);
	memset(DrvTxGfx, 0x0f, tx_gfx_len);
	GfxDecode(0x80000 / 32, 4, 8, 8, Plane8, XOffs8, YOffs8, 0x100, tmp, DrvTxGfx);

	for (INT32 i = 0; i < 0x80000; i++) {
		tmp[i] = BITSWAP08(DrvTextROM[i], 0,1,2,3,4,5,6,7);
	}

	memset(DrvBgGfx, 0, bg_gfx_len);
	GfxDecode(bg_tile_count, 5, 32, 32, Plane32, XOffs32, YOffs32, 32 * 32 * 5, tmp, DrvBgGfx);
	BurnFree(tmp);

	return 0;
}

static UINT8 videoram_read_byte(UINT32 address)
{
	address &= 0x7fff;

	if (address < 0x4000) return DrvBgRAM[address & 0x0fff];
	if (address < 0x7000) return DrvTxRAM[(address - 0x4000) & 0x1fff];
	return DrvRowRAM[address - 0x7000];
}

static void videoram_write_byte(UINT32 address, UINT8 data)
{
	address &= 0x7fff;

	if (address < 0x4000)
	{
		INT32 offs = address & 0x0fff;
		DrvBgRAM[offs] = data;
		GenericTilemapSetTileDirty(0, offs >> 2);
		return;
	}

	if (address < 0x7000)
	{
		INT32 offs = (address - 0x4000) & 0x1fff;
		DrvTxRAM[offs] = data;
		GenericTilemapSetTileDirty(1, offs >> 2);
		return;
	}

	DrvRowRAM[address - 0x7000] = data;
}

static inline UINT16 get_sprite_pix()
{
	UINT16 srcdat = (DrvSprColROM[sprite_aoffset & ((0x400000 / 2) - 1)] >> sprite_abit) & 0x1f;

	sprite_abit += 5;
	if (sprite_abit >= 15) {
		sprite_aoffset++;
		sprite_abit = 0;
	}

	return srcdat;
}

static inline void draw_sprite_pix(INT32 xdrawpos, INT32 pri, UINT16 *dest, UINT8 *destpri, UINT16 srcdat)
{
	if (xdrawpos < 0 || xdrawpos >= nScreenWidth) return;

	if (!(destpri[xdrawpos] & 1))
	{
		if (!pri || !(destpri[xdrawpos] & 2)) {
			dest[xdrawpos] = srcdat;
		}
	}

	destpri[xdrawpos] |= 1;
}

static inline void draw_sprite_line(INT32 wide, UINT16 *dest, UINT8 *destpri, UINT32 xzoom, INT32 xgrow, INT32 flip, INT32 xpos, INT32 pri, INT32 realxsize, INT32 palt, INT32 draw)
{
	INT32 xoffset = 0;
	INT32 xcntdraw = 0;

	for (INT32 xcnt = 0; xcnt < wide; xcnt++)
	{
		UINT16 msk = DrvSprMaskROM[sprite_boffset & ((0x400000 / 2) - 1)];

		for (INT32 x = 0; x < 16; x++)
		{
			if (!BIT(msk, 0))
			{
				UINT16 srcdat = get_sprite_pix() + (palt * 32);

				if (draw)
				{
					INT32 xzoombit = BIT(xzoom, xoffset & 0x1f);
					xoffset++;

					if (xzoombit && xgrow)
					{
						INT32 xdrawpos = (!BIT(flip, 0)) ? (xpos + xcntdraw) : (xpos + realxsize - xcntdraw);
						draw_sprite_pix(xdrawpos, pri, dest, destpri, srcdat);
						xcntdraw++;

						xdrawpos = (!BIT(flip, 0)) ? (xpos + xcntdraw) : (xpos + realxsize - xcntdraw);
						draw_sprite_pix(xdrawpos, pri, dest, destpri, srcdat);
						xcntdraw++;
					}
					else if (xzoombit && !xgrow)
					{
					}
					else
					{
						INT32 xdrawpos = (!BIT(flip, 0)) ? (xpos + xcntdraw) : (xpos + realxsize - xcntdraw);
						draw_sprite_pix(xdrawpos, pri, dest, destpri, srcdat);
						xcntdraw++;
					}
				}
			}
			else
			{
				INT32 xzoombit = BIT(xzoom, xoffset & 0x1f);
				xoffset++;
				if (xzoombit && xgrow) xcntdraw += 2;
				else if (!xzoombit || xgrow) xcntdraw++;
			}

			msk >>= 1;
		}

		sprite_boffset++;
	}
}

static void draw_sprite_new_zoomed(INT32 wide, INT32 high, INT32 xpos, INT32 ypos, INT32 palt, INT32 flip, UINT32 xzoom, INT32 xgrow, UINT32 yzoom, INT32 ygrow, INT32 pri)
{
	INT32 ycnt = 0;
	INT32 ycntdraw = 0;
	INT32 realysize = 0;
	INT32 realxsize = 0;
	INT32 xcnt = 0;

	sprite_aoffset = ((UINT32)DrvSprMaskROM[(sprite_boffset + 1) & ((0x400000 / 2) - 1)] << 16) | DrvSprMaskROM[(sprite_boffset + 0) & ((0x400000 / 2) - 1)];
	sprite_aoffset >>= 2;
	sprite_abit = 0;
	sprite_boffset += 2;

	while (ycnt < high) {
		INT32 yzoombit = BIT(yzoom, ycnt & 0x1f);
		if (yzoombit && ygrow) realysize += 2;
		else if (!yzoombit || ygrow) realysize++;
		ycnt++;
	}
	realysize--;

	while (xcnt < (wide * 16))
	{
		INT32 xzoombit = BIT(xzoom, xcnt & 0x1f);
		if (xzoombit && xgrow) realxsize += 2;
		else if (!xzoombit || xgrow) realxsize++;
		xcnt++;
	}
	realxsize--;

	ycnt = 0;
	ycntdraw = 0;

	while (ycnt < high)
	{
		INT32 yzoombit = BIT(yzoom, ycnt & 0x1f);
		INT32 ydrawpos = !BIT(flip, 1) ? (ypos + ycntdraw) : (ypos + realysize - ycntdraw);

		if (yzoombit && ygrow)
		{
			UINT32 temp_aoffset = sprite_aoffset;
			UINT32 temp_boffset = sprite_boffset;
			UINT8 temp_abit = sprite_abit;

			if (ydrawpos >= 0 && ydrawpos < nScreenHeight) {
				draw_sprite_line(wide, pTransDraw + (ydrawpos * nScreenWidth), pPrioDraw + (ydrawpos * nScreenWidth), xzoom, xgrow, flip, xpos, pri, realxsize, palt, 1);
			} else {
				draw_sprite_line(wide, NULL, NULL, xzoom, xgrow, flip, xpos, pri, realxsize, palt, 0);
			}
			ycntdraw++;

			sprite_aoffset = temp_aoffset;
			sprite_boffset = temp_boffset;
			sprite_abit = temp_abit;

			ydrawpos = !BIT(flip, 1) ? (ypos + ycntdraw) : (ypos + realysize - ycntdraw);
			if (ydrawpos >= 0 && ydrawpos < nScreenHeight) {
				draw_sprite_line(wide, pTransDraw + (ydrawpos * nScreenWidth), pPrioDraw + (ydrawpos * nScreenWidth), xzoom, xgrow, flip, xpos, pri, realxsize, palt, 1);
			} else {
				draw_sprite_line(wide, NULL, NULL, xzoom, xgrow, flip, xpos, pri, realxsize, palt, 0);
			}
			ycntdraw++;
		}
		else if (yzoombit && !ygrow)
		{
			draw_sprite_line(wide, NULL, NULL, xzoom, xgrow, flip, xpos, pri, realxsize, palt, 0);
		}
		else
		{
			if (ydrawpos >= 0 && ydrawpos < nScreenHeight) {
				draw_sprite_line(wide, pTransDraw + (ydrawpos * nScreenWidth), pPrioDraw + (ydrawpos * nScreenWidth), xzoom, xgrow, flip, xpos, pri, realxsize, palt, 1);
			} else {
				draw_sprite_line(wide, NULL, NULL, xzoom, xgrow, flip, xpos, pri, realxsize, palt, 0);
			}
			ycntdraw++;
		}

		ycnt++;
	}
}

static void draw_sprite_line_basic(INT32 wide, UINT16 *dest, UINT8 *destpri, INT32 flip, INT32 xpos, INT32 pri, INT32 realxsize, INT32 palt, INT32 draw)
{
	INT32 xcntdraw = 0;

	for (INT32 xcnt = 0; xcnt < wide; xcnt++)
	{
		UINT16 msk = DrvSprMaskROM[sprite_boffset & ((0x400000 / 2) - 1)];

		for (INT32 x = 0; x < 16; x++)
		{
			if (!BIT(msk, 0))
			{
				UINT16 srcdat = get_sprite_pix() + (palt * 32);

				if (draw) {
					INT32 xdrawpos = !BIT(flip, 0) ? (xpos + xcntdraw) : (xpos + realxsize - xcntdraw);
					draw_sprite_pix(xdrawpos, pri, dest, destpri, srcdat);
				}

				xcntdraw++;
			}
			else
			{
				xcntdraw++;
			}

			msk >>= 1;
		}

		sprite_boffset++;
	}
}

static void draw_sprite_new_basic(INT32 wide, INT32 high, INT32 xpos, INT32 ypos, INT32 palt, INT32 flip, INT32 pri)
{
	sprite_aoffset = ((UINT32)DrvSprMaskROM[(sprite_boffset + 1) & ((0x400000 / 2) - 1)] << 16) | DrvSprMaskROM[(sprite_boffset + 0) & ((0x400000 / 2) - 1)];
	sprite_aoffset >>= 2;
	sprite_abit = 0;
	sprite_boffset += 2;

	INT32 realysize = high - 1;
	INT32 realxsize = (wide * 16) - 1;

	for (INT32 ycnt = 0; ycnt < high; ycnt++)
	{
		INT32 ydrawpos = !BIT(flip, 1) ? (ypos + ycnt) : (ypos + realysize - ycnt);

		if (ydrawpos >= 0 && ydrawpos < nScreenHeight) {
			draw_sprite_line_basic(wide, pTransDraw + (ydrawpos * nScreenWidth), pPrioDraw + (ydrawpos * nScreenWidth), flip, xpos, pri, realxsize, palt, 1);
		} else {
			draw_sprite_line_basic(wide, NULL, NULL, flip, xpos, pri, realxsize, palt, 0);
		}
	}
}

static void get_sprites()
{
	sprite_ptr_pre = spritelist;

	UINT16 *source = (UINT16*)DrvSprBuf;
	UINT16 *zoomtable = (UINT16*)(DrvVidRegs + 0x1000);

	for (INT32 sprite_num = 0; sprite_num < (0x0a00 / 2); sprite_num += 5)
	{
		UINT16 spr4 = source[sprite_num + 4];
		if (!spr4) break;

		UINT16 spr0 = source[sprite_num + 0];
		UINT16 spr1 = source[sprite_num + 1];
		UINT16 spr2 = source[sprite_num + 2];
		UINT16 spr3 = source[sprite_num + 3];

		INT32 xzom = (spr0 & 0x7800) >> 11;
		INT32 yzom = (spr1 & 0x7800) >> 11;
		INT32 xgrow = (spr0 & 0x8000) >> 15;
		INT32 ygrow = (spr1 & 0x8000) >> 15;

		sprite_ptr_pre->x = (spr0 & 0x03ff) - (spr0 & 0x0400);
		sprite_ptr_pre->y = (spr1 & 0x01ff) - (spr1 & 0x0200);
		sprite_ptr_pre->flip = (spr2 & 0x6000) >> 13;
		sprite_ptr_pre->color = (spr2 & 0x1f00) >> 8;
		sprite_ptr_pre->pri = (spr2 & 0x0080) >> 7;
		sprite_ptr_pre->offs = ((spr2 & 0x007f) << 16) | spr3;
		sprite_ptr_pre->width = (spr4 & 0x7e00) >> 9;
		sprite_ptr_pre->height = spr4 & 0x01ff;
		sprite_ptr_pre->xgrow = xgrow;
		sprite_ptr_pre->ygrow = ygrow;

		if (xgrow) xzom = 0x10 - xzom;
		if (ygrow) yzom = 0x10 - yzom;

		sprite_ptr_pre->xzoom = (xzom == 0x0f) ? 1 : (((UINT32)zoomtable[xzom * 2 + 0] << 16) | zoomtable[xzom * 2 + 1]);
		sprite_ptr_pre->yzoom = (yzom == 0x0f) ? 1 : (((UINT32)zoomtable[yzom * 2 + 0] << 16) | zoomtable[yzom * 2 + 1]);
		sprite_ptr_pre++;
	}
}

static void draw_sprites()
{
	while (sprite_ptr_pre != spritelist)
	{
		sprite_ptr_pre--;
		sprite_boffset = sprite_ptr_pre->offs;

		if (!sprite_ptr_pre->xzoom && !sprite_ptr_pre->yzoom) {
			draw_sprite_new_basic(sprite_ptr_pre->width, sprite_ptr_pre->height, sprite_ptr_pre->x, sprite_ptr_pre->y, sprite_ptr_pre->color, sprite_ptr_pre->flip, sprite_ptr_pre->pri);
		} else {
			draw_sprite_new_zoomed(sprite_ptr_pre->width, sprite_ptr_pre->height, sprite_ptr_pre->x, sprite_ptr_pre->y, sprite_ptr_pre->color, sprite_ptr_pre->flip, sprite_ptr_pre->xzoom, sprite_ptr_pre->xgrow, sprite_ptr_pre->yzoom, sprite_ptr_pre->ygrow, sprite_ptr_pre->pri);
		}
	}
}

static INT32 MemIndex()
{
	UINT8 *Next = AllMem;

	DrvArmROM     = Next; Next += 0x004000;
	DrvArmPrgROM  = Next; Next += 0x080000;
	DrvArmPrgMap  = Next; Next += 0x080000;
	DrvTextROM    = Next; Next += 0x080000;
	DrvTxGfx      = Next; Next += 0x100000;
	DrvBgGfx      = Next; Next += 0x0d0000;
	DrvSprColROM  = (UINT16*)Next; Next += 0x400000;
	DrvSprMaskROM = (UINT16*)Next; Next += 0x400000;
	DrvSndROM     = Next; Next += 0x400000;

	DrvPalette    = (UINT32*)Next; Next += 0x0900 * sizeof(UINT32);

	AllRam        = Next;

	DrvNVRAM      = Next; Next += 0x008000;
	DrvArmRAM     = Next; Next += 0x000400;
	DrvMainRAM    = Next; Next += 0x001000;
	DrvBgRAM      = Next; Next += 0x001000;
	DrvTxRAM      = Next; Next += 0x002000;
	DrvRowRAM     = Next; Next += 0x001000;
	DrvPalRAM     = Next; Next += 0x001200;
	DrvVidRegs    = Next; Next += 0x010000;
	DrvSprBuf     = Next; Next += 0x000a00;

	RamEnd        = Next;
	MemEnd        = Next;

	return 0;
}

static INT32 DrvDoReset(INT32 clear_mem)
{
	if (clear_mem) {
		memset(DrvNVRAM, 0, 0x8000);
	}

	Arm7Open(0);
	Arm7Reset();
	Arm7Close();

	ics2115_reset();
	ics2115_set_xa_compat(1);
	memset(DrvArmRAM, 0, RamEnd - DrvArmRAM);

	memset(xor_table, 0, sizeof(xor_table));
	memset(DrvInputs, 0xff, sizeof(DrvInputs));
	memset(DrvMahjong, 0, sizeof(DrvMahjong));
	memset(DrvSprBuf, 0, 0x0a00);

	gpio_out = 0x1f;
	kbd_sel = 0x1f;
	irq_source = 0;
	cpu_fiq_enable = 1;
	cpu_irq_enable = 0xff;
	cpu_irq_pending = 0xff;
	cpu_timer_rate[0] = 0;
	cpu_timer_rate[1] = 0;
	cpu_timer_period[0] = 0;
	cpu_timer_period[1] = 0;
	cpu_timer_count[0] = 0;
	cpu_timer_count[1] = 0;
	ics_irq_state = 0;
	ext_fiq_state = 0;
	clear_mem_previous = 0;
	clear_mem_requested = 0;
	DrvHopperMotor = 0;
	DrvCoin1Pulse = 0;
	DrvCoin1Prev = 0;
	DrvOutLatch[0] = 0;
	DrvOutLatch[1] = 0;
	DrvRecalc = 1;

	rebuild_arm_program_map();

	return 0;
}

static INT32 DrvInit()
{
	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8*)0;

	if ((AllMem = (UINT8*)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	if (BurnLoadRom(DrvArmROM,            0, 1)) return 1;
	if (BurnLoadRom(DrvArmPrgROM,         1, 1)) return 1;
	if (BurnLoadRom(DrvTextROM,           2, 1)) return 1;
	if (BurnLoadRom((UINT8*)DrvSprColROM, 3, 1)) return 1;
	if (BurnLoadRom((UINT8*)DrvSprMaskROM,4, 1)) return 1;
	if (BurnLoadRom(DrvSndROM,            5, 1)) return 1;

	mxsqy_decrypt();
	rebuild_arm_program_map();
	if (DrvGfxDecode()) return 1;

	Arm7Init(0);
	Arm7SetThumbStackAlignFix(1);
	Arm7Open(0);
	Arm7MapMemory(DrvArmROM,    0x00000000, 0x00003fff, MAP_ROM);
	Arm7MapMemory(DrvArmRAM,    0x10000000, 0x100003ff, MAP_RAM);
	Arm7MapMemory(DrvNVRAM,     0x18000000, 0x18007fff, MAP_RAM);
	Arm7MapMemory(DrvMainRAM,   0x28000000, 0x28000fff, MAP_RAM);
	Arm7SetWriteByteHandler(m027_023vid_write_byte);
	Arm7SetWriteWordHandler(m027_023vid_write_word);
	Arm7SetWriteLongHandler(m027_023vid_write_long);
	Arm7SetReadByteHandler(m027_023vid_read_byte);
	Arm7SetReadWordHandler(m027_023vid_read_word);
	Arm7SetReadLongHandler(m027_023vid_read_long);
	Arm7Close();

	ics2115_init(ics2115_irq, DrvSndROM, 0x400000);
	BurnTimerAttachArm7(33000000);
	ics_2115_set_volume(5.0);

	GenericTilesInit();
	GenericTilemapInit(0, TILEMAP_SCAN_ROWS, mxsqy_bg_map_callback, 32, 32, 64, 16);
	GenericTilemapInit(1, TILEMAP_SCAN_ROWS, mxsqy_tx_map_callback, 8, 8, 64, 32);
	GenericTilemapUseDirtyTiles(0);
	GenericTilemapUseDirtyTiles(1);
	GenericTilemapSetGfx(0, DrvBgGfx, 5, 32, 32, bg_gfx_len, 0x400, 0x1f);
	GenericTilemapSetGfx(1, DrvTxGfx, 4, 8, 8, tx_gfx_len, 0x800, 0x0f);
	GenericTilemapSetTransparent(0, 0x1f);
	GenericTilemapSetTransparent(1, 0x0f);
	GenericTilemapSetScrollRows(0, 16 * 32);

	BurnSetRefreshRate(60.00);

	return DrvDoReset(0);
}

static INT32 DrvExit()
{
	GenericTilesExit();
	Arm7Exit();
	ics2115_exit();
	BurnTimerExit();

	BurnFree(AllMem);
	AllMem = NULL;

	return 0;
}

static void DrvPaletteUpdate()
{
	for (INT32 i = 0; i < 0x1200; i += 2) {
		palram_write(i + 0, DrvPalRAM[i + 0]);
	}
}

static inline void draw_bg_scanline(INT32 scanline, INT32 xscroll, INT32 yscroll)
{
	INT32 scrollx = (xscroll + (INT16)read_le16(DrvRowRAM, scanline * 2)) & 0x7ff;
	INT32 scrolly = (yscroll + scanline) & 0x1ff;

	UINT16 *dst = pTransDraw + (scanline * nScreenWidth);
	UINT8 *pri = pPrioDraw + (scanline * nScreenWidth);

	for (INT32 x = 0; x < nScreenWidth + 32; x += 32)
	{
		INT32 sx = x - (scrollx & 0x1f);
		if (sx >= nScreenWidth) break;

		INT32 offs = ((scrolly & 0x1e0) << 2) | (((scrollx + x) & 0x7e0) >> 4);
		UINT16 code = read_le16(DrvBgRAM, offs * 2);
		if (code >= bg_tile_count) continue;

		UINT16 attr = read_le16(DrvBgRAM, offs * 2 + 2);
		UINT16 color = ((attr & 0x3e) << 4);
		INT32 flipx = ((attr & 0x40) >> 6) * 0x1f;
		INT32 flipy = ((attr & 0x80) >> 7) * 0x1f;

		UINT8 *src = DrvBgGfx + (code * 1024) + (((scrolly ^ flipy) & 0x1f) << 5);

		if (sx >= 0 && sx <= (nScreenWidth - 32))
		{
			for (INT32 xx = 0; xx < 32; xx++, sx++) {
				UINT16 pxl = src[xx ^ flipx];
				if (pxl != 0x1f) {
					dst[sx] = color | pxl;
					pri[sx] |= 2;
				}
			}
		}
		else
		{
			for (INT32 xx = 0; xx < 32; xx++, sx++) {
				if (sx < 0) continue;
				if (sx >= nScreenWidth) break;

				UINT16 pxl = src[xx ^ flipx];
				if (pxl != 0x1f) {
					dst[sx] = color | pxl;
					pri[sx] |= 2;
				}
			}
		}
	}
}

static INT32 bg_ram_blank()
{
	const UINT32 *src = (const UINT32*)DrvBgRAM;

	for (INT32 i = 0; i < (0x1000 / 4); i++) {
		if (src[i] != 0) return 0;
	}

	return 1;
}

static void draw_background_layer()
{
	if (bg_ram_blank()) return;

	INT32 bg_scrolly = (INT16)read_le16(DrvVidRegs, 0x2000);
	INT32 bg_scrollx = (INT16)read_le16(DrvVidRegs, 0x3000);

	for (INT32 y = 0; y < nScreenHeight; y++) {
		draw_bg_scanline(y, bg_scrollx, bg_scrolly);
	}
}

static void draw_text_layer()
{
	INT32 scrollx = (INT16)read_le16(DrvVidRegs, 0x6000) & 0x1ff;
	INT32 scrolly = (INT16)read_le16(DrvVidRegs, 0x5000) & 0x0ff;

	for (INT32 offs = 0; offs < 64 * 32; offs++)
	{
		UINT16 code = read_le16(DrvTxRAM, offs * 4);
		if ((code * 64) >= tx_gfx_len) continue;

		INT32 sx = ((offs & 0x3f) << 3) - scrollx;
		INT32 sy = ((offs >> 6) << 3) - scrolly;

		if (sx < -7) sx += 512;
		if (sy < -7) sy += 256;
		if (sx >= nScreenWidth || sy >= nScreenHeight) continue;

		UINT16 attr = read_le16(DrvTxRAM, offs * 4 + 2);
		UINT16 color = ((attr & 0x3e) << 3) | 0x800;
		INT32 flipx = ((attr & 0x40) >> 6) * 0x07;
		INT32 flipy = ((attr & 0x80) >> 7) * 0x07;

		UINT8 *src = DrvTxGfx + (code * 64);

		for (INT32 y = 0; y < 8; y++)
		{
			INT32 yy = sy + y;
			if (yy < 0 || yy >= nScreenHeight) continue;

			UINT8 *row = src + ((y ^ flipy) << 3);
			UINT16 *dst = pTransDraw + (yy * nScreenWidth);

			for (INT32 x = 0; x < 8; x++)
			{
				INT32 xx = sx + x;
				if (xx < 0 || xx >= nScreenWidth) continue;

				UINT16 pxl = row[x ^ flipx];
				if (pxl != 0x0f) {
					dst[xx] = color | pxl;
				}
			}
		}
	}
}

static INT32 DrvDraw()
{
	DrvPaletteUpdate();
	DrvRecalc = 0;

	BurnTransferClear(0x3ff);

	if (nBurnLayer & 1) draw_background_layer();
	if (nSpriteEnable & 1) draw_sprites();
	if (nBurnLayer & 2) draw_text_layer();

	BurnTransferCopy(DrvPalette);

	return 0;
}

static INT32 DrvFrame()
{
	if (DrvReset) {
		DrvDoReset(0);
	}

	build_inputs();

	if (DrvJoy1[8] && !DrvCoin1Prev) {
		DrvCoin1Pulse = 5;
	}
	DrvCoin1Prev = DrvJoy1[8] ? 1 : 0;

	if (clear_mem_requested) {
		DrvDoReset(1);
		clear_mem_requested = 0;
	}

	Arm7NewFrame();

	INT32 nInterleave = 256;
	INT32 nCyclesTotal[1] = { 33000000 / 60 };
	INT32 nPrevCycles = 0;

	Arm7Open(0);

	for (INT32 i = 0; i < nInterleave; i++)
	{
		CPU_RUN_TIMER(0);
		INT32 executed = Arm7TotalCycles() - nPrevCycles;
		nPrevCycles = Arm7TotalCycles();

		for (INT32 t = 0; t < 2; t++) {
			if (cpu_timer_period[t] != 0) {
				cpu_timer_count[t] += executed;
				while (cpu_timer_count[t] >= cpu_timer_period[t]) {
					cpu_timer_count[t] -= cpu_timer_period[t];
					igs027a_irq_raise(t);
				}
			}
		}

		if (i == 0) {
			memcpy(DrvSprBuf, DrvMainRAM, 0x0a00);
			get_sprites();
			igs027a_external_firq(1);
			igs027a_external_firq(0);
		}

		if (i == nScreenHeight) {
			irq_source = 0;
			igs027a_external_irq();
		}
	}

	Arm7Close();

	if (pBurnSoundOut) {
		ics2115_update(nBurnSoundLen);
	}

	if (DrvCoin1Pulse) {
		DrvCoin1Pulse--;
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	if (pnMin) {
		*pnMin = 0x029743;
	}

	if (nAction & ACB_MEMORY_RAM)
	{
		struct BurnArea ba;
		memset(&ba, 0, sizeof(ba));
		ba.Data = AllRam;
		ba.nLen = RamEnd - AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA)
	{
		Arm7Scan(nAction);
		ics2115_scan(nAction, pnMin);

		SCAN_VAR(DrvRecalc);
		SCAN_VAR(DrvReset);
		SCAN_VAR(DrvInputs);
		SCAN_VAR(gpio_out);
		SCAN_VAR(kbd_sel);
		SCAN_VAR(irq_source);
		SCAN_VAR(cpu_fiq_enable);
		SCAN_VAR(cpu_irq_enable);
		SCAN_VAR(cpu_irq_pending);
		SCAN_VAR(cpu_timer_rate);
		SCAN_VAR(cpu_timer_period);
		SCAN_VAR(cpu_timer_count);
		SCAN_VAR(ics_irq_state);
		SCAN_VAR(ext_fiq_state);
		SCAN_VAR(clear_mem_previous);
		SCAN_VAR(clear_mem_requested);
		SCAN_VAR(DrvHopperMotor);
		SCAN_VAR(DrvCoin1Pulse);
		SCAN_VAR(DrvCoin1Prev);
		SCAN_VAR(DrvOutLatch);
		SCAN_VAR(xor_table);
	}

	if (nAction & ACB_WRITE) {
		rebuild_arm_program_map();
	}

	return 0;
}


static struct BurnRomInfo mxsqyRomDesc[] = {
	{ "a8_027a.u41",   0x004000, 0xf9ada8c4, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m2401.u39", 0x080000, 0x32e69540, 1 | BRF_PRG | BRF_ESS },
	{ "igs_l2405.u38", 0x080000, 0x2f20eade, 2 | BRF_GRA },
	{ "igs_l2404.u23", 0x400000, 0xdc8ff7ae, 3 | BRF_GRA },
	{ "igs_m2403.u22", 0x400000, 0x53940332, 4 | BRF_GRA },
	{ "igs_s2402.u21", 0x400000, 0xa3e3b2e0, 5 | BRF_SND },
};

STD_ROM_PICK(mxsqy)
STD_ROM_FN(mxsqy)

static struct BurnRomInfo mxsqy102twRomDesc[] = {
	{ "n6_027a.u41", 0x004000, 0xf9ada8c4, 1 | BRF_PRG | BRF_ESS },
	{ "v-102tw.u39", 0x080000, 0x16095b98, 1 | BRF_PRG | BRF_ESS },
	{ "text.u38",    0x080000, 0x2f20eade, 2 | BRF_GRA },
	{ "cg_u23.u23",  0x400000, 0xdc8ff7ae, 3 | BRF_GRA },
	{ "cg_u22.u22",  0x400000, 0x53940332, 4 | BRF_GRA },
	{ "u21",         0x400000, 0xa3e3b2e0, 5 | BRF_SND },
};

STD_ROM_PICK(mxsqy102tw)
STD_ROM_FN(mxsqy102tw)

struct BurnDriver BurnDrvMxsqy = {
	"mxsqy", NULL, NULL, NULL, "2003",
	"Mingxing San Que Yi (China, V201CN)\0", NULL, "IGS", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	NULL, mxsqyRomInfo, mxsqyRomName, NULL, NULL, NULL, NULL, MxsqyInputInfo, MxsqyDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x900,
	448, 224, 4, 3
};

struct BurnDriver BurnDrvMxsqy102tw = {
	"mxsqy102tw", "mxsqy", NULL, NULL, "2003",
	"Mingxing San Que Yi (Taiwan, V102TW)\0", NULL, "IGS", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	NULL, mxsqy102twRomInfo, mxsqy102twRomName, NULL, NULL, NULL, NULL, MxsqyInputInfo, MxsqyDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x900,
	448, 224, 4, 3
};

static UINT8 m027_023vid_read_byte(UINT32 address)
{
	if (address >= 0x08000000 && address <= 0x0807ffff) {
		UINT32 data = external_rom_read_long(address);
		return (data >> ((address & 3) * 8)) & 0xff;
	}

	if (address >= 0x38900000 && address <= 0x38907fff) return videoram_read_byte(address - 0x38900000);
	if (address >= 0x38a00000 && address <= 0x38a011ff) return DrvPalRAM[address - 0x38a00000];
	if (address >= 0x38b00000 && address <= 0x38b0ffff) return DrvVidRegs[address - 0x38b00000];

	if (address >= 0x48000000 && address <= 0x48000007)
	{
		UINT16 data = 0xffff;
		switch ((address - 0x48000000) & ~1)
		{
			case 0x00: data = in0_r(); break;
			case 0x02: data = in1_r(); break;
			case 0x04: data = in2_r(); break;
			case 0x06: data = dsw_r(); break;
		}
		return data >> ((address & 1) * 8);
	}

	if (address >= 0x58000000 && address <= 0x58000007) {
		return (address & 1) ? 0xff : ics2115read((address >> 1) & 3);
	}

	if (address >= 0x4000000c && address <= 0x4000000f) {
		UINT32 data = igs027a_in_port_value();
		return (data >> ((address & 3) * 8)) & 0xff;
	}

	if (address >= 0x70000200 && address <= 0x70000203) {
		return ((address & 3) == 0) ? igs027a_irq_pending_r() : 0xff;
	}

	return 0xff;
}

static UINT16 m027_023vid_read_word(UINT32 address)
{
	return m027_023vid_read_byte(address + 0) | (m027_023vid_read_byte(address + 1) << 8);
}

static UINT32 m027_023vid_read_long(UINT32 address)
{
	return m027_023vid_read_word(address + 0) | (m027_023vid_read_word(address + 2) << 16);
}

static void m027_023vid_write_byte(UINT32 address, UINT8 data)
{
	if (address >= 0x38900000 && address <= 0x38907fff) {
		videoram_write_byte(address - 0x38900000, data);
		return;
	}

	if (address >= 0x38a00000 && address <= 0x38a011ff) {
		palram_write(address - 0x38a00000, data);
		return;
	}

	if (address >= 0x38b00000 && address <= 0x38b0ffff) {
		DrvVidRegs[address - 0x38b00000] = data;
		return;
	}

	if (address >= 0x50000000 && address <= 0x500003ff)
	{
		if ((address & 3) == 0)
		{
			INT32 offset = (address & 0x3ff) >> 2;
			xor_table[offset] = (UINT32(data) << 24) | (UINT32(data) << 8);
			rebuild_arm_program_map();
		}
		return;
	}

	if (address >= 0x58000000 && address <= 0x58000007) {
		if ((address & 1) == 0) ics2115write((address >> 1) & 3, data);
		return;
	}

	if (address >= 0x68000000 && address <= 0x68000001) {
		if ((address & 1) == 0) kbd_sel = data & 0x1f;
		return;
	}

	if (address >= 0x40000014 && address <= 0x40000017) {
		if ((address & 3) == 0) {
			UINT8 old = cpu_fiq_enable;
			cpu_fiq_enable = data;
			if (!BIT(old, 0) && BIT(cpu_fiq_enable, 0) && ext_fiq_state) {
				Arm7SetIRQLine(ARM7_FIRQ_LINE, CPU_IRQSTATUS_AUTO);
			}
		}
		return;
	}

	if (address >= 0x40000018 && address <= 0x4000001b) {
		if ((address & 3) == 0) gpio_w(data);
		return;
	}

	if (address == 0x70000100 || address == 0x70000104) {
		igs027a_timer_rate_w((address >> 2) & 1, data);
		return;
	}

	if (address == 0x70000200) {
		igs027a_irq_enable_w(data);
		return;
	}
}

static void m027_023vid_write_word(UINT32 address, UINT16 data)
{
	if (address == 0x48000006) {
		output_w(0, data);
		return;
	}

	if (address == 0x68000000) {
		kbd_w(data);
		return;
	}

	if (address == 0x68000002) {
		output_w(1, data);
		return;
	}

	m027_023vid_write_byte(address + 0, data & 0xff);
	m027_023vid_write_byte(address + 1, data >> 8);
}

static void m027_023vid_write_long(UINT32 address, UINT32 data)
{
	if (address == 0x48000004) {
		output_w(0, data >> 16);
		return;
	}

	if (address == 0x68000000) {
		kbd_w(data & 0xffff);
		output_w(1, data >> 16);
		return;
	}

	if (address == 0x68000002) {
		output_w(1, data & 0xffff);
		return;
	}

	m027_023vid_write_word(address + 0, data & 0xffff);
	m027_023vid_write_word(address + 2, data >> 16);
}

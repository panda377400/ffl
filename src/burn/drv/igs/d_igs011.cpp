// FinalBurn Neo IGS011 hardware driver module
// Based on MAME driver by Luca Elia, Olivier Galibert

// known issues:
// - vbowl & clones ICS2115 emulation has issues - sound dies, gives error on boot

#include "tiles_generic.h"
#include "m68000_intf.h"
#include "ics2115.h"
#include "burn_ym3812.h"
#include "burn_ym2413.h"
#include "msm6295.h"
#include "bitswap.h"
#include "burn_pal.h"
#include "timer.h"

#ifndef ROM_NAME
#define ROM_NAME(name) NULL, name##RomInfo, name##RomName, NULL, NULL, NULL, NULL
#endif

#ifndef INPUT_FN
#define INPUT_FN(name) name##InputInfo
#endif

#ifndef DIP_FN
#define DIP_FN(name) name##DIPInfo
#endif

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;
static UINT8 *Drv68KROM;
static UINT8 *DrvGfxROM[2] = { NULL, NULL };
static UINT32 DrvGfxROMLen[2] = { 0, 0 };
static UINT8 *DrvSndROM;
static UINT8 *DrvLayerRAM[4];
static UINT8 *DrvPalRAM;
static UINT8 *DrvNVRAM;
static UINT8 *DrvExtraRAM;
static UINT8 *DrvPrioRAM;

static UINT32 *DrvPalette;
static UINT8 DrvRecalc;

static UINT8 igs012_prot;
static UINT8 igs012_prot_swap;
static UINT8 igs012_prot_mode;

static UINT8 igs011_prot2;
static UINT8 igs011_prot1;
static UINT8 igs011_prot1_swap;
static UINT32 igs011_prot1_addr;

static UINT16 igs003_reg;
static UINT16 igs003_prot_hold;
static UINT8 igs003_prot_z;
static UINT8 igs003_prot_y;
static UINT8 igs003_prot_x;
static UINT8 igs003_prot_h1;
static UINT8 igs003_prot_h2;

static UINT16 priority;

struct blitter_t
{
	UINT16 x;
	UINT16 y;
	UINT16 w;
	UINT16 h;
	UINT16 gfx_lo;
	UINT16 gfx_hi;
	UINT16 depth;
	UINT16 pen;
	UINT16 flags;
	UINT8 pen_hi;
};

static blitter_t s_blitter;

static UINT16 dips_select;
static UINT16 trackball[2];
static UINT16 input_select;
static UINT16 lhb_irq_enable;

enum
{
	SOUND_ICS2115 = 0,
	SOUND_YM3812_OKI,
	SOUND_OKI,
	SOUND_YM2413_OKI,
};

static INT32 sound_system = SOUND_ICS2115;
static INT32 timer_irq = 3;		// 3 = vbowl, 5 = drgnwrld
static INT32 timer_pulses = 4;	// 4 = 240hz, 2 = 120hz, 1 = 60hz
static INT32 timer_irq_gated = 0;
static INT32 vblank_irq_gated = 0;
static INT32 has_hopper = 0;
static INT32 hopper_motor = 0;
static UINT32 lhb2_prot_base = 0x020000;
static INT32 lhb2_has_irq_enable = 0;
static UINT16 (*lhb2_coin_read_cb)() = NULL;
static UINT16 (*lhb2_igs003_read_cb)() = NULL;
static UINT16 (*igs011_prot2_read)() = NULL;

typedef void (__fastcall *igs011_write_word_handler)(UINT32, UINT16);
typedef void (__fastcall *igs011_write_byte_handler)(UINT32, UINT8);
typedef UINT16 (__fastcall *igs011_read_word_handler)(UINT32);
typedef UINT8 (__fastcall *igs011_read_byte_handler)(UINT32);

static UINT16 igs003_magic_read();
static inline void oki_bank_write(INT32 bank);
static UINT16 lhb_coin_read();
static UINT16 lhb2_coin_read();
static UINT16 nkishusp_coin_read();
static UINT16 xymg_coin_read();
static UINT16 wlcc_coin_read();
static UINT16 tygn_coin_read();
static inline void lhb_inputs_write(UINT16 data);
static UINT16 lhb_inputs_read(INT32 offset);
static inline void igs003_reg_write(UINT16 data);
static void lhb2_igs003_write(UINT16 data);
static UINT16 lhb2_igs003_read();
static void wlcc_igs003_write(UINT16 data);
static UINT16 wlcc_igs003_read();
static void xymg_igs003_write(UINT16 data);
static UINT16 xymg_igs003_read();

static UINT8 DrvDips[5];
static UINT8 DrvReset;
static UINT8 DrvJoy1[8];
static UINT8 DrvJoy2[8];
static UINT8 DrvJoy3[8];
static UINT8 DrvJoy4[8];
static UINT8 DrvInputs[4];
static UINT8 DrvMahjong[40];
static UINT8 DrvMahjongInputs[5];
static UINT8 DrvCoin1Pulse;
static UINT8 DrvCoin1Prev;

static struct BurnInputInfo VbowlInputList[] = {
	{"P1 Coin",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy2 + 0,	"p1 start"	},
	{"P1 Up",			BIT_DIGITAL,	DrvJoy2 + 1,	"p1 up"		},
	{"P1 Down",			BIT_DIGITAL,	DrvJoy2 + 2,	"p1 down"	},
	{"P1 Left",			BIT_DIGITAL,	DrvJoy2 + 3,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 4,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy2 + 5,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy2 + 6,	"p1 fire 2"	},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy2 + 7,	"p1 fire 3"	},

	{"P2 Coin",			BIT_DIGITAL,	DrvJoy1 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy3 + 0,	"p2 start"	},
	{"P2 Up",			BIT_DIGITAL,	DrvJoy3 + 1,	"p2 up"		},
	{"P2 Down",			BIT_DIGITAL,	DrvJoy3 + 2,	"p2 down"	},
	{"P2 Left",			BIT_DIGITAL,	DrvJoy3 + 3,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy3 + 4,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy3 + 5,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy3 + 6,	"p2 fire 2"	},
	{"P2 Button 3",		BIT_DIGITAL,	DrvJoy3 + 7,	"p2 fire 3"	},

	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 3,	"service"	},
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
	{"Dip D",			BIT_DIPSWITCH,	DrvDips + 3,	"dip"		},
};

STDINPUTINFO(Vbowl)

static struct BurnInputInfo DrgnwrldInputList[] = {
	{"P1 Coin",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy2 + 0,	"p1 start"	},
	{"P1 Up",			BIT_DIGITAL,	DrvJoy4 + 0,	"p1 up"		},
	{"P1 Down",			BIT_DIGITAL,	DrvJoy2 + 2,	"p1 down"	},
	{"P1 Left",			BIT_DIGITAL,	DrvJoy4 + 1,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 4,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy4 + 2,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy2 + 6,	"p1 fire 2"	},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy4 + 3,	"p1 fire 3"	},

	{"P2 Coin",			BIT_DIGITAL,	DrvJoy1 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy4 + 4,	"p2 start"	},
	{"P2 Up",			BIT_DIGITAL,	DrvJoy3 + 1,	"p2 up"		},
	{"P2 Down",			BIT_DIGITAL,	DrvJoy4 + 5,	"p2 down"	},
	{"P2 Left",			BIT_DIGITAL,	DrvJoy3 + 3,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy4 + 6,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy3 + 5,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy4 + 7,	"p2 fire 2"	},
	{"P2 Button 3",		BIT_DIGITAL,	DrvJoy3 + 7,	"p2 fire 3"	},

	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 3,	"service"	},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 5,	"service"	},
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Drgnwrld)

#define IGS011_MAHJONG_INPUTS \
	{"Mahjong A",		BIT_DIGITAL,	DrvMahjong +  0,	"mah a"		}, \
	{"Mahjong E",		BIT_DIGITAL,	DrvMahjong +  1,	"mah e"		}, \
	{"Mahjong I",		BIT_DIGITAL,	DrvMahjong +  2,	"mah i"		}, \
	{"Mahjong M",		BIT_DIGITAL,	DrvMahjong +  3,	"mah m"		}, \
	{"Mahjong Kan",		BIT_DIGITAL,	DrvMahjong +  4,	"mah kan"	}, \
	{"Mahjong Start",	BIT_DIGITAL,	DrvMahjong +  5,	"p1 start"	}, \
	{"Mahjong B",		BIT_DIGITAL,	DrvMahjong +  6,	"mah b"		}, \
	{"Mahjong F",		BIT_DIGITAL,	DrvMahjong +  7,	"mah f"		}, \
	{"Mahjong J",		BIT_DIGITAL,	DrvMahjong +  8,	"mah j"		}, \
	{"Mahjong N",		BIT_DIGITAL,	DrvMahjong +  9,	"mah n"		}, \
	{"Mahjong Reach",	BIT_DIGITAL,	DrvMahjong + 10,	"mah reach"	}, \
	{"Mahjong Bet",		BIT_DIGITAL,	DrvMahjong + 11,	"mah bet"	}, \
	{"Mahjong C",		BIT_DIGITAL,	DrvMahjong + 12,	"mah c"		}, \
	{"Mahjong G",		BIT_DIGITAL,	DrvMahjong + 13,	"mah g"		}, \
	{"Mahjong K",		BIT_DIGITAL,	DrvMahjong + 14,	"mah k"		}, \
	{"Mahjong Chi",		BIT_DIGITAL,	DrvMahjong + 15,	"mah chi"	}, \
	{"Mahjong Ron",		BIT_DIGITAL,	DrvMahjong + 16,	"mah ron"	}, \
	{"Mahjong D",		BIT_DIGITAL,	DrvMahjong + 18,	"mah d"		}, \
	{"Mahjong H",		BIT_DIGITAL,	DrvMahjong + 19,	"mah h"		}, \
	{"Mahjong L",		BIT_DIGITAL,	DrvMahjong + 20,	"mah l"		}, \
	{"Mahjong Pon",		BIT_DIGITAL,	DrvMahjong + 21,	"mah pon"	}, \
	{"Mahjong Last",	BIT_DIGITAL,	DrvMahjong + 24,	"mah last"	}, \
	{"Mahjong Score",	BIT_DIGITAL,	DrvMahjong + 25,	"mah score"	}, \
	{"Mahjong Double",	BIT_DIGITAL,	DrvMahjong + 26,	"mah double"}, \
	{"Mahjong Big",		BIT_DIGITAL,	DrvMahjong + 28,	"mah big"	}, \
	{"Mahjong Small",	BIT_DIGITAL,	DrvMahjong + 29,	"mah small"	}

static struct BurnInputInfo LhbInputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Key In",			BIT_DIGITAL,	DrvJoy1 + 1,	"service2"	},
	{"Key Out",			BIT_DIGITAL,	DrvJoy1 + 2,	"service3"	},
	{"Payout",			BIT_DIGITAL,	DrvJoy1 + 3,	"service4"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Memory Reset",	BIT_DIGITAL,	DrvJoy1 + 5,	"diag"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	IGS011_MAHJONG_INPUTS,
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
	{"Dip D",			BIT_DIPSWITCH,	DrvDips + 3,	"dip"		},
	{"Dip E",			BIT_DIPSWITCH,	DrvDips + 4,	"dip"		},
};

STDINPUTINFO(Lhb)

static struct BurnInputInfo Lhb2InputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Key In",			BIT_DIGITAL,	DrvJoy1 + 1,	"service2"	},
	{"Key Out",			BIT_DIGITAL,	DrvJoy1 + 2,	"service3"	},
	{"Payout",			BIT_DIGITAL,	DrvJoy1 + 3,	"service4"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Memory Reset",	BIT_DIGITAL,	DrvJoy1 + 5,	"diag"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	IGS011_MAHJONG_INPUTS,
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Lhb2)

static struct BurnInputInfo Lhb2cpgsInputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Key In",			BIT_DIGITAL,	DrvJoy1 + 1,	"service2"	},
	{"Key Out",			BIT_DIGITAL,	DrvJoy1 + 2,	"service3"	},
	{"Payout",			BIT_DIGITAL,	DrvJoy1 + 3,	"service4"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Memory Reset",	BIT_DIGITAL,	DrvJoy1 + 5,	"diag"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	IGS011_MAHJONG_INPUTS,
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Lhb2cpgs)

static struct BurnInputInfo NkishuspInputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Key In",			BIT_DIGITAL,	DrvJoy1 + 1,	"service2"	},
	{"Key Out",			BIT_DIGITAL,	DrvJoy1 + 2,	"service3"	},
	{"Payout",			BIT_DIGITAL,	DrvJoy1 + 3,	"service4"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Memory Reset",	BIT_DIGITAL,	DrvJoy1 + 5,	"diag"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	IGS011_MAHJONG_INPUTS,
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Nkishusp)

static struct BurnInputInfo XymgInputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Key In",			BIT_DIGITAL,	DrvJoy1 + 1,	"service2"	},
	{"Key Out",			BIT_DIGITAL,	DrvJoy1 + 2,	"service3"	},
	{"Payout",			BIT_DIGITAL,	DrvJoy1 + 3,	"service4"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Memory Reset",	BIT_DIGITAL,	DrvJoy1 + 5,	"diag"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	IGS011_MAHJONG_INPUTS,
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Xymg)

static struct BurnInputInfo WlccInputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Key In",			BIT_DIGITAL,	DrvJoy1 + 1,	"service2"	},
	{"Key Out",			BIT_DIGITAL,	DrvJoy1 + 2,	"service3"	},
	{"Payout",			BIT_DIGITAL,	DrvJoy1 + 3,	"service4"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Memory Reset",	BIT_DIGITAL,	DrvJoy1 + 5,	"diag"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy2 + 0,	"p1 start"	},
	{"P1 Up",			BIT_DIGITAL,	DrvJoy2 + 1,	"p1 up"		},
	{"P1 Down",			BIT_DIGITAL,	DrvJoy2 + 2,	"p1 down"	},
	{"P1 Left",			BIT_DIGITAL,	DrvJoy2 + 3,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 4,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy2 + 5,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy2 + 6,	"p1 fire 2"	},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy2 + 7,	"p1 fire 3"	},
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
	{"Dip D",			BIT_DIPSWITCH,	DrvDips + 3,	"dip"		},
};

STDINPUTINFO(Wlcc)

static struct BurnInputInfo TygnInputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy2 + 0,	"p1 start"	},
	{"P1 Up",			BIT_DIGITAL,	DrvJoy2 + 1,	"p1 up"		},
	{"P1 Down",			BIT_DIGITAL,	DrvJoy2 + 2,	"p1 down"	},
	{"P1 Left",			BIT_DIGITAL,	DrvJoy2 + 3,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 4,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy2 + 5,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy2 + 6,	"p1 fire 2"	},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy2 + 7,	"p1 fire 3"	},
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Tygn)

static struct BurnDIPInfo VbowlhkDIPList[]=
{
	{0x14, 0xff, 0xff, 0xff, NULL							},
	{0x15, 0xff, 0xff, 0xff, NULL							},
	{0x16, 0xff, 0xff, 0xff, NULL							},
	{0x17, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    8, "Coinage"						},
	{0x14, 0x01, 0x07, 0x00, "4 Coins 1 Credits"			},
	{0x14, 0x01, 0x07, 0x01, "3 Coins 1 Credits"			},
	{0x14, 0x01, 0x07, 0x02, "2 Coins 1 Credits"			},
	{0x14, 0x01, 0x07, 0x07, "1 Coin  1 Credits"			},
	{0x14, 0x01, 0x07, 0x06, "1 Coin  2 Credits"			},
	{0x14, 0x01, 0x07, 0x05, "1 Coin  3 Credits"			},
	{0x14, 0x01, 0x07, 0x04, "1 Coin  4 Credits"			},
	{0x14, 0x01, 0x07, 0x03, "1 Coin  5 Credits"			},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"					},
	{0x14, 0x01, 0x08, 0x00, "Off"							},
	{0x14, 0x01, 0x08, 0x08, "On"							},

	{0   , 0xfe, 0   ,    2, "Sexy Interlude"				},
	{0x14, 0x01, 0x10, 0x00, "No"							},
	{0x14, 0x01, 0x10, 0x10, "Yes"							},

	{0   , 0xfe, 0   ,    2, "Open Picture"					},
	{0x14, 0x01, 0x20, 0x00, "No"							},
	{0x14, 0x01, 0x20, 0x20, "Yes"							},

	{0   , 0xfe, 0   ,    2, "Free Play"					},
	{0x14, 0x01, 0x80, 0x80, "Off"							},
	{0x14, 0x01, 0x80, 0x00, "On"							},

	{0   , 0xfe, 0   ,    4, "Difficulty"					},
	{0x15, 0x01, 0x03, 0x03, "Easy  "						},
	{0x15, 0x01, 0x03, 0x02, "Normal"						},
	{0x15, 0x01, 0x03, 0x01, "Medium"						},
	{0x15, 0x01, 0x03, 0x00, "Hard  "						},

	{0   , 0xfe, 0   ,    2, "Spares To Win (Frames 1-5)"	},
	{0x15, 0x01, 0x04, 0x04, "3"							},
	{0x15, 0x01, 0x04, 0x00, "4"							},

	{0   , 0xfe, 0   ,    4, "Points To Win (Frames 6-10)"	},
	{0x15, 0x01, 0x18, 0x18, "160"							},
	{0x15, 0x01, 0x18, 0x10, "170"							},
	{0x15, 0x01, 0x18, 0x08, "180"							},
	{0x15, 0x01, 0x18, 0x00, "190"							},

	{0   , 0xfe, 0   ,    2, "Service Mode"					},
	{0x16, 0x01, 0x80, 0x80, "Off"							},
	{0x16, 0x01, 0x80, 0x00, "On"							},

	{0   , 0xfe, 0   ,    4, "Protection & Comm Test"		},
	{0x17, 0x01, 0x81, 0x81, "No (0)"						},
	{0x17, 0x01, 0x81, 0x80, "No (1)"						},
	{0x17, 0x01, 0x81, 0x01, "No (2)"						},
	{0x17, 0x01, 0x81, 0x00, "Yes"							},
};

STDDIPINFO(Vbowlhk)

static struct BurnDIPInfo VbowlDIPList[] = {
	{0   , 0xfe, 0   ,    2, "Controls"						},
	{0x14, 0x01, 0x40, 0x40, "Joystick"						},
	{0x14, 0x01, 0x40, 0x00, "Trackball"					},

	{0   , 0xfe, 0   ,    4, "Cabinet ID"					},
	{0x16, 0x01, 0x03, 0x03, "1"							},
	{0x16, 0x01, 0x03, 0x02, "2"							},
	{0x16, 0x01, 0x03, 0x01, "3"							},
	{0x16, 0x01, 0x03, 0x00, "4"							},

	{0   , 0xfe, 0   ,    2, "Linked Cabinets"				},
	{0x16, 0x01, 0x04, 0x04, "Off"							},
	{0x16, 0x01, 0x04, 0x00, "On"							},
};

STDDIPINFOEXT(Vbowl, Vbowlhk, Vbowl)

static struct BurnDIPInfo VbowljDIPList[] = {
	{0   , 0xfe, 0   ,    2, "Controls"						},
	{0x14, 0x01, 0x20, 0x20, "Joystick"						},
	{0x14, 0x01, 0x20, 0x00, "Trackball"					},

	{0   , 0xfe, 0   ,    4, "Cabinet ID"					},
	{0x16, 0x01, 0x03, 0x03, "1"							},
	{0x16, 0x01, 0x03, 0x02, "2"							},
	{0x16, 0x01, 0x03, 0x01, "3"							},
	{0x16, 0x01, 0x03, 0x00, "4"							},

	{0   , 0xfe, 0   ,    2, "Linked Cabinets"				},
	{0x16, 0x01, 0x04, 0x04, "Off"							},
	{0x16, 0x01, 0x04, 0x00, "On"							},
};

STDDIPINFOEXT(Vbowlj, Vbowlhk, Vbowlj)

static struct BurnDIPInfo DrgnwrldCommonDIPList[]=
{
	{0x16, 0xff, 0xff, 0xff, NULL							},
	{0x17, 0xff, 0xff, 0xff, NULL							},
	{0x18, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    1, "Service Mode"					},
	{0x16, 0x01, 0x04, 0x04, "Off"							},
	{0x16, 0x01, 0x04, 0x00, "On"							},

	{0   , 0xfe, 0   ,    8, "Coinage"						},
	{0x16, 0x01, 0x07, 0x00, "5 Coins 1 Credits"			},
	{0x16, 0x01, 0x07, 0x01, "4 Coins 1 Credits"			},
	{0x16, 0x01, 0x07, 0x02, "3 Coins 1 Credits"			},
	{0x16, 0x01, 0x07, 0x03, "2 Coins 1 Credits"			},
	{0x16, 0x01, 0x07, 0x07, "1 Coin  1 Credits"			},
	{0x16, 0x01, 0x07, 0x06, "1 Coin  2 Credits"			},
	{0x16, 0x01, 0x07, 0x05, "1 Coin  3 Credits"			},
	{0x16, 0x01, 0x07, 0x04, "1 Coin  4 Credits"			},

	{0   , 0xfe, 0   ,    4, "Difficulty"					},
	{0x16, 0x01, 0x18, 0x18, "Easy   "						},
	{0x16, 0x01, 0x18, 0x10, "Normal "						},
	{0x16, 0x01, 0x18, 0x08, "Hard   "						},
	{0x16, 0x01, 0x18, 0x00, "Hardest"						},
};

//STDDIPINFO(DrgnwrldCommon)

static struct BurnDIPInfo DrgnwrldDIPList[]=
{
	{0   , 0xfe, 0   ,    2, "Strip Girl"					},
	{0x17, 0x01, 0x01, 0x00, "No"							},
	{0x17, 0x01, 0x01, 0x01, "Yes"							},

	{0   , 0xfe, 0   ,    2, "Background"					},
	{0x17, 0x01, 0x02, 0x02, "Girl"							},
	{0x17, 0x01, 0x02, 0x00, "Scene"						},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"					},
	{0x17, 0x01, 0x04, 0x00, "Off"							},
	{0x17, 0x01, 0x04, 0x04, "On"							},

	{0   , 0xfe, 0   ,    2, "Fever Game"					},
	{0x17, 0x01, 0x08, 0x08, "One Kind Only"				},
	{0x17, 0x01, 0x08, 0x00, "Several Kinds"				},
};

STDDIPINFOEXT(Drgnwrld, DrgnwrldCommon, Drgnwrld)

static struct BurnDIPInfo DrgnwrldcDIPList[]=
{
	{0   , 0xfe, 0   ,    2, "Strip Girl"					},
	{0x17, 0x01, 0x01, 0x00, "No"							},
	{0x17, 0x01, 0x01, 0x01, "Yes"							},

	{0   , 0xfe, 0   ,    2, "Sex Question"					},
	{0x17, 0x01, 0x02, 0x00, "No"							},
	{0x17, 0x01, 0x02, 0x02, "Yes"							},

	{0   , 0xfe, 0   ,    2, "Background"					},
	{0x17, 0x01, 0x04, 0x04, "Girl"							},
	{0x17, 0x01, 0x04, 0x00, "Scene"						},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"					},
	{0x17, 0x01, 0x08, 0x00, "Off"							},
	{0x17, 0x01, 0x08, 0x08, "On"							},

	{0   , 0xfe, 0   ,    2, "Tiles"						},
	{0x17, 0x01, 0x10, 0x10, "Mahjong"						},
	{0x17, 0x01, 0x10, 0x00, "Symbols"						},

	{0   , 0xfe, 0   ,    2, "Fever Game"					},
	{0x17, 0x01, 0x40, 0x40, "One Kind Only"				},
	{0x17, 0x01, 0x40, 0x00, "Several Kinds"				},
};

STDDIPINFOEXT(Drgnwrldc, DrgnwrldCommon, Drgnwrldc)

static struct BurnDIPInfo DrgnwrldjDIPList[]=
{
	{0   , 0xfe, 0   ,    2, "Background"					},
	{0x17, 0x01, 0x01, 0x01, "Girl"							},
	{0x17, 0x01, 0x01, 0x00, "Scene"						},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"					},
	{0x17, 0x01, 0x02, 0x00, "Off"							},
	{0x17, 0x01, 0x02, 0x02, "On"							},

	{0   , 0xfe, 0   ,    2, "Tiles"						},
	{0x17, 0x01, 0x04, 0x04, "Mahjong"						},
	{0x17, 0x01, 0x04, 0x00, "Symbols"						},
};

STDDIPINFOEXT(Drgnwrldj, DrgnwrldCommon, Drgnwrldj)

static struct BurnDIPInfo LhbDIPList[]=
{
	{0x22, 0xff, 0xff, 0x02, NULL							},
	{0x23, 0xff, 0xff, 0x7f, NULL							},
	{0x24, 0xff, 0xff, 0xff, NULL							},
	{0x25, 0xff, 0xff, 0x50, NULL							},
	{0x26, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,   16, "Payout Rate"					},
	{0x22, 0x01, 0x0f, 0x00, "50%"							},
	{0x22, 0x01, 0x0f, 0x01, "53%"							},
	{0x22, 0x01, 0x0f, 0x02, "56%"							},
	{0x22, 0x01, 0x0f, 0x03, "59%"							},
	{0x22, 0x01, 0x0f, 0x04, "62%"							},
	{0x22, 0x01, 0x0f, 0x05, "65%"							},
	{0x22, 0x01, 0x0f, 0x06, "68%"							},
	{0x22, 0x01, 0x0f, 0x07, "71%"							},
	{0x22, 0x01, 0x0f, 0x08, "75%"							},
	{0x22, 0x01, 0x0f, 0x09, "78%"							},
	{0x22, 0x01, 0x0f, 0x0a, "81%"							},
	{0x22, 0x01, 0x0f, 0x0b, "84%"							},
	{0x22, 0x01, 0x0f, 0x0c, "87%"							},
	{0x22, 0x01, 0x0f, 0x0d, "90%"							},
	{0x22, 0x01, 0x0f, 0x0e, "93%"							},
	{0x22, 0x01, 0x0f, 0x0f, "96%"							},

	{0   , 0xfe, 0   ,    4, "YAKUMAN Point"				},
	{0x22, 0x01, 0x30, 0x30, "1"							},
	{0x22, 0x01, 0x30, 0x20, "2"							},
	{0x22, 0x01, 0x30, 0x10, "3"							},
	{0x22, 0x01, 0x30, 0x00, "4"							},

	{0   , 0xfe, 0   ,    4, "Maximum Bet"					},
	{0x22, 0x01, 0xc0, 0xc0, "1"							},
	{0x22, 0x01, 0xc0, 0x80, "5"							},
	{0x22, 0x01, 0xc0, 0x40, "10"							},
	{0x22, 0x01, 0xc0, 0x00, "20"							},

	{0   , 0xfe, 0   ,    4, "Coinage"						},
	{0x23, 0x01, 0x03, 0x00, "2 Coins 1 Credit"			},
	{0x23, 0x01, 0x03, 0x03, "1 Coin 1 Credit"				},
	{0x23, 0x01, 0x03, 0x02, "1 Coin 2 Credits"			},
	{0x23, 0x01, 0x03, 0x01, "1 Coin 3 Credits"			},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"					},
	{0x23, 0x01, 0x0c, 0x0c, "1"							},
	{0x23, 0x01, 0x0c, 0x08, "2"							},
	{0x23, 0x01, 0x0c, 0x04, "3"							},
	{0x23, 0x01, 0x0c, 0x00, "5"							},

	{0   , 0xfe, 0   ,    1, "DAI MANGUAN Cycle"			},
	{0x23, 0x01, 0x70, 0x70, "300"							},

	{0   , 0xfe, 0   ,    1, "DAI MANGUAN Times"			},
	{0x23, 0x01, 0x80, 0x80, "2"							},

	{0   , 0xfe, 0   ,    4, "Score Limit"					},
	{0x24, 0x01, 0x03, 0x03, "1000"						},
	{0x24, 0x01, 0x03, 0x02, "2000"						},
	{0x24, 0x01, 0x03, 0x01, "5000"						},
	{0x24, 0x01, 0x03, 0x00, "Unlimited"					},

	{0   , 0xfe, 0   ,    4, "Credit Limit"				},
	{0x24, 0x01, 0x0c, 0x0c, "1000"						},
	{0x24, 0x01, 0x0c, 0x08, "2000"						},
	{0x24, 0x01, 0x0c, 0x04, "5000"						},
	{0x24, 0x01, 0x0c, 0x00, "Unlimited"					},

	{0   , 0xfe, 0   ,    2, "CPU Strength"				},
	{0x24, 0x01, 0x10, 0x10, "Strong"						},
	{0x24, 0x01, 0x10, 0x00, "Weak"						},

	{0   , 0xfe, 0   ,    2, "Money Type"					},
	{0x24, 0x01, 0x20, 0x20, "Coins"						},
	{0x24, 0x01, 0x20, 0x00, "Notes"						},

	{0   , 0xfe, 0   ,    4, "Don Den Times"				},
	{0x24, 0x01, 0xc0, 0xc0, "0"							},
	{0x24, 0x01, 0xc0, 0x80, "3"							},
	{0x24, 0x01, 0xc0, 0x40, "5"							},
	{0x24, 0x01, 0xc0, 0x00, "8"							},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"					},
	{0x25, 0x01, 0x01, 0x01, "Off"							},
	{0x25, 0x01, 0x01, 0x00, "On"							},

	{0   , 0xfe, 0   ,    2, "In-Game Music"				},
	{0x25, 0x01, 0x02, 0x02, "Off"							},
	{0x25, 0x01, 0x02, 0x00, "On"							},

	{0   , 0xfe, 0   ,    4, "Girls"						},
	{0x25, 0x01, 0x0c, 0x0c, "No"							},
	{0x25, 0x01, 0x0c, 0x08, "Dressed"					},
	{0x25, 0x01, 0x0c, 0x04, "Underwear"					},
	{0x25, 0x01, 0x0c, 0x00, "Nude"						},

	{0   , 0xfe, 0   ,    2, "Key-In Rate"				},
	{0x25, 0x01, 0x10, 0x10, "5"							},
	{0x25, 0x01, 0x10, 0x00, "10"							},

	{0   , 0xfe, 0   ,    2, "Payout Mode"				},
	{0x25, 0x01, 0x20, 0x20, "Key-Out"					},
	{0x25, 0x01, 0x20, 0x00, "Return Coins"				},

	{0   , 0xfe, 0   ,    2, "Credit Mode"				},
	{0x25, 0x01, 0x40, 0x40, "Coin Acceptor"				},
	{0x25, 0x01, 0x40, 0x00, "Key-In"						},

	{0   , 0xfe, 0   ,    2, "Last Chance"				},
	{0x25, 0x01, 0x80, 0x80, "Off"							},
	{0x25, 0x01, 0x80, 0x00, "On"							},

	{0   , 0xfe, 0   ,    2, "In-Game Bet"				},
	{0x26, 0x01, 0x01, 0x01, "No"							},
	{0x26, 0x01, 0x01, 0x00, "Yes"							},
};

STDDIPINFO(Lhb)

static struct BurnDIPInfo Lhb2DIPList[]=
{
	{0x22, 0xff, 0xff, 0xe4, NULL							},
	{0x23, 0xff, 0xff, 0xff, NULL							},
	{0x24, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    8, "Payout Rate"					},
	{0x22, 0x01, 0x07, 0x07, "50%"							},
	{0x22, 0x01, 0x07, 0x06, "54%"							},
	{0x22, 0x01, 0x07, 0x05, "58%"							},
	{0x22, 0x01, 0x07, 0x04, "62%"							},
	{0x22, 0x01, 0x07, 0x03, "66%"							},
	{0x22, 0x01, 0x07, 0x02, "70%"							},
	{0x22, 0x01, 0x07, 0x01, "74%"							},
	{0x22, 0x01, 0x07, 0x00, "78%"							},

	{0   , 0xfe, 0   ,    2, "Odds Rate"					},
	{0x22, 0x01, 0x08, 0x00, "1,2,3,4,5,6,7,8"			},
	{0x22, 0x01, 0x08, 0x08, "1,2,3,5,8,15,30,50"			},

	{0   , 0xfe, 0   ,    2, "Maximum Bet"					},
	{0x22, 0x01, 0x10, 0x00, "5"							},
	{0x22, 0x01, 0x10, 0x10, "10"							},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"					},
	{0x22, 0x01, 0x60, 0x60, "1"							},
	{0x22, 0x01, 0x60, 0x40, "2"							},
	{0x22, 0x01, 0x60, 0x20, "3"							},
	{0x22, 0x01, 0x60, 0x00, "5"							},

	{0   , 0xfe, 0   ,    2, "Credit Timer"				},
	{0x22, 0x01, 0x80, 0x80, "Off"							},
	{0x22, 0x01, 0x80, 0x00, "On"							},

	{0   , 0xfe, 0   ,    4, "Coinage"						},
	{0x23, 0x01, 0x03, 0x00, "2 Coins 1 Credit"			},
	{0x23, 0x01, 0x03, 0x03, "1 Coin 1 Credit"				},
	{0x23, 0x01, 0x03, 0x02, "1 Coin 2 Credits"			},
	{0x23, 0x01, 0x03, 0x01, "1 Coin 3 Credits"			},

	{0   , 0xfe, 0   ,    2, "Key-in Rate"					},
	{0x23, 0x01, 0x04, 0x04, "10"							},
	{0x23, 0x01, 0x04, 0x00, "100"							},

	{0   , 0xfe, 0   ,    2, "Credit Limit"				},
	{0x23, 0x01, 0x08, 0x08, "100"							},
	{0x23, 0x01, 0x08, 0x00, "500"							},

	{0   , 0xfe, 0   ,    2, "Credit Mode"				},
	{0x23, 0x01, 0x10, 0x10, "Coin Acceptor"				},
	{0x23, 0x01, 0x10, 0x00, "Key-In"						},

	{0   , 0xfe, 0   ,    2, "Payout Mode"				},
	{0x23, 0x01, 0x20, 0x20, "Key-Out"					},
	{0x23, 0x01, 0x20, 0x00, "Return Coins"				},

	{0   , 0xfe, 0   ,    2, "Auto Reach"					},
	{0x23, 0x01, 0x40, 0x00, "Off"							},
	{0x23, 0x01, 0x40, 0x40, "On"							},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"					},
	{0x23, 0x01, 0x80, 0x00, "Off"							},
	{0x23, 0x01, 0x80, 0x80, "On"							},

	{0   , 0xfe, 0   ,    4, "Jackpot Limit"				},
	{0x24, 0x01, 0x03, 0x03, "500"							},
	{0x24, 0x01, 0x03, 0x02, "1000"						},
	{0x24, 0x01, 0x03, 0x01, "2000"						},
	{0x24, 0x01, 0x03, 0x00, "Unlimited"					},

	{0   , 0xfe, 0   ,    4, "Gals"						},
	{0x24, 0x01, 0x0c, 0x0c, "Off"							},
	{0x24, 0x01, 0x0c, 0x08, "Clothed"					},
	{0x24, 0x01, 0x0c, 0x04, "Nudity"						},
	{0x24, 0x01, 0x0c, 0x00, "Nudity"						},
};

STDDIPINFO(Lhb2)

static struct BurnDIPInfo Lhb2cpgsDIPList[]=
{
	{0x22, 0xff, 0xff, 0xe4, NULL							},
	{0x23, 0xff, 0xff, 0xff, NULL							},
	{0x24, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    8, "Payout Rate"					},
	{0x22, 0x01, 0x07, 0x07, "50%"							},
	{0x22, 0x01, 0x07, 0x06, "54%"							},
	{0x22, 0x01, 0x07, 0x05, "58%"							},
	{0x22, 0x01, 0x07, 0x04, "62%"							},
	{0x22, 0x01, 0x07, 0x03, "66%"							},
	{0x22, 0x01, 0x07, 0x02, "70%"							},
	{0x22, 0x01, 0x07, 0x01, "74%"							},
	{0x22, 0x01, 0x07, 0x00, "78%"							},
	{0   , 0xfe, 0   ,    2, "Odds Rate"					},
	{0x22, 0x01, 0x08, 0x00, "1,2,3,4,5,6,7,8"			},
	{0x22, 0x01, 0x08, 0x08, "1,2,3,5,8,15,30,50"			},
	{0   , 0xfe, 0   ,    2, "Maximum Bet"					},
	{0x22, 0x01, 0x10, 0x00, "5"							},
	{0x22, 0x01, 0x10, 0x10, "10"							},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"					},
	{0x22, 0x01, 0x60, 0x60, "1"							},
	{0x22, 0x01, 0x60, 0x40, "2"							},
	{0x22, 0x01, 0x60, 0x20, "3"							},
	{0x22, 0x01, 0x60, 0x00, "5"							},
	{0   , 0xfe, 0   ,    2, "Credit Timer"				},
	{0x22, 0x01, 0x80, 0x80, "Off"							},
	{0x22, 0x01, 0x80, 0x00, "On"							},
	{0   , 0xfe, 0   ,    4, "Coinage"						},
	{0x23, 0x01, 0x03, 0x00, "2 Coins 1 Credit"			},
	{0x23, 0x01, 0x03, 0x03, "1 Coin 1 Credit"				},
	{0x23, 0x01, 0x03, 0x02, "1 Coin 2 Credits"			},
	{0x23, 0x01, 0x03, 0x01, "1 Coin 3 Credits"			},
	{0   , 0xfe, 0   ,    2, "Key-in Rate"					},
	{0x23, 0x01, 0x04, 0x04, "10"							},
	{0x23, 0x01, 0x04, 0x00, "100"							},
	{0   , 0xfe, 0   ,    2, "Credit Limit"				},
	{0x23, 0x01, 0x08, 0x08, "100"							},
	{0x23, 0x01, 0x08, 0x00, "500"							},
	{0   , 0xfe, 0   ,    2, "Credit Mode"				},
	{0x23, 0x01, 0x10, 0x10, "Coin Acceptor"				},
	{0x23, 0x01, 0x10, 0x00, "Key-In"						},
	{0   , 0xfe, 0   ,    2, "Payout Mode"				},
	{0x23, 0x01, 0x20, 0x20, "Key-Out"					},
	{0x23, 0x01, 0x20, 0x00, "Return Coins"				},
	{0   , 0xfe, 0   ,    2, "Auto Reach"					},
	{0x23, 0x01, 0x40, 0x00, "Off"							},
	{0x23, 0x01, 0x40, 0x40, "On"							},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"					},
	{0x23, 0x01, 0x80, 0x00, "Off"							},
	{0x23, 0x01, 0x80, 0x80, "On"							},
	{0   , 0xfe, 0   ,    8, "Lottery Ticket Ratio"		},
	{0x24, 0x01, 0x70, 0x70, "1:1"							},
	{0x24, 0x01, 0x70, 0x60, "1:2"							},
	{0x24, 0x01, 0x70, 0x50, "1:5"							},
	{0x24, 0x01, 0x70, 0x40, "1:6"							},
	{0x24, 0x01, 0x70, 0x30, "1:7"							},
	{0x24, 0x01, 0x70, 0x20, "1:8"							},
	{0x24, 0x01, 0x70, 0x10, "1:9"							},
	{0x24, 0x01, 0x70, 0x00, "1:10"						},
};

STDDIPINFO(Lhb2cpgs)

static struct BurnDIPInfo NkishuspDIPList[]=
{
	{0x22, 0xff, 0xff, 0xe2, NULL							},
	{0x23, 0xff, 0xff, 0xff, NULL							},
	{0x24, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    8, "Payout Rate"					},
	{0x22, 0x01, 0x07, 0x07, "74%"							},
	{0x22, 0x01, 0x07, 0x06, "77%"							},
	{0x22, 0x01, 0x07, 0x05, "80%"							},
	{0x22, 0x01, 0x07, 0x04, "83%"							},
	{0x22, 0x01, 0x07, 0x03, "86%"							},
	{0x22, 0x01, 0x07, 0x02, "89%"							},
	{0x22, 0x01, 0x07, 0x01, "92%"							},
	{0x22, 0x01, 0x07, 0x00, "95%"							},
	{0   , 0xfe, 0   ,    2, "Odds Rate"					},
	{0x22, 0x01, 0x08, 0x00, "1,2,3,4,5,6,7,8"			},
	{0x22, 0x01, 0x08, 0x08, "1,2,3,5,8,15,30,50"			},
	{0   , 0xfe, 0   ,    2, "Maximum Bet"					},
	{0x22, 0x01, 0x10, 0x00, "5"							},
	{0x22, 0x01, 0x10, 0x10, "10"							},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"					},
	{0x22, 0x01, 0x60, 0x60, "1"							},
	{0x22, 0x01, 0x60, 0x40, "2"							},
	{0x22, 0x01, 0x60, 0x20, "3"							},
	{0x22, 0x01, 0x60, 0x00, "5"							},
	{0   , 0xfe, 0   ,    2, "Credit Timer"				},
	{0x22, 0x01, 0x80, 0x80, "Off"					},
	{0x22, 0x01, 0x80, 0x00, "On"					},

	{0   , 0xfe, 0   ,    4, "Coinage"				},
	{0x23, 0x01, 0x03, 0x00, "2 Coins 1 Credit"				},
	{0x23, 0x01, 0x03, 0x03, "1 Coin 1 Credit"				},
	{0x23, 0x01, 0x03, 0x02, "1 Coin 2 Credits"				},
	{0x23, 0x01, 0x03, 0x01, "1 Coin 3 Credits"				},
	{0   , 0xfe, 0   ,    2, "Key-in Rate"					},
	{0x23, 0x01, 0x04, 0x04, "10"							},
	{0x23, 0x01, 0x04, 0x00, "100"							},
	{0   , 0xfe, 0   ,    2, "Credit Mode"				},
	{0x23, 0x01, 0x08, 0x08, "Coin Acceptor"				},
	{0x23, 0x01, 0x08, 0x00, "Key-In"						},
	{0   , 0xfe, 0   ,    2, "Auto Play"					},
	{0x23, 0x01, 0x10, 0x00, "Off"					},
	{0x23, 0x01, 0x10, 0x10, "On"					},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"			},
	{0x23, 0x01, 0x20, 0x00, "Off"					},
	{0x23, 0x01, 0x20, 0x20, "On"					},
	{0   , 0xfe, 0   ,    2, "Nudity"						},
	{0x23, 0x01, 0x40, 0x40, "Off"					},
	{0x23, 0x01, 0x40, 0x00, "On"					},

	{0   , 0xfe, 0   ,    4, "Credit Limit"				},
	{0x24, 0x01, 0x03, 0x03, "500"							},
	{0x24, 0x01, 0x03, 0x02, "1000"						},
	{0x24, 0x01, 0x03, 0x01, "2000"						},
	{0x24, 0x01, 0x03, 0x00, "Unlimited"					},
};

STDDIPINFO(Nkishusp)

static struct BurnDIPInfo TygnDIPList[]=
{
	{0x0c, 0xff, 0xff, 0xf2, NULL							},
	{0x0d, 0xff, 0xff, 0xff, NULL							},
	{0x0e, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    8, "Payout Rate"					},
	{0x0c, 0x01, 0x07, 0x07, "50%"							},
	{0x0c, 0x01, 0x07, 0x06, "54%"							},
	{0x0c, 0x01, 0x07, 0x05, "58%"							},
	{0x0c, 0x01, 0x07, 0x04, "62%"							},
	{0x0c, 0x01, 0x07, 0x03, "66%"							},
	{0x0c, 0x01, 0x07, 0x02, "70%"							},
	{0x0c, 0x01, 0x07, 0x01, "74%"							},
	{0x0c, 0x01, 0x07, 0x00, "78%"							},
	{0   , 0xfe, 0   ,    2, "Minimum Bet"					},
	{0x0c, 0x01, 0x08, 0x08, "1"							},
	{0x0c, 0x01, 0x08, 0x00, "2"							},
	{0   , 0xfe, 0   ,    4, "Coinage"				},
	{0x0c, 0x01, 0x30, 0x00, "2 Coins 1 Credit"				},
	{0x0c, 0x01, 0x30, 0x30, "1 Coin 1 Credit"				},
	{0x0c, 0x01, 0x30, 0x20, "1 Coin 2 Credits"				},
	{0x0c, 0x01, 0x30, 0x10, "1 Coin 3 Credits"				},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"			},
	{0x0c, 0x01, 0x40, 0x00, "Off"					},
	{0x0c, 0x01, 0x40, 0x40, "On"					},
	{0   , 0xfe, 0   ,    2, "Credit Timer"				},
	{0x0c, 0x01, 0x80, 0x80, "Off"					},
	{0x0c, 0x01, 0x80, 0x00, "On"					},

	{0   , 0xfe, 0   ,    1, "Unused"				},
	{0x0e, 0x01, 0xff, 0xff, "Off"							},
};

STDDIPINFO(Tygn)

static struct BurnDIPInfo XymgDIPList[]=
{
	{0x22, 0xff, 0xff, 0xff, NULL							},
	{0x23, 0xff, 0xff, 0xff, NULL							},
	{0x24, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    4, "Coinage"				},
	{0x22, 0x01, 0x03, 0x03, "1 Coin 1 Credit"				},
	{0x22, 0x01, 0x03, 0x02, "1 Coin 2 Credits"				},
	{0x22, 0x01, 0x03, 0x01, "1 Coin 3 Credits"				},
	{0x22, 0x01, 0x03, 0x00, "1 Coin 4 Credits"				},
	{0   , 0xfe, 0   ,    4, "Key-in Rate"					},
	{0x22, 0x01, 0x0c, 0x0c, "10"							},
	{0x22, 0x01, 0x0c, 0x08, "20"							},
	{0x22, 0x01, 0x0c, 0x04, "50"							},
	{0x22, 0x01, 0x0c, 0x00, "100"							},
	{0   , 0xfe, 0   ,    2, "Credit Limit"				},
	{0x22, 0x01, 0x10, 0x10, "500"							},
	{0x22, 0x01, 0x10, 0x00, "Unlimited"					},
	{0   , 0xfe, 0   ,    2, "Credit Mode"				},
	{0x22, 0x01, 0x20, 0x20, "Coin Acceptor"				},
	{0x22, 0x01, 0x20, 0x00, "Key-In"						},
	{0   , 0xfe, 0   ,    2, "Payout Mode"				},
	{0x22, 0x01, 0x40, 0x40, "Return Coins"				},
	{0x22, 0x01, 0x40, 0x00, "Key-Out"					},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"			},
	{0x22, 0x01, 0x80, 0x00, "Off"					},
	{0x22, 0x01, 0x80, 0x80, "On"					},

	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"			},
	{0x23, 0x01, 0x03, 0x03, "1000"						},
	{0x23, 0x01, 0x03, 0x02, "1500"						},
	{0x23, 0x01, 0x03, 0x01, "2000"						},
	{0x23, 0x01, 0x03, 0x00, "3000"						},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"					},
	{0x23, 0x01, 0x0c, 0x0c, "1"							},
	{0x23, 0x01, 0x0c, 0x08, "2"							},
	{0x23, 0x01, 0x0c, 0x04, "3"							},
	{0x23, 0x01, 0x0c, 0x00, "5"							},
	{0   , 0xfe, 0   ,    2, "Double Up Game"				},
	{0x23, 0x01, 0x10, 0x00, "Off"					},
	{0x23, 0x01, 0x10, 0x10, "On"					},
	{0   , 0xfe, 0   ,    2, "Title Screen"				},
	{0x23, 0x01, 0x20, 0x00, "Off"					},
	{0x23, 0x01, 0x20, 0x20, "On"					},
};

STDDIPINFO(Xymg)

static struct BurnDIPInfo WlccDIPList[]=
{
	{0x10, 0xff, 0xff, 0xff, NULL							},
	{0x11, 0xff, 0xff, 0xff, NULL							},
	{0x12, 0xff, 0xff, 0xff, NULL							},
	{0x13, 0xff, 0xff, 0xff, NULL							},

	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"			},
	{0x11, 0x01, 0x03, 0x03, "1000"						},
	{0x11, 0x01, 0x03, 0x02, "1500"						},
	{0x11, 0x01, 0x03, 0x01, "2000"						},
	{0x11, 0x01, 0x03, 0x00, "3000"						},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"					},
	{0x11, 0x01, 0x0c, 0x0c, "1"							},
	{0x11, 0x01, 0x0c, 0x08, "2"							},
	{0x11, 0x01, 0x0c, 0x04, "3"							},
	{0x11, 0x01, 0x0c, 0x00, "5"							},
	{0   , 0xfe, 0   ,    2, "Double Up Game"				},
	{0x11, 0x01, 0x10, 0x00, "Off"					},
	{0x11, 0x01, 0x10, 0x10, "On"					},
	{0   , 0xfe, 0   ,    2, "Title Screen"				},
	{0x11, 0x01, 0x40, 0x00, "Off"					},
	{0x11, 0x01, 0x40, 0x40, "On"					},

	{0   , 0xfe, 0   ,    4, "Coinage"				},
	{0x12, 0x01, 0x03, 0x03, "1 Coin 1 Credit"				},
	{0x12, 0x01, 0x03, 0x02, "1 Coin 2 Credits"				},
	{0x12, 0x01, 0x03, 0x01, "1 Coin 3 Credits"				},
	{0x12, 0x01, 0x03, 0x00, "1 Coin 4 Credits"				},
	{0   , 0xfe, 0   ,    4, "Key-in Rate"					},
	{0x12, 0x01, 0x0c, 0x0c, "10"							},
	{0x12, 0x01, 0x0c, 0x08, "20"							},
	{0x12, 0x01, 0x0c, 0x04, "50"							},
	{0x12, 0x01, 0x0c, 0x00, "100"							},
	{0   , 0xfe, 0   ,    2, "Credit Limit"				},
	{0x12, 0x01, 0x10, 0x10, "500"							},
	{0x12, 0x01, 0x10, 0x00, "Unlimited"					},
	{0   , 0xfe, 0   ,    2, "Credit Mode"				},
	{0x12, 0x01, 0x20, 0x20, "Coin Acceptor"				},
	{0x12, 0x01, 0x20, 0x00, "Key-In"						},
	{0   , 0xfe, 0   ,    2, "Payout Mode"				},
	{0x12, 0x01, 0x40, 0x40, "Return Coins"				},
	{0x12, 0x01, 0x40, 0x00, "Key-Out"					},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"			},
	{0x12, 0x01, 0x80, 0x00, "Off"					},
	{0x12, 0x01, 0x80, 0x80, "On"					},
};

STDDIPINFO(Wlcc)

static UINT16 inline dips_read(INT32 num)
{
	UINT16 ret = 0xff;

	for (INT32 i = 0; i < num; i++)
		if (BIT(~dips_select, i))
			ret &= DrvDips[i];

	return  (ret & 0xff) | 0x0000;	// 0x0100 is blitter busy
}

static UINT16 key_matrix_read()
{
	UINT16 ret = 0xff;

	for (INT32 i = 0; i < 5; i++) {
		if (BIT(~input_select, i)) {
			ret &= DrvMahjongInputs[i];
		}
	}

	return ret;
}

static void pack_mahjong_inputs()
{
	memset(DrvMahjongInputs, 0xff, sizeof(DrvMahjongInputs));

	if (DrvMahjong[0])  DrvMahjongInputs[0] &= ~0x01;
	if (DrvMahjong[1])  DrvMahjongInputs[0] &= ~0x02;
	if (DrvMahjong[2])  DrvMahjongInputs[0] &= ~0x04;
	if (DrvMahjong[3])  DrvMahjongInputs[0] &= ~0x08;
	if (DrvMahjong[4])  DrvMahjongInputs[0] &= ~0x10;
	if (DrvMahjong[5])  DrvMahjongInputs[0] &= ~0x20;

	if (DrvMahjong[6])  DrvMahjongInputs[1] &= ~0x01;
	if (DrvMahjong[7])  DrvMahjongInputs[1] &= ~0x02;
	if (DrvMahjong[8])  DrvMahjongInputs[1] &= ~0x04;
	if (DrvMahjong[9])  DrvMahjongInputs[1] &= ~0x08;
	if (DrvMahjong[10]) DrvMahjongInputs[1] &= ~0x10;
	if (DrvMahjong[11]) DrvMahjongInputs[1] &= ~0x20;

	if (DrvMahjong[12]) DrvMahjongInputs[2] &= ~0x01;
	if (DrvMahjong[13]) DrvMahjongInputs[2] &= ~0x02;
	if (DrvMahjong[14]) DrvMahjongInputs[2] &= ~0x04;
	if (DrvMahjong[15]) DrvMahjongInputs[2] &= ~0x08;
	if (DrvMahjong[16]) DrvMahjongInputs[2] &= ~0x10;

	if (DrvMahjong[18]) DrvMahjongInputs[3] &= ~0x01;
	if (DrvMahjong[19]) DrvMahjongInputs[3] &= ~0x02;
	if (DrvMahjong[20]) DrvMahjongInputs[3] &= ~0x04;
	if (DrvMahjong[21]) DrvMahjongInputs[3] &= ~0x08;

	if (DrvMahjong[24]) DrvMahjongInputs[4] &= ~0x01;
	if (DrvMahjong[25]) DrvMahjongInputs[4] &= ~0x02;
	if (DrvMahjong[26]) DrvMahjongInputs[4] &= ~0x04;
	if (DrvMahjong[28]) DrvMahjongInputs[4] &= ~0x10;
	if (DrvMahjong[29]) DrvMahjongInputs[4] &= ~0x20;
}

static UINT8 hopper_line()
{
	if (!has_hopper) return 1;
	return (hopper_motor && ((GetCurrentFrame() % 10) == 0)) ? 0 : 1;
}

static void igs011_blit()
{
	int    const layer  =      s_blitter.flags & 0x0007;
	bool     const opaque = BIT(~s_blitter.flags,  3);
	bool     const clear  = BIT( s_blitter.flags,  4);
	bool     const flipx  = BIT( s_blitter.flags,  5);
	bool     const flipy  = BIT( s_blitter.flags,  6);
	bool     const blit   = BIT( s_blitter.flags, 10);

	if (!blit)
		return;

	int const pen_hi = (s_blitter.pen_hi & 0x07) << 5;
	int const hibpp_layers = 4 - (s_blitter.depth & 0x07);
	bool  const dst4     = layer >= hibpp_layers;

	bool const src4   = dst4 || (s_blitter.gfx_hi & 0x80); // see lhb2
	int const shift   = dst4 ? (((layer - hibpp_layers) & 0x01) << 2) : 0;
	int const mask    = dst4 ? (0xf0 >> shift) : 0x00;
	int const buffer  = dst4 ? (hibpp_layers + ((layer - hibpp_layers) >> 1)) : layer;

	if (buffer >= 4)
	{
		return;
	}

	UINT8 *dest = DrvLayerRAM[buffer];

	UINT32 z = ((s_blitter.gfx_hi & 0x7f) << 16) | s_blitter.gfx_lo;

	UINT8 const clear_pen = src4 ? (s_blitter.pen | 0xf0) : s_blitter.pen;

	UINT8 trans_pen;
	if (src4)
	{
		z <<= 1;
		if (DrvGfxROM[1] && (s_blitter.gfx_hi & 0x80)) trans_pen = 0x1f;   // lhb2
		else                                       trans_pen = 0x0f;
	}
	else
	{
		if (DrvGfxROM[1]) trans_pen = 0x1f;   // vbowl
		else          trans_pen = 0xff;
	}

	int xstart = s_blitter.x; //util::sext(m_blitter.x, 10);
	if (xstart & 0x400) xstart |= 0xfffffc00;
	int ystart = s_blitter.y; //util::sext(m_blitter.y, 9);
	if (ystart & 0x200) ystart |= 0xfffffe00;
	int const xsize = (s_blitter.w & 0x1ff) + 1;
	int const ysize = (s_blitter.h & 0x0ff) + 1;
	int const xend = flipx ? (xstart - xsize) : (xstart + xsize);
	int const yend = flipy ? (ystart - ysize) : (ystart + ysize);
	int const xinc = flipx ? -1 : 1;
	int const yinc = flipy ? -1 : 1;

	for (int y = ystart; y != yend; y += yinc)
	{
		for (int x = xstart; x != xend; x += xinc)
		{
			if (y >= 0 && y < 256 && x >= 0 && x < 512)
			{
				UINT8 pen = 0;
				if (!clear)
				{
					if (src4)
						pen = (DrvGfxROM[0][(z >> 1) & 0x7fffff] >> (BIT(z, 0) << 2)) & 0x0f;
					else
						pen = DrvGfxROM[0][z & 0x7fffff];

					if (DrvGfxROM[1])
						pen = (pen & 0x0f) | (BIT(DrvGfxROM[1][(z >> 3) & 0xfffff], z & 0x07) << 4);
				}

				if (clear || (pen != trans_pen) || opaque)
				{
					UINT8 const val = clear ? clear_pen : (pen != trans_pen) ? (pen | pen_hi) : 0xff;
					UINT8 &destbyte = dest[(y << 9) | x];
					if (dst4)
						destbyte = (destbyte & mask) | ((val & 0x0f) << shift);
					else
						destbyte = val;
				}
			}
			++z;
		}
	}
}

static void __fastcall igs011_blit_write_word(UINT32 address, UINT16 data)
{
	switch (address & 0x7fff)
	{
		case 0x0000: s_blitter.x = data; break;
		case 0x0800: s_blitter.y = data; break;
		case 0x1000: s_blitter.w = data; break;
		case 0x1800: s_blitter.h = data; break;
		case 0x2000: s_blitter.gfx_lo = data; break;
		case 0x2800: s_blitter.gfx_hi = data; break;
		case 0x3000: s_blitter.flags = data; igs011_blit(); break;
		case 0x3800: s_blitter.pen = data; break;
		case 0x4000: s_blitter.depth = data; break;
	}
}

static void __fastcall igs011_blit_write_byte(UINT32 address, UINT8 data)
{
	bprintf (0, _T("Blit_wb: %5.5x, %2.2x\n"), address, data);
}

static void __fastcall igs011_palette_write_word(UINT32 address, UINT16 data)
{
	DrvPalRAM[(address >> 1) & 0xfff] = data;
}

static void __fastcall igs011_palette_write_byte(UINT32 address, UINT8 data)
{
	DrvPalRAM[(address >> 1) & 0xfff] = data;
}

static UINT16 __fastcall igs011_palette_read_word(UINT32 address)
{
	bprintf (0, _T("pal_rw: %5.5x\n"), address);
	return 0;
}	

static UINT8 __fastcall igs011_palette_read_byte(UINT32 address)
{
	return DrvPalRAM[(address >> 1) & 0xfff];
}

static void __fastcall igs011_layerram_write_word(UINT32 address, UINT16 data)
{
	bprintf (0, _T("pal_ww: %5.5x, %4.4x\n"), address, data);
}

static void __fastcall igs011_layerram_write_byte(UINT32 address, UINT8 data)
{
	DrvLayerRAM[((address >> 17) & 2) | (address & 1)][(address & 0x3fffe) >> 1] = data;
}

static UINT16 __fastcall igs011_layerram_read_word(UINT32 address)
{
	bprintf (0, _T("lay_rw: %5.5x\n"), address);
	return 0;
}	

static UINT8 __fastcall igs011_layerram_read_byte(UINT32 address)
{
	return DrvLayerRAM[((address >> 17) & 2) | (address & 1)][(address & 0x3fffe) >> 1];
}

#define MODE_AND_DATA(_MODE,_DATA)  (igs012_prot_mode == (_MODE) && ((data & 0xff) == (_DATA)) )

static inline void igs012_prot_swap_write(UINT8 data)
{
	if (MODE_AND_DATA(0, 0x55) || MODE_AND_DATA(1, 0xa5) )
	{
		UINT8 x = igs012_prot;
		igs012_prot_swap = (((BIT(x,3)|BIT(x,1))^1)<<3) | ((BIT(x,2)&BIT(x,1))<<2) | ((BIT(x,3)^BIT(x,0))<<1) | (BIT(x,2)^1);
	}
/*
	if ((igs012_prot_mode == 0 && data == 0x55) || (igs012_prot_mode == 1 && data == 0xa5))
	{
		UINT8 x = igs012_prot;
		igs012_prot_swap = (((BIT(x,3)|BIT(x,1))^1)<<3) | ((BIT(x,2)&BIT(x,1))<<2) | ((BIT(x,3)^BIT(x,0))<<1) | (BIT(x,2)^1);
	}
*/
}

static inline void igs012_prot_dec_inc_write(UINT8 data)
{
	if (MODE_AND_DATA(0, 0xaa) )
	{
		igs012_prot = (igs012_prot - 1) & 0x1f;
	}
	else if (MODE_AND_DATA(1, 0xfa) )
	{
		igs012_prot = (igs012_prot + 1) & 0x1f;
	}
/*
	if (igs012_prot_mode == 0 && data == 0xaa)
	{
		igs012_prot = (igs012_prot - 1) & 0x1f;
	}
	else if (igs012_prot_mode == 1 && data == 0xfa)
	{
		igs012_prot = (igs012_prot + 1) & 0x1f;
	}
*/
}

static inline void igs012_prot_inc_write(UINT8 data)
{
	if (MODE_AND_DATA(0, 0xff) )
	{
		igs012_prot = (igs012_prot + 1) & 0x1f;
	}
/*
	if (igs012_prot_mode == 0 && data == 0xff)
	{
		igs012_prot = (igs012_prot + 1) & 0x1f;
	}
*/
}

static inline void igs012_prot_copy_write(UINT8 data)
{
	if (MODE_AND_DATA(1, 0x22) )
	{
		igs012_prot = igs012_prot_swap;
	}
/*
	if (igs012_prot_mode == 1 && data == 0x22)
	{
		igs012_prot = igs012_prot_swap;
	}
*/
}

static inline void igs012_prot_dec_copy_write(UINT8 data)
{
	if (MODE_AND_DATA(0, 0x33) )
	{
		igs012_prot = igs012_prot_swap;
	}
	else if (MODE_AND_DATA(1, 0x5a) )
	{
		igs012_prot = (igs012_prot - 1) & 0x1f;
	}
/*
	if (igs012_prot_mode == 0 && data == 0x33)
	{
		igs012_prot = igs012_prot_swap;
	}
	else if (igs012_prot_mode == 1 && data == 0x5a)
	{
		igs012_prot = (igs012_prot - 1) & 0x1f;
	}
*/
}

static inline void igs012_prot_mode_write(UINT8 data)
{
	if (MODE_AND_DATA(0, 0xcc) || MODE_AND_DATA(1, 0xcc) || MODE_AND_DATA(0, 0xdd) || MODE_AND_DATA(1, 0xdd))
	{
		igs012_prot_mode = igs012_prot_mode ^ 1;
	}
/*
	if (data == 0xcc || data == 0xdd)
	{
		igs012_prot_mode ^= 1;
	}
*/
}

static UINT16 drgnwrldv20j_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = ((BIT(x,4)^1) | (BIT(x,0)^1)) | ((BIT(x,3) | BIT(x,1))^1) | ((BIT(x,2) & BIT(x,0))^1);
	return (b9 << 9);
}

static UINT16 drgnwrldv40k_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = ((BIT(x, 4) ^ 1) & (BIT(x, 0) ^ 1)) | (((BIT(x, 3)) & ((BIT(x, 2)) ^ 1)) & (((BIT(x, 1)) ^ (BIT(x, 0))) ^ 1));
	return (b9 << 9);
}

static UINT16 drgnwrldv21_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = (BIT(x,4)^1) | ((BIT(x,0)^1) & BIT(x,2)) | ((BIT(x,3)^BIT(x,1)^1) & ((((BIT(x,4)^1) & BIT(x,0)) | BIT(x,2))^1) );
	return (b9 << 9);
}

static UINT16 inline vbowl_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = ((BIT(x,4)^1) & (BIT(x,3)^1)) | ((BIT(x,2) & BIT(x,1))^1) | ((BIT(x,4) | BIT(x,0))^1);
	return (b9 << 9);
}

static UINT16 inline vbowlhk_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = ((BIT(x,4)^1) & (BIT(x,3)^1)) | (((BIT(x,2)^1) & (BIT(x,1)^1))^1) | ((BIT(x,4) | (BIT(x,0)^1))^1);
	return (b9 << 9);
}

static UINT16 lhb_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = (BIT(x,2) ^ 1) | (BIT(x,1) & BIT(x,0));
	return (b9 << 9);
}

static UINT16 dbc_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = (BIT(x,1) ^ 1) | ((BIT(x,0) ^ 1) & BIT(x,2));
	return (b9 << 9);
}

static UINT16 ryukobou_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b9 = ((BIT(x,1) ^ 1) | BIT(x,2)) & BIT(x,0);
	return (b9 << 9);
}

static UINT16 lhb2_igs011_prot2_read()
{
	UINT8 x = igs011_prot2;
	UINT8 b3 = (BIT(x,2) ^ 1) | (BIT(x,1) ^ 1) | BIT(x,0);
	return (b3 << 3);
}

static inline void igs011_prot2_reset()
{
	igs011_prot2 = 0;
}

static inline UINT16 igs011_prot2_reset_read()
{
	igs011_prot2 = 0;
	return 0;
}

static inline void igs011_prot2_inc()
{
	igs011_prot2++;
}

static inline void igs011_prot2_dec()
{
	igs011_prot2--;
}

static inline void drgnwrld_igs011_prot2_swap()
{
	UINT8 x = igs011_prot2;
	igs011_prot2 = ((BIT(x,3) & BIT(x,0)) << 4) | (BIT(x,2) << 3) | ((BIT(x,0) | BIT(x,1)) << 2) | ((BIT(x,2) ^ BIT(x,4) ^ 1) << 1) | (BIT(x,1) ^ 1 ^ BIT(x,3));
}

static inline void lhb_igs011_prot2_swap()
{
	UINT8 x = igs011_prot2;
	igs011_prot2 = (((BIT(x,0) ^ 1) | BIT(x,1)) << 2) | (BIT(x,2) << 1) | (BIT(x,0) & BIT(x,1));
}

static inline void wlcc_igs011_prot2_swap()
{
	UINT8 x = igs011_prot2;
	igs011_prot2 = ((BIT(x,3) ^ BIT(x,2)) << 4) | ((BIT(x,2) ^ BIT(x,1)) << 3) | ((BIT(x,1) ^ BIT(x,0)) << 2) | ((BIT(x,4) ^ BIT(x,0) ^ 1) << 1) | (BIT(x,4) ^ BIT(x,3) ^ 1);
}

static void igs011_prot1_write(UINT8 offset, UINT8 data)
{
	switch (offset << 1)
	{
		case 0: // COPY
			if (data == 0x33)
			{
				igs011_prot1 = igs011_prot1_swap;
			}
		break;

		case 2: // INC
			if (data == 0xff)
			{
				igs011_prot1++;
			}
		break;

		case 4: // DEC
			if (data == 0xaa)
			{
				igs011_prot1--;
			}
		break;

		case 6: // SWAP
			if (data == 0x55)
			{
				// b1 . (b2|b3) . b2 . (b0&b3)
				UINT8 x = igs011_prot1;
				igs011_prot1_swap = (BIT(x,1)<<3) | ((BIT(x,2)|BIT(x,3))<<2) | (BIT(x,2)<<1) | (BIT(x,0)&BIT(x,3));
			}
		break;
	}
}

static UINT16 inline igs011_prot1_read()
{
	UINT16 x = igs011_prot1;
	return (((BIT(x,1)&BIT(x,2))^1)<<5) | ((BIT(x,0)^BIT(x,3))<<2);
}

static void igs011_prot_addr_write(UINT16 data)
{
	igs011_prot1 = 0;
	igs011_prot1_swap = 0;
	igs011_prot1_addr = (data << 4) ^ 0x8340;
}

static void vbowl_igs003_write(UINT16 data)
{
	switch (igs003_reg)
	{
		case 0x02: break;

		case 0x40:
			igs003_prot_h2 = igs003_prot_h1;
			igs003_prot_h1 = data;
		break;

		case 0x48:
			igs003_prot_x = 0;
			if ((igs003_prot_h2 & 0x0a) != 0x0a) igs003_prot_x |= 0x08;
			if ((igs003_prot_h2 & 0x90) != 0x90) igs003_prot_x |= 0x04;
			if ((igs003_prot_h1 & 0x06) != 0x06) igs003_prot_x |= 0x02;
			if ((igs003_prot_h1 & 0x90) != 0x90) igs003_prot_x |= 0x01;
		break;

		case 0x50: // reset?
			igs003_prot_hold = 0;
		break;

		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		{
			const UINT16 old = igs003_prot_hold;
			igs003_prot_y = igs003_reg & 0x07;
			igs003_prot_z = data;
			igs003_prot_hold <<= 1;
			igs003_prot_hold |= BIT(old, 15);
			igs003_prot_hold ^= 0x2bad;
			igs003_prot_hold ^= BIT(old, 10);
			igs003_prot_hold ^= BIT(old,  8);
			igs003_prot_hold ^= BIT(old,  5);
			igs003_prot_hold ^= BIT(igs003_prot_z, igs003_prot_y);
			igs003_prot_hold ^= BIT(igs003_prot_x, 0) <<  4;
			igs003_prot_hold ^= BIT(igs003_prot_x, 1) <<  6;
			igs003_prot_hold ^= BIT(igs003_prot_x, 2) << 10;
			igs003_prot_hold ^= BIT(igs003_prot_x, 3) << 12;
		}
		break;
	}
}

static void vbowlhk_igs003_write(UINT16 data)
{
	switch (igs003_reg)
	{
		case 0x02: break;

		case 0x40:
			igs003_prot_h2 = igs003_prot_h1;
			igs003_prot_h1 = data;
		break;

		case 0x48:
			igs003_prot_x = 0;
			if ((igs003_prot_h2 & 0x0a) != 0x0a) igs003_prot_x |= 0x08;
			if ((igs003_prot_h2 & 0x90) != 0x90) igs003_prot_x |= 0x04;
			if ((igs003_prot_h1 & 0x06) != 0x06) igs003_prot_x |= 0x02;
			if ((igs003_prot_h1 & 0x90) != 0x90) igs003_prot_x |= 0x01;
		break;

		case 0x50: // reset?
			igs003_prot_hold = 0;
		break;

		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		{
			const UINT16 old = igs003_prot_hold;
			igs003_prot_y = igs003_reg & 0x07;
			igs003_prot_z = data;
			igs003_prot_hold <<= 1;
			igs003_prot_hold |= BIT(old, 15);
			igs003_prot_hold ^= 0x2bad;
			igs003_prot_hold ^= BIT(old, 7);
			igs003_prot_hold ^= BIT(old, 6);
			igs003_prot_hold ^= BIT(old, 5);
			igs003_prot_hold ^= BIT(igs003_prot_z, igs003_prot_y);
			igs003_prot_hold ^= BIT(igs003_prot_x, 0) <<  3;
			igs003_prot_hold ^= BIT(igs003_prot_x, 1) <<  8;
			igs003_prot_hold ^= BIT(igs003_prot_x, 2) << 10;
			igs003_prot_hold ^= BIT(igs003_prot_x, 3) << 14;
		}
		break;
	}
}

static UINT16 vbowl_igs003_read()
{
	switch (igs003_reg)
	{
		case 0x00: return DrvInputs[1];
		case 0x01: return DrvInputs[2];
		case 0x02: return DrvInputs[3]; // drgnwrld

		case 0x03: return BITSWAP08(igs003_prot_hold, 5,2,9,7,10,13,12,15);

		case 0x20: return 0x49;
		case 0x21: return 0x47;
		case 0x22: return 0x53;

		case 0x24: return 0x41;
		case 0x25: return 0x41;
		case 0x26: return 0x7f;
		case 0x27: return 0x41;
		case 0x28: return 0x41;

		case 0x2a: return 0x3e;
		case 0x2b: return 0x41;
		case 0x2c: return 0x49;
		case 0x2d: return 0xf9;
		case 0x2e: return 0x0a;

		case 0x30: return 0x26;
		case 0x31: return 0x49;
		case 0x32: return 0x49;
		case 0x33: return 0x49;
		case 0x34: return 0x32;
	}

	return 0;
}

static void __fastcall vbowl_main_write_word(UINT32 address, UINT16 data)
{	
	switch (address & ~0x1c00f)
	{
		case 0x001600:
			igs012_prot_swap_write(data);
		return;

		case 0x001620:
			igs012_prot_dec_inc_write( data);
		return;

		case 0x001630:
			igs012_prot_inc_write(data);
		return;

		case 0x001640:
			igs012_prot_copy_write(data);
		return;

		case 0x001650:
			igs012_prot_dec_copy_write(data);
		return;

		case 0x001670:
			igs012_prot_mode_write(data);
		return;
	}

	switch (address & ~0x3f)
	{
		case 0x00d400:
			igs011_prot2--;
		return;

		case 0x00d440: {
			UINT8 x = igs011_prot2;
			igs011_prot2 = ((BIT(x,3)&BIT(x,0))<<4) | (BIT(x,2)<<3) | ((BIT(x,0)|BIT(x,1))<<2) | ((BIT(x,2)^BIT(x,4)^1)<<1) | (BIT(x,1)^1^BIT(x,3));
		}
		return;

		case 0x00d480:
			igs011_prot2 = 0;
		return;
	}

	switch (address & ~0x1ff)
	{
		case 0x50f000:
			igs011_prot2--;
		return;

		case 0x50f200: {
			UINT8 x = igs011_prot2;
			igs011_prot2 = ((BIT(x,3)^BIT(x,2))<<4) | ((BIT(x,2)^BIT(x,1))<<3) | ((BIT(x,1)^BIT(x,0))<<2) | ((BIT(x,4)^BIT(x,0))<<1) | (BIT(x,4)^BIT(x,3));
		}
		return;

		case 0x50f400:
			igs011_prot2 = 0;
		return;
	}

	if ((address & 0xfff000) == 0x902000) {
		igs012_prot = 0;
		igs012_prot_swap = 0;
		igs012_prot_mode = 0;
		return;
	}

	switch (address)
	{      //0,1
		case 0x600002:
			ics2115write(1, data & 0xff);
		return;

		case 0x600004:
			ics2115write(2, data & 0xff);
			ics2115write(3, data >> 8);
		return;
		   //6,7
		case 0x600000:
		case 0x600006:
		return;

		case 0x700000:
		case 0x700002:
			trackball[(address & 2) / 2] = data;
		return;

		case 0x700004:
			s_blitter.pen_hi = data & 0x07;
		return;

		case 0x800000:
			igs003_reg = data;
		return;

		case 0x800002:
			vbowl_igs003_write(data);
		return;

		case 0xa00000:
		case 0xa08000:
		case 0xa10000:
		case 0xa18000:
			// link_#_w
		return;

		case 0xa20000:
			priority = data;
		return;

		case 0xa28000:
		case 0xa38000:
		return; // ??


		case 0xa40000:
			dips_select = data;
		return;

		case 0xa48000:
			igs011_prot_addr_write(data);
		return;
	}

//	bprintf (0, _T("MWW: %5.5x, %4.4x\n"), address, data);
}

static void __fastcall vbowl_main_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xffffff8) == igs011_prot1_addr) {
		igs011_prot1_write((address & 6) / 2, data);
		return;
	}

	switch (address & ~0x1c00f)
	{
		case 0x001600:
			igs012_prot_swap_write(data);
		return;

		case 0x001620:
			igs012_prot_dec_inc_write( data);
		return;

		case 0x001630:
			igs012_prot_inc_write(data);
		return;

		case 0x001640:
			igs012_prot_copy_write(data);
		return;

		case 0x001650:
			igs012_prot_dec_copy_write(data);
		return;

		case 0x001670:
			igs012_prot_mode_write(data);
		return;
	}

	switch (address & ~0x3f)
	{
		case 0x00d400:
			igs011_prot2--;
		return;

		case 0x00d440: {
			UINT8 x = igs011_prot2;
			igs011_prot2 = ((BIT(x,3)&BIT(x,0))<<4) | (BIT(x,2)<<3) | ((BIT(x,0)|BIT(x,1))<<2) | ((BIT(x,2)^BIT(x,4)^1)<<1) | (BIT(x,1)^1^BIT(x,3));
		}
		return;

		case 0x00d480:
			igs011_prot2 = 0;
		return;
	}

	if ((address & 0xfff000) == 0x902000) {
		igs012_prot = 0;
		igs012_prot_swap = 0;
		igs012_prot_mode = 0;
		return;
	}

	switch (address)
	{
		case 0xa40001:
			dips_select = data;
		return;

		case 0x700005:
			s_blitter.pen_hi = data & 0x07;
		return;

		case 0x800001:
			igs003_reg = data;
		return;

		case 0x800003:
			vbowl_igs003_write(data);
		return;
	}

	bprintf (0, _T("MWB: %5.5x, %2.2x\n"), address, data);
}

static UINT16 __fastcall vbowl_main_read_word(UINT32 address)
{
	switch (address & ~0x1c00f)
	{
		case 0x001610:
		case 0x001660:
		{
			UINT8 x = igs012_prot;

			UINT8 b1 = (BIT(x,3) | BIT(x,1))^1;
			UINT8 b0 = BIT(x,3) ^ BIT(x,0);

			return (b1 << 1) | (b0 << 0);
		}
	}

	switch (address & ~0x3f)
	{
		case 0x00d4c0:
			if (igs011_prot2_read)
				return igs011_prot2_read();
			return 0;
	}

	if (address < 0x80000) {
		return *((UINT16*)(Drv68KROM + address));
	}

	switch (address & ~0x1ff)
	{
		case 0x50f600:
		{
		//	return vbowlhk_igs011_prot2_read();
			return vbowl_igs011_prot2_read();
		}
	}

	switch (address)
	{
		case 0x500000:
		case 0x520000:
			return DrvInputs[0]; // coin

		case 0x600000:
			return ics2115read(0);
		case 0x600002:
			return ics2115read(1);
		case 0x600004:
			return ics2115read(2) | (ics2115read(3) << 8);
		case 0x600006:
			return 0;


		case 0x700000:
		case 0x700002:
			return trackball[(address / 2) & 1];

		case 0x800002:
			return vbowl_igs003_read();

		case 0xa80000:
			return ~0;

		case 0xa88000:
			return dips_read(4);

		case 0xa90000:
			return ~0;

		case 0xa98000:
			return ~0;
	}

//	bprintf (0, _T("MRW: %5.5x\n"), address);
	
	return 0;
}

static UINT8 __fastcall vbowl_main_read_byte(UINT32 address)
{

	if ((address & 0xffffff8) == igs011_prot1_addr+8) {
		return igs011_prot1_read();
	}

	switch (address)
	{
		case 0x500001:
		case 0x520001:
			return DrvInputs[0]; // coin

		case 0x800003:
			return vbowl_igs003_read();

		case 0xa88001:
			return dips_read(4);
	}

	if (address < 0x80000)
	{
		return vbowl_main_read_word(address & ~1) >> ((~address & 1) * 8);
	}
	
//	bprintf (0, _T("MRB: %5.5x\n"), address);

	return 0;
}

static void __fastcall vbowlhk_main_write_word(UINT32 address, UINT16 data)
{
	switch (address)
	{
		case 0x800002:
			vbowlhk_igs003_write(data);
		return;
	}

	vbowl_main_write_word(address, data);
}

static UINT16 __fastcall vbowlhk_main_read_word(UINT32 address)
{
	switch (address & ~0x1ff)
	{
		case 0x50f600:
			return vbowlhk_igs011_prot2_read();
	}

	return vbowl_main_read_word(address);
}


static void __fastcall drgnwrld_main_write_word(UINT32 address, UINT16 data)
{
	switch (address)
	{
		case 0x600000:
			MSM6295Write(0, data & 0xff);
		return;

		case 0x700000:
		case 0x700002:
			BurnYM3812Write(0, (address / 2) & 1, data & 0xff);
		return;
	}

	vbowl_main_write_word(address, data);
}

static void __fastcall drgnwrld_main_write_byte(UINT32 address, UINT8 data)
{
	switch (address)
	{
		case 0x600000:
		case 0x600001:
			MSM6295Write(0, data & 0xff);
		return;

		case 0x700000:
		case 0x700001:
		case 0x700002:
		case 0x700003:
			BurnYM3812Write(0, (address / 2) & 1, data & 0xff);
		return;
	}

	vbowl_main_write_byte(address, data);
}

static UINT16 __fastcall drgnwrld_main_read_word(UINT32 address)
{
	switch (address)
	{
		case 0x600000:
			return MSM6295Read(0);

		case 0xa88000:
			return dips_read(3);
	}

	return vbowl_main_read_word(address);
}

static UINT8 __fastcall drgnwrld_main_read_byte(UINT32 address)
{
	switch (address)
	{
		case 0x600000:
		case 0x600001:
			return MSM6295Read(0);

		case 0xa88000:
		case 0xa88001:
			return dips_read(3);
	}

	return vbowl_main_read_byte(address);
}

static UINT8 read_word_low_byte(UINT16 word, UINT32 address)
{
	return word >> ((~address & 1) * 8);
}

static void __fastcall lhb_main_write_word(UINT32 address, UINT16 data)
{
	if (address == 0x010000) {
		oki_bank_write(BIT(data, 1));
		return;
	}

	if ((address & ~0x01ff) == 0x010200) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x010400) {
		lhb_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x600000:
			MSM6295Write(0, data & 0xff);
		return;

		case 0x700002:
			lhb_inputs_write(data);
		return;

		case 0x820000:
			priority = data;
		return;

		case 0x838000:
			lhb_irq_enable = data;
		return;

		case 0x840000:
			dips_select = data;
		return;

		case 0x850000:
			igs011_prot_addr_write(data);
		return;
	}
}

static void __fastcall lhb_main_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xffffff8) == igs011_prot1_addr) {
		igs011_prot1_write((address & 6) / 2, data);
		return;
	}

	if ((address & ~0x01ff) == 0x010200) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x010400) {
		lhb_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x010000:
		case 0x010001:
			oki_bank_write(BIT(data, 1));
		return;

		case 0x600001:
			MSM6295Write(0, data);
		return;

		case 0x700002:
		case 0x700003:
			lhb_inputs_write(data);
		return;

		case 0x820001:
			priority = data;
		return;

		case 0x838001:
			lhb_irq_enable = data;
		return;

		case 0x840001:
			dips_select = data;
		return;
	}
}

static UINT16 __fastcall lhb_main_read_word(UINT32 address)
{
	if ((address & ~0x01ff) == 0x010600) {
		return igs011_prot2_read ? igs011_prot2_read() : 0;
	}

	if (address < 0x80000) {
		return *((UINT16*)(Drv68KROM + address));
	}

	switch (address)
	{
		case 0x600000:
			return MSM6295Read(0);

		case 0x700000:
			return lhb_coin_read();

		case 0x700002:
			return lhb_inputs_read(0);

		case 0x700004:
			return lhb_inputs_read(1);

		case 0x888000:
			return dips_read(5);
	}

	return 0;
}

static UINT8 __fastcall lhb_main_read_byte(UINT32 address)
{
	if ((address & 0xffffff8) == igs011_prot1_addr + 8) {
		return igs011_prot1_read();
	}

	switch (address)
	{
		case 0x600001:
			return MSM6295Read(0);

		case 0x700001:
			return lhb_coin_read();

		case 0x700003:
			return lhb_inputs_read(0);

		case 0x700005:
			return lhb_inputs_read(1);

		case 0x888001:
			return dips_read(5);
	}

	if (address < 0x80000) {
		return read_word_low_byte(lhb_main_read_word(address & ~1), address);
	}

	return 0;
}

static void __fastcall xymg_main_write_word(UINT32 address, UINT16 data)
{
	if (address == 0x010000) {
		oki_bank_write(BIT(data, 1));
		return;
	}

	if ((address & ~0x01ff) == 0x010200) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x010400) {
		lhb_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x600000:
			MSM6295Write(0, data & 0xff);
		return;

		case 0x700000:
			igs003_reg_write(data);
		return;

		case 0x700002:
			xymg_igs003_write(data);
		return;

		case 0x820000:
			priority = data;
		return;

		case 0x840000:
			dips_select = data;
		return;

		case 0x850000:
			igs011_prot_addr_write(data);
		return;
	}
}

static void __fastcall xymg_main_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xffffff8) == igs011_prot1_addr) {
		igs011_prot1_write((address & 6) / 2, data);
		return;
	}

	if ((address & ~0x01ff) == 0x010200) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x010400) {
		lhb_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x010000:
		case 0x010001:
			oki_bank_write(BIT(data, 1));
		return;

		case 0x600001:
			MSM6295Write(0, data);
		return;

		case 0x700001:
			igs003_reg_write(data);
		return;

		case 0x700003:
			xymg_igs003_write(data);
		return;

		case 0x820001:
			priority = data;
		return;

		case 0x840001:
			dips_select = data;
		return;
	}
}

static UINT16 __fastcall xymg_main_read_word(UINT32 address)
{
	if ((address & ~0x01ff) == 0x010600) {
		return igs011_prot2_read ? igs011_prot2_read() : 0;
	}

	if (address < 0x80000) {
		return *((UINT16*)(Drv68KROM + address));
	}

	switch (address)
	{
		case 0x600000:
			return MSM6295Read(0);

		case 0x700002:
			return xymg_igs003_read();

		case 0x888000:
			return dips_read(3);
	}

	return 0;
}

static UINT8 __fastcall xymg_main_read_byte(UINT32 address)
{
	if ((address & 0xffffff8) == igs011_prot1_addr + 8) {
		return igs011_prot1_read();
	}

	switch (address)
	{
		case 0x600001:
			return MSM6295Read(0);

		case 0x700003:
			return xymg_igs003_read();

		case 0x888001:
			return dips_read(3);
	}

	if (address < 0x80000) {
		return read_word_low_byte(xymg_main_read_word(address & ~1), address);
	}

	return 0;
}

static UINT16 xymga_coin_read()
{
	return xymg_coin_read();
}

static void __fastcall xymga_main_write_word(UINT32 address, UINT16 data)
{
	if (address == 0x010000) {
		oki_bank_write(BIT(data, 1));
		return;
	}

	if ((address & ~0x01ff) == 0x010200) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x010400) {
		lhb_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x600000:
			MSM6295Write(0, data & 0xff);
		return;

		case 0x700002:
			lhb_inputs_write(data);
		return;

		case 0x820000:
			priority = data;
		return;

		case 0x840000:
			dips_select = data;
		return;

		case 0x850000:
			igs011_prot_addr_write(data);
		return;
	}
}

static void __fastcall xymga_main_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xffffff8) == igs011_prot1_addr) {
		igs011_prot1_write((address & 6) / 2, data);
		return;
	}

	if ((address & ~0x01ff) == 0x010200) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x010400) {
		lhb_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x010000:
		case 0x010001:
			oki_bank_write(BIT(data, 1));
		return;

		case 0x600001:
			MSM6295Write(0, data);
		return;

		case 0x700002:
		case 0x700003:
			lhb_inputs_write(data);
		return;

		case 0x820001:
			priority = data;
		return;

		case 0x840001:
			dips_select = data;
		return;
	}
}

static UINT16 __fastcall xymga_main_read_word(UINT32 address)
{
	if ((address & ~0x01ff) == 0x010600) {
		return igs011_prot2_read ? igs011_prot2_read() : 0;
	}

	if (address < 0x80000) {
		return *((UINT16*)(Drv68KROM + address));
	}

	switch (address)
	{
		case 0x600000:
			return MSM6295Read(0);

		case 0x700000:
			return xymga_coin_read();

		case 0x700002:
			return lhb_inputs_read(0);

		case 0x700004:
			return lhb_inputs_read(1);

		case 0x888000:
			return dips_read(3);
	}

	return 0;
}

static UINT8 __fastcall xymga_main_read_byte(UINT32 address)
{
	if ((address & 0xffffff8) == igs011_prot1_addr + 8) {
		return igs011_prot1_read();
	}

	switch (address)
	{
		case 0x600001:
			return MSM6295Read(0);

		case 0x700001:
			return xymga_coin_read();

		case 0x700003:
			return lhb_inputs_read(0);

		case 0x700005:
			return lhb_inputs_read(1);

		case 0x888001:
			return dips_read(3);
	}

	if (address < 0x80000) {
		return read_word_low_byte(xymga_main_read_word(address & ~1), address);
	}

	return 0;
}

static void __fastcall wlcc_main_write_word(UINT32 address, UINT16 data)
{
	if ((address & ~0x01ff) == 0x518000) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x518200) {
		wlcc_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x600000:
			MSM6295Write(0, data & 0xff);
		return;

		case 0x800000:
			igs003_reg_write(data);
		return;

		case 0x800002:
			wlcc_igs003_write(data);
		return;

		case 0xa20000:
			priority = data;
		return;

		case 0xa40000:
			dips_select = data;
		return;

		case 0xa50000:
			igs011_prot_addr_write(data);
		return;
	}
}

static void __fastcall wlcc_main_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xffffff8) == igs011_prot1_addr) {
		igs011_prot1_write((address & 6) / 2, data);
		return;
	}

	if ((address & ~0x01ff) == 0x518000) {
		igs011_prot2_inc();
		return;
	}

	if ((address & ~0x01ff) == 0x518200) {
		wlcc_igs011_prot2_swap();
		return;
	}

	switch (address)
	{
		case 0x600001:
			MSM6295Write(0, data);
		return;

		case 0x800001:
			igs003_reg_write(data);
		return;

		case 0x800003:
			wlcc_igs003_write(data);
		return;

		case 0xa20001:
			priority = data;
		return;

		case 0xa40001:
			dips_select = data;
		return;
	}
}

static UINT16 __fastcall wlcc_main_read_word(UINT32 address)
{
	if (address >= 0x519000 && address <= 0x5195ff) {
		return igs011_prot2_read ? igs011_prot2_read() : 0;
	}

	if (address >= 0x518800 && address <= 0x5189ff) {
		return igs011_prot2_reset_read();
	}

	if (address < 0x80000) {
		return *((UINT16*)(Drv68KROM + address));
	}

	switch (address)
	{
		case 0x520000:
			return wlcc_coin_read();

		case 0x600000:
			return MSM6295Read(0);

		case 0x800002:
			return wlcc_igs003_read();

		case 0xa88000:
			return dips_read(4);
	}

	return 0;
}

static UINT8 __fastcall wlcc_main_read_byte(UINT32 address)
{
	if ((address & 0xffffff8) == igs011_prot1_addr + 8) {
		return igs011_prot1_read();
	}

	switch (address)
	{
		case 0x520001:
			return wlcc_coin_read();

		case 0x600001:
			return MSM6295Read(0);

		case 0x800003:
			return wlcc_igs003_read();

		case 0xa88001:
			return dips_read(4);
	}

	if (address < 0x80000 || (address >= 0x518800 && address <= 0x5195ff)) {
		return read_word_low_byte(wlcc_main_read_word(address & ~1), address);
	}

	return 0;
}

static void __fastcall lhb2_main_write_word(UINT32 address, UINT16 data)
{
	if (address >= lhb2_prot_base && address < (lhb2_prot_base + 0x0200)) {
		igs011_prot2_inc();
		return;
	}

	if (address >= (lhb2_prot_base + 0x0200) && address < (lhb2_prot_base + 0x0400)) {
		lhb_igs011_prot2_swap();
		return;
	}

	if (address >= (lhb2_prot_base + 0x0600) && address < (lhb2_prot_base + 0x0800)) {
		igs011_prot2_reset();
		return;
	}

	switch (address)
	{
		case 0x200000:
			MSM6295Write(0, data & 0xff);
		return;

		case 0x204000:
		case 0x204002:
			BurnYM2413Write((address / 2) & 1, data & 0xff);
		return;

		case 0x208000:
			igs003_reg_write(data);
		return;

		case 0x208002:
			lhb2_igs003_write(data);
		return;

		case 0xa20000:
			priority = data;
		return;

		case 0xa38000:
			if (lhb2_has_irq_enable) {
				lhb_irq_enable = data;
				return;
			}
		break;

		case 0xa40000:
			dips_select = data;
		return;

		case 0xa50000:
			igs011_prot_addr_write(data);
		return;
	}
}

static void __fastcall lhb2_main_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xffffff8) == igs011_prot1_addr) {
		igs011_prot1_write((address & 6) / 2, data);
		return;
	}

	if (address >= lhb2_prot_base && address < (lhb2_prot_base + 0x0200)) {
		igs011_prot2_inc();
		return;
	}

	if (address >= (lhb2_prot_base + 0x0200) && address < (lhb2_prot_base + 0x0400)) {
		lhb_igs011_prot2_swap();
		return;
	}

	if (address >= (lhb2_prot_base + 0x0600) && address < (lhb2_prot_base + 0x0800)) {
		igs011_prot2_reset();
		return;
	}

	switch (address)
	{
		case 0x200001:
			MSM6295Write(0, data);
		return;

		case 0x204001:
		case 0x204003:
			BurnYM2413Write((address / 2) & 1, data);
		return;

		case 0x208001:
			igs003_reg_write(data);
		return;

		case 0x208003:
			lhb2_igs003_write(data);
		return;

		case 0xa20001:
			priority = data;
		return;

		case 0xa38001:
			if (lhb2_has_irq_enable) {
				lhb_irq_enable = data;
				return;
			}
		break;

		case 0xa40001:
			dips_select = data;
		return;
	}
}

static UINT16 __fastcall lhb2_main_read_word(UINT32 address)
{
	if (address >= (lhb2_prot_base + 0x0400) && address < (lhb2_prot_base + 0x0600)) {
		return igs011_prot2_read ? igs011_prot2_read() : 0;
	}

	if (address < 0x80000) {
		return *((UINT16*)(Drv68KROM + address));
	}

	switch (address)
	{
		case 0x200000:
			return MSM6295Read(0);

		case 0x208002:
			return lhb2_igs003_read_cb ? lhb2_igs003_read_cb() : 0;

		case 0x214000:
			return lhb2_coin_read_cb ? lhb2_coin_read_cb() : 0;

		case 0xa88000:
			return dips_read(3);
	}

	return 0;
}

static UINT8 __fastcall lhb2_main_read_byte(UINT32 address)
{
	if ((address & 0xffffff8) == igs011_prot1_addr + 8) {
		return igs011_prot1_read();
	}

	switch (address)
	{
		case 0x200001:
			return MSM6295Read(0);

		case 0x208003:
			return lhb2_igs003_read_cb ? lhb2_igs003_read_cb() : 0;

		case 0x214001:
			return lhb2_coin_read_cb ? lhb2_coin_read_cb() : 0;

		case 0xa88001:
			return dips_read(3);
	}

	if (address < 0x80000 || (address >= (lhb2_prot_base + 0x0400) && address < (lhb2_prot_base + 0x0600))) {
		return read_word_low_byte(lhb2_main_read_word(address & ~1), address);
	}

	return 0;
}

static void vbowl_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x4100) == 0x0100)
			x ^= 0x0200;

		if ((i & 0x4000) == 0x4000 && (i & 0x0300) != 0x0100)
			x ^= 0x0200;

		if ((i & 0x5700) == 0x5100)
			x ^= 0x0200;

		if ((i & 0x5500) == 0x1000)
			x ^= 0x0200;

		if ((i & 0x0140) != 0x0000 || (i & 0x0012) == 0x0012)
			x ^= 0x0004;

		if ((i & 0x2004) != 0x2004 || (i & 0x0090) == 0x0000)
			x ^= 0x0020;

		src[i] = x;
	}
}

static void vbowlhk_decrypt()
{
	vbowl_decrypt();

	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0xd700) == 0x0100)
			x ^= 0x0200;

		if ((i & 0xc700) == 0x0500)
			x ^= 0x0200;

		if ((i & 0xd500) == 0x4000)
			x ^= 0x0200;

		if ((i & 0xc500) == 0x4400)
			x ^= 0x0200;

		if ((i & 0xd100) == 0x8000)
			x ^= 0x0200;

		if ((i & 0xd700) == 0x9500)
			x ^= 0x0200;

		if ((i & 0xd300) == 0xc100)
			x ^= 0x0200;

		if ((i & 0xd500) == 0xd400)
			x ^= 0x0200;

		src[i] = x;
	}
}

static void drgnwrld_type1_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x2000) == 0x0000 || (i & 0x0004) == 0x0000 || (i & 0x0090) == 0x0000)
			x ^= 0x0004;

		if ((i & 0x0100) == 0x0100 || (i & 0x0040) == 0x0040 || (i & 0x0012) == 0x0012)
			x ^= 0x0020;

		if ((x & 0x0024) == 0x0004 || (x & 0x0024) == 0x0020)
			x ^= 0x0024;

		src[i] = x;
	}
}

static void drgnwrld_type2_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000/2; i++)
	{
		UINT16 x = src[i];

		if (((i & 0x000090) == 0x000000) || ((i & 0x002004) != 0x002004))
			x ^= 0x0004;

		if ((((i & 0x000050) == 0x000000) || ((i & 0x000142) != 0x000000)) && ((i & 0x000150) != 0x000000))
			x ^= 0x0020;

		if (((i & 0x004280) == 0x004000) || ((i & 0x004080) == 0x000000))
			x ^= 0x0200;

		if ((i & 0x0011a0) != 0x001000)
			x ^= 0x0200;

		if ((i & 0x000180) == 0x000100)
			x ^= 0x0200;

		if ((x & 0x0024) == 0x0020 || (x & 0x0024) == 0x0004)
			x ^= 0x0024;

		src[i] = x;
	}
}

static void drgnwrld_type3_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x2000) == 0x0000 || (i & 0x0004) == 0x0000 || (i & 0x0090) == 0x0000)
			x ^= 0x0004;

		if ((i & 0x0100) == 0x0100 || (i & 0x0040) == 0x0040 || (i & 0x0012) == 0x0012)
			x ^= 0x0020;

		if ((((i & 0x1000) == 0x1000) ^ ((i & 0x0100) == 0x0100))
			|| (i & 0x0880) == 0x0800 || (i & 0x0240) == 0x0240)
				x ^= 0x0200;

		if ((x & 0x0024) == 0x0004 || (x & 0x0024) == 0x0020)
			x ^= 0x0024;

		src[i] = x;
	}
}

static void drgnwrldv40k_decrypt()
{
	drgnwrld_type1_decrypt();

	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x0800) != 0x0800)
			x ^= 0x0200;

		if (((i & 0x3a00) == 0x0a00) ^ ((i & 0x3a00) == 0x2a00))
			x ^= 0x0200;

		if (((i & 0x3ae0) == 0x0860) ^ ((i & 0x3ae0) == 0x2860))
			x ^= 0x0200;

		if (((i & 0x1c00) == 0x1800) ^ ((i & 0x1e00) == 0x1e00))
			x ^= 0x0200;

		if ((i & 0x1ee0) == 0x1c60)
			x ^= 0x0200;

		src[i] = x;
	}
}

static void wlcc_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x2000) == 0x0000 || (i & 0x0004) == 0x0000 || (i & 0x0090) == 0x0000)
			x ^= 0x0004;
		if ((i & 0x0100) == 0x0100 || (i & 0x0040) == 0x0040 || (i & 0x0012) == 0x0012)
			x ^= 0x0020;
		if ((i & 0x2400) == 0x0000 || (i & 0x4100) == 0x4100 || ((i & 0x2000) == 0x2000 && (i & 0x0c00) != 0x0000))
			x ^= 0x0200;
		if ((x & 0x0024) == 0x0004 || (x & 0x0024) == 0x0020)
			x ^= 0x0024;

		src[i] = x;
	}
    
	src[0x16b96/2] = 0x6000; // 016B96: 6700 02FE    beq 16e96  (fills palette with red otherwise)
	src[0x16e5e/2] = 0x6036; // 016E5E: 6736         beq 16e96  (fills palette with green otherwise)
	src[0x17852/2] = 0x6000; // 017852: 6700 01F2    beq 17a46  (fills palette with green otherwise)
	src[0x17a0e/2] = 0x6036; // 017A0E: 6736         beq 17a46  (fills palette with red otherwise)
	src[0x23636/2] = 0x6036; // 023636: 6736         beq 2366e  (fills palette with red otherwise)
	src[0x2b1e6/2] = 0x6000; // 02B1E6: 6700 0218    beq 2b400  (fills palette with green otherwise)
	src[0x2f9f2/2] = 0x6000; // 02F9F2: 6700 04CA    beq 2febe  (fills palette with green otherwise)
	src[0x2fb2e/2] = 0x6000; // 02FB2E: 6700 038E    beq 2febe  (fills palette with red otherwise)
	src[0x2fcf2/2] = 0x6000; // 02FCF2: 6700 01CA    beq 2febe  (fills palette with red otherwise)
	src[0x2fe86/2] = 0x6036; // 02FE86: 6736         beq 2febe  (fills palette with red otherwise)
	src[0x3016e/2] = 0x6000; // 03016E: 6700 03F6    beq 30566  (fills palette with green otherwise)
	src[0x303c8/2] = 0x6000; // 0303C8: 6700 019C    beq 30566  (fills palette with green otherwise)
	src[0x3052e/2] = 0x6036; // 03052E: 6736         beq 30566  (fills palette with green otherwise)
}

static void lhb_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x1100) != 0x0100)
			x ^= 0x0200;

		if ((i & 0x0150) != 0x0000 && (i & 0x0152) != 0x0010)
			x ^= 0x0004;

		if ((i & 0x2084) != 0x2084 && (i & 0x2094) != 0x2014)
			x ^= 0x0020;

		src[i] = x;
	}
}

static void xymga_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x2300) == 0x2100 || ((i & 0x2000) == 0x0000 && ((i & 0x0300) != 0x0200) && ((i & 0x0300) != 0x0100)))
			x ^= 0x0200;

		if (!(i & 0x0004) || !(i & 0x2000) || (!(i & 0x0080) && !(i & 0x0010)))
			x ^= 0x0020;

		if ((i & 0x0100) || (i & 0x0040) || ((i & 0x0010) && (i & 0x0002)))
			x ^= 0x0004;

		src[i] = x;
	}
}

static void xymg_apply_rom_patches()
{
	UINT16 *rom = (UINT16*)Drv68KROM;

	// Latest MAME marks these protection-sensitive branches as palette corruption points.
	rom[0x00502/2] = 0x6006;
	rom[0x0fc1c/2] = 0x6036;
	rom[0x1232a/2] = 0x6036;
	rom[0x18244/2] = 0x6036;
	rom[0x1e15e/2] = 0x6036;
	rom[0x22286/2] = 0x6000;
	rom[0x298ce/2] = 0x6036;
	rom[0x2e07c/2] = 0x6036;
	rom[0x38f1c/2] = 0x6000;
	rom[0x390e8/2] = 0x6000;
	rom[0x3933a/2] = 0x6000;
	rom[0x3955c/2] = 0x6000;
	rom[0x397f4/2] = 0x6000;
	rom[0x39976/2] = 0x6000;
	rom[0x39a7e/2] = 0x6036;
	rom[0x4342c/2] = 0x4e75;
	rom[0x49966/2] = 0x6036;
	rom[0x58140/2] = 0x6036;
	rom[0x5e05a/2] = 0x6036;
	rom[0x5ebf0/2] = 0x6000;
	rom[0x5edc2/2] = 0x6036;
	rom[0x5f71c/2] = 0x6000;
	rom[0x5f8d8/2] = 0x6036;
	rom[0x64836/2] = 0x6036;
}

static void dbc_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];

		if (i & (0x1000 / 2)) {
			if (~i & (0x400 / 2)) x ^= 0x0200;
		}

		if (i & (0x4000 / 2)) {
			if (i & (0x100 / 2)) {
				if (~i & (0x08 / 2)) x ^= 0x0020;
			} else {
				if (~i & (0x28 / 2)) x ^= 0x0020;
			}
		} else {
			x ^= 0x0020;
		}

		if (i & (0x200 / 2)) {
			x ^= 0x0004;
		} else {
			if ((i & (0x80 / 2)) == (0x80 / 2) || (i & (0x24 / 2)) == (0x24 / 2))
				x ^= 0x0004;
		}

		src[i] = x;
	}
}

static void ryukobou_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x00100) && (i & 0x00400))
			x ^= 0x0200;

		if (!(i & 0x00004) || !(i & 0x02000) || (!(i & 0x00080) && !(i & 0x00010)))
			x ^= 0x0020;

		if ((i & 0x00100) || (i & 0x00040) || ((i & 0x00010) && (i & 0x00002)))
			x ^= 0x0004;

		src[i] = x;
	}
}

static void lhb2_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;
	UINT16 *tmp = (UINT16*)BurnMalloc(0x80000);

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x0054) != 0x0000 && (i & 0x0056) != 0x0010)
			x ^= 0x0004;

		if ((i & 0x0204) == 0x0000)
			x ^= 0x0008;

		if ((i & 0x3080) != 0x3080 && (i & 0x3090) != 0x3010)
			x ^= 0x0020;

		tmp[BITSWAP24(i, 23,22,21,20,19,18,17,16,15,14,13,8,11,10,9,2,7,6,5,4,3,12,1,0)] = x;
	}

	memcpy(src, tmp, 0x80000);
	BurnFree(tmp);
}

static void lhb3_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;
	UINT16 *tmp = (UINT16*)BurnMalloc(0x80000);

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];
		INT32 j = BITSWAP24(i, 23,22,21,20,19,18,17,16,15,14,13,8,11,10,9,2,7,6,5,4,3,12,1,0);

		if ((j & 0x0100) || (j & 0x0040) || ((j & 0x0010) && (j & 0x0002)))
			x ^= 0x0004;

		if ((j & 0x5000) == 0x1000)
			x ^= 0x0008;

		if (!(j & 0x0004) || !(j & 0x2000) || (!(j & 0x0080) && !(j & 0x0010)))
			x ^= 0x0020;

		tmp[j] = x;
	}

	memcpy(src, tmp, 0x80000);
	BurnFree(tmp);
}

static void nkishusp_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;
	UINT16 *tmp = (UINT16*)BurnMalloc(0x80000);

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];
		INT32 j = BITSWAP24(i, 23,22,21,20,19,18,17,16,15,14,13,8,11,10,9,2,7,6,5,4,3,12,1,0);

		if (!(j & 0x00004) || !(j & 0x02000) || (!(j & 0x00080) && !(j & 0x00010)))
			x ^= 0x0020;

		if ((j & 0x00100) || (j & 0x00040) || ((j & 0x00010) && (j & 0x00002)))
			x ^= 0x0004;

		if (!(j & 0x4000) && (j & 0x1000) && (j & 0x00200))
			x ^= 0x0008;

		tmp[j] = x;
	}

	memcpy(src, tmp, 0x80000);
	BurnFree(tmp);
}

static void tygn_decrypt()
{
	UINT16 *src = (UINT16*)Drv68KROM;
	UINT16 *tmp = (UINT16*)BurnMalloc(0x80000);

	for (INT32 i = 0; i < 0x80000 / 2; i++)
	{
		UINT16 x = src[i];
		INT32 j = BITSWAP24(i, 23,22,21,20,19,18,17,0,2,9,5,12,14,11,8,6,3,4,16,15,13,10,7,1);

		if ((j & (0x0280 / 2)) || ((j & (0x0024 / 2)) == (0x0024 / 2)))
			x ^= 0x0004;

		if (j & (0x4000 / 2)) {
			if ((((j & (0x100 / 2)) == (0x000 / 2)) ^ ((j & (0x128 / 2)) == (0x028 / 2))) || ((j & (0x108 / 2)) == (0x100 / 2)))
				x ^= 0x0020;
		} else {
			x ^= 0x0020;
		}

		if (!(j & (0x8000 / 2)) && (j & (0x2000 / 2)))
			x ^= 0x0008;

		tmp[j] = x;
	}

	memcpy(src, tmp, 0x80000);
	BurnFree(tmp);
}

static void lhb2_gfx_decrypt()
{
	UINT8 *src = DrvGfxROM[0];
	UINT8 *tmp = (UINT8*)BurnMalloc(0x200000);

	for (INT32 i = 0; i < 0x200000; i++) {
		tmp[i] = src[BITSWAP24(i, 23,22,21,20,19,17,16,15,13,12,10,9,8,7,6,5,4,2,1,3,11,14,18,0)];
	}

	memcpy(src, tmp, 0x200000);
	BurnFree(tmp);
}

static void ics2115_sound_irq(INT32 )
{

}

static INT32 DrvDoReset()
{
	memset (AllRam, 0, RamEnd - AllRam);

	SekOpen(0);
	SekReset();

	switch (sound_system)
	{
		case SOUND_ICS2115:
			ics2115_reset();
		break;

		case SOUND_YM3812_OKI:
			BurnYM3812Reset();
			MSM6295Reset(0);
			MSM6295SetBank(0, DrvSndROM + 0x00000, 0, 0x3ffff);
		break;

		case SOUND_OKI:
			MSM6295Reset(0);
			MSM6295SetBank(0, DrvSndROM + 0x00000, 0, 0x3ffff);
		break;

		case SOUND_YM2413_OKI:
			BurnYM2413Reset();
			MSM6295Reset(0);
			MSM6295SetBank(0, DrvSndROM + 0x00000, 0, 0x3ffff);
		break;
	}

	SekClose();

	igs012_prot = 0;
	igs012_prot_swap = 0;
	igs012_prot_mode = 0;

	igs011_prot2 = 0;

	igs011_prot1 = 0;
	igs011_prot1_swap = 0;
	igs011_prot1_addr = 0;

	igs003_reg = 0;
	igs003_prot_hold = 0;
	igs003_prot_z = 0;
	igs003_prot_y = 0;
	igs003_prot_x = 0;
	igs003_prot_h1 = 0;
	igs003_prot_h2 = 0;

	priority = 0;

//	memset ((void*)s_blitter, 0, sizeof(blitter_t));
	s_blitter.x = s_blitter.y = s_blitter.w = s_blitter.h = 0;
	s_blitter.gfx_lo = s_blitter.gfx_hi = 0;
	s_blitter.depth = 0;
	s_blitter.pen = 0;
	s_blitter.flags = 0;
	s_blitter.pen_hi = 0;

	dips_select = 0;
	input_select = 0;
	lhb_irq_enable = 0;
	hopper_motor = 0;
	DrvCoin1Pulse = 0;
	DrvCoin1Prev = 0;
	trackball[0] = trackball[1] = 0;

	return 0;
}

static UINT16 igs003_magic_read()
{
	switch (igs003_reg)
	{
		case 0x20: return 0x49;
		case 0x21: return 0x47;
		case 0x22: return 0x53;

		case 0x24: return 0x41;
		case 0x25: return 0x41;
		case 0x26: return 0x7f;
		case 0x27: return 0x41;
		case 0x28: return 0x41;

		case 0x2a: return 0x3e;
		case 0x2b: return 0x41;
		case 0x2c: return 0x49;
		case 0x2d: return 0xf9;
		case 0x2e: return 0x0a;

		case 0x30: return 0x26;
		case 0x31: return 0x49;
		case 0x32: return 0x49;
		case 0x33: return 0x49;
		case 0x34: return 0x32;
	}

	return 0;
}

static inline void oki_bank_write(INT32 bank)
{
	MSM6295SetBank(0, DrvSndROM + ((bank & 1) * 0x40000), 0, 0x3ffff);
}

static UINT16 lhb_coin_read()
{
	UINT16 ret = 0xff;

	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x08;

	if (DrvDips[3] & 0x40) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x10;
	}

	if (DrvDips[3] & 0x20) {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x20;
	} else {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x20;
	}

	return ret;
}

static UINT16 lhb2_coin_read()
{
	UINT16 ret = 0xff;

	if (DrvDips[1] & 0x10) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x01;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x01;
	}

	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x04;
	if (hopper_line() == 0) ret &= ~0x08;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x10;

	if (DrvDips[1] & 0x20) {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x20;
	} else {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x20;
	}

	return ret;
}

static UINT16 nkishusp_coin_read()
{
	UINT16 ret = 0xff;

	if (DrvDips[1] & 0x08) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x01;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x01;
	}

	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x04;
	if (hopper_line() == 0) ret &= ~0x08;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x20;

	return ret;
}

static UINT16 xymg_coin_read()
{
	UINT16 ret = 0xff;

	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x08;

	if (DrvDips[0] & 0x20) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x10;
	}

	if (DrvDips[0] & 0x40) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x20;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x20;
	}

	return ret;
}

static UINT16 wlcc_coin_read()
{
	UINT16 ret = 0xff;

	if (DrvDips[2] & 0x20) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x01;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x01;
	}

	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x08;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x10;

	if (DrvDips[2] & 0x40) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x40;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x40;
	}

	if (hopper_line() == 0) ret &= ~0x80;

	return ret;
}

static UINT16 tygn_coin_read()
{
	UINT16 ret = 0xff;

	if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x08;
	if (hopper_line() == 0) ret &= ~0x80;

	return ret;
}

static inline void lhb_inputs_write(UINT16 data)
{
	input_select = data & 0x00ff;
	hopper_motor = BIT(data, 7);
}

static UINT16 lhb_inputs_read(INT32 offset)
{
	switch (offset)
	{
		case 0: return input_select;
		case 1: return key_matrix_read();
	}

	return 0;
}

static inline void igs003_reg_write(UINT16 data)
{
	igs003_reg = data;
}

static void lhb2_igs003_write(UINT16 data)
{
	switch (igs003_reg)
	{
		case 0x00:
			input_select = data & 0x00ff;
			hopper_motor = BIT(data, 7);
		return;

		case 0x02:
			s_blitter.pen_hi = data & 0x07;
			oki_bank_write(BIT(data, 3));
		return;

		case 0x40:
			igs003_prot_h2 = igs003_prot_h1;
			igs003_prot_h1 = data;
		return;

		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		return;

		case 0x48:
			igs003_prot_x = 0;
			if ((igs003_prot_h2 & 0x0a) != 0x0a) igs003_prot_x |= 0x08;
			if ((igs003_prot_h2 & 0x90) != 0x90) igs003_prot_x |= 0x04;
			if ((igs003_prot_h1 & 0x06) != 0x06) igs003_prot_x |= 0x02;
			if ((igs003_prot_h1 & 0x90) != 0x90) igs003_prot_x |= 0x01;
		return;

		case 0x50:
			igs003_prot_hold = 0;
		return;

		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		{
			UINT16 old = igs003_prot_hold;
			igs003_prot_y = igs003_reg & 0x07;
			igs003_prot_z = data;
			igs003_prot_hold <<= 1;
			igs003_prot_hold |= BIT(old, 15);
			igs003_prot_hold ^= 0x2bad;
			igs003_prot_hold ^= BIT(old, 12);
			igs003_prot_hold ^= BIT(old,  8);
			igs003_prot_hold ^= BIT(old,  3);
			igs003_prot_hold ^= BIT(igs003_prot_z, igs003_prot_y);
			igs003_prot_hold ^= BIT(igs003_prot_x, 0) <<  4;
			igs003_prot_hold ^= BIT(igs003_prot_x, 1) <<  6;
			igs003_prot_hold ^= BIT(igs003_prot_x, 2) << 10;
			igs003_prot_hold ^= BIT(igs003_prot_x, 3) << 12;
		}
		return;
	}
}

static UINT16 lhb2_igs003_read()
{
	switch (igs003_reg)
	{
		case 0x01:
			return key_matrix_read();

		case 0x03:
			return BITSWAP16(igs003_prot_hold, 14,11,8,6,4,3,1,0,5,2,9,7,10,13,12,15) & 0xff;
	}

	return igs003_magic_read();
}

static void wlcc_igs003_write(UINT16 data)
{
	switch (igs003_reg)
	{
		case 0x02:
			oki_bank_write(BIT(data, 4));
			hopper_motor = BIT(data, 5);
		return;
	}
}

static UINT16 wlcc_igs003_read()
{
	if (igs003_reg == 0x00) return DrvInputs[1];

	return igs003_magic_read();
}

static void xymg_igs003_write(UINT16 data)
{
	switch (igs003_reg)
	{
		case 0x01:
			input_select = data & 0x00ff;
			hopper_motor = BIT(data, 7);
		return;
	}
}

static UINT16 xymg_igs003_read()
{
	switch (igs003_reg)
	{
		case 0x00:
			return xymg_coin_read();

		case 0x02:
			return key_matrix_read();
	}

	return igs003_magic_read();
}

static UINT16 tygn_igs003_read()
{
	switch (igs003_reg)
	{
		case 0x00: return DrvInputs[1];
		case 0x01: return DrvInputs[2];
		case 0x02: return DrvInputs[3];
	}

	return igs003_magic_read();
}

static INT32 MemIndex(UINT32 gfxlen0, UINT32 gfxlen1)
{
	UINT8 *Next; Next = AllMem;

	Drv68KROM		= Next; Next += 0x080000;

	DrvGfxROM[0]	= Next; Next += gfxlen0 * 2;
	if (gfxlen1) {
		DrvGfxROM[1]= Next; Next += gfxlen1;
	}

	MSM6295ROM		= Next;
	DrvSndROM		= Next; Next += 0x400000;

	DrvPalette		= (UINT32*)Next; Next += 0x0800 * sizeof(UINT32);

	AllRam			= Next;

	DrvNVRAM		= Next; Next += 0x004000;
	DrvExtraRAM		= Next; Next += 0x004000;
	DrvPalRAM		= Next; Next += 0x002000;
	DrvPrioRAM		= Next; Next += 0x001000;
	DrvLayerRAM[0]	= Next; Next += 0x020000;
	DrvLayerRAM[1]	= Next; Next += 0x020000;
	DrvLayerRAM[2]	= Next; Next += 0x020000;
	DrvLayerRAM[3]	= Next; Next += 0x020000;

	RamEnd		= Next;

	MemEnd		= Next;

	return 0;
}

static INT32 AllocDrvMem(UINT32 gfxlen0, UINT32 gfxlen1)
{
	AllMem = NULL;
	MemIndex(gfxlen0, gfxlen1);

	INT32 nLen = MemEnd - (UINT8 *)0;
	AllMem = (UINT8 *)BurnMalloc(nLen);
	if (AllMem == NULL) return 1;

	memset(AllMem, 0, nLen);
	MemIndex(gfxlen0, gfxlen1);

	return 0;
}

static INT32 VbowlInit()
{
	AllMem = NULL;
	MemIndex(0x400000, 0x100000);
	INT32 nLen = MemEnd - (UINT8 *)0;
	if ((AllMem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex(0x400000, 0x100000);

	{
		if (BurnLoadRom(Drv68KROM + 0x000000, 0, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM[0] + 0x00000, 1, 1)) return 1;
		if (BurnLoadRomExt(DrvGfxROM[1] + 0x00000, 2, 1, LD_INVERT)) return 1;
		
		if (BurnLoadRom(DrvSndROM + 0x00000, 3, 1)) return 1;
		if (BurnLoadRom(DrvSndROM + 0x80000, 4, 1)) return 1;

		vbowl_decrypt();
		BurnNibbleExpand(DrvGfxROM[0], NULL, 0x400000, 1, 0x00);

		*((UINT16*)(Drv68KROM + 0x080e0)) = 0xe549; // patch bad opcode
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x07ffff, MAP_FETCH); // read in handler
	SekMapMemory(DrvNVRAM,		0x100000, 0x103fff, MAP_RAM);
	SekMapMemory(DrvPrioRAM,	0x200000, 0x200fff, MAP_RAM);
	// SekMapHandler(1,			0x300000, 0x3fffff, MAP_RAM);
	SekMapHandler(2,			0x400000, 0x401fff, MAP_RAM);
	SekMapHandler(3,			0xa58000, 0xa5ffff, MAP_WRITE);
	SekSetWriteWordHandler(0,	vbowl_main_write_word);
	SekSetWriteByteHandler(0,	vbowl_main_write_byte);
	SekSetReadWordHandler(0,	vbowl_main_read_word);
	SekSetReadByteHandler(0,	vbowl_main_read_byte);
	// SekSetWriteWordHandler(1,	igs011_layerram_write_word);
	// SekSetWriteByteHandler(1,	igs011_layerram_write_byte);
	// SekSetReadWordHandler(1,	igs011_layerram_read_word);
	// SekSetReadByteHandler(1,	igs011_layerram_read_byte);
	SekSetWriteWordHandler(2,	igs011_palette_write_word);
	SekSetWriteByteHandler(2,	igs011_palette_write_byte);
	SekSetReadWordHandler(2,	igs011_palette_read_word);
	SekSetReadByteHandler(2,	igs011_palette_read_byte);
	SekSetWriteWordHandler(3,	igs011_blit_write_word);
	SekSetWriteByteHandler(3, 	igs011_blit_write_byte);
	SekClose();

	igs011_prot2_read = drgnwrldv20j_igs011_prot2_read;

	ics2115_init(ics2115_sound_irq, DrvSndROM, 0x100000);
	BurnTimerAttachSek(7333333);
	sound_system = SOUND_ICS2115;
	timer_irq = 3;
	timer_pulses = 4;
	timer_irq_gated = 0;
	vblank_irq_gated = 0;
	has_hopper = 0;
	
	GenericTilesInit();
	DrvGfxROMLen[0] = 0x800000;
	DrvGfxROMLen[1] = 0x100000;

	DrvDoReset();

	return 0;
}

static INT32 VbowlhkInit()
{
	AllMem = NULL;
	MemIndex(0x400000, 0x100000);
	INT32 nLen = MemEnd - (UINT8 *)0;
	if ((AllMem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex(0x400000, 0x100000);

	{
		if (BurnLoadRom(Drv68KROM + 0x000000, 0, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM[0] + 0x00000, 1, 1)) return 1;
		if (BurnLoadRomExt(DrvGfxROM[1] + 0x00000, 2, 1, LD_INVERT)) return 1;
		
		if (BurnLoadRom(DrvSndROM + 0x00000, 3, 1)) return 1;
		if (BurnLoadRom(DrvSndROM + 0x80000, 4, 1)) return 1;

		vbowlhk_decrypt();
		BurnNibbleExpand(DrvGfxROM[0], NULL, 0x400000, 1, 0x00);
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x07ffff, MAP_FETCH); // read in handler
	SekMapMemory(DrvNVRAM,		0x100000, 0x103fff, MAP_RAM);
	SekMapMemory(DrvPrioRAM,	0x200000, 0x200fff, MAP_RAM);
	// SekMapHandler(1,			0x300000, 0x3fffff, MAP_RAM);
	SekMapHandler(2,			0x400000, 0x401fff, MAP_RAM);
	SekMapHandler(3,			0xa58000, 0xa5ffff, MAP_WRITE);
	SekSetWriteWordHandler(0,	vbowlhk_main_write_word);
	SekSetWriteByteHandler(0,	vbowl_main_write_byte);
	SekSetReadWordHandler(0,	vbowlhk_main_read_word);
	SekSetReadByteHandler(0,	vbowl_main_read_byte);
	// SekSetWriteWordHandler(1,	igs011_layerram_write_word);
	// SekSetWriteByteHandler(1,	igs011_layerram_write_byte);
	// SekSetReadWordHandler(1,	igs011_layerram_read_word);
	// SekSetReadByteHandler(1,	igs011_layerram_read_byte);
	SekSetWriteWordHandler(2,	igs011_palette_write_word);
	SekSetWriteByteHandler(2,	igs011_palette_write_byte);
	SekSetReadWordHandler(2,	igs011_palette_read_word);
	SekSetReadByteHandler(2,	igs011_palette_read_byte);
	SekSetWriteWordHandler(3,	igs011_blit_write_word);
	SekSetWriteByteHandler(3, 	igs011_blit_write_byte);
	SekClose();

	igs011_prot2_read = drgnwrldv20j_igs011_prot2_read;

	ics2115_init(ics2115_sound_irq, DrvSndROM, 0x100000);
	BurnTimerAttachSek(7333333);
	sound_system = SOUND_ICS2115;
	timer_irq = 3;
	timer_pulses = 4;
	timer_irq_gated = 0;
	vblank_irq_gated = 0;
	has_hopper = 0;
	
	GenericTilesInit();
	DrvGfxROMLen[0] = 0x800000;
	DrvGfxROMLen[1] = 0x100000;

	DrvDoReset();

	return 0;
}

static void drgnwrld_gfx_decrypt()
{
	UINT8 *tmp = (UINT8*)BurnMalloc(0x400000);

	memcpy (tmp, DrvGfxROM[0], 0x400000);

	for (INT32 i = 0; i < 0x400000; i++)
	{
		DrvGfxROM[0][i] = tmp[(i & ~0x5000) | ((i & 0x1000) << 2) | ((i & 0x4000) >> 2)];
	}

	BurnFree (tmp);
}

static INT32 DrgnwrldCommonInit(void (*decrypt_cb)(), UINT16 (*igs011_prot2_cb)())
{
	AllMem = NULL;
	MemIndex(0x400000, 0);
	INT32 nLen = MemEnd - (UINT8 *)0;
	if ((AllMem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex(0x400000, 0);

	{
		INT32 k = 0;
		struct BurnRomInfo ri;

		if (BurnLoadRom(Drv68KROM + 0x000000, k++, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM[0] + 0x00000, k++, 1)) return 1;
		BurnDrvGetRomInfo(&ri, k);
		if (ri.nType & BRF_GRA) {
			if (BurnLoadRom(DrvGfxROM[0] + 0x400000, k++, 1)) return 1;
		}

		if (BurnLoadRom(DrvSndROM + 0x00000, k++, 1)) return 1;

		if (decrypt_cb) decrypt_cb();
		drgnwrld_gfx_decrypt();
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x07ffff, MAP_FETCH); // read in handler
	SekMapMemory(DrvNVRAM,		0x100000, 0x103fff, MAP_RAM);
	SekMapMemory(DrvPrioRAM,	0x200000, 0x200fff, MAP_RAM);
//	SekMapHandler(1,			0x300000, 0x3fffff, MAP_RAM);
	SekMapHandler(2,			0x400000, 0x401fff, MAP_RAM);
	SekMapHandler(3,			0xa58000, 0xa5ffff, MAP_WRITE);
	SekSetWriteWordHandler(0,	drgnwrld_main_write_word);
	SekSetWriteByteHandler(0,	drgnwrld_main_write_byte);
	SekSetReadWordHandler(0,	drgnwrld_main_read_word);
	SekSetReadByteHandler(0,	drgnwrld_main_read_byte);
//	SekSetWriteWordHandler(1,	igs011_layerram_write_word);
//	SekSetWriteByteHandler(1,	igs011_layerram_write_byte);
//	SekSetReadWordHandler(1,	igs011_layerram_read_word);
//	SekSetReadByteHandler(1,	igs011_layerram_read_byte);
	SekSetWriteWordHandler(2,	igs011_palette_write_word);
	SekSetWriteByteHandler(2,	igs011_palette_write_byte);
	SekSetReadWordHandler(2,	igs011_palette_read_word);
	SekSetReadByteHandler(2,	igs011_palette_read_byte);
	SekSetWriteWordHandler(3,	igs011_blit_write_word);
	SekSetWriteByteHandler(3, 	igs011_blit_write_byte);
	SekClose();

	igs011_prot2_read = igs011_prot2_cb;

	BurnYM3812Init(1, 3579545, NULL, 0);
	BurnTimerAttachSek(7333333);
	BurnYM3812SetRoute(0, BURN_SND_YM3812_ROUTE, 1.90, BURN_SND_ROUTE_BOTH);

	MSM6295Init(0, 1047619 / 132, 1);
	MSM6295SetRoute(0, 0.95, BURN_SND_ROUTE_BOTH);
	
	sound_system = SOUND_YM3812_OKI;
	timer_irq = 5;
	timer_pulses = 4;
	timer_irq_gated = 0;
	vblank_irq_gated = 0;
	has_hopper = 0;

	GenericTilesInit();
	DrvGfxROMLen[0] = 0x400000;
	DrvGfxROMLen[1] = 0;

	DrvDoReset();

	return 0;
}

static INT32 OkiOnlyCommonInit(UINT32 gfxlen0, UINT32 second_gfx_offset, UINT32 blit_base, INT32 use_extra_ram, void (*decrypt_cb)(), UINT16 (*prot2_cb)(), igs011_write_word_handler write_word, igs011_write_byte_handler write_byte, igs011_read_word_handler read_word, igs011_read_byte_handler read_byte, INT32 irq, INT32 pulses, INT32 irq_gate, INT32 vblank_gate)
{
	if (AllocDrvMem(gfxlen0, 0)) return 1;

	INT32 k = 0;
	struct BurnRomInfo ri;

	if (BurnLoadRom(Drv68KROM + 0x000000, k++, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x000000, k++, 1)) return 1;

	BurnDrvGetRomInfo(&ri, k);
	if (ri.nType & BRF_GRA) {
		if (BurnLoadRom(DrvGfxROM[0] + second_gfx_offset, k++, 1)) return 1;
	}

	if (BurnLoadRom(DrvSndROM + 0x000000, k++, 1)) return 1;

	if (decrypt_cb) decrypt_cb();

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM, 0x000000, 0x07ffff, MAP_FETCH);
	if (use_extra_ram) {
		SekMapMemory(DrvExtraRAM, 0x100000, 0x103fff, MAP_RAM);
		SekMapMemory(DrvNVRAM, 0x1f0000, 0x1f3fff, MAP_RAM);
	} else {
		SekMapMemory(DrvNVRAM, 0x100000, 0x103fff, MAP_RAM);
	}
	SekMapMemory(DrvPrioRAM, 0x200000, 0x200fff, MAP_RAM);
	// SekMapHandler(1, 0x300000, 0x3fffff, MAP_RAM);
	SekMapHandler(2, 0x400000, 0x401fff, MAP_RAM);
	SekMapHandler(3, blit_base, blit_base + 0x7fff, MAP_WRITE);
	SekSetWriteWordHandler(0, write_word);
	SekSetWriteByteHandler(0, write_byte);
	SekSetReadWordHandler(0, read_word);
	SekSetReadByteHandler(0, read_byte);
	// SekSetWriteWordHandler(1, igs011_layerram_write_word);
	// SekSetWriteByteHandler(1, igs011_layerram_write_byte);
	// SekSetReadWordHandler(1, igs011_layerram_read_word);
	// SekSetReadByteHandler(1, igs011_layerram_read_byte);
	SekSetWriteWordHandler(2, igs011_palette_write_word);
	SekSetWriteByteHandler(2, igs011_palette_write_byte);
	SekSetReadWordHandler(2, igs011_palette_read_word);
	SekSetReadByteHandler(2, igs011_palette_read_byte);
	SekSetWriteWordHandler(3, igs011_blit_write_word);
	SekSetWriteByteHandler(3, igs011_blit_write_byte);
	SekClose();

	MSM6295Init(0, 1047619 / 132, 0);
	MSM6295SetRoute(0, 1.00, BURN_SND_ROUTE_BOTH);

	igs011_prot2_read = prot2_cb;
	sound_system = SOUND_OKI;
	timer_irq = irq;
	timer_pulses = pulses;
	timer_irq_gated = irq_gate;
	vblank_irq_gated = vblank_gate;
	has_hopper = 1;

	GenericTilesInit();
	DrvGfxROMLen[0] = gfxlen0;
	DrvGfxROMLen[1] = 0;

	DrvDoReset();
	return 0;
}

static INT32 Ym2413OkiInitHardware(UINT32 gfxlen0, UINT32 gfxlen1, UINT32 prio_base, UINT32 palette_base, igs011_write_word_handler write_word, igs011_write_byte_handler write_byte, igs011_read_word_handler read_word, igs011_read_byte_handler read_byte, INT32 irq, INT32 pulses)
{
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM, 0x000000, 0x07ffff, MAP_FETCH);
	SekMapMemory(DrvNVRAM, 0x100000, 0x103fff, MAP_RAM);
	SekMapMemory(DrvPrioRAM, prio_base, prio_base + 0x0fff, MAP_RAM);
	// SekMapHandler(1, 0x300000, 0x3fffff, MAP_RAM);
	SekMapHandler(2, palette_base, palette_base + 0x1fff, MAP_RAM);
	SekMapHandler(3, 0xa58000, 0xa5ffff, MAP_WRITE);
	SekSetWriteWordHandler(0, write_word);
	SekSetWriteByteHandler(0, write_byte);
	SekSetReadWordHandler(0, read_word);
	SekSetReadByteHandler(0, read_byte);
	// SekSetWriteWordHandler(1, igs011_layerram_write_word);
	// SekSetWriteByteHandler(1, igs011_layerram_write_byte);
	// SekSetReadWordHandler(1, igs011_layerram_read_word);
	// SekSetReadByteHandler(1, igs011_layerram_read_byte);
	SekSetWriteWordHandler(2, igs011_palette_write_word);
	SekSetWriteByteHandler(2, igs011_palette_write_byte);
	SekSetReadWordHandler(2, igs011_palette_read_word);
	SekSetReadByteHandler(2, igs011_palette_read_byte);
	SekSetWriteWordHandler(3, igs011_blit_write_word);
	SekSetWriteByteHandler(3, igs011_blit_write_byte);
	SekClose();

	BurnYM2413Init(3579545);
	BurnYM2413SetAllRoutes(1.90, BURN_SND_ROUTE_BOTH);
	MSM6295Init(0, 1047619 / 132, 1);
	MSM6295SetRoute(0, 0.95, BURN_SND_ROUTE_BOTH);

	sound_system = SOUND_YM2413_OKI;
	timer_irq = irq;
	timer_pulses = pulses;
	timer_irq_gated = 0;
	vblank_irq_gated = 0;
	has_hopper = 1;

	GenericTilesInit();
	DrvGfxROMLen[0] = gfxlen0;
	DrvGfxROMLen[1] = gfxlen1;

	DrvDoReset();
	return 0;
}

static INT32 DrvExit()
{
	GenericTilesExit();

	SekExit();

	switch (sound_system)
	{
		case SOUND_ICS2115:
			ics2115_exit();
		break;

		case SOUND_YM3812_OKI:
			BurnYM3812Exit();
			MSM6295Exit(0);
		break;

		case SOUND_OKI:
			MSM6295Exit(0);
		break;

		case SOUND_YM2413_OKI:
			BurnYM2413Exit();
			MSM6295Exit(0);
		break;
	}

	timer_irq = 3;
	timer_pulses = 4;
	timer_irq_gated = 0;
	vblank_irq_gated = 0;
	has_hopper = 0;
	lhb2_prot_base = 0x020000;
	lhb2_has_irq_enable = 0;
	lhb2_coin_read_cb = NULL;
	lhb2_igs003_read_cb = NULL;
	sound_system = SOUND_ICS2115;
	igs011_prot2_read = NULL;

	BurnFree (AllMem);

	return 0;
}

static void DrvPaletteUpdate()
{
	for (int i = 0; i < 0x1000/2; i++)
	{
		UINT16 p = (DrvPalRAM[0x800 + i] << 8) | (DrvPalRAM[i]);

		DrvPalette[i] = BurnHighCol(pal5bit(p >>  0), pal5bit(p >>  5), pal5bit(p >> 10), 0);
	}
}

static void screen_update()
{
	UINT16 *priority_ram = (UINT16*)DrvPrioRAM;
	UINT16 const *const pri_ram = &priority_ram[(priority & 7) * 512/2];
	unsigned hibpp_layers = 4 - (s_blitter.depth & 0x07);
	if (hibpp_layers > 4) hibpp_layers = 4;

	if (BIT(s_blitter.depth, 4))
	{
		UINT16 const pri = pri_ram[0xff] & 7;
		BurnTransferClear((pri << 8) | 0xff);
		return;
	}

	UINT8 layerpix[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	for (int y = 0; y < nScreenHeight; y++)
	{
		for (int x = 0; x < nScreenWidth; x++)
		{
			const int scr_addr = (y << 9) | x;
			int pri_addr = 0xff;

			int l = 0;
			unsigned i = 0;
			while (hibpp_layers > i)
			{
				layerpix[l] = DrvLayerRAM[i++][scr_addr];
				if (layerpix[l] != 0xff)
					pri_addr &= ~(1 << l);
				++l;
			}

			while (4 > i)
			{
				UINT8 const pixdata = DrvLayerRAM[i++][scr_addr];
				layerpix[l] = pixdata & 0x0f;
				if (layerpix[l] != 0x0f)
					pri_addr &= ~(1 << l);
				++l;
				layerpix[l] = (pixdata >> 4) & 0x0f;
				if (layerpix[l] != 0x0f)
					pri_addr &= ~(1 << l);
				++l;
			}

			UINT16 const pri = pri_ram[pri_addr] & 7;
			pTransDraw[y * nScreenWidth + x] = layerpix[pri] | (pri << 8);
		}
	}
}

static INT32 DrvDraw()
{
	DrvPaletteUpdate();

	screen_update();

	BurnTransferCopy(DrvPalette);

	return 0;
}

static INT32 DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	{
		memset (DrvInputs, 0xff, sizeof(DrvInputs));

		for (INT32 i = 0; i < 8; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
			DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
			DrvInputs[3] ^= (DrvJoy4[i] & 1) << i;
		}

		if (has_hopper) {
			if (DrvJoy1[0] && !DrvCoin1Prev) {
				DrvCoin1Pulse = 5;
			}
			DrvCoin1Prev = DrvJoy1[0] ? 1 : 0;

			DrvInputs[0] |= 0x01;
			if (DrvCoin1Pulse) {
				DrvInputs[0] &= ~0x01;
				DrvCoin1Pulse--;
			}
		}

		pack_mahjong_inputs();
	}

	INT32 nInterleave = 260;
	INT32 nCyclesTotal[1] = { 7333333 / 60 };
	INT32 nCyclesDone[1] = { 0 };

	SekNewFrame();
	
	SekOpen(0);

	for (INT32 i = 0; i < nInterleave; i++)
	{
		switch (sound_system)
		{
			case SOUND_ICS2115:
				CPU_RUN_TIMER(0);
			break;

			case SOUND_YM3812_OKI:
				CPU_RUN_TIMER(0);
			break;

			case SOUND_OKI:
			case SOUND_YM2413_OKI:
				CPU_RUN(0, Sek);
			break;
		}

		if (timer_pulses == 4) {
			if (i == 64 || i == 129 || i == 194 || i == 259) {
				if (!timer_irq_gated || lhb_irq_enable) {
					SekSetIRQLine(timer_irq, CPU_IRQSTATUS_AUTO);
				}
			}
		} else if (timer_pulses == 2) {
			if (i == 129 || i == 259) {
				if (!timer_irq_gated || lhb_irq_enable) {
					SekSetIRQLine(timer_irq, CPU_IRQSTATUS_AUTO);
				}
			}
		} else {
			if (i == 259) {
				if (!timer_irq_gated || lhb_irq_enable) {
					SekSetIRQLine(timer_irq, CPU_IRQSTATUS_AUTO);
				}
			}
		}

		if (i == 239) {
			if (!vblank_irq_gated || lhb_irq_enable) {
				SekSetIRQLine(6, CPU_IRQSTATUS_AUTO);
			}
		}
	}

	if (pBurnSoundOut) {
		switch (sound_system)
		{
			case SOUND_ICS2115:
				ics2115_update(nBurnSoundLen);
			break;

			case SOUND_YM3812_OKI:
				BurnYM3812Update(pBurnSoundOut, nBurnSoundLen);
				MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
			break;

			case SOUND_OKI:
				MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
			break;

			case SOUND_YM2413_OKI:
				BurnYM2413Render(pBurnSoundOut, nBurnSoundLen);
				MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
			break;
		}
	}

	SekClose();

	if (pBurnDraw) {
		BurnDrvRedraw();
	}

	return 0;
}

static INT32 DrvScan(INT32 nAction,INT32 *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin =  0x029702;
	}

	DrvRecalc = 1;

	if (nAction & ACB_MEMORY_ROM) {	
		ba.Data		= Drv68KROM;
		ba.nLen		= 0x80000;
		ba.nAddress	= 0;
		ba.szName	= "68K ROM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_MEMORY_RAM) {	
		ba.Data		= DrvLayerRAM[0];
		ba.nLen		= 0x020000;
		ba.nAddress	= 0x300000;
		ba.szName	= "Layer RAM 0";
		BurnAcb(&ba);
		
		ba.Data		= DrvLayerRAM[1];
		ba.nLen		= 0x020000;
		ba.nAddress	= 0x340000;
		ba.szName	= "Layer RAM 1";
		BurnAcb(&ba);

		ba.Data		= DrvLayerRAM[2];
		ba.nLen		= 0x020000;
		ba.nAddress	= 0x360000;
		ba.szName	= "Layer RAM 2";
		BurnAcb(&ba);
		
		ba.Data		= DrvLayerRAM[3];
		ba.nLen		= 0x020000;
		ba.nAddress	= 0x380000;
		ba.szName	= "Layer RAM 3";
		BurnAcb(&ba);
		
		ba.Data			= DrvPalRAM;
		ba.nLen			= 0x001000;
		ba.nAddress		= 0x400000;
		ba.szName		= "Palette Regs";
		BurnAcb(&ba);

		ba.Data			= DrvExtraRAM;
		ba.nLen			= 0x004000;
		ba.nAddress		= 0x1f0000;
		ba.szName		= "Extra RAM";
		BurnAcb(&ba);
		
		ba.Data			= DrvPrioRAM;
		ba.nLen			= 0x001000;
		ba.nAddress		= 0x200000;
		ba.szName		= "Priority RAM";
		BurnAcb(&ba);
		
		ba.Data			= DrvNVRAM;
		ba.nLen			= 0x004000;
		ba.nAddress		= 0x100000;
		ba.szName		= "NV/WORK RAM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA)
	{
		SekScan(nAction);

		switch (sound_system)
		{
			case SOUND_ICS2115:
				ics2115_scan(nAction, pnMin);
			break;

			case SOUND_YM3812_OKI:
				BurnYM3812Scan(nAction, pnMin);
				MSM6295Scan(nAction, pnMin);
			break;

			case SOUND_OKI:
				MSM6295Scan(nAction, pnMin);
			break;

			case SOUND_YM2413_OKI:
				BurnYM2413Scan(nAction, pnMin);
				MSM6295Scan(nAction, pnMin);
			break;
		}

		SCAN_VAR(igs012_prot);
		SCAN_VAR(igs012_prot_swap);
		SCAN_VAR(igs012_prot_mode);
		SCAN_VAR(igs011_prot2);
		SCAN_VAR(igs011_prot1);
		SCAN_VAR(igs011_prot1_swap);
		SCAN_VAR(igs011_prot1_addr);
		SCAN_VAR(igs003_reg);
		SCAN_VAR(igs003_prot_hold);
		SCAN_VAR(igs003_prot_z);
		SCAN_VAR(igs003_prot_y);
		SCAN_VAR(igs003_prot_x);
		SCAN_VAR(igs003_prot_h1);
		SCAN_VAR(igs003_prot_h2);
		SCAN_VAR(priority);
		SCAN_VAR(s_blitter);
		SCAN_VAR(dips_select);
		SCAN_VAR(input_select);
		SCAN_VAR(lhb_irq_enable);
		SCAN_VAR(hopper_motor);
		SCAN_VAR(DrvCoin1Pulse);
		SCAN_VAR(DrvCoin1Prev);
		SCAN_VAR(timer_pulses);
		SCAN_VAR(timer_irq_gated);
		SCAN_VAR(vblank_irq_gated);
		SCAN_VAR(has_hopper);
		SCAN_VAR(trackball);
	}

 	return 0;
}


// Virtua Bowling (World, V101XCM)

static struct BurnRomInfo vbowlRomDesc[] = {
	{ "bowlingv101xcm.u45",	0x080000, 0xab8e3f1f, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "vrbowlng.u69",		0x400000, 0xb0d339e8, 2 | BRF_GRA },           //  1 Blitter Data
	{ "vrbowlng.u68",		0x100000, 0xb0ce27e7, 3 | BRF_GRA },           //  2

	{ "vrbowlng.u67",		0x080000, 0x53000936, 4 | BRF_SND },           //  3 ICS2115 Samples
	{ "vrbowlng.u66",		0x080000, 0xf62cf8ed, 4 | BRF_SND },           //  4
};

STD_ROM_PICK(vbowl)
STD_ROM_FN(vbowl)

struct BurnDriver BurnDrvVbowl = {
	"vbowl", NULL, NULL, NULL, "1996",
	"Virtua Bowling (World, V101XCM)\0", "Sound doesn't work", "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_PREFIX_IGS, GBF_SPORTSMISC, 0,
	ROM_NAME(vbowl), INPUT_FN(Vbowl), DIP_FN(Vbowl),
	VbowlInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Virtua Bowling (Japan, V100JCM)

static struct BurnRomInfo vbowljRomDesc[] = {
	{ "vrbowlng.u45",		0x080000, 0x091c19c1, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "vrbowlng.u69",		0x400000, 0xb0d339e8, 2 | BRF_GRA },           //  1 Blitter Data
	{ "vrbowlng.u68",		0x100000, 0xb0ce27e7, 3 | BRF_GRA },           //  2

	{ "vrbowlng.u67",		0x080000, 0x53000936, 4 | BRF_SND },           //  3 ICS2115 Samples
	{ "vrbowlng.u66",		0x080000, 0xf62cf8ed, 4 | BRF_SND },           //  4
};

STD_ROM_PICK(vbowlj)
STD_ROM_FN(vbowlj)

struct BurnDriver BurnDrvVbowlj = {
	"vbowlj", "vbowl", NULL, NULL, "1996",
	"Virtua Bowling (Japan, V100JCM)\0", "Sound doesn't work", "IGS / Alta", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_SPORTSMISC, 0,
	ROM_NAME(vbowlj), INPUT_FN(Vbowl), DIP_FN(Vbowlj),
	VbowlInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Virtua Bowling (Hong Kong, V101HJS)

static struct BurnRomInfo vbowlhkRomDesc[] = {
	{ "bowlingv101hjs.u45",	0x080000, 0x92fbfa72, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "vrbowlng.u69",		0x400000, 0xb0d339e8, 2 | BRF_GRA },           //  1 Blitter Data
	{ "vrbowlng.u68",		0x100000, 0xb0ce27e7, 3 | BRF_GRA },           //  2

	{ "vrbowlng.u67",		0x080000, 0x53000936, 4 | BRF_SND },           //  3 ICS2115 Samples
	{ "vrbowlng.u66",		0x080000, 0xf62cf8ed, 4 | BRF_SND },           //  4
};

STD_ROM_PICK(vbowlhk)
STD_ROM_FN(vbowlhk)

struct BurnDriver BurnDrvVbowlhk = {
	"vbowlhk", "vbowl", NULL, NULL, "1996",
	"Virtua Bowling (Hong Kong, V101HJS)\0", "Sound doesn't work", "IGS / Tai Tin Amusement", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_SPORTSMISC, 0,
	ROM_NAME(vbowlhk), INPUT_FN(Vbowl), DIP_FN(Vbowlhk),
	VbowlhkInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Dragon World (World, V040O)

static struct BurnRomInfo drgnwrldRomDesc[] = {
	{ "chinadr-v0400.u3",	0x080000, 0xa6daa2b8, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  2 MSM6295 Samples
};

STD_ROM_PICK(drgnwrld)
STD_ROM_FN(drgnwrld)

static INT32 DrgnwrldInit()
{
	return DrgnwrldCommonInit(drgnwrld_type1_decrypt, drgnwrldv20j_igs011_prot2_read);
}

struct BurnDriver BurnDrvDrgnwrld = {
	"drgnwrld", NULL, NULL, NULL, "1997",
	"Dragon World (World, V040O)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrld), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrld),
	DrgnwrldInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Dongbang Jiju (Korea, V040K)

static struct BurnRomInfo drgnwrldv40kRomDesc[] = {
	{ "v-040k.u3",			0x080000, 0x397404ef, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  2 MSM6295 Samples

	{ "ccdu15.u15",			0x0002e5, 0xa15fce69, 0 | BRF_OPT },           //  3 PLDs
	{ "ccdu45.u45", 		0x0002e5, 0xa15fce69, 0 | BRF_OPT },           //  4
};

STD_ROM_PICK(drgnwrldv40k)
STD_ROM_FN(drgnwrldv40k)

static INT32 Drgnwrldv40kInit()
{
	return DrgnwrldCommonInit(drgnwrldv40k_decrypt, drgnwrldv40k_igs011_prot2_read);
}

struct BurnDriver BurnDrvDrgnwrldv40k = {
	"drgnwrldv40k", "drgnwrld", NULL, NULL, "1995",
	"Dongbang Jiju (Korea, V040K)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv40k), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrldc),
	Drgnwrldv40kInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Dragon World (World, V030O)

static struct BurnRomInfo drgnwrldv30RomDesc[] = {
	{ "chinadr-v0300.u3",	0x080000, 0x5ac243e5, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  2 MSM6295 Samples
};

STD_ROM_PICK(drgnwrldv30)
STD_ROM_FN(drgnwrldv30)

struct BurnDriver BurnDrvDrgnwrldv30 = {
	"drgnwrldv30", "drgnwrld", NULL, NULL, "1995",
	"Dragon World (World, V030O)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv30), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrld),
	DrgnwrldInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Dragon World (World, V021O)

static struct BurnRomInfo drgnwrldv21RomDesc[] = {
	{ "china-dr-v-0210.u3",	0x080000, 0x60c2b018, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  2 MSM6295 Samples
};

STD_ROM_PICK(drgnwrldv21)
STD_ROM_FN(drgnwrldv21)

static INT32 Drgnwrldv21Init()
{
	return DrgnwrldCommonInit(drgnwrld_type2_decrypt, drgnwrldv21_igs011_prot2_read);
}

struct BurnDriver BurnDrvDrgnwrldv21 = {
	"drgnwrldv21", "drgnwrld", NULL, NULL, "1995",
	"Dragon World (World, V021O)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv21), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrld),
	Drgnwrldv21Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Chuugokuryuu (Japan, V021J)

static struct BurnRomInfo drgnwrldv21jRomDesc[] = {
	{ "v-021j",				0x080000, 0x2f87f6e4, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data
	{ "cg",           		0x020000, 0x2dda0be3, 2 | BRF_GRA },           //  2

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  3 MSM6295 Samples
};

STD_ROM_PICK(drgnwrldv21j)
STD_ROM_FN(drgnwrldv21j)

static INT32 Drgnwrldv21jInit()
{
	return DrgnwrldCommonInit(drgnwrld_type3_decrypt, drgnwrldv20j_igs011_prot2_read);
}

struct BurnDriver BurnDrvDrgnwrldv21j = {
	"drgnwrldv21j", "drgnwrld", NULL, NULL, "1995",
	"Chuugokuryuu (Japan, V021J)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv21j), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrldj),
	Drgnwrldv21jInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Chuugokuryuu (Japan, V020J)

static struct BurnRomInfo drgnwrldv20jRomDesc[] = {
	{ "china_jp.v20",		0x080000, 0x9e018d1a, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data
	{ "china.u44",     		0x020000, 0x10549746, 2 | BRF_GRA },           //  2

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  3 MSM6295 Samples
};

STD_ROM_PICK(drgnwrldv20j)
STD_ROM_FN(drgnwrldv20j)

static INT32 Drgnwrldv20jInit()
{
	return DrgnwrldCommonInit(drgnwrld_type3_decrypt, drgnwrldv20j_igs011_prot2_read);
}

struct BurnDriver BurnDrvDrgnwrldv20j = {
	"drgnwrldv20j", "drgnwrld", NULL, NULL, "1995",
	"Chuugokuryuu (Japan, V020J)\0", NULL, "IGS / Alta", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv20j), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrldj),
	Drgnwrldv20jInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Dung Fong Zi Zyu (Hong Kong, V011H, set 1)

static struct BurnRomInfo drgnwrldv11hRomDesc[] = {
	{ "c_drgn_hk.u3",		0x080000, 0x182037ce, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  2 MSM6295 Samples
};

STD_ROM_PICK(drgnwrldv11h)
STD_ROM_FN(drgnwrldv11h)

static INT32 Drgnwrldv11hInit()
{
	return DrgnwrldCommonInit(drgnwrld_type1_decrypt, drgnwrldv20j_igs011_prot2_read);
}

struct BurnDriver BurnDrvDrgnwrldv11h = {
	"drgnwrldv11h", "drgnwrld", NULL, NULL, "1995",
	"Dung Fong Zi Zyu (Hong Kong, V011H, set 1)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv11h), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrldc),
	Drgnwrldv11hInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Dung Fong Zi Zyu (Hong Kong, V011H, set 2)

static struct BurnRomInfo drgnwrldv11haRomDesc[] = {
	{ "u3",			 		0x080000, 0xb68113c4, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data

	{ "china-dr-sp.u43",	0x040000, 0xfde63ce1, 4 | BRF_SND },           //  2 MSM6295 Samples
};

STD_ROM_PICK(drgnwrldv11ha)
STD_ROM_FN(drgnwrldv11ha)

struct BurnDriver BurnDrvDrgnwrldv11ha = {
	"drgnwrldv11ha", "drgnwrld", NULL, NULL, "1995",
	"Dung Fong Zi Zyu (Hong Kong, V011H, set 2)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv11ha), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrldc),
	Drgnwrldv40kInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};


// Zhongguo Long (China, V010C)

static struct BurnRomInfo drgnwrldv10cRomDesc[] = {
	{ "igs-d0303.u3",		0x080000, 0x3b3c29bb, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code

	{ "igs-d0301.u39",		0x400000, 0x78ab45d9, 2 | BRF_GRA },           //  1 Blitter Data

	{ "igs-s0302.u43",		0x040000, 0xfde63ce1, 4 | BRF_SND },           //  2 MSM6295 Samples
	
	{ "ccdu15.u15", 		0x0002e5, 0xa15fce69, 0 | BRF_OPT },           //  3 PLDs
	{ "ccdu45.u45", 		0x0002e5, 0xa15fce69, 0 | BRF_OPT },           //  4
};

STD_ROM_PICK(drgnwrldv10c)
STD_ROM_FN(drgnwrldv10c)

static INT32 Drgnwrldv10cInit()
{
	return DrgnwrldCommonInit(drgnwrld_type1_decrypt, drgnwrldv20j_igs011_prot2_read);
}

struct BurnDriver BurnDrvDrgnwrldv10c = {
	"drgnwrldv10c", "drgnwrld", NULL, NULL, "1995",
	"Zhongguo Long (China, V010C)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_PUZZLE, 0,
	ROM_NAME(drgnwrldv10c), INPUT_FN(Drgnwrld), DIP_FN(Drgnwrldc),
	Drgnwrldv10cInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

static INT32 LhbInit()
{
	return OkiOnlyCommonInit(0x200000, 0x200000, 0x858000, 0, lhb_decrypt, lhb_igs011_prot2_read, lhb_main_write_word, lhb_main_write_byte, lhb_main_read_word, lhb_main_read_byte, 5, 2, 1, 1);
}

static INT32 Lhbv33cInit()
{
	return OkiOnlyCommonInit(0x200000, 0x200000, 0x858000, 0, lhb_decrypt, lhb_igs011_prot2_read, lhb_main_write_word, lhb_main_write_byte, lhb_main_read_word, lhb_main_read_byte, 5, 2, 1, 1);
}

static INT32 DbcInit()
{
	return OkiOnlyCommonInit(0x280000, 0x200000, 0x858000, 0, dbc_decrypt, dbc_igs011_prot2_read, lhb_main_write_word, lhb_main_write_byte, lhb_main_read_word, lhb_main_read_byte, 5, 2, 1, 1);
}

static INT32 RyukobouInit()
{
	return OkiOnlyCommonInit(0x280000, 0x200000, 0x858000, 0, ryukobou_decrypt, ryukobou_igs011_prot2_read, lhb_main_write_word, lhb_main_write_byte, lhb_main_read_word, lhb_main_read_byte, 5, 2, 1, 1);
}

static INT32 XymgInit()
{
	INT32 ret = OkiOnlyCommonInit(0x280000, 0x200000, 0x858000, 1, lhb_decrypt, lhb_igs011_prot2_read, xymg_main_write_word, xymg_main_write_byte, xymg_main_read_word, xymg_main_read_byte, 3, 1, 0, 0);
	if (ret) return ret;

	xymg_apply_rom_patches();
	return 0;
}

static INT32 XymgaInit()
{
	return OkiOnlyCommonInit(0x280000, 0x200000, 0x858000, 1, xymga_decrypt, lhb_igs011_prot2_read, xymga_main_write_word, xymga_main_write_byte, xymga_main_read_word, xymga_main_read_byte, 3, 1, 0, 0);
}

static INT32 WlccInit()
{
	if (AllocDrvMem(0x280000, 0)) return 1;

	INT32 k = 0;
	struct BurnRomInfo ri;
	UINT8 *tmp68K = (UINT8*)BurnMalloc(0x100000);
	if (tmp68K == NULL) return 1;
	memset(tmp68K, 0, 0x100000);

	if (BurnLoadRom(tmp68K, k++, 1)) {
		BurnFree(tmp68K);
		return 1;
	}

	memcpy(Drv68KROM, tmp68K + 0x80000, 0x80000);
	BurnFree(tmp68K);

	if (BurnLoadRom(DrvGfxROM[0] + 0x000000, k++, 1)) return 1;

	BurnDrvGetRomInfo(&ri, k);
	if (ri.nType & BRF_GRA) {
		if (BurnLoadRom(DrvGfxROM[0] + 0x200000, k++, 1)) return 1;
	}

	if (BurnLoadRom(DrvSndROM + 0x000000, k++, 1)) return 1;

	wlcc_decrypt();

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM, 0x000000, 0x07ffff, MAP_FETCH);
	SekMapMemory(DrvNVRAM, 0x100000, 0x103fff, MAP_RAM);
	SekMapMemory(DrvPrioRAM, 0x200000, 0x200fff, MAP_RAM);
	SekMapHandler(2, 0x400000, 0x401fff, MAP_RAM);
	SekMapHandler(3, 0xa58000, 0xa5ffff, MAP_WRITE);
	SekSetWriteWordHandler(0, wlcc_main_write_word);
	SekSetWriteByteHandler(0, wlcc_main_write_byte);
	SekSetReadWordHandler(0, wlcc_main_read_word);
	SekSetReadByteHandler(0, wlcc_main_read_byte);
	SekSetWriteWordHandler(2, igs011_palette_write_word);
	SekSetWriteByteHandler(2, igs011_palette_write_byte);
	SekSetReadWordHandler(2, igs011_palette_read_word);
	SekSetReadByteHandler(2, igs011_palette_read_byte);
	SekSetWriteWordHandler(3, igs011_blit_write_word);
	SekSetWriteByteHandler(3, igs011_blit_write_byte);
	SekClose();

	MSM6295Init(0, 1047619 / 132, 0);
	MSM6295SetRoute(0, 1.00, BURN_SND_ROUTE_BOTH);

	igs011_prot2_read = lhb_igs011_prot2_read;
	sound_system = SOUND_OKI;
	timer_irq = 3;
	timer_pulses = 1;
	timer_irq_gated = 0;
	vblank_irq_gated = 0;
	has_hopper = 1;

	GenericTilesInit();
	DrvGfxROMLen[0] = 0x280000;
	DrvGfxROMLen[1] = 0;

	DrvDoReset();
	return 0;
}

static INT32 Lhb2Init()
{
	if (AllocDrvMem(0x200000, 0x080000)) return 1;

	if (BurnLoadRom(Drv68KROM + 0x000000, 0, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x000000, 1, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[1] + 0x000000, 2, 1)) return 1;
	if (BurnLoadRom(DrvSndROM + 0x000000, 4, 1)) return 1;

	lhb2_decrypt();
	lhb2_gfx_decrypt();

	igs011_prot2_read = lhb2_igs011_prot2_read;
	lhb2_prot_base = 0x020000;
	lhb2_has_irq_enable = 0;
	lhb2_coin_read_cb = lhb2_coin_read;
	lhb2_igs003_read_cb = lhb2_igs003_read;

	return Ym2413OkiInitHardware(0x200000, 0x080000, 0x20c000, 0x210000, lhb2_main_write_word, lhb2_main_write_byte, lhb2_main_read_word, lhb2_main_read_byte, 5, 4);
}

static INT32 Lhb2cpgsInit()
{
	UINT16 *rom;

	if (AllocDrvMem(0x200000, 0x080000)) return 1;

	if (BurnLoadRom(Drv68KROM + 0x000000, 0, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x000000, 1, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[1] + 0x000000, 2, 1)) return 1;
	if (BurnLoadRom(DrvSndROM + 0x000000, 3, 1)) return 1;

	lhb3_decrypt();
	lhb2_gfx_decrypt();

	rom = (UINT16*)Drv68KROM;
	rom[0x2807a / 2] = 0x6036;
	rom[0x2cd98 / 2] = 0x6036;
	rom[0x31444 / 2] = 0x6036;
	rom[0x32b7e / 2] = 0x6036;
	rom[0x34e88 / 2] = 0x6036;
	rom[0x3f144 / 2] = 0x6036;

	igs011_prot2_read = lhb2_igs011_prot2_read;
	lhb2_prot_base = 0x021000;
	lhb2_has_irq_enable = 1;
	lhb2_coin_read_cb = lhb2_coin_read;
	lhb2_igs003_read_cb = lhb2_igs003_read;

	return Ym2413OkiInitHardware(0x200000, 0x080000, 0x20c000, 0x210000, lhb2_main_write_word, lhb2_main_write_byte, lhb2_main_read_word, lhb2_main_read_byte, 5, 4);
}

static INT32 Lhb3Init()
{
	UINT16 *rom;

	if (AllocDrvMem(0x200000, 0x080000)) return 1;

	if (BurnLoadRom(Drv68KROM + 0x000000, 0, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x000000, 1, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[1] + 0x000000, 2, 1)) return 1;
	if (BurnLoadRom(DrvSndROM + 0x000000, 3, 1)) return 1;

	lhb3_decrypt();
	lhb2_gfx_decrypt();

	rom = (UINT16*)Drv68KROM;
	rom[0x034a6 / 2] = 0x6042;
	rom[0x1a236 / 2] = 0x6034;
	rom[0x2534a / 2] = 0x6036;
	rom[0x283c8 / 2] = 0x6038;
	rom[0x2a8d6 / 2] = 0x6036;
	rom[0x2f076 / 2] = 0x6036;
	rom[0x3093e / 2] = 0x6036;
	rom[0x3321e / 2] = 0x6036;
	rom[0x33b68 / 2] = 0x6038;
	rom[0x3e608 / 2] = 0x6034;
	rom[0x3fb66 / 2] = 0x6036;
	rom[0x42bee / 2] = 0x6034;
	rom[0x45724 / 2] = 0x6034;
	rom[0x465e0 / 2] = 0x6036;
	rom[0x48e26 / 2] = 0x6000;
	rom[0x49496 / 2] = 0x6036;
	rom[0x4b85a / 2] = 0x6038;

	igs011_prot2_read = lhb2_igs011_prot2_read;
	lhb2_prot_base = 0x021000;
	lhb2_has_irq_enable = 1;
	lhb2_coin_read_cb = lhb2_coin_read;
	lhb2_igs003_read_cb = lhb2_igs003_read;

	return Ym2413OkiInitHardware(0x200000, 0x080000, 0x20c000, 0x210000, lhb2_main_write_word, lhb2_main_write_byte, lhb2_main_read_word, lhb2_main_read_byte, 3, 4);
}

static INT32 NkishuspInit()
{
	UINT16 *rom;

	if (AllocDrvMem(0x800000, 0x200000)) return 1;
	memset(DrvGfxROM[0], 0xff, 0x800000);
	memset(DrvGfxROM[1], 0xff, 0x200000);

	if (BurnLoadRom(Drv68KROM + 0x000000, 0, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x000000, 1, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x400000, 2, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[1] + 0x000000, 3, 1)) return 1;
	if (BurnLoadRom(DrvSndROM + 0x000000, 4, 1)) return 1;

	nkishusp_decrypt();
	lhb2_gfx_decrypt();

	rom = (UINT16*)Drv68KROM;
	rom[0x03624 / 2] = 0x6042;
	rom[0x1a9d2 / 2] = 0x6034;
	rom[0x26306 / 2] = 0x6036;
	rom[0x29190 / 2] = 0x6038;
	rom[0x2b82a / 2] = 0x6036;
	rom[0x2ff20 / 2] = 0x6036;
	rom[0x3151c / 2] = 0x6036;
	rom[0x33dfc / 2] = 0x6036;
	rom[0x3460e / 2] = 0x6038;
	rom[0x3f09e / 2] = 0x6034;
	rom[0x406a8 / 2] = 0x6036;
	rom[0x4376a / 2] = 0x6034;
	rom[0x462d6 / 2] = 0x6034;
	rom[0x471ec / 2] = 0x6036;
	rom[0x49c46 / 2] = 0x6000;
	rom[0x4a2b6 / 2] = 0x6036;
	rom[0x4c67a / 2] = 0x6038;

	igs011_prot2_read = lhb2_igs011_prot2_read;
	lhb2_prot_base = 0x023000;
	lhb2_has_irq_enable = 1;
	lhb2_coin_read_cb = nkishusp_coin_read;
	lhb2_igs003_read_cb = lhb2_igs003_read;

	return Ym2413OkiInitHardware(0x800000, 0x200000, 0x20c000, 0x210000, lhb2_main_write_word, lhb2_main_write_byte, lhb2_main_read_word, lhb2_main_read_byte, 3, 4);
}

static INT32 TygnInit()
{
	UINT16 *rom;

	if (AllocDrvMem(0x800000, 0x200000)) return 1;
	memset(DrvGfxROM[0], 0xff, 0x800000);
	memset(DrvGfxROM[1], 0xff, 0x200000);

	if (BurnLoadRom(Drv68KROM + 0x000000, 0, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x000000, 1, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[1] + 0x000000, 2, 1)) return 1;
	if (BurnLoadRom(DrvSndROM + 0x000000, 3, 1)) return 1;

	tygn_decrypt();
	lhb2_gfx_decrypt();

	rom = (UINT16*)Drv68KROM;
	rom[0x036d8 / 2] = 0x6042;
	rom[0x1c6a0 / 2] = 0x6034;
	rom[0x2412a / 2] = 0x6036;
	rom[0x26de6 / 2] = 0x6038;
	rom[0x292cc / 2] = 0x6036;
	rom[0x2da36 / 2] = 0x6036;
	rom[0x2f2dc / 2] = 0x6036;
	rom[0x31bae / 2] = 0x6036;
	rom[0x3d2a6 / 2] = 0x6034;
	rom[0x3e824 / 2] = 0x6036;
	rom[0x418ae / 2] = 0x6034;
	rom[0x443ac / 2] = 0x6034;
	rom[0x45272 / 2] = 0x6036;
	rom[0x479b0 / 2] = 0x6000;
	rom[0x48020 / 2] = 0x6036;
	rom[0x4a3e4 / 2] = 0x6038;

	igs011_prot2_read = lhb2_igs011_prot2_read;
	lhb2_prot_base = 0x021000;
	lhb2_has_irq_enable = 1;
	lhb2_coin_read_cb = tygn_coin_read;
	lhb2_igs003_read_cb = tygn_igs003_read;

	return Ym2413OkiInitHardware(0x800000, 0x200000, 0x20c000, 0x210000, lhb2_main_write_word, lhb2_main_write_byte, lhb2_main_read_word, lhb2_main_read_byte, 3, 4);
}

static struct BurnRomInfo lhbRomDesc[] = {
	{ "v305j-409",       0x080000, 0x701de8ef, 1 | BRF_PRG | BRF_ESS },
	{ "m0201-ig.160",    0x200000, 0xec54452c, 2 | BRF_GRA },
	{ "m0202.snd",       0x100000, 0x220949aa, 4 | BRF_SND },
};
STD_ROM_PICK(lhb)
STD_ROM_FN(lhb)

static struct BurnRomInfo lhbv33cRomDesc[] = {
	{ "maj_v-033c.u30",  0x080000, 0x02a0b716, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m0201.u15",   0x200000, 0xec54452c, 2 | BRF_GRA },
	{ "igs_m0202.u39",   0x080000, 0x106ac5f7, 4 | BRF_SND },
};
STD_ROM_PICK(lhbv33c)
STD_ROM_FN(lhbv33c)

static struct BurnRomInfo dbcRomDesc[] = {
	{ "maj-h_v027h.u30", 0x080000, 0x5d5ccd5b, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m0201.u15",   0x200000, 0xec54452c, 2 | BRF_GRA },
	{ "maj-h_cg.u8",     0x080000, 0xee45cc46, 2 | BRF_GRA },
	{ "igs_m0202.u39",   0x080000, 0x106ac5f7, 4 | BRF_SND },
};
STD_ROM_PICK(dbc)
STD_ROM_FN(dbc)

static struct BurnRomInfo ryukobouRomDesc[] = {
	{ "maj_v030j.u30",   0x080000, 0x186f2b4e, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m0201.u15",   0x200000, 0xec54452c, 2 | BRF_GRA },
	{ "maj-j_cg.u8",     0x080000, 0x3c8de5d1, 2 | BRF_GRA },
	{ "maj-j_sp.u39",    0x080000, 0x82f670f3, 4 | BRF_SND },
};
STD_ROM_PICK(ryukobou)
STD_ROM_FN(ryukobou)

static struct BurnRomInfo lhb2RomDesc[] = {
	{ "maj2v185h.u29",   0x080000, 0x2572d59a, 1 | BRF_PRG | BRF_ESS },
	{ "igsm0501.u7",     0x200000, 0x1c952bd6, 2 | BRF_GRA },
	{ "igsm0502.u4",     0x080000, 0x5d73ae99, 3 | BRF_GRA },
	{ "igsm0502.u5",     0x080000, 0x5d73ae99, 3 | BRF_GRA },
	{ "igss0503.u38",    0x080000, 0xc9609c9c, 4 | BRF_SND },
};
STD_ROM_PICK(lhb2)
STD_ROM_FN(lhb2)

static struct BurnRomInfo lhb2cpgsRomDesc[] = {
	{ "v127c.u29",       0x080000, 0xd6025580, 1 | BRF_PRG | BRF_ESS },
	{ "igsm0501.u7",     0x200000, 0x1c952bd6, 2 | BRF_GRA },
	{ "igsm0502.u4",     0x080000, 0x5d73ae99, 3 | BRF_GRA },
	{ "igss0503.u38",    0x080000, 0xc9609c9c, 4 | BRF_SND },
};
STD_ROM_PICK(lhb2cpgs)
STD_ROM_FN(lhb2cpgs)

static struct BurnRomInfo lhb3RomDesc[] = {
	{ "rom.u29",         0x080000, 0xc5985452, 1 | BRF_PRG | BRF_ESS },
	{ "rom.u7",          0x200000, 0x1c952bd6, 2 | BRF_GRA },
	{ "rom.u6",          0x080000, 0x5d73ae99, 3 | BRF_GRA },
	{ "rom.u38",         0x080000, 0xc9609c9c, 4 | BRF_SND },
};
STD_ROM_PICK(lhb3)
STD_ROM_FN(lhb3)

static struct BurnRomInfo nkishuspRomDesc[] = {
	{ "v250j.u29",       0x080000, 0x500cb919, 1 | BRF_PRG | BRF_ESS },
	{ "m0501.u7",        0x200000, 0x1c952bd6, 2 | BRF_GRA },
	{ "cg.u4",           0x080000, 0xfe60f485, 2 | BRF_GRA },
	{ "m0502.u6",        0x080000, 0x5d73ae99, 3 | BRF_GRA },
	{ "sp.u38",          0x080000, 0xd80e28e2, 4 | BRF_SND },
};
STD_ROM_PICK(nkishusp)
STD_ROM_FN(nkishusp)

static struct BurnRomInfo tygnRomDesc[] = {
	{ "27c4096.u13",     0x080000, 0x25b1de1e, 1 | BRF_PRG | BRF_ESS },
	{ "m0501.u7",        0x200000, 0x1c952bd6, 2 | BRF_GRA },
	{ "m0502.u3",        0x080000, 0x5d73ae99, 3 | BRF_GRA },
	{ "s0502.u42",       0x080000, 0xc9609c9c, 4 | BRF_SND },
};
STD_ROM_PICK(tygn)
STD_ROM_FN(tygn)

static struct BurnRomInfo xymgRomDesc[] = {
	{ "u30-ebac.rom",    0x080000, 0x7d272b6f, 1 | BRF_PRG | BRF_ESS },
	{ "m0201-ig.160",    0x200000, 0xec54452c, 2 | BRF_GRA },
	{ "ygxy-u8.rom",     0x080000, 0x56a2706f, 2 | BRF_GRA },
	{ "m0202.snd",       0x100000, 0x220949aa, 4 | BRF_SND },
};
STD_ROM_PICK(xymg)
STD_ROM_FN(xymg)

static struct BurnRomInfo xymgaRomDesc[] = {
	{ "rom.u30",         0x080000, 0xecc871fb, 1 | BRF_PRG | BRF_ESS },
	{ "rom.u15",         0x200000, 0xec54452c, 2 | BRF_GRA },
	{ "igs_0203.u8",     0x080000, 0x56a2706f, 2 | BRF_GRA },
	{ "igs_s0202.u39",   0x080000, 0x106ac5f7, 4 | BRF_SND },
};
STD_ROM_PICK(xymga)
STD_ROM_FN(xymga)

static struct BurnRomInfo wlccRomDesc[] = {
	{ "wlcc4096.rom",    0x100000, 0x3b16729f, 1 | BRF_PRG | BRF_ESS },
	{ "m0201-ig.160",    0x200000, 0xec54452c, 2 | BRF_GRA },
	{ "wlcc.gfx",        0x080000, 0x1f7ad299, 2 | BRF_GRA },
	{ "040-c3c2.snd",    0x080000, 0x220949aa, 4 | BRF_SND },
};
STD_ROM_PICK(wlcc)
STD_ROM_FN(wlcc)

struct BurnDriver BurnDrvLhb = {
	"lhb", NULL, NULL, NULL, "1995",
	"Long Hu Bang (China, V035C)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhb), INPUT_FN(Lhb), DIP_FN(Lhb),
	LhbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhbv33c = {
	"lhbv33c", "lhb", NULL, NULL, "1995",
	"Long Hu Bang (China, V033C)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhbv33c), INPUT_FN(Lhb), DIP_FN(Lhb),
	Lhbv33cInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvDbc = {
	"dbc", "lhb", NULL, NULL, "1995",
	"Daai Baan Sing (Hong Kong, V027H)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(dbc), INPUT_FN(Lhb), DIP_FN(Lhb),
	DbcInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvRyukobou = {
	"ryukobou", "lhb", NULL, NULL, "1995",
	"Mahjong Ryukobou (Japan, V030J)\0", NULL, "IGS / Alta", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(ryukobou), INPUT_FN(Lhb), DIP_FN(Lhb),
	RyukobouInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhb2 = {
	"lhb2", NULL, NULL, NULL, "1996",
	"Lung Fu Bong II (Hong Kong, V185H)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhb2), INPUT_FN(Lhb2), DIP_FN(Lhb2),
	Lhb2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhb2cpgs = {
	"lhb2cpgs", "lhb2", NULL, NULL, "1996",
	"Long Hu Bang II: Cuo Pai Gaoshou (China, V127C)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhb2cpgs), INPUT_FN(Lhb2cpgs), DIP_FN(Lhb2cpgs),
	Lhb2cpgsInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvTygn = {
	"tygn", "lhb2", NULL, NULL, "1996",
	"Te Yi Gong Neng (China, V632C)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_CARD, 0,
	ROM_NAME(tygn), INPUT_FN(Tygn), DIP_FN(Tygn),
	TygnInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhb3 = {
	"lhb3", "lhb2", NULL, NULL, "1996",
	"Long Hu Bang III: Cuo Pai Gaoshou (China, V242C)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhb3), INPUT_FN(Lhb2), DIP_FN(Lhb2),
	Lhb3Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvXymg = {
	"xymg", NULL, NULL, NULL, "1996",
	"Xingyun Manguan (China, V651C, set 1)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_PREFIX_IGS, GBF_CARD, 0,
	ROM_NAME(xymg), INPUT_FN(Xymg), DIP_FN(Xymg),
	XymgInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvXymga = {
	"xymga", "xymg", NULL, NULL, "1996",
	"Xingyun Manguan (China, V651C, set 2)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_CARD, 0,
	ROM_NAME(xymga), INPUT_FN(Xymg), DIP_FN(Xymg),
	XymgaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvWlcc = {
	"wlcc", "xymg", NULL, NULL, "1996",
	"Wanli Changcheng (China, V638C)\0", NULL, "IGS", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_CARD, 0,
	ROM_NAME(wlcc), INPUT_FN(Wlcc), DIP_FN(Wlcc),
	WlccInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvNkishusp = {
	"nkishusp", "lhb2", NULL, NULL, "1998",
	"Mahjong Nenrikishu SP (Japan, V250J)\0", NULL, "IGS / Alta", "IGS011",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_PREFIX_IGS, GBF_MAHJONG, 0,
	ROM_NAME(nkishusp), INPUT_FN(Nkishusp), DIP_FN(Nkishusp),
	NkishuspInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	512, 240, 4, 3
};

#include "tiles_generic.h"
#include "z180_intf.h"
#include "msm6295.h"
#include "burn_ym2413.h"
#include "bitswap.h"
#include "igs017_igs031.h"
#include "igs017.h"

#ifndef ROM_NAME
#define ROM_NAME(name) NULL, name##RomInfo, name##RomName, NULL, NULL, NULL, NULL
#endif

#ifndef INPUT_FN
#define INPUT_FN(name) name##InputInfo
#endif

#ifndef DIP_FN
#define DIP_FN(name) name##DIPInfo
#endif

enum {
	PROFILE_IQBLOCK = 0,
	PROFILE_GENIUS6,
	PROFILE_TJSB,
	PROFILE_TARZAN,
	PROFILE_STARZAN,
	PROFILE_TARZAN103M,
	PROFILE_HAPPYSKL,
	PROFILE_CPOKER2,
	PROFILE_SPKRFORM,
};

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *DrvZ180ROM;
static UINT8 *DrvOpcodeROM;
static UINT8 *DrvTileROM;
static UINT8 *DrvSpriteROM;
static UINT8 *DrvSndROM;
static UINT8 *DrvStringROM;
static UINT8 *DrvNVRAM;
static UINT8 *DrvPortRAM;
static UINT8 *DrvWorkRAM;
static UINT8 *DrvFixedData;

static INT32 DrvZ180Len;
static INT32 DrvOpcodeLen;
static INT32 DrvTileLen;
static INT32 DrvSpriteLen;
static INT32 DrvSndLen;
static INT32 DrvStringLen;
static INT32 DrvNVRAMLen;
static INT32 DrvPortRAMLen;
static INT32 DrvWorkRAMLen;
static INT32 DrvFixedLen;

static UINT8 DrvRecalc;
static UINT8 DrvReset;
static UINT8 DrvDips[3] = { 0xff, 0xff, 0xff };
static UINT8 DrvJoy1[8];
static UINT8 DrvJoy2[8];
static UINT8 DrvJoy3[8];
static UINT8 DrvJoy4[8];
static UINT8 DrvMahjong[40];
static UINT8 DrvInputs[4];
static UINT8 DrvMahjongInputs[5];

static UINT8 DrvProfile;
static UINT8 DrvMuxAddr;
static UINT8 DrvInputSelect;
static UINT8 DrvOkiBank;
static UINT8 DrvHopperMotor;
static UINT8 DrvCoin1Pulse;
static UINT8 DrvCoin1Prev;
static UINT8 DrvCoin2Pulse;
static UINT8 DrvCoin2Prev;
static UINT8 DrvIncVal;
static UINT8 DrvIncDecVal;
static UINT8 DrvIncAltVal;
static UINT8 DrvDswSelect;
static UINT8 DrvScrambleData;
static UINT8 DrvStringOffs;
static UINT16 DrvStringWord;
static UINT16 DrvStringValue;
static INT32 DrvRemapAddr;
static UINT8 DrvBitswapM3;
static UINT8 DrvBitswapMF;
static UINT16 DrvBitswapVal;
static UINT16 DrvBitswapWord;
static UINT16 DrvBitswapValXor;
static UINT8 DrvBitswapM3Bits[4][4];
static UINT8 DrvBitswapMFBits[4];

static UINT8 string_result_r();
static void string_bitswap_w(UINT8 offset, UINT8 data);
static UINT8 string_advance_r();
static UINT8 bitswap_result_r();
static void bitswap_word_w(UINT8 data);
static void bitswap_mode_f_w();
static void bitswap_mode_3_w();
static void bitswap_exec_w(UINT8 offset, UINT8 data);
static UINT8 iqblock_player1_r();
static UINT8 iqblock_player2_r();
static UINT8 iqblock_coins_r();
static UINT8 iqblock_buttons_r();
static UINT8 tjsb_player1_r();
static UINT8 tjsb_player2_r();
static UINT8 tjsb_coins_r();
static UINT8 tjsb_buttons_r();
static UINT8 spkrform_player1_r();
static UINT8 spkrform_player2_r();
static UINT8 spkrform_coins_r();
static UINT8 spkrform_buttons_r();
static UINT8 tarzan_coins_r();
static UINT8 tarzan_matrix_r();
static UINT8 starzan_player1_r();
static UINT8 starzan_player2_r();
static UINT8 starzan_coins_r();
static UINT8 tarzan103m_player1_r();
static UINT8 tarzan103m_player2_r();
static UINT8 happyskl_player1_r();
static UINT8 happyskl_player2_r();
static UINT8 happyskl_coins_r();
static UINT8 cpoker2_player1_r();
static UINT8 cpoker2_player2_r();
static UINT8 cpoker2_coins_r();
static UINT8 profile_dsw_r();
static inline INT32 profile_has_ym2413();
static inline INT32 profile_has_split_opcodes();
static inline INT32 z180_tarzan_mahjong_controls();

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

static void DrvMakeInputs()
{
	memset(DrvInputs, 0xff, sizeof(DrvInputs));

	for (INT32 i = 0; i < 8; i++) {
		if (DrvJoy1[i]) DrvInputs[0] &= ~(1 << i);
		if (DrvJoy2[i]) DrvInputs[1] &= ~(1 << i);
		if (DrvJoy3[i]) DrvInputs[2] &= ~(1 << i);
		if (DrvJoy4[i]) DrvInputs[3] &= ~(1 << i);
	}

	DrvInputs[0] |= 0x81;

	if (DrvJoy1[0] && !DrvCoin1Prev) {
		DrvCoin1Pulse = 5;
	}
	DrvCoin1Prev = DrvJoy1[0] ? 1 : 0;

	DrvInputs[0] |= 0x01;
	if (DrvCoin1Pulse) {
		DrvInputs[0] &= ~0x01;
		DrvCoin1Pulse--;
	}

	if (DrvJoy1[7] && !DrvCoin2Prev) {
		DrvCoin2Pulse = 5;
	}
	DrvCoin2Prev = DrvJoy1[7] ? 1 : 0;

	DrvInputs[0] |= 0x80;
	if (DrvCoin2Pulse) {
		DrvInputs[0] &= ~0x80;
		DrvCoin2Pulse--;
	}

	pack_mahjong_inputs();
}

static inline UINT8 read_dip_a() { return DrvDips[0]; }
static inline UINT8 read_dip_b() { return DrvDips[1]; }
static inline UINT8 read_dip_c() { return DrvDips[2]; }
static UINT8 read_ff() { return 0xff; }

static inline INT32 profile_has_ym2413()
{
	switch (DrvProfile)
	{
		case PROFILE_IQBLOCK:
		case PROFILE_GENIUS6:
		case PROFILE_TJSB:
		case PROFILE_TARZAN:
		case PROFILE_STARZAN:
		case PROFILE_SPKRFORM:
			return 1;
	}

	return 0;
}

static inline INT32 profile_has_split_opcodes()
{
	switch (DrvProfile)
	{
		case PROFILE_TARZAN:
		case PROFILE_STARZAN:
		case PROFILE_TARZAN103M:
		case PROFILE_HAPPYSKL:
			return 1;
	}

	return 0;
}

static inline INT32 z180_tarzan_mahjong_controls()
{
	return (DrvProfile == PROFILE_TARZAN) && ((DrvDips[2] & 0x01) != 0);
}

static void apply_default_dips_if_invalid()
{
	if (DrvDips[0] == 0x00 && DrvDips[1] == 0x00 && DrvDips[2] == 0x00) {
		DrvDips[0] = 0xff;
		DrvDips[1] = 0xff;
		DrvDips[2] = 0xff;
	}
}

static UINT16 tjsb_palette_bitswap(UINT16 bgr)
{
	return BITSWAP16(bgr, 15,12,3,6,10,5,4,2,9,13,8,7,11,1,0,14);
}

static UINT16 tarzan_palette_bitswap(UINT16 bgr)
{
	return BITSWAP16(bgr, 15,0,1,2,3,4,10,11,12,13,14,5,6,7,8,9);
}

static inline UINT8 hopper_line()
{
	return (DrvHopperMotor && ((GetCurrentFrame() % 10) == 0)) ? 0 : 1;
}

static inline UINT8 rotate_right(UINT8 data, INT32 rotate)
{
	if (rotate == 0) return data;
	return ((data >> (8 - rotate)) | (data << rotate)) & 0xff;
}

static UINT8 mahjong_matrix_read(INT32 base, INT32 rotate)
{
	UINT8 ret = 0xff;

	for (INT32 i = 0; i < 5; i++) {
		if ((DrvInputSelect & (1 << (base + i))) == 0) {
			ret &= DrvMahjongInputs[i];
		}
	}

	return rotate_right(ret, rotate);
}

static void set_oki_bank(INT32 bank)
{
	DrvOkiBank = bank & 1;
	INT32 bank_base = (DrvSndLen > 0x40000) ? (DrvOkiBank * 0x40000) : 0;
	MSM6295SetBank(0, DrvSndROM + bank_base, 0x00000, 0x3ffff);
}

static void mux_data_write(UINT8 data)
{
	if (DrvMuxAddr >= 0x20 && DrvMuxAddr <= 0x27 && DrvStringLen > 0) {
		string_bitswap_w(DrvMuxAddr, data);
		return;
	}

	if (DrvProfile == PROFILE_IQBLOCK || DrvProfile == PROFILE_GENIUS6) {
		if (DrvMuxAddr == 0x40) {
			bitswap_word_w(data);
			return;
		}
		if (DrvMuxAddr == 0x48) {
			bitswap_mode_f_w();
			return;
		}
		if (DrvMuxAddr == 0x50) {
			bitswap_mode_3_w();
			return;
		}
		if (DrvMuxAddr >= 0x80 && DrvMuxAddr <= 0x87) {
			bitswap_exec_w(DrvMuxAddr, data);
			return;
		}
		if (DrvMuxAddr == 0xa0) {
			DrvBitswapVal = 0;
			return;
		}
	}

	switch (DrvProfile)
	{
		case PROFILE_IQBLOCK:
		case PROFILE_GENIUS6:
			switch (DrvMuxAddr)
			{
				case 0x00:
				case 0x01:
					return;
			}
		break;

		case PROFILE_TJSB:
			switch (DrvMuxAddr)
			{
				case 0x02:
					set_oki_bank(BIT(data, 4));
				return;

				case 0x03:
					DrvHopperMotor = BIT(data, 6);
				return;
			}
		break;

		case PROFILE_SPKRFORM:
			switch (DrvMuxAddr)
			{
				case 0x02:
					DrvHopperMotor = BIT(data, 4);
				return;
			}
		break;

		case PROFILE_TARZAN:
			switch (DrvMuxAddr)
			{
				case 0x00:
					DrvInputSelect = data;
				return;

				case 0x01:
					DrvHopperMotor = BIT(data, 1);
				return;

				case 0x03:
					DrvDswSelect = data & 0x07;
					set_oki_bank(BIT(data, 7));
				return;
			}
		break;

		case PROFILE_STARZAN:
		case PROFILE_TARZAN103M:
			switch (DrvMuxAddr)
			{
				case 0x00:
					DrvHopperMotor = BIT(data, 1);
				return;

				case 0x02:
					set_oki_bank(BIT(data, 7));
				return;

				case 0x03:
					DrvDswSelect = data;
				return;
			}
		break;

		case PROFILE_HAPPYSKL:
		case PROFILE_CPOKER2:
			switch (DrvMuxAddr)
			{
				case 0x00:
					DrvHopperMotor = BIT(data, 1);
				return;

				case 0x02:
					set_oki_bank(BIT(data, 7));
				return;

				case 0x03:
					DrvDswSelect = data;
				return;
			}
		break;

	}
}

static UINT8 iqblock_player1_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x04) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x20) == 0) ret &= ~0x20;
	if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x40;
	return ret;
}

static UINT8 iqblock_player2_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[2] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[2] & 0x10) == 0) ret &= ~0x20;
	if ((DrvInputs[2] & 0x20) == 0) ret &= ~0x40;
	return ret;
}

static UINT8 iqblock_coins_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x40;
	ret &= ~0x80;
	if ((DrvInputs[0] & 0x40) == 0) ret |= 0x80;
	return ret;
}

static UINT8 iqblock_buttons_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[2] & 0x08) == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x80) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x08;
	if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x20;
	return ret;
}

static UINT8 tjsb_player1_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x04) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x20) == 0) ret &= ~0x20;
	if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x40;
	return ret;
}

static UINT8 tjsb_player2_r()
{
	return 0xff;
}

static UINT8 tjsb_coins_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvDips[0] & 0x20) ? ((DrvInputs[0] & 0x01) == 0) : ((DrvInputs[0] & 0x02) == 0)) ret &= ~0x80;
	return ret;
}

static UINT8 tjsb_buttons_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x02;
	if ((DrvDips[0] & 0x40) ? ((DrvInputs[0] & 0x08) == 0) : ((DrvInputs[0] & 0x04) == 0)) ret &= ~0x10;
	if (hopper_line() == 0) ret &= ~0x20;
	return ret;
}

static UINT8 spkrform_player1_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x01;
	if ((DrvInputs[2] & 0x01) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x20;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x40;
	return ret;
}

static UINT8 spkrform_player2_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[2] & 0x10) == 0) ret &= ~0x02;
	if ((DrvInputs[2] & 0x20) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[2] & 0x02) == 0) ret &= ~0x20;
	if ((DrvInputs[2] & 0x04) == 0) ret &= ~0x40;
	return ret;
}

static UINT8 spkrform_coins_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x01;
	if ((DrvInputs[2] & 0x08) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x08;
	return ret;
}

static UINT8 spkrform_buttons_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x80) == 0) ret &= ~0x02;
	if (hopper_line() == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x08;
	if ((DrvInputs[2] & 0x80) == 0) ret &= ~0x10;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x20;
	return ret;
}

static UINT8 tarzan_coins_r()
{
	UINT8 ret = 0xff;
	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x04;
	if ((DrvDips[1] & 0x01) ? ((DrvInputs[0] & 0x01) == 0) : ((DrvInputs[0] & 0x02) == 0)) ret &= ~0x08;
	if ((DrvDips[2] & 0x20) ? ((DrvInputs[0] & 0x08) == 0) : ((DrvInputs[0] & 0x04) == 0)) ret &= ~0x10;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x20;
	if (!z180_tarzan_mahjong_controls()) {
		if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x40;
		if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x80;
	}
	return ret;
}

static UINT8 tarzan_matrix_r()
{
	if (z180_tarzan_mahjong_controls()) {
		return mahjong_matrix_read(3, 0);
	}

	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x02) == 0 || (DrvInputs[3] & 0x01) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x04) == 0 || (DrvInputs[3] & 0x02) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x08) == 0 || (DrvInputs[3] & 0x04) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x10) == 0 || (DrvInputs[3] & 0x08) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x20) == 0 || (DrvInputs[3] & 0x10) == 0) ret &= ~0x20;
	return ret;
}

static UINT8 starzan_player1_r()
{
	UINT8 ret = 0xff;
	if (hopper_line() == 0) ret &= ~0x01;

	if (DrvDips[0] & 0x20) {
		if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x20;
		if ((DrvInputs[3] & 0x04) == 0 || (DrvInputs[3] & 0x20) == 0) ret &= ~0x40;
		if ((DrvInputs[2] & 0x40) == 0 || (DrvInputs[1] & 0x40) == 0 || (DrvInputs[3] & 0x80) == 0) ret &= ~0x80;
	} else {
		if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x20;
		if ((DrvInputs[3] & 0x02) == 0 || (DrvInputs[3] & 0x20) == 0) ret &= ~0x40;
		if ((DrvInputs[3] & 0x08) == 0 || (DrvInputs[1] & 0x40) == 0 || (DrvInputs[3] & 0x40) == 0) ret &= ~0x80;
	}

	return ret;
}

static UINT8 starzan_player2_r()
{
	UINT8 ret = 0xff;

	if (DrvDips[0] & 0x20) {
		if ((DrvInputs[3] & 0x02) == 0 || (DrvInputs[3] & 0x10) == 0) ret &= ~0x01;
		if ((DrvInputs[3] & 0x08) == 0 || (DrvInputs[3] & 0x80) == 0) ret &= ~0x02;
		if ((DrvInputs[3] & 0x01) == 0 || (DrvInputs[3] & 0x80) == 0) ret &= ~0x04;
	} else {
		if ((DrvInputs[2] & 0x40) == 0 || (DrvInputs[3] & 0x10) == 0) ret &= ~0x01;
		if ((DrvInputs[3] & 0x01) == 0 || (DrvInputs[3] & 0x80) == 0) ret &= ~0x02;
		if ((DrvInputs[3] & 0x04) == 0 || (DrvInputs[3] & 0x80) == 0) ret &= ~0x04;
	}

	return ret;
}

static UINT8 starzan_coins_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x01; // Key In
	if ((DrvInputs[0] & 0x80) == 0) ret &= ~0x02; // Coin 2
	if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x04; // Coin 1
	if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x08; // Payout
	if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x10; // Key Out
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x20; // Service / Test
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x40; // Book
	return ret;
}

static UINT8 tarzan103m_player1_r()
{
	UINT8 ret = 0xff;
	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x20) == 0 || (DrvInputs[3] & 0x10) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x80) == 0 || (DrvInputs[3] & 0x20) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x20;
	if ((DrvInputs[1] & 0x08) == 0 || (DrvInputs[3] & 0x04) == 0) ret &= ~0x40;
	if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x80;
	return ret;
}

static UINT8 tarzan103m_player2_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x04) == 0 || (DrvInputs[3] & 0x02) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x10) == 0 || (DrvInputs[3] & 0x08) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x02) == 0 || (DrvInputs[3] & 0x01) == 0) ret &= ~0x04;
	return ret;
}

static UINT8 happyskl_player1_r()
{
	UINT8 ret = 0xff;
	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x20;
	if ((DrvInputs[2] & 0x02) == 0) ret &= ~0x40;
	if ((DrvInputs[2] & 0x20) == 0) ret &= ~0x80;
	return ret;
}

static UINT8 happyskl_player2_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[2] & 0x04) == 0) ret &= ~0x01;
	if ((DrvInputs[2] & 0x08) == 0) ret &= ~0x02;
	if ((DrvInputs[2] & 0x10) == 0) ret &= ~0x04;
	return ret;
}

static UINT8 happyskl_coins_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x01; // Key In
	if ((DrvInputs[0] & 0x80) == 0) ret &= ~0x02; // Coin 2
	if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x04; // Coin 1
	if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x08; // Payout
	if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x10; // Key Out
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x20; // Service / Test
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x40; // Book
	return ret;
}

static UINT8 cpoker2_player1_r()
{
	UINT8 ret = 0xff;
	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x20;
	if ((DrvInputs[2] & 0x08) == 0) ret &= ~0x40;
	if ((DrvInputs[2] & 0x20) == 0) ret &= ~0x80;
	return ret;
}

static UINT8 cpoker2_player2_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[2] & 0x02) == 0) ret &= ~0x01;
	if ((DrvInputs[2] & 0x04) == 0) ret &= ~0x02;
	if ((DrvInputs[2] & 0x10) == 0) ret &= ~0x04;
	return ret;
}

static UINT8 cpoker2_coins_r()
{
	return happyskl_coins_r();
}

static UINT8 profile_dsw_r()
{
	UINT8 ret = 0xff;
	if ((DrvDswSelect & 0x01) == 0) ret &= DrvDips[0];
	if ((DrvDswSelect & 0x02) == 0) ret &= DrvDips[1];
	if ((DrvDswSelect & 0x04) == 0) ret &= DrvDips[2];
	return ret;
}

static UINT8 mux_data_read()
{
	if (DrvMuxAddr == 0x05 && DrvStringLen > 0) {
		return string_result_r();
	}

	if (DrvMuxAddr == 0x40 && DrvStringLen > 0) {
		return string_advance_r();
	}

	if ((DrvProfile == PROFILE_IQBLOCK || DrvProfile == PROFILE_GENIUS6) &&
		DrvMuxAddr == 0x03) {
		return bitswap_result_r();
	}

	if ((DrvProfile == PROFILE_IQBLOCK || DrvProfile == PROFILE_GENIUS6) &&
		DrvMuxAddr >= 0x20 && DrvMuxAddr <= 0x34 && DrvFixedLen > 0) {
		static const UINT8 invalid_offsets[] = { 0x23, 0x29, 0x2f };
		for (INT32 i = 0; i < 3; i++) {
			if (DrvMuxAddr == invalid_offsets[i]) return 0xff;
		}
		return (DrvMuxAddr - 0x20) < DrvFixedLen ? DrvFixedData[DrvMuxAddr - 0x20] : 0xff;
	}

	switch (DrvProfile)
	{
		case PROFILE_IQBLOCK:
		case PROFILE_GENIUS6:
			switch (DrvMuxAddr)
			{
				case 0x00: return iqblock_player1_r();
				case 0x01: return iqblock_player2_r();
				case 0x02: return iqblock_coins_r();
			}
		break;

		case PROFILE_TJSB:
			switch (DrvMuxAddr)
			{
				case 0x00: return tjsb_player1_r();
				case 0x01: return tjsb_player2_r();
				case 0x02: return tjsb_coins_r();
				case 0x03: return tjsb_buttons_r();
			}
		break;

		case PROFILE_SPKRFORM:
			switch (DrvMuxAddr)
			{
				case 0x00: return spkrform_player1_r();
				case 0x01: return spkrform_player2_r();
				case 0x02: return spkrform_coins_r();
				case 0x03: return spkrform_buttons_r();
			}
		break;

		case PROFILE_TARZAN:
			switch (DrvMuxAddr)
			{
				case 0x02: return profile_dsw_r();
			}
		break;

		case PROFILE_STARZAN:
			switch (DrvMuxAddr)
			{
				case 0x01: return starzan_player2_r();
			}
		break;

		case PROFILE_TARZAN103M:
			switch (DrvMuxAddr)
			{
				case 0x01: return tarzan103m_player2_r();
			}
		break;

		case PROFILE_HAPPYSKL:
			switch (DrvMuxAddr)
			{
				case 0x01: return happyskl_player2_r();
			}
		break;

		case PROFILE_CPOKER2:
			switch (DrvMuxAddr)
			{
				case 0x01: return cpoker2_player2_r();
			}
		break;

	}

	return 0xff;
}

static UINT8 incdec_result()
{
	return (BIT(DrvIncDecVal, 0) << 7) |
		   (BIT(DrvIncDecVal, 3) << 5) |
		   (BIT(DrvIncDecVal, 2) << 4) |
		   (BIT(DrvIncDecVal, 1) << 2);
}

static void incalt_write(UINT8 data)
{
	switch (data)
	{
		case 0x21:
			DrvIncAltVal = (BIT(DrvIncAltVal, 1) & BIT(DrvIncAltVal, 0)) |
						   (BIT(DrvIncAltVal, 2) << 1) |
						   ((BIT(~DrvIncAltVal, 0) | BIT(DrvIncAltVal, 1)) << 2);
		return;

		case 0x55:
			DrvIncAltVal++;
		return;

		case 0xff:
			DrvIncAltVal = 0;
		return;
	}
}

static UINT8 incalt_result()
{
	return ((!BIT(DrvIncAltVal, 0) && !BIT(DrvIncAltVal, 2)) || BIT(DrvIncAltVal, 1)) ? 2 : 0;
}

static UINT8 inc_result()
{
	return (BIT(~DrvIncVal, 0) | (BIT(DrvIncVal, 1) & BIT(DrvIncVal, 2))) << 5;
}

static UINT8 string_result_r()
{
	return BITSWAP08(DrvStringValue, 5, 2, 9, 7, 10, 13, 12, 15);
}

static void string_bitswap_w(UINT8 offset, UINT8 data)
{
	DrvStringValue = igs017_string_bitswap_step(DrvStringValue, DrvStringWord, offset, data);
}

static UINT8 string_advance_r()
{
	if (DrvStringLen <= 0) return 0xff;

	UINT8 next_offs = (DrvStringOffs + 1) < DrvStringLen ? (DrvStringOffs + 1) : 0;
	UINT8 shift = (next_offs & 1) ? 8 : 0;
	UINT8 next_byte = DrvStringROM[next_offs];
	UINT16 mask = (shift == 0) ? 0xff00 : 0x00ff;
	DrvStringWord = (DrvStringWord & mask) | (next_byte << shift);
	DrvStringOffs = next_offs;

	return 0xff;
}

static UINT8 bitswap_result_r()
{
	UINT8 ret = BITSWAP08(DrvBitswapVal, 5, 2, 9, 7, 10, 13, 12, 15);
	return ret;
}

static void bitswap_word_w(UINT8 data)
{
	DrvBitswapWord = (DrvBitswapWord << 8) | data;
}

static void bitswap_mode_f_w()
{
	DrvBitswapMF = 0;
	if ((DrvBitswapWord & 0x0f00) != 0x0a00) DrvBitswapMF |= 0x08;
	if ((DrvBitswapWord & 0xf000) != 0x9000) DrvBitswapMF |= 0x04;
	if ((DrvBitswapWord & 0x000f) != 0x0006) DrvBitswapMF |= 0x02;
	if ((DrvBitswapWord & 0x00f0) != 0x0090) DrvBitswapMF |= 0x01;
}

static void bitswap_mode_3_w()
{
	DrvBitswapM3 = 0;
	if ((DrvBitswapWord & 0x000f) != 0x0003) DrvBitswapM3 |= 0x02;
	if ((DrvBitswapWord & 0x00f0) != 0x0050) DrvBitswapM3 |= 0x01;
}

static UINT16 bitswap_pick(UINT8 spec, UINT16 value)
{
	if (spec & 0x80) {
		return BIT(value, spec ^ 0xff) ^ 1;
	}
	return BIT(value, spec);
}

static void bitswap_exec_w(UINT8 offset, UINT8 data)
{
	UINT16 x = DrvBitswapVal;

	DrvBitswapVal ^= DrvBitswapValXor;
	for (INT32 i = 0; i < 4; i++) {
		DrvBitswapVal ^= BIT(DrvBitswapMF, i) << DrvBitswapMFBits[i];
	}
	DrvBitswapVal <<= 1;

	UINT16 bit0 = 0;
	for (INT32 i = 0; i < 4; i++) {
		bit0 ^= bitswap_pick(DrvBitswapM3Bits[DrvBitswapM3 & 3][i], x);
	}

	DrvBitswapVal |= bit0 ^ BIT(data, offset & 7);
}

static INT32 MemIndex()
{
	UINT8 *Next = AllMem;

	DrvZ180ROM = Next; Next += DrvZ180Len;
	DrvOpcodeROM = Next; Next += DrvOpcodeLen;
	DrvTileROM = Next; Next += DrvTileLen;
	DrvSpriteROM = Next; Next += DrvSpriteLen;
	DrvSndROM = Next; Next += DrvSndLen;
	DrvStringROM = Next; Next += DrvStringLen;
	DrvNVRAM = Next; Next += DrvNVRAMLen;
	DrvPortRAM = Next; Next += DrvPortRAMLen;
	DrvWorkRAM = Next; Next += DrvWorkRAMLen;
	DrvFixedData = Next; Next += DrvFixedLen;

	MemEnd = Next;

	return 0;
}

static INT32 drv_get_roms(INT32 bLoad)
{
	BurnRomInfo ri;
	UINT8 *mainLoad = DrvZ180ROM;
	UINT8 *tileLoad = DrvTileROM;
	UINT8 *spriteLoad = DrvSpriteROM;
	UINT8 *sndLoad = DrvSndROM;
	UINT8 *stringLoad = DrvStringROM;
	UINT8 *fixedLoad = DrvFixedData;

	for (INT32 i = 0; BurnDrvGetRomInfo(&ri, i) == 0; i++) {
		INT32 type = ri.nType & 0x0f;
		INT32 word_swap = ri.nType & IGS017_WORD_SWAP;

		switch (type)
		{
			case 1:
				if (bLoad) {
					if (BurnLoadRom(mainLoad, i, 1)) return 1;
					mainLoad += ri.nLen;
				} else {
					DrvZ180Len += ri.nLen;
				}
			break;

			case 2:
				if (bLoad) {
					if (BurnLoadRom(spriteLoad, i, 1)) return 1;
					if (word_swap) BurnByteswap(spriteLoad, ri.nLen);
					spriteLoad += ri.nLen;
				} else {
					DrvSpriteLen += ri.nLen;
				}
			break;

			case 3:
				if (bLoad) {
					if (BurnLoadRom(tileLoad, i, 1)) return 1;
					if (word_swap) BurnByteswap(tileLoad, ri.nLen);
					tileLoad += ri.nLen;
				} else {
					DrvTileLen += ri.nLen;
				}
			break;

			case 4:
				if (bLoad) {
					if (BurnLoadRom(sndLoad, i, 1)) return 1;
					sndLoad += ri.nLen;
				} else {
					DrvSndLen += ri.nLen;
				}
			break;

			case 5:
				if (bLoad) {
					if (BurnLoadRom(stringLoad, i, 1)) return 1;
					stringLoad += ri.nLen;
				} else {
					DrvStringLen += ri.nLen;
				}
			break;

			case 7:
				if (bLoad) {
					if (BurnLoadRom(fixedLoad, i, 1)) return 1;
					fixedLoad += ri.nLen;
				} else {
					DrvFixedLen += ri.nLen;
				}
			break;
		}
	}

	return 0;
}

static void decrypt_program_rom(UINT8 mask, INT32 a7, INT32 a6, INT32 a5, INT32 a4, INT32 a3, INT32 a2, INT32 a1, INT32 a0) { igs017_crypt_program_rom(DrvZ180ROM, DrvZ180Len, mask, a7, a6, a5, a4, a3, a2, a1, a0); }

static void decrypt_iqblocka()
{
	decrypt_program_rom(0x11, 7, 6, 5, 4, 3, 2, 1, 0);
}

static void decrypt_tjsb()
{
	decrypt_program_rom(0x05, 7, 6, 3, 2, 5, 4, 1, 0);
	igs017_igs031_tjsb_decrypt_sprites(DrvSpriteROM, DrvSpriteLen);
}

static void decrypt_tarzanc()
{
	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x001a0) != 0x00020) x ^= 0x04;
		if ((i & 0x00080) != 0x00080) x ^= 0x10;
		if ((i & 0x000e0) == 0x000c0) x ^= 0x10;
		if (DrvOpcodeROM) DrvOpcodeROM[i] = x;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if (((i & 0x00020) == 0x00020) || ((i & 0x000260) == 0x00040)) x ^= 0x04;
		if ((i & 0x001a0) != 0x00020) x ^= 0x10;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 1);
	igs017_igs031_tarzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0, 0);
}

static void decrypt_tarzan()
{
	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x001a0) != 0x00020) x ^= 0x04;
		if ((i & 0x00080) != 0x00080) x ^= 0x10;
		if ((i & 0x000e0) == 0x000c0) x ^= 0x10;
		if (DrvOpcodeROM) DrvOpcodeROM[i] = x;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if (((i & 0x00020) == 0x00020) || ((i & 0x000260) == 0x00040)) x ^= 0x04;
		if ((i & 0x001a0) != 0x00020) x ^= 0x10;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 0);
}

static void decrypt_tarzana()
{
	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x00080) != 0x00080) x ^= 0x20;
		if ((i & 0x000e0) == 0x000c0) x ^= 0x20;
		if ((i & 0x00280) != 0x00080) x ^= 0x40;
		if ((i & 0x001a0) != 0x00020) x ^= 0x80;
		if (DrvOpcodeROM) DrvOpcodeROM[i] = x;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x001a0) != 0x00020) x ^= 0x20;
		if ((i & 0x00260) != 0x00200) x ^= 0x40;
		if ((i & 0x00060) != 0x00000 && (i & 0x00260) != 0x00240) x ^= 0x80;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 0);
}

static void decrypt_starzan()
{
	if (DrvSpriteLen > 0x400000) {
		memmove(DrvSpriteROM + 0x200000, DrvSpriteROM + 0x400000, DrvSpriteLen - 0x400000);
		DrvSpriteLen = 0x400000;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x00020) != 0x00020) x ^= 0x20;
		if ((i & 0x002a0) == 0x00220) x ^= 0x20;
		if ((i & 0x00220) != 0x00200) x ^= 0x40;
		if ((i & 0x001c0) != 0x00040) x ^= 0x80;
		if (DrvOpcodeROM) DrvOpcodeROM[i] = x;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x000a0) != 0x00000) x ^= 0x20;
		if ((i & 0x001a0) == 0x00000) x ^= 0x20;
		if ((i & 0x00060) != 0x00020) x ^= 0x40;
		if ((i & 0x00260) == 0x00220) x ^= 0x40;
		if ((i & 0x00020) == 0x00020) x ^= 0x80;
		if ((i & 0x001a0) == 0x00080) x ^= 0x80;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 1);
	igs017_igs031_starzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0x200000, 0x400000);
}

static void decrypt_jking103a()
{
	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x00020) != 0x00020) x ^= 0x20;
		if ((i & 0x002a0) == 0x00220) x ^= 0x20;
		if ((i & 0x00220) != 0x00200) x ^= 0x40;
		if ((i & 0x001c0) != 0x00040) x ^= 0x80;
		if (DrvOpcodeROM) DrvOpcodeROM[i] = x;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x000a0) != 0x00000) x ^= 0x20;
		if ((i & 0x001a0) == 0x00000) x ^= 0x20;
		if ((i & 0x00060) != 0x00020) x ^= 0x40;
		if ((i & 0x00260) == 0x00220) x ^= 0x40;
		if ((i & 0x00020) == 0x00020) x ^= 0x80;
		if ((i & 0x001a0) == 0x00080) x ^= 0x80;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 1);
	igs017_igs031_tarzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0, 0);
}

static void decrypt_jking200pr()
{
	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if (((i & 0x00220) == 0x00020) || ((i & 0x002a0) == 0x002a0)) x ^= 0x20;
		if (((i & 0x00200) == 0x00000) || ((i & 0x00220) == 0x00220)) x ^= 0x40;
		if ((i & 0x001c0) != 0x00040) x ^= 0x80;
		if (DrvOpcodeROM) DrvOpcodeROM[i] = x;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if (((i & 0x001e0) == 0x00100) || ((i & 0x001e0) == 0x00140)) x ^= 0x20;
		if ((i & 0x00260) != 0x00020) x ^= 0x40;
		if (((i & 0x00020) == 0x00020) || ((i & 0x00180) == 0x00080)) x ^= 0x80;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 1);
	igs017_igs031_tarzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0, 0);
}

static void decrypt_happyskl()
{
	if (DrvSpriteLen > 0x400000) {
		memmove(DrvSpriteROM + 0x200000, DrvSpriteROM + 0x400000, DrvSpriteLen - 0x400000);
		DrvSpriteLen = 0x400000;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x0280) != 0x00080) x ^= 0x20;
		if ((i & 0x02a0) == 0x00280) x ^= 0x20;
		if ((i & 0x0280) != 0x00080) x ^= 0x40;
		if ((i & 0x01a0) != 0x00080) x ^= 0x80;
		if (DrvOpcodeROM) DrvOpcodeROM[i] = x;
	}

	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x000a0) != 0x00000) x ^= 0x20;
		if ((i & 0x001a0) == 0x00000) x ^= 0x20;
		if ((i & 0x00060) != 0x00040) x ^= 0x40;
		if ((i & 0x00260) == 0x00240) x ^= 0x40;
		if ((i & 0x00020) == 0x00020) x ^= 0x80;
		if ((i & 0x00260) == 0x00040) x ^= 0x80;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 1);
	igs017_igs031_starzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0x200000, 0);
}

static void decrypt_cpoker2()
{
	for (INT32 i = 0; i < DrvZ180Len; i++) {
		UINT8 x = DrvZ180ROM[i];
		if ((i & 0x00011) == 0x00011) x ^= 0x01;
		if ((i & 0x02180) == 0x00000) x ^= 0x01;
		if ((i & 0x001a0) != 0x00020) x ^= 0x20;
		if ((i & 0x00260) != 0x00020) x ^= 0x40;
		if ((i & 0x00020) == 0x00020) x ^= 0x80;
		if ((i & 0x00260) == 0x00240) x ^= 0x80;
		DrvZ180ROM[i] = x;
	}

	igs017_igs031_tarzan_decrypt_tiles(DrvTileROM, DrvTileLen, 1);
	igs017_igs031_tarzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0, 0);
}

static void decrypt_spkrform()
{
	decrypt_program_rom(0x14, 7, 6, 5, 4, 3, 0, 1, 2);
	igs017_igs031_spkrform_decrypt_sprites(DrvSpriteROM, DrvSpriteLen);

	if (DrvZ180Len > 0x32ef9) {
		DrvZ180ROM[0x32ea9] = 0x00;
		DrvZ180ROM[0x32ef9] = 0x00;
	}
}

static void z180_set_video_inputs()
{
	switch (DrvProfile)
	{
		case PROFILE_IQBLOCK:
		case PROFILE_GENIUS6:
		case PROFILE_TJSB:
		case PROFILE_SPKRFORM:
			igs017_igs031_set_inputs(read_dip_a, read_dip_b, read_dip_c);
		return;

		case PROFILE_TARZAN:
			igs017_igs031_set_inputs(tarzan_coins_r, tarzan_matrix_r, read_ff);
		return;

		case PROFILE_STARZAN:
			igs017_igs031_set_inputs(starzan_coins_r, starzan_player1_r, profile_dsw_r);
		return;

		case PROFILE_TARZAN103M:
			igs017_igs031_set_inputs(starzan_coins_r, tarzan103m_player1_r, profile_dsw_r);
		return;

		case PROFILE_HAPPYSKL:
			igs017_igs031_set_inputs(happyskl_coins_r, happyskl_player1_r, profile_dsw_r);
		return;

		case PROFILE_CPOKER2:
			igs017_igs031_set_inputs(cpoker2_coins_r, cpoker2_player1_r, profile_dsw_r);
		return;
	}

	igs017_igs031_set_inputs(read_ff, read_ff, read_ff);
}

static void z180_set_incdec_remap(UINT32 address, UINT8 data, INT32 tarzan_style)
{
	if ((address & 1) == 0) {
		DrvRemapAddr = (DrvRemapAddr & 0xff00) | data;
	} else {
		if (tarzan_style) data ^= 0x40;
		DrvRemapAddr = (DrvRemapAddr & 0x00ff) | (data << 8);
	}
}

static UINT8 __fastcall z180_program_read(UINT32 address)
{
	address &= 0xfffff;

	if ((DrvProfile == PROFILE_IQBLOCK || DrvProfile == PROFILE_GENIUS6 ||
		DrvProfile == PROFILE_TARZAN || DrvProfile == PROFILE_STARZAN || DrvProfile == PROFILE_TARZAN103M || DrvProfile == PROFILE_CPOKER2 || DrvProfile == PROFILE_SPKRFORM) && DrvRemapAddr >= 0) {
		if (address == (UINT32)(DrvRemapAddr + 0x5)) {
			return incdec_result();
		}
	}

	if (DrvProfile == PROFILE_CPOKER2 && address == 0x0a460) {
		return inc_result();
	}

	if (DrvProfile == PROFILE_TJSB) {
		if (address == 0x0e000) {
			return 0xff;
		}
		if (address == 0x0e001) {
			return mux_data_read();
		}
		if (address >= 0x0e002 && address <= 0x0ffff) {
			return DrvNVRAM[address - 0x0e000];
		}
	}

	if (address < (UINT32)DrvZ180Len) {
		return DrvZ180ROM[address];
	}

	return 0xff;
}

static void __fastcall z180_program_write(UINT32 address, UINT8 data)
{
	address &= 0xfffff;

	if ((DrvProfile == PROFILE_IQBLOCK || DrvProfile == PROFILE_GENIUS6 ||
		DrvProfile == PROFILE_TARZAN || DrvProfile == PROFILE_STARZAN || DrvProfile == PROFILE_TARZAN103M || DrvProfile == PROFILE_CPOKER2 || DrvProfile == PROFILE_SPKRFORM) && DrvRemapAddr >= 0) {
		if (address == (UINT32)(DrvRemapAddr + 0x0)) { DrvIncDecVal = 0; return; }
		if (address == (UINT32)(DrvRemapAddr + 0x1)) { DrvIncDecVal--; return; }
		if (address == (UINT32)(DrvRemapAddr + 0x3)) { DrvIncDecVal++; return; }
	}

	if (DrvProfile == PROFILE_CPOKER2) {
		if (address == 0x0a400) { DrvIncVal++; return; }
		if (address == 0x0a420) { DrvIncVal = 0; return; }
	}

	if (DrvProfile == PROFILE_TJSB) {
		if (address == 0x0e000) { DrvMuxAddr = data; return; }
		if (address == 0x0e001) { mux_data_write(data); return; }
		if (address >= 0x0e002 && address <= 0x0ffff) {
			DrvNVRAM[address - 0x0e000] = data;
			return;
		}
	}
}

static UINT8 __fastcall z180_port_read(UINT32 address)
{
	address &= 0xffff;

	if (address <= 0x003f) {
		return DrvPortRAM[address & 0x3f];
	}

	if (address <= 0x7fff) {
		return igs017_igs031_read(address);
	}

	switch (DrvProfile)
	{
		case PROFILE_IQBLOCK:
		case PROFILE_GENIUS6:
			if (address == 0x8001) return mux_data_read();
			if (address == 0x9000) return MSM6295Read(0);
			if (address == 0xa000) return iqblock_buttons_r();
		return 0xff;

		case PROFILE_TJSB:
			if (address == 0x9000) return MSM6295Read(0);
		return 0xff;

		case PROFILE_TARZAN:
		case PROFILE_STARZAN:
		case PROFILE_TARZAN103M:
		case PROFILE_HAPPYSKL:
		case PROFILE_CPOKER2:
			if (address == 0x8001) return mux_data_read();
			if (address == 0x9000) return MSM6295Read(0);
		return 0xff;

		case PROFILE_SPKRFORM:
			if (address == 0x8000) return MSM6295Read(0);
			if (address == 0xb001) return mux_data_read();
		return 0xff;
	}

	return 0xff;
}

static void __fastcall z180_port_write(UINT32 address, UINT8 data)
{
	address &= 0xffff;

	if (address <= 0x003f) {
		DrvPortRAM[address & 0x3f] = data;
		return;
	}

	if ((DrvProfile == PROFILE_IQBLOCK || DrvProfile == PROFILE_GENIUS6 ||
		DrvProfile == PROFILE_TARZAN || DrvProfile == PROFILE_STARZAN || DrvProfile == PROFILE_TARZAN103M || DrvProfile == PROFILE_CPOKER2 ||
		DrvProfile == PROFILE_SPKRFORM) && (address == 0x2010 || address == 0x2011)) {
		z180_set_incdec_remap(address, data, (DrvProfile == PROFILE_TARZAN));
		return;
	}

	if (address <= 0x7fff) {
		igs017_igs031_write(address, data);
		return;
	}

	switch (DrvProfile)
	{
		case PROFILE_IQBLOCK:
		case PROFILE_GENIUS6:
			if (address == 0x8000) { DrvMuxAddr = data; return; }
			if (address == 0x8001) { mux_data_write(data); return; }
			if (address == 0x9000) { MSM6295Write(0, data); return; }
			if (address == 0xb000 || address == 0xb001) { BurnYM2413Write(address & 1, data); return; }
		return;

		case PROFILE_TJSB:
			if (address == 0x9000) { MSM6295Write(0, data); return; }
			if (address == 0xb000 || address == 0xb001) { BurnYM2413Write(address & 1, data); return; }
		return;

		case PROFILE_TARZAN:
			if (address == 0x8000) { DrvMuxAddr = data; return; }
			if (address == 0x8001) { mux_data_write(data); return; }
			if (address == 0x9000) { MSM6295Write(0, data); return; }
			if (address == 0xb000 || address == 0xb001) { BurnYM2413Write(address & 1, data); return; }
		return;

		case PROFILE_STARZAN:
		case PROFILE_TARZAN103M:
		case PROFILE_HAPPYSKL:
		case PROFILE_CPOKER2:
			if (address == 0x8000) { DrvMuxAddr = data; return; }
			if (address == 0x8001) { mux_data_write(data); return; }
			if (address == 0x9000) { MSM6295Write(0, data); return; }
		return;

		case PROFILE_SPKRFORM:
			if (address == 0x8000) { MSM6295Write(0, data); return; }
			if (address == 0x9000 || address == 0x9001) { BurnYM2413Write(address & 1, data); return; }
			if (address == 0xb000) { DrvMuxAddr = data; return; }
			if (address == 0xb001) { mux_data_write(data); return; }
		return;
	}
}

static INT32 DrvDoReset()
{
	DrvMuxAddr = 0;
	DrvInputSelect = 0xff;
	DrvOkiBank = 0;
	DrvHopperMotor = 0;
	DrvCoin1Pulse = 0;
	DrvCoin1Prev = 0;
	DrvCoin2Pulse = 0;
	DrvCoin2Prev = 0;
	DrvIncVal = 0;
	DrvIncDecVal = 0;
	DrvIncAltVal = 0;
	DrvDswSelect = 0xff;
	DrvScrambleData = 0;
	DrvStringOffs = 0;
	DrvStringWord = 0;
	DrvStringValue = 0;
	DrvRemapAddr = -1;
	DrvBitswapM3 = 0;
	DrvBitswapMF = 0;
	DrvBitswapVal = 0;
	DrvBitswapWord = 0;
	memset(DrvNVRAM, 0, DrvNVRAMLen);
	memset(DrvPortRAM, 0, DrvPortRAMLen);
	memset(DrvWorkRAM, 0, DrvWorkRAMLen);

	Z180Open(0);
	Z180Reset();
	Z180Close();

	if (profile_has_ym2413()) {
		BurnYM2413Reset();
	}

	MSM6295Reset(0);
	set_oki_bank(0);
	igs017_igs031_reset();

	return 0;
}

static INT32 common_init(INT32 profile, void (*decrypt)())
{
	DrvProfile = profile;
	DrvNVRAMLen = (profile == PROFILE_IQBLOCK || profile == PROFILE_GENIUS6 || profile == PROFILE_CPOKER2 || profile == PROFILE_SPKRFORM) ? 0x1000 : 0x2000;
	DrvPortRAMLen = 0x40;
	DrvWorkRAMLen = (profile == PROFILE_IQBLOCK || profile == PROFILE_GENIUS6 || profile == PROFILE_CPOKER2 || profile == PROFILE_SPKRFORM) ? 0x1000 : 0;
	DrvZ180Len = DrvOpcodeLen = DrvTileLen = DrvSpriteLen = DrvSndLen = DrvStringLen = DrvFixedLen = 0;
	DrvBitswapValXor = 0;
	memset(DrvBitswapM3Bits, 0, sizeof(DrvBitswapM3Bits));
	memset(DrvBitswapMFBits, 0, sizeof(DrvBitswapMFBits));
	apply_default_dips_if_invalid();

	if (drv_get_roms(0)) return 1;

	if (profile_has_split_opcodes()) {
		DrvOpcodeLen = DrvZ180Len;
	}

	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8*)0;
	AllMem = (UINT8*)BurnMalloc(nLen);
	if (AllMem == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	if (drv_get_roms(1)) return 1;
	if (decrypt) decrypt();

	igs017_igs031_set_text_reverse_bits(0);
	igs017_igs031_set_palette_scramble(NULL);
	switch (DrvProfile)
	{
		case PROFILE_TJSB:
			igs017_igs031_set_palette_scramble(tjsb_palette_bitswap);
		break;

		case PROFILE_TARZAN:
		case PROFILE_STARZAN:
		case PROFILE_TARZAN103M:
		case PROFILE_HAPPYSKL:
		case PROFILE_CPOKER2:
			igs017_igs031_set_palette_scramble(tarzan_palette_bitswap);
		break;
	}

	if (igs017_igs031_init(DrvTileROM, DrvTileLen, DrvSpriteROM, DrvSpriteLen)) return 1;
	z180_set_video_inputs();

	Z180Init(0);
	Z180Open(0);
	Z180MapMemory(DrvZ180ROM + 0x00000, 0x00000, 0x0dfff, MAP_FETCHARG);
	Z180MapMemory((DrvOpcodeLen > 0) ? (DrvOpcodeROM + 0x00000) : (DrvZ180ROM + 0x00000), 0x00000, 0x0dfff, MAP_FETCHOP);
	Z180MapMemory(DrvZ180ROM + 0x10000, 0x10000, 0x3ffff, MAP_FETCHARG);
	Z180MapMemory((DrvOpcodeLen > 0) ? (DrvOpcodeROM + 0x10000) : (DrvZ180ROM + 0x10000), 0x10000, 0x3ffff, MAP_FETCHOP);

	if (DrvProfile != PROFILE_TJSB) {
		if (DrvWorkRAMLen) {
			Z180MapMemory(DrvNVRAM, 0x0e000, 0x0efff, MAP_READ | MAP_WRITE);
			Z180MapMemory(DrvWorkRAM, 0x0f000, 0x0ffff, MAP_READ | MAP_WRITE);
		} else {
			Z180MapMemory(DrvNVRAM, 0x0e000, 0x0ffff, MAP_READ | MAP_WRITE);
		}
	}

	Z180SetReadHandler(z180_program_read);
	Z180SetWriteHandler(z180_program_write);
	Z180SetReadPortHandler(z180_port_read);
	Z180SetWritePortHandler(z180_port_write);
	Z180Close();

	MSM6295ROM = DrvSndROM;
	if (profile_has_ym2413()) {
		BurnYM2413Init(3579545);
		BurnYM2413SetAllRoutes(0.50, BURN_SND_ROUTE_BOTH);
		MSM6295Init(0, 1000000 / MSM6295_PIN7_HIGH, 1);
	} else {
		MSM6295Init(0, 1000000 / MSM6295_PIN7_HIGH, 0);
	}
	MSM6295SetRoute(0, 1.00, BURN_SND_ROUTE_BOTH);

	return DrvDoReset();
}

static INT32 DrvExit()
{
	igs017_igs031_exit();
	Z180Exit();
	MSM6295Exit(0);
	if (profile_has_ym2413()) {
		BurnYM2413Exit();
	}

	BurnFree(AllMem);
	DrvZ180ROM = DrvOpcodeROM = DrvTileROM = DrvSpriteROM = DrvSndROM = DrvStringROM = DrvNVRAM = DrvPortRAM = DrvWorkRAM = DrvFixedData = NULL;
	DrvZ180Len = DrvOpcodeLen = DrvTileLen = DrvSpriteLen = DrvSndLen = DrvStringLen = DrvNVRAMLen = DrvPortRAMLen = DrvWorkRAMLen = DrvFixedLen = 0;

	return 0;
}

static INT32 DrvDraw()
{
	if (DrvRecalc) {
		igs017_igs031_recalc_palette();
		DrvRecalc = 0;
	}

	igs017_igs031_draw();
	BurnTransferCopy(igs017_igs031_palette());

	return 0;
}

static INT32 DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	Z180NewFrame();
	DrvMakeInputs();

	INT32 nInterleave = 256;
	INT32 nCyclesTotal[1] = { 16000000 / (nBurnFPS / 100) };
	INT32 nCyclesDone[1] = { 0 };

	Z180Open(0);

	for (INT32 i = 0; i < nInterleave; i++) {
		
		nCyclesDone[0] += Z180Run(nCyclesTotal[0] / nInterleave);

		if (i == 240 && igs017_igs031_irq_enable()) {
			Z180SetIRQLine(0, CPU_IRQSTATUS_ACK);
		}
		if (i == 241) {
			Z180SetIRQLine(0, CPU_IRQSTATUS_NONE);
		}
		if (i == 0 && igs017_igs031_nmi_enable()) {
			Z180SetIRQLine(CPU_IRQLINE_NMI, CPU_IRQSTATUS_ACK);
			Z180SetIRQLine(CPU_IRQLINE_NMI, CPU_IRQSTATUS_NONE);
		}
	}

	Z180Close();

	if (pBurnSoundOut) {
		if (profile_has_ym2413()) {
			BurnYM2413Render(pBurnSoundOut, nBurnSoundLen);
		}
		MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029743;
	}

	DrvRecalc = 1;

	if (nAction & ACB_MEMORY_RAM) {
		ba.Data = DrvNVRAM;
		ba.nLen = DrvNVRAMLen;
		ba.nAddress = 0x0e000;
		ba.szName = "NVRAM";
		BurnAcb(&ba);

		ba.Data = DrvPortRAM;
		ba.nLen = DrvPortRAMLen;
		ba.nAddress = 0x0000;
		ba.szName = "Port RAM";
		BurnAcb(&ba);

		if (DrvWorkRAMLen) {
			ba.Data = DrvWorkRAM;
			ba.nLen = DrvWorkRAMLen;
			ba.nAddress = 0x0f000;
			ba.szName = "Work RAM";
			BurnAcb(&ba);
		}
	}

	Z180Scan(nAction);
	MSM6295Scan(nAction, pnMin);
	if (profile_has_ym2413()) {
		BurnYM2413Scan(nAction, pnMin);
	}
	igs017_igs031_scan(nAction, pnMin);

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(DrvMuxAddr);
		SCAN_VAR(DrvInputSelect);
		SCAN_VAR(DrvOkiBank);
		SCAN_VAR(DrvHopperMotor);
		SCAN_VAR(DrvCoin1Pulse);
		SCAN_VAR(DrvCoin1Prev);
		SCAN_VAR(DrvCoin2Pulse);
		SCAN_VAR(DrvCoin2Prev);
		SCAN_VAR(DrvIncVal);
		SCAN_VAR(DrvIncDecVal);
		SCAN_VAR(DrvIncAltVal);
		SCAN_VAR(DrvDswSelect);
		SCAN_VAR(DrvScrambleData);
		SCAN_VAR(DrvStringOffs);
		SCAN_VAR(DrvStringWord);
		SCAN_VAR(DrvStringValue);
		SCAN_VAR(DrvRemapAddr);
		SCAN_VAR(DrvBitswapM3);
		SCAN_VAR(DrvBitswapMF);
		SCAN_VAR(DrvBitswapVal);
		SCAN_VAR(DrvBitswapWord);
	}

	if ((nAction & ACB_DRIVER_DATA) && (nAction & ACB_WRITE)) {
		set_oki_bank(DrvOkiBank);
	}

	return 0;
}

#define IGS017_MAHJONG_INPUTS \
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

static struct BurnInputInfo Igs017InputList[] = {
	{"Coin 1",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"Key In",			BIT_DIGITAL,	DrvJoy1 + 1,	"service2"	},
	{"Key Out",			BIT_DIGITAL,	DrvJoy1 + 2,	"service3"	},
	{"Payout",			BIT_DIGITAL,	DrvJoy1 + 3,	"service4"	},
	{"Book",			BIT_DIGITAL,	DrvJoy1 + 4,	"service"	},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 5,	"diag"		},
	{"Show Credits",	BIT_DIGITAL,	DrvJoy1 + 6,	"service1"	},
	{"Service 3",		BIT_DIGITAL,	DrvJoy1 + 7,	"service5"	},
	{"Start",			BIT_DIGITAL,	DrvJoy2 + 0,	"p1 start"	},
	{"Up",				BIT_DIGITAL,	DrvJoy2 + 1,	"p1 up"		},
	{"Down",			BIT_DIGITAL,	DrvJoy2 + 2,	"p1 down"	},
	{"Left",			BIT_DIGITAL,	DrvJoy2 + 3,	"p1 left"	},
	{"Right",			BIT_DIGITAL,	DrvJoy2 + 4,	"p1 right"	},
	{"Play / Big",		BIT_DIGITAL,	DrvJoy2 + 5,	"p1 fire 1"	},
	{"Bet / Small",		BIT_DIGITAL,	DrvJoy2 + 6,	"p1 fire 2"	},
	{"Function / Double",	BIT_DIGITAL,	DrvJoy2 + 7,	"p1 fire 3"	},
	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	IGS017_MAHJONG_INPUTS,
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Igs017)

static struct BurnInputInfo Igs017zInputList[] = {
	{"Coin 1",          BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In",          BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Service 1",       BIT_DIGITAL,    DrvJoy1 + 6,    "service1"  },
	{"Coin 2",          BIT_DIGITAL,    DrvJoy1 + 7,    "p2 coin"   },
	{"Start 1",         BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Up",              BIT_DIGITAL,    DrvJoy2 + 1,    "p1 up"     },
	{"Down",            BIT_DIGITAL,    DrvJoy2 + 2,    "p1 down"   },
	{"Left",            BIT_DIGITAL,    DrvJoy2 + 3,    "p1 left"   },
	{"Right",           BIT_DIGITAL,    DrvJoy2 + 4,    "p1 right"  },
	{"Button 1",        BIT_DIGITAL,    DrvJoy2 + 5,    "p1 fire 1" },
	{"Button 2",        BIT_DIGITAL,    DrvJoy2 + 6,    "p1 fire 2" },
	{"Button 3",        BIT_DIGITAL,    DrvJoy2 + 7,    "p1 fire 3" },
	{"Start 2 / Deal",  BIT_DIGITAL,    DrvJoy3 + 0,    "p2 start"  },
	{"Hold 1",          BIT_DIGITAL,    DrvJoy3 + 1,    "p1 fire 4" },
	{"Hold 2",          BIT_DIGITAL,    DrvJoy3 + 2,    "p1 fire 5" },
	{"Hold 3",          BIT_DIGITAL,    DrvJoy3 + 3,    "p1 fire 6" },
	{"Hold 4",          BIT_DIGITAL,    DrvJoy3 + 4,    "p1 fire 7" },
	{"Hold 5",          BIT_DIGITAL,    DrvJoy3 + 5,    "p1 fire 8" },
	{"Stop All / Extra",BIT_DIGITAL,    DrvJoy3 + 6,    "p2 fire 1" },
	{"Extra Service",   BIT_DIGITAL,    DrvJoy3 + 7,    "service5"  },
	{"Stop 1",          BIT_DIGITAL,    DrvJoy4 + 0,    "p2 fire 2" },
	{"Stop 2",          BIT_DIGITAL,    DrvJoy4 + 1,    "p2 fire 3" },
	{"Stop 3",          BIT_DIGITAL,    DrvJoy4 + 2,    "p2 fire 4" },
	{"Stop 4",          BIT_DIGITAL,    DrvJoy4 + 3,    "p2 fire 5" },
	{"Big / High",      BIT_DIGITAL,    DrvJoy4 + 4,    "p2 fire 6" },
	{"Small / Low",     BIT_DIGITAL,    DrvJoy4 + 5,    "p2 fire 7" },
	{"Half",            BIT_DIGITAL,    DrvJoy4 + 6,    "p2 fire 8" },
	{"Take / W-Up",     BIT_DIGITAL,    DrvJoy4 + 7,    "p2 fire 9" },
	{"Reset",           BIT_DIGITAL,    &DrvReset,      "reset"     },
	IGS017_MAHJONG_INPUTS,
	{"Dip A",           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Igs017z)

static struct BurnInputInfo IqblockzInputList[] = {
	{"Coin 1",                  BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In",                  BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                 BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                  BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                    BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",          BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Toggle Gambling",         BIT_DIGITAL,    DrvJoy1 + 6,    "service1"  },
	{"Coin 2 / Sensor",         BIT_DIGITAL,    DrvJoy1 + 7,    "p2 coin"   },
	{"Start 1",                 BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Up",                      BIT_DIGITAL,    DrvJoy2 + 1,    "p1 up"     },
	{"Down / Collect Win",      BIT_DIGITAL,    DrvJoy2 + 2,    "p1 down"   },
	{"Left",                    BIT_DIGITAL,    DrvJoy2 + 3,    "p1 left"   },
	{"Right",                   BIT_DIGITAL,    DrvJoy2 + 4,    "p1 right"  },
	{"Hold 1 / Big / Help",     BIT_DIGITAL,    DrvJoy2 + 5,    "p1 fire 1" },
	{"Hold 2 / Double Up",      BIT_DIGITAL,    DrvJoy2 + 6,    "p1 fire 2" },
	{"Bet 1 Credit",            BIT_DIGITAL,    DrvJoy2 + 7,    "p1 fire 7" },
	{"Deal / Last Bet",         BIT_DIGITAL,    DrvJoy3 + 0,    "p1 fire 3" },
	{"Hold 3 / Small",          BIT_DIGITAL,    DrvJoy3 + 3,    "p1 fire 4" },
	{"Hold 4 / Half Double",    BIT_DIGITAL,    DrvJoy3 + 4,    "p1 fire 5" },
	{"Hold 5",                  BIT_DIGITAL,    DrvJoy3 + 5,    "p1 fire 6" },
	{"Reset",                   BIT_DIGITAL,    &DrvReset,       "reset"     },
	{"Dip A",                   BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                   BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                   BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Iqblockz)

static struct BurnInputInfo SpkrformInputList[] = {
	{"Coin 1",                          BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Coin 2",                          BIT_DIGITAL,    DrvJoy1 + 7,    "p2 coin"   },
	{"Key In",                          BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",                  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Hide Gambling (Switch To Formosa)", BIT_DIGITAL,  DrvJoy1 + 6,    "service1"  },
	{"Start / Draw / Take / Formosa Down", BIT_DIGITAL, DrvJoy2 + 0,    "p1 start"  },
	{"Formosa Up",                      BIT_DIGITAL,    DrvJoy2 + 1,    "p1 up"     },
	{"Formosa Left",                    BIT_DIGITAL,    DrvJoy2 + 3,    "p1 left"   },
	{"Formosa Right",                   BIT_DIGITAL,    DrvJoy2 + 4,    "p1 right"  },
	{"Bet / W-Up",                      BIT_DIGITAL,    DrvJoy2 + 7,    "p1 fire 1" },
	{"Start (Formosa)",                 BIT_DIGITAL,    DrvJoy3 + 0,    "p2 start"  },
	{"Hold 1 / High / Formosa Button",  BIT_DIGITAL,    DrvJoy3 + 1,    "p1 fire 2" },
	{"Hold 2 / Double Up",              BIT_DIGITAL,    DrvJoy3 + 2,    "p1 fire 3" },
	{"Hold 3 / Low",                    BIT_DIGITAL,    DrvJoy3 + 3,    "p1 fire 4" },
	{"Hold 4 / Half",                   BIT_DIGITAL,    DrvJoy3 + 4,    "p1 fire 5" },
	{"Hold 5",                          BIT_DIGITAL,    DrvJoy3 + 5,    "p1 fire 6" },
	{"Return To Gambling",              BIT_DIGITAL,    DrvJoy3 + 7,    "service5"  },
	{"Reset",                           BIT_DIGITAL,    &DrvReset,       "reset"     },
	{"Dip A",                           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Spkrform)

static struct BurnInputInfo TjsbInputList[] = {
	{"Coin 1 / Key In",                 BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In / Coin 1",                 BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",                  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Show Credits",                    BIT_DIGITAL,    DrvJoy1 + 6,    "service1"  },
	{"Start 1",                         BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Up",                              BIT_DIGITAL,    DrvJoy2 + 1,    "p1 up"     },
	{"Down",                            BIT_DIGITAL,    DrvJoy2 + 2,    "p1 down"   },
	{"Left",                            BIT_DIGITAL,    DrvJoy2 + 3,    "p1 left"   },
	{"Right",                           BIT_DIGITAL,    DrvJoy2 + 4,    "p1 right"  },
	{"Button 1",                        BIT_DIGITAL,    DrvJoy2 + 5,    "p1 fire 1" },
	{"Button 2 / Bet",                  BIT_DIGITAL,    DrvJoy2 + 6,    "p1 fire 2" },
	{"Button 3 / Function",             BIT_DIGITAL,    DrvJoy2 + 7,    "p1 fire 3" },
	{"Reset",                           BIT_DIGITAL,    &DrvReset,       "reset"     },
	{"Dip A",                           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Tjsb)

static struct BurnInputInfo TarzanInputList[] = {
	{"Coin 1 / Key In",                 BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In / Coin 1",                 BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",                  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Show Credits",                    BIT_DIGITAL,    DrvJoy1 + 6,    "service1"  },
	{"Start / Stop All / Take Score",   BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Up / Stop 1",                     BIT_DIGITAL,    DrvJoy2 + 1,    "p1 up"     },
	{"Down / Stop 2",                   BIT_DIGITAL,    DrvJoy2 + 2,    "p1 down"   },
	{"Left / Stop 3",                   BIT_DIGITAL,    DrvJoy2 + 3,    "p1 left"   },
	{"Right / Stop 4",                  BIT_DIGITAL,    DrvJoy2 + 4,    "p1 right"  },
	{"Button A / Big / Show Odds",      BIT_DIGITAL,    DrvJoy2 + 5,    "p1 fire 1" },
	{"Button B / Bet / Double Up",      BIT_DIGITAL,    DrvJoy2 + 6,    "p1 fire 2" },
	{"Button C / Small / Half",         BIT_DIGITAL,    DrvJoy2 + 7,    "p1 fire 3" },
	{"Reset",                           BIT_DIGITAL,    &DrvReset,       "reset"     },
	IGS017_MAHJONG_INPUTS,
	{"Dip A",                           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Tarzan)

static struct BurnInputInfo StarzanInputList[] = {
	{"Coin 1",                          BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In",                          BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",                  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Coin 2",                          BIT_DIGITAL,    DrvJoy1 + 7,    "p2 coin"   },
	{"Start / 2W-Up / HW-Up",           BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Stop All / Bet",                  BIT_DIGITAL,    DrvJoy3 + 6,    "p1 fire 1" },
	{"Stop 1 / Take",                   BIT_DIGITAL,    DrvJoy4 + 0,    "p1 fire 2" },
	{"Stop 2 / High",                   BIT_DIGITAL,    DrvJoy4 + 1,    "p1 fire 3" },
	{"Stop 3 / Low",                    BIT_DIGITAL,    DrvJoy4 + 2,    "p1 fire 4" },
	{"Stop 4 / W-Up",                   BIT_DIGITAL,    DrvJoy4 + 3,    "p1 fire 5" },
	{"High / Low Alt",                  BIT_DIGITAL,    DrvJoy4 + 4,    "p1 fire 6" },
	{"Bet / 2W-Up Alt",                 BIT_DIGITAL,    DrvJoy4 + 7,    "p1 fire 7" },
	{"Reset",                           BIT_DIGITAL,    &DrvReset,       "reset"     },
	{"Dip A",                           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Starzan)

static struct BurnInputInfo Tarzan103mInputList[] = {
	{"Coin 1",                          BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In",                          BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",                  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Coin 2",                          BIT_DIGITAL,    DrvJoy1 + 7,    "p2 coin"   },
	{"Start / Take",                    BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Up / Stop 1",                     BIT_DIGITAL,    DrvJoy2 + 1,    "p1 up"     },
	{"Down / Stop 2",                   BIT_DIGITAL,    DrvJoy2 + 2,    "p1 down"   },
	{"Left / Stop 3",                   BIT_DIGITAL,    DrvJoy2 + 3,    "p1 left"   },
	{"Right / Stop 4",                  BIT_DIGITAL,    DrvJoy2 + 4,    "p1 right"  },
	{"Button A / Big",                  BIT_DIGITAL,    DrvJoy2 + 5,    "p1 fire 1" },
	{"Button B / Bet",                  BIT_DIGITAL,    DrvJoy2 + 6,    "p1 fire 2" },
	{"Button C / Small",                BIT_DIGITAL,    DrvJoy2 + 7,    "p1 fire 3" },
	{"Reset",                           BIT_DIGITAL,    &DrvReset,       "reset"     },
	{"Dip A",                           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Tarzan103m)

static struct BurnInputInfo HappysklInputList[] = {
	{"Coin 1",                          BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In",                          BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",                  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Coin 2",                          BIT_DIGITAL,    DrvJoy1 + 7,    "p2 coin"   },
	{"Start",                           BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Hold 1",                          BIT_DIGITAL,    DrvJoy2 + 5,    "p1 fire 1" },
	{"Hold 2",                          BIT_DIGITAL,    DrvJoy3 + 0,    "p1 fire 2" },
	{"Hold 3",                          BIT_DIGITAL,    DrvJoy3 + 1,    "p1 fire 3" },
	{"Hold 4",                          BIT_DIGITAL,    DrvJoy3 + 2,    "p1 fire 4" },
	{"Hold 5",                          BIT_DIGITAL,    DrvJoy2 + 7,    "p1 fire 5" },
	{"Reset",                           BIT_DIGITAL,    &DrvReset,       "reset"     },
	{"Dip A",                           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Happyskl)

static struct BurnInputInfo Cpoker2InputList[] = {
	{"Coin 1",                          BIT_DIGITAL,    DrvJoy1 + 0,    "p1 coin"   },
	{"Key In",                          BIT_DIGITAL,    DrvJoy1 + 1,    "service2"  },
	{"Key Out",                         BIT_DIGITAL,    DrvJoy1 + 2,    "service3"  },
	{"Payout",                          BIT_DIGITAL,    DrvJoy1 + 3,    "service4"  },
	{"Book",                            BIT_DIGITAL,    DrvJoy1 + 4,    "service"   },
	{"Service / Test",                  BIT_DIGITAL,    DrvJoy1 + 5,    "diag"      },
	{"Coin 2",                          BIT_DIGITAL,    DrvJoy1 + 7,    "p2 coin"   },
	{"Start",                           BIT_DIGITAL,    DrvJoy2 + 0,    "p1 start"  },
	{"Hold 1",                          BIT_DIGITAL,    DrvJoy3 + 1,    "p1 fire 1" },
	{"Hold 2",                          BIT_DIGITAL,    DrvJoy3 + 2,    "p1 fire 2" },
	{"Hold 3",                          BIT_DIGITAL,    DrvJoy3 + 3,    "p1 fire 3" },
	{"Hold 4",                          BIT_DIGITAL,    DrvJoy3 + 4,    "p1 fire 4" },
	{"Hold 5",                          BIT_DIGITAL,    DrvJoy3 + 5,    "p1 fire 5" },
	{"Reset",                           BIT_DIGITAL,    &DrvReset,       "reset"     },
	{"Dip A",                           BIT_DIPSWITCH,  DrvDips + 0,    "dip"       },
	{"Dip B",                           BIT_DIPSWITCH,  DrvDips + 1,    "dip"       },
	{"Dip C",                           BIT_DIPSWITCH,  DrvDips + 2,    "dip"       },
};

STDINPUTINFO(Cpoker2)

#define IGS017_DIP_DEFAULTS(_a, _b, _c) \
	{0x3b, 0xff, 0xff, _a, NULL}, \
	{0x3c, 0xff, 0xff, _b, NULL}, \
	{0x3d, 0xff, 0xff, _c, NULL},

#define IQBLOCKZ_DIP_DEFAULTS(_a, _b, _c) \
	{0x15, 0xff, 0xff, _a, NULL}, \
	{0x16, 0xff, 0xff, _b, NULL}, \
	{0x17, 0xff, 0xff, _c, NULL},

#define SPKRFORM_DIP_DEFAULTS(_a, _b, _c) \
	{0x15, 0xff, 0xff, _a, NULL}, \
	{0x16, 0xff, 0xff, _b, NULL}, \
	{0x17, 0xff, 0xff, _c, NULL},

#define TJSB_DIP_DEFAULTS(_a, _b, _c) \
	{0x10, 0xff, 0xff, _a, NULL}, \
	{0x11, 0xff, 0xff, _b, NULL}, \
	{0x12, 0xff, 0xff, _c, NULL},

#define TARZAN_DIP_DEFAULTS(_a, _b, _c) \
	{0x2a, 0xff, 0xff, _a, NULL}, \
	{0x2b, 0xff, 0xff, _b, NULL}, \
	{0x2c, 0xff, 0xff, _c, NULL},

#define STARZAN_DIP_DEFAULTS(_a, _b, _c) \
	{0x10, 0xff, 0xff, _a, NULL}, \
	{0x11, 0xff, 0xff, _b, NULL}, \
	{0x12, 0xff, 0xff, _c, NULL},

#define HAPPYSKL_DIP_DEFAULTS(_a, _b, _c) \
	{0x0e, 0xff, 0xff, _a, NULL}, \
	{0x0f, 0xff, 0xff, _b, NULL}, \
	{0x10, 0xff, 0xff, _c, NULL},

static struct BurnDIPInfo IqblockaDIPList[] = {
	IQBLOCKZ_DIP_DEFAULTS(0xfe, 0xdf, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x15, 0x01, 0x01, 0x01, "Off"},
	{0x15, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Hold Mode"},
	{0x15, 0x01, 0x02, 0x02, "In Win"},
	{0x15, 0x01, 0x02, 0x00, "Always"},
	{0   , 0xfe, 0   ,    2, "Max Credit"},
	{0x15, 0x01, 0x04, 0x04, "4000"},
	{0x15, 0x01, 0x04, 0x00, "None"},
	{0   , 0xfe, 0   ,    8, "Cigarette Bet"},
	{0x15, 0x01, 0x38, 0x38, "1"},
	{0x15, 0x01, 0x38, 0x30, "10"},
	{0x15, 0x01, 0x38, 0x28, "20"},
	{0x15, 0x01, 0x38, 0x20, "50"},
	{0x15, 0x01, 0x38, 0x18, "80"},
	{0x15, 0x01, 0x38, 0x10, "100"},
	{0x15, 0x01, 0x38, 0x08, "120"},
	{0x15, 0x01, 0x38, 0x00, "150"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x15, 0x01, 0xc0, 0xc0, "1"},
	{0x15, 0x01, 0xc0, 0x80, "10"},
	{0x15, 0x01, 0xc0, 0x40, "20"},
	{0x15, 0x01, 0xc0, 0x00, "50"},

	{0   , 0xfe, 0   ,    8, "Key In"},
	{0x16, 0x01, 0x07, 0x07, "10"},
	{0x16, 0x01, 0x07, 0x06, "20"},
	{0x16, 0x01, 0x07, 0x05, "40"},
	{0x16, 0x01, 0x07, 0x04, "50"},
	{0x16, 0x01, 0x07, 0x03, "100"},
	{0x16, 0x01, 0x07, 0x02, "200"},
	{0x16, 0x01, 0x07, 0x01, "250"},
	{0x16, 0x01, 0x07, 0x00, "500"},
	{0   , 0xfe, 0   ,    2, "Key Out"},
	{0x16, 0x01, 0x08, 0x08, "10"},
	{0x16, 0x01, 0x08, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Open Mode"},
	{0x16, 0x01, 0x10, 0x10, "Gaming (Gambling)"},
	{0x16, 0x01, 0x10, 0x00, "Amuse"},
	{0   , 0xfe, 0   ,    2, "Demo Game"},
	{0x16, 0x01, 0x20, 0x20, "Off"},
	{0x16, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    4, "Bonus Base"},
	{0x16, 0x01, 0xc0, 0xc0, "100"},
	{0x16, 0x01, 0xc0, 0x80, "200"},
	{0x16, 0x01, 0xc0, 0x40, "300"},
	{0x16, 0x01, 0xc0, 0x00, "400"},

	{0   , 0xfe, 0   ,    4, "Win Up Pool"},
	{0x17, 0x01, 0x03, 0x03, "300"},
	{0x17, 0x01, 0x03, 0x02, "500"},
	{0x17, 0x01, 0x03, 0x01, "800"},
	{0x17, 0x01, 0x03, 0x00, "1000"},
	{0   , 0xfe, 0   ,    4, "Max Double Up"},
	{0x17, 0x01, 0x0c, 0x0c, "20000"},
	{0x17, 0x01, 0x0c, 0x08, "30000"},
	{0x17, 0x01, 0x0c, 0x04, "40000"},
	{0x17, 0x01, 0x0c, 0x00, "50000"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x17, 0x01, 0x10, 0x10, "A,J,Q,K"},
	{0x17, 0x01, 0x10, 0x00, "Number"},
	{0   , 0xfe, 0   ,    2, "Show Title"},
	{0x17, 0x01, 0x20, 0x20, "Yes"},
	{0x17, 0x01, 0x20, 0x00, "No"},
	{0   , 0xfe, 0   ,    2, "Double Up"},
	{0x17, 0x01, 0x40, 0x00, "Off"},
	{0x17, 0x01, 0x40, 0x40, "On"},
	{0   , 0xfe, 0   ,    2, "CG Select"},
	{0x17, 0x01, 0x80, 0x80, "Low"},
	{0x17, 0x01, 0x80, 0x00, "High"},
};

STDDIPINFO(Iqblocka)

static struct BurnDIPInfo IqblockfDIPList[] = {
	IQBLOCKZ_DIP_DEFAULTS(0xfe, 0xdf, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x15, 0x01, 0x01, 0x01, "Off"},
	{0x15, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Hold Mode"},
	{0x15, 0x01, 0x02, 0x02, "In Win"},
	{0x15, 0x01, 0x02, 0x00, "Always"},
	{0   , 0xfe, 0   ,    4, "Coin In"},
	{0x15, 0x01, 0x0c, 0x0c, "1"},
	{0x15, 0x01, 0x0c, 0x08, "5"},
	{0x15, 0x01, 0x0c, 0x04, "10"},
	{0x15, 0x01, 0x0c, 0x00, "20"},
	{0   , 0xfe, 0   ,    4, "Key In"},
	{0x15, 0x01, 0x30, 0x30, "10"},
	{0x15, 0x01, 0x30, 0x20, "100"},
	{0x15, 0x01, 0x30, 0x10, "200"},
	{0x15, 0x01, 0x30, 0x00, "500"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x15, 0x01, 0xc0, 0xc0, "1"},
	{0x15, 0x01, 0xc0, 0x80, "2"},
	{0x15, 0x01, 0xc0, 0x40, "5"},
	{0x15, 0x01, 0xc0, 0x00, "10"},

	{0   , 0xfe, 0   ,    4, "Max Bet"},
	{0x16, 0x01, 0x03, 0x03, "5"},
	{0x16, 0x01, 0x03, 0x02, "10"},
	{0x16, 0x01, 0x03, 0x01, "20"},
	{0x16, 0x01, 0x03, 0x00, "50"},
	{0   , 0xfe, 0   ,    2, "Register"},
	{0x16, 0x01, 0x04, 0x04, "Off"},
	{0x16, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Key Out Base"},
	{0x16, 0x01, 0x08, 0x08, "1"},
	{0x16, 0x01, 0x08, 0x00, "10"},
	{0   , 0xfe, 0   ,    2, "Open Mode"},
	{0x16, 0x01, 0x10, 0x10, "Gaming (Gambling)"},
	{0x16, 0x01, 0x10, 0x00, "Amuse"},
	{0   , 0xfe, 0   ,    2, "Demo Game"},
	{0x16, 0x01, 0x20, 0x20, "Off"},
	{0x16, 0x01, 0x20, 0x00, "On"},

	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x17, 0x01, 0x10, 0x10, "A,J,Q,K"},
	{0x17, 0x01, 0x10, 0x00, "Number"},
	{0   , 0xfe, 0   ,    2, "CG Select"},
	{0x17, 0x01, 0x80, 0x80, "Low"},
	{0x17, 0x01, 0x80, 0x00, "High"},
};

STDDIPINFO(Iqblockf)

static struct BurnDIPInfo Genius6DIPList[] = {
	IQBLOCKZ_DIP_DEFAULTS(0xfe, 0xdf, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x15, 0x01, 0x01, 0x01, "Off"},
	{0x15, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Auto Hold"},
	{0x15, 0x01, 0x02, 0x02, "No"},
	{0x15, 0x01, 0x02, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    4, "Coin In"},
	{0x15, 0x01, 0x0c, 0x0c, "1"},
	{0x15, 0x01, 0x0c, 0x08, "10"},
	{0x15, 0x01, 0x0c, 0x04, "20"},
	{0x15, 0x01, 0x0c, 0x00, "50"},
	{0   , 0xfe, 0   ,    4, "Key In"},
	{0x15, 0x01, 0x30, 0x30, "10"},
	{0x15, 0x01, 0x30, 0x20, "100"},
	{0x15, 0x01, 0x30, 0x10, "200"},
	{0x15, 0x01, 0x30, 0x00, "500"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x15, 0x01, 0xc0, 0xc0, "1"},
	{0x15, 0x01, 0xc0, 0x80, "2"},
	{0x15, 0x01, 0xc0, 0x40, "5"},
	{0x15, 0x01, 0xc0, 0x00, "10"},

	{0   , 0xfe, 0   ,    4, "Max Bet"},
	{0x16, 0x01, 0x03, 0x03, "5"},
	{0x16, 0x01, 0x03, 0x02, "10"},
	{0x16, 0x01, 0x03, 0x01, "20"},
	{0x16, 0x01, 0x03, 0x00, "50"},
	{0   , 0xfe, 0   ,    2, "Key Out"},
	{0x16, 0x01, 0x08, 0x08, "1"},
	{0x16, 0x01, 0x08, 0x00, "10"},
	{0   , 0xfe, 0   ,    2, "Open Mode"},
	{0x16, 0x01, 0x10, 0x10, "Gaming (Gambling)"},
	{0x16, 0x01, 0x10, 0x00, "Amuse"},
	{0   , 0xfe, 0   ,    2, "Demo Game"},
	{0x16, 0x01, 0x20, 0x20, "Off"},
	{0x16, 0x01, 0x20, 0x00, "On"},

	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x17, 0x01, 0x10, 0x10, "A,J,Q,K"},
	{0x17, 0x01, 0x10, 0x00, "Number"},
};

STDDIPINFO(Genius6)

static struct BurnDIPInfo TjsbDIPList[] = {
	TJSB_DIP_DEFAULTS(0xff, 0xff, 0xf5)

	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x3b, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x3b, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x3b, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x3b, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x3b, 0x01, 0x0c, 0x0c, "10"},
	{0x3b, 0x01, 0x0c, 0x08, "20"},
	{0x3b, 0x01, 0x0c, 0x04, "50"},
	{0x3b, 0x01, 0x0c, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x3b, 0x01, 0x10, 0x10, "1000"},
	{0x3b, 0x01, 0x10, 0x00, "5000"},
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x3b, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x3b, 0x01, 0x20, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x3b, 0x01, 0x40, 0x40, "Return Coins"},
	{0x3b, 0x01, 0x40, 0x00, "Key-Out"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x3b, 0x01, 0x80, 0x00, "Off"},
	{0x3b, 0x01, 0x80, 0x80, "On"},

	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x3c, 0x01, 0x03, 0x03, "1000"},
	{0x3c, 0x01, 0x03, 0x02, "2000"},
	{0x3c, 0x01, 0x03, 0x01, "3000"},
	{0x3c, 0x01, 0x03, 0x00, "4000"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x3c, 0x01, 0x0c, 0x0c, "1"},
	{0x3c, 0x01, 0x0c, 0x08, "3"},
	{0x3c, 0x01, 0x0c, 0x04, "5"},
	{0x3c, 0x01, 0x0c, 0x00, "10"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x3c, 0x01, 0x10, 0x00, "Off"},
	{0x3c, 0x01, 0x10, 0x10, "On"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x3c, 0x01, 0x20, 0x20, "Numbers"},
	{0x3c, 0x01, 0x20, 0x00, "Blocks"},

	{0   , 0xfe, 0   ,    2, "Double Up Game Protection Check"},
	{0x3d, 0x01, 0xff, 0xf5, "Off"},
	{0x3d, 0x01, 0xff, 0xff, "On"},
};

STDDIPINFO(Tjsb)

static struct BurnDIPInfo TarzanDIPList[] = {
	TARZAN_DIP_DEFAULTS(0xfe, 0xff, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x3b, 0x01, 0x01, 0x01, "Off"},
	{0x3b, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    8, "Coinage"},
	{0x3b, 0x01, 0x0e, 0x0e, "1 Coin 1 Credit"},
	{0x3b, 0x01, 0x0e, 0x0c, "1 Coin 2 Credits"},
	{0x3b, 0x01, 0x0e, 0x0a, "1 Coin 4 Credits"},
	{0x3b, 0x01, 0x0e, 0x08, "1 Coin 5 Credits"},
	{0x3b, 0x01, 0x0e, 0x06, "1 Coin 10 Credits"},
	{0x3b, 0x01, 0x0e, 0x04, "1 Coin 20 Credits"},
	{0x3b, 0x01, 0x0e, 0x02, "1 Coin 50 Credits"},
	{0x3b, 0x01, 0x0e, 0x00, "1 Coin 100 Credits"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x3b, 0x01, 0x30, 0x30, "100"},
	{0x3b, 0x01, 0x30, 0x20, "200"},
	{0x3b, 0x01, 0x30, 0x10, "500"},
	{0x3b, 0x01, 0x30, 0x00, "1000"},
	{0   , 0xfe, 0   ,    4, "Key-out Rate"},
	{0x3b, 0x01, 0xc0, 0xc0, "1"},
	{0x3b, 0x01, 0xc0, 0x80, "10"},
	{0x3b, 0x01, 0xc0, 0x40, "100"},
	{0x3b, 0x01, 0xc0, 0x00, "100 (2)"},

	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x3c, 0x01, 0x01, 0x01, "Coin Acceptor"},
	{0x3c, 0x01, 0x01, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    8, "Minimum Bet"},
	{0x3c, 0x01, 0x0e, 0x0e, "5"},
	{0x3c, 0x01, 0x0e, 0x0c, "25"},
	{0x3c, 0x01, 0x0e, 0x0a, "50"},
	{0x3c, 0x01, 0x0e, 0x08, "75"},
	{0x3c, 0x01, 0x0e, 0x06, "100"},
	{0x3c, 0x01, 0x0e, 0x04, "125"},
	{0x3c, 0x01, 0x0e, 0x02, "125 (2)"},
	{0x3c, 0x01, 0x0e, 0x00, "125 (3)"},
	{0   , 0xfe, 0   ,    4, "Bonus Bet"},
	{0x3c, 0x01, 0x30, 0x30, "75"},
	{0x3c, 0x01, 0x30, 0x20, "125"},
	{0x3c, 0x01, 0x30, 0x10, "200"},
	{0x3c, 0x01, 0x30, 0x00, "250"},
	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x3c, 0x01, 0xc0, 0xc0, "50,000"},
	{0x3c, 0x01, 0xc0, 0x80, "100,000"},
	{0x3c, 0x01, 0xc0, 0x40, "150,000"},
	{0x3c, 0x01, 0xc0, 0x00, "200,000"},

	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x3d, 0x01, 0x01, 0x01, "Mahjong"},
	{0x3d, 0x01, 0x01, 0x00, "Joystick"},
	{0   , 0xfe, 0   ,    2, "Background Color"},
	{0x3d, 0x01, 0x02, 0x02, "Black and White"},
	{0x3d, 0x01, 0x02, 0x00, "Color"},
	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x3d, 0x01, 0x04, 0x04, "Off"},
	{0x3d, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x3d, 0x01, 0x08, 0x08, "Numbers"},
	{0x3d, 0x01, 0x08, 0x00, "Circle Tiles"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x3d, 0x01, 0x10, 0x10, "Off"},
	{0x3d, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x3d, 0x01, 0x20, 0x20, "Return Coins"},
	{0x3d, 0x01, 0x20, 0x00, "Key-Out"},
};

STDDIPINFO(Tarzan)

static struct BurnDIPInfo StarzanDIPList[] = {
	STARZAN_DIP_DEFAULTS(0xf7, 0xff, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x3b, 0x01, 0x01, 0x01, "Off"},
	{0x3b, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "System Limit"},
	{0x3b, 0x01, 0x02, 0x02, "Unlimited"},
	{0x3b, 0x01, 0x02, 0x00, "Limited"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x3b, 0x01, 0x04, 0x04, "Off"},
	{0x3b, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Back Color"},
	{0x3b, 0x01, 0x08, 0x08, "Black"},
	{0x3b, 0x01, 0x08, 0x00, "Color"},
	{0   , 0xfe, 0   ,    2, "Auto Stop"},
	{0x3b, 0x01, 0x10, 0x10, "No"},
	{0x3b, 0x01, 0x10, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "Control Panel Type"},
	{0x3b, 0x01, 0x20, 0x20, "Mode 1"},
	{0x3b, 0x01, 0x20, 0x00, "Mode 2"},
	{0   , 0xfe, 0   ,    2, "Credit Level"},
	{0x3b, 0x01, 0x40, 0x40, "Off"},
	{0x3b, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Show Odds Table"},
	{0x3b, 0x01, 0x80, 0x00, "Off"},
	{0x3b, 0x01, 0x80, 0x80, "On"},

	{0   , 0xfe, 0   ,    2, "Normal Level"},
	{0x3c, 0x01, 0x01, 0x01, "Low"},
	{0x3c, 0x01, 0x01, 0x00, "High"},
};

STDDIPINFO(Starzan)

static struct BurnDIPInfo Tarzan202faDIPList[] = {
	STARZAN_DIP_DEFAULTS(0xf7, 0xff, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x3b, 0x01, 0x01, 0x01, "Off"},
	{0x3b, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "System Limit"},
	{0x3b, 0x01, 0x02, 0x02, "Unlimited"},
	{0x3b, 0x01, 0x02, 0x00, "Limited"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x3b, 0x01, 0x04, 0x04, "Off"},
	{0x3b, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Back Color"},
	{0x3b, 0x01, 0x08, 0x08, "Black"},
	{0x3b, 0x01, 0x08, 0x00, "Color"},
	{0   , 0xfe, 0   ,    2, "Auto Stop"},
	{0x3b, 0x01, 0x10, 0x10, "No"},
	{0x3b, 0x01, 0x10, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "Control Panel Type"},
	{0x3b, 0x01, 0x20, 0x20, "Mode 1"},
	{0x3b, 0x01, 0x20, 0x00, "Mode 2"},
};

STDDIPINFO(Tarzan202fa)

static struct BurnDIPInfo Tarzan103mDIPList[] = {
	STARZAN_DIP_DEFAULTS(0xf7, 0xff, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x3b, 0x01, 0x01, 0x01, "Off"},
	{0x3b, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "System Limit"},
	{0x3b, 0x01, 0x02, 0x02, "Unlimited"},
	{0x3b, 0x01, 0x02, 0x00, "Limited"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x3b, 0x01, 0x04, 0x04, "Off"},
	{0x3b, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Back Color"},
	{0x3b, 0x01, 0x08, 0x08, "Black"},
	{0x3b, 0x01, 0x08, 0x00, "Color"},
};

STDDIPINFO(Tarzan103m)

static struct BurnDIPInfo HappysklDIPList[] = {
	HAPPYSKL_DIP_DEFAULTS(0xff, 0xff, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x3b, 0x01, 0x01, 0x01, "Off"},
	{0x3b, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "System Limit"},
	{0x3b, 0x01, 0x02, 0x02, "Unlimited"},
	{0x3b, 0x01, 0x02, 0x00, "Limited"},
	{0   , 0xfe, 0   ,    2, "W-Up Game"},
	{0x3b, 0x01, 0x04, 0x04, "Off"},
	{0x3b, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Game Speed"},
	{0x3b, 0x01, 0x08, 0x08, "Normal"},
	{0x3b, 0x01, 0x08, 0x00, "Quick"},
	{0   , 0xfe, 0   ,    2, "Replay Game"},
	{0x3b, 0x01, 0x10, 0x10, "Off"},
	{0x3b, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Show Table"},
	{0x3b, 0x01, 0x20, 0x20, "Off"},
	{0x3b, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Skill"},
	{0x3b, 0x01, 0x40, 0x40, "Off"},
	{0x3b, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Payment Type"},
	{0x3b, 0x01, 0x80, 0x80, "Normal"},
	{0x3b, 0x01, 0x80, 0x00, "Automatic"},

	{0   , 0xfe, 0   ,    2, "Auto Payment"},
	{0x3c, 0x01, 0x01, 0x01, "1"},
	{0x3c, 0x01, 0x01, 0x00, "10"},
	{0   , 0xfe, 0   ,    4, "Enable Payment"},
	{0x3c, 0x01, 0x06, 0x06, "Everything"},
	{0x3c, 0x01, 0x06, 0x04, "1/Tickets"},
	{0x3c, 0x01, 0x06, 0x02, "10/Tickets"},
	{0x3c, 0x01, 0x06, 0x00, "1/Tickets (Alt)"},
	{0   , 0xfe, 0   ,    2, "Select Balls"},
	{0x3c, 0x01, 0x08, 0x08, "Off"},
	{0x3c, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Select Cubes"},
	{0x3c, 0x01, 0x10, 0x10, "Off"},
	{0x3c, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Select Cans"},
	{0x3c, 0x01, 0x20, 0x20, "Off"},
	{0x3c, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Select Cards"},
	{0x3c, 0x01, 0x40, 0x40, "Off"},
	{0x3c, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x3c, 0x01, 0x80, 0x80, "Off"},
	{0x3c, 0x01, 0x80, 0x00, "On"},

	{0   , 0xfe, 0   ,    2, "Level Limit"},
	{0x3d, 0x01, 0x01, 0x01, "Low"},
	{0x3d, 0x01, 0x01, 0x00, "High"},
	{0   , 0xfe, 0   ,    2, "Background"},
	{0x3d, 0x01, 0x02, 0x02, "Off"},
	{0x3d, 0x01, 0x02, 0x00, "On"},
};

STDDIPINFO(Happyskl)

static struct BurnDIPInfo Cpoker2DIPList[] = {
	HAPPYSKL_DIP_DEFAULTS(0x7f, 0xff, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x3b, 0x01, 0x01, 0x01, "Off"},
	{0x3b, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "System Limit"},
	{0x3b, 0x01, 0x02, 0x02, "Unlimited"},
	{0x3b, 0x01, 0x02, 0x00, "Limited"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x3b, 0x01, 0x04, 0x04, "Off"},
	{0x3b, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Type"},
	{0x3b, 0x01, 0x08, 0x08, "Big-Small"},
	{0x3b, 0x01, 0x08, 0x00, "Red-Black"},
	{0   , 0xfe, 0   ,    2, "Game Speed"},
	{0x3b, 0x01, 0x10, 0x10, "Normal"},
	{0x3b, 0x01, 0x10, 0x00, "Quick"},
	{0   , 0xfe, 0   ,    2, "Card Type"},
	{0x3b, 0x01, 0x20, 0x20, "Poker"},
	{0x3b, 0x01, 0x20, 0x00, "Symbol"},
	{0   , 0xfe, 0   ,    2, "Sexy Girl"},
	{0x3b, 0x01, 0x40, 0x40, "Off"},
	{0x3b, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Show Title"},
	{0x3b, 0x01, 0x80, 0x00, "Yes"},
	{0x3b, 0x01, 0x80, 0x80, "No"},

	{0   , 0xfe, 0   ,    2, "Show Hold"},
	{0x3c, 0x01, 0x01, 0x01, "Off"},
	{0x3c, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x3c, 0x01, 0x02, 0x02, "Number"},
	{0x3c, 0x01, 0x02, 0x00, "A,J,Q,K"},
};

STDDIPINFO(Cpoker2)

static struct BurnDIPInfo SpkrformDIPList[] = {
	SPKRFORM_DIP_DEFAULTS(0xfe, 0xff, 0xff)

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x15, 0x01, 0x01, 0x01, "Off"},
	{0x15, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    8, "Credits Per Coin"},
	{0x15, 0x01, 0x1c, 0x1c, "1"},
	{0x15, 0x01, 0x1c, 0x18, "4"},
	{0x15, 0x01, 0x1c, 0x14, "5"},
	{0x15, 0x01, 0x1c, 0x10, "10"},
	{0x15, 0x01, 0x1c, 0x0c, "20"},
	{0x15, 0x01, 0x1c, 0x08, "40"},
	{0x15, 0x01, 0x1c, 0x04, "50"},
	{0x15, 0x01, 0x1c, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Hopper"},
	{0x15, 0x01, 0x20, 0x20, "No"},
	{0x15, 0x01, 0x20, 0x00, "Yes"},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x16, 0x01, 0x03, 0x03, "1"},
	{0x16, 0x01, 0x03, 0x02, "5"},
	{0x16, 0x01, 0x03, 0x01, "10"},
	{0x16, 0x01, 0x03, 0x00, "20"},
	{0   , 0xfe, 0   ,    8, "Credits Per Note"},
	{0x16, 0x01, 0x1c, 0x1c, "10"},
	{0x16, 0x01, 0x1c, 0x18, "20"},
	{0x16, 0x01, 0x1c, 0x14, "40"},
	{0x16, 0x01, 0x1c, 0x10, "50"},
	{0x16, 0x01, 0x1c, 0x0c, "100"},
	{0x16, 0x01, 0x1c, 0x08, "200"},
	{0x16, 0x01, 0x1c, 0x04, "250"},
	{0x16, 0x01, 0x1c, 0x00, "500"},

	{0   , 0xfe, 0   ,    4, "Unknown"},
	{0x17, 0x01, 0x03, 0x03, "100"},
	{0x17, 0x01, 0x03, 0x02, "200"},
	{0x17, 0x01, 0x03, 0x01, "300"},
	{0x17, 0x01, 0x03, 0x00, "400"},
	{0   , 0xfe, 0   ,    4, "Win Up Pool"},
	{0x17, 0x01, 0x0c, 0x0c, "300"},
	{0x17, 0x01, 0x0c, 0x08, "500"},
	{0x17, 0x01, 0x0c, 0x04, "800 (1)"},
	{0x17, 0x01, 0x0c, 0x00, "800 (2)"},
};

STDDIPINFO(Spkrform)

static struct BurnRomInfo iqblockaRomDesc[] = {
	{ "v.u18",              0x040000, 0x2e2b7d43, 1 | BRF_PRG | BRF_ESS },
	{ "cg.u7",              0x080000, 0xcb48a66e, 2 | BRF_GRA },
	{ "text.u8",            0x080000, 0x48c4f4e6, 3 | BRF_GRA },
	{ "speech.u17",         0x040000, 0xd9e3d39f, 4 | BRF_SND },
	{ "igs_fixed_data.key", 0x000015, 0x9159ecbf, 7 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(iqblocka)
STD_ROM_FN(iqblocka)

static INT32 IqblockaInit()
{
	INT32 ret = common_init(PROFILE_IQBLOCK, decrypt_iqblocka);
	if (ret) return ret;

	DrvBitswapValXor = 0x15d6;
	DrvBitswapMFBits[0] = 3; DrvBitswapMFBits[1] = 5; DrvBitswapMFBits[2] = 9; DrvBitswapMFBits[3] = 11;
	DrvBitswapM3Bits[0][0] = (UINT8)~5;  DrvBitswapM3Bits[0][1] = 8;         DrvBitswapM3Bits[0][2] = (UINT8)~10; DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = 3;         DrvBitswapM3Bits[1][1] = (UINT8)~8;  DrvBitswapM3Bits[1][2] = (UINT8)~12; DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = 2;         DrvBitswapM3Bits[2][1] = (UINT8)~6;  DrvBitswapM3Bits[2][2] = (UINT8)~11; DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = 0;         DrvBitswapM3Bits[3][1] = (UINT8)~1;  DrvBitswapM3Bits[3][2] = (UINT8)~3;  DrvBitswapM3Bits[3][3] = (UINT8)~15;

	return 0;
}

struct BurnDriver BurnDrvIqblocka = {
	"iqblocka", NULL, NULL, NULL, "1996",
	"Shuzi Leyuan (China, V127M, gambling)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_PUZZLE, 0,
	ROM_NAME(iqblocka), INPUT_FN(Iqblockz), DIP_FN(Iqblocka),
	IqblockaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo iqblockfRomDesc[] = {
	{ "v113fr.u18",         0x040000, 0x346c68af, 1 | BRF_PRG | BRF_ESS },
	{ "cg.u7",              0x080000, 0xcb48a66e, 2 | BRF_GRA },
	{ "text.u8",            0x080000, 0x48c4f4e6, 3 | BRF_GRA },
	{ "sp.u17",             0x040000, 0x71357845, 4 | BRF_SND },
	{ "igs_fixed_data.key", 0x000015, 0x9159ecbf, 7 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(iqblockf)
STD_ROM_FN(iqblockf)

static INT32 IqblockfInit()
{
	INT32 ret = common_init(PROFILE_IQBLOCK, decrypt_iqblocka);
	if (ret) return ret;

	DrvBitswapValXor = 0x15d6;
	DrvBitswapMFBits[0] = 0; DrvBitswapMFBits[1] = 5; DrvBitswapMFBits[2] = 9; DrvBitswapMFBits[3] = 13;
	DrvBitswapM3Bits[0][0] = (UINT8)~5;  DrvBitswapM3Bits[0][1] = 8;         DrvBitswapM3Bits[0][2] = (UINT8)~10; DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = 3;         DrvBitswapM3Bits[1][1] = (UINT8)~8;  DrvBitswapM3Bits[1][2] = (UINT8)~12; DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = 2;         DrvBitswapM3Bits[2][1] = (UINT8)~6;  DrvBitswapM3Bits[2][2] = (UINT8)~11; DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = 0;         DrvBitswapM3Bits[3][1] = (UINT8)~1;  DrvBitswapM3Bits[3][2] = (UINT8)~3;  DrvBitswapM3Bits[3][3] = (UINT8)~15;

	return 0;
}

struct BurnDriver BurnDrvIqblockf = {
	"iqblockf", "iqblocka", NULL, NULL, "1997",
	"IQ Block (V113FR, gambling)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_PUZZLE, 0,
	ROM_NAME(iqblockf), INPUT_FN(Iqblockz), DIP_FN(Iqblockf),
	IqblockfInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo genius6RomDesc[] = {
	{ "genius6_v110f.u18",  0x040000, 0x2630ad44, 1 | BRF_PRG | BRF_ESS },
	{ "genius6_cg.u7",      0x080000, 0x1842d021, 2 | BRF_GRA },
	{ "text.u8",            0x080000, 0x48c4f4e6, 3 | BRF_GRA },
	{ "speech.u17",         0x040000, 0xd9e3d39f, 4 | BRF_SND },
	{ "igs_fixed_data.key", 0x000015, 0x9159ecbf, 7 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(genius6)
STD_ROM_FN(genius6)

static INT32 Genius6Init()
{
	INT32 ret = common_init(PROFILE_GENIUS6, decrypt_iqblocka);
	if (ret) return ret;

	DrvBitswapValXor = 0x15d6;
	DrvBitswapMFBits[0] = 2; DrvBitswapMFBits[1] = 7; DrvBitswapMFBits[2] = 9; DrvBitswapMFBits[3] = 13;
	DrvBitswapM3Bits[0][0] = (UINT8)~5;  DrvBitswapM3Bits[0][1] = 6;         DrvBitswapM3Bits[0][2] = (UINT8)~7;  DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = 1;         DrvBitswapM3Bits[1][1] = (UINT8)~6;  DrvBitswapM3Bits[1][2] = (UINT8)~9;  DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = 4;         DrvBitswapM3Bits[2][1] = (UINT8)~8;  DrvBitswapM3Bits[2][2] = (UINT8)~12; DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = 3;         DrvBitswapM3Bits[3][1] = (UINT8)~5;  DrvBitswapM3Bits[3][2] = (UINT8)~6;  DrvBitswapM3Bits[3][3] = (UINT8)~15;

	return 0;
}

struct BurnDriver BurnDrvGenius6 = {
	"genius6", NULL, NULL, NULL, "1998",
	"Genius 6 (V110F)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_PUZZLE, 0,
	ROM_NAME(genius6), INPUT_FN(Iqblockz), DIP_FN(Genius6),
	Genius6Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo genius6aRomDesc[] = {
	{ "genius6_v133.u18",   0x040000, 0xb34ce8c6, 1 | BRF_PRG | BRF_ESS },
	{ "genius6_cg.u7",      0x080000, 0x1842d021, 2 | BRF_GRA },
	{ "text.u8",            0x040000, 0x7716b601, 3 | BRF_GRA },
	{ "speech.u17",         0x040000, 0xd9e3d39f, 4 | BRF_SND },
	{ "igs_fixed_data.key", 0x000015, 0x9159ecbf, 7 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(genius6a)
STD_ROM_FN(genius6a)

static INT32 Genius6aInit()
{
	INT32 ret = common_init(PROFILE_GENIUS6, decrypt_iqblocka);
	if (ret) return ret;

	DrvBitswapValXor = 0x15d6;
	DrvBitswapMFBits[0] = 2; DrvBitswapMFBits[1] = 7; DrvBitswapMFBits[2] = 9; DrvBitswapMFBits[3] = 13;
	DrvBitswapM3Bits[0][0] = (UINT8)~5;  DrvBitswapM3Bits[0][1] = 6;         DrvBitswapM3Bits[0][2] = (UINT8)~7;  DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = 1;         DrvBitswapM3Bits[1][1] = (UINT8)~6;  DrvBitswapM3Bits[1][2] = (UINT8)~9;  DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = 4;         DrvBitswapM3Bits[2][1] = (UINT8)~8;  DrvBitswapM3Bits[2][2] = (UINT8)~12; DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = 3;         DrvBitswapM3Bits[3][1] = (UINT8)~5;  DrvBitswapM3Bits[3][2] = (UINT8)~6;  DrvBitswapM3Bits[3][3] = (UINT8)~15;

	return 0;
}

struct BurnDriver BurnDrvGenius6a = {
	"genius6a", "genius6", NULL, NULL, "1997",
	"Genius 6 (V133F)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_PUZZLE, 0,
	ROM_NAME(genius6a), INPUT_FN(Iqblockz), DIP_FN(Genius6),
	Genius6aInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo genius6bRomDesc[] = {
	{ "genius6_v-132f.u18", 0x040000, 0x231be791, 1 | BRF_PRG | BRF_ESS },
	{ "genius6_cg.u7",      0x080000, 0x1842d021, 2 | BRF_GRA },
	{ "text.u8",            0x040000, 0x7716b601, 3 | BRF_GRA },
	{ "speech.u17",         0x040000, 0xd9e3d39f, 4 | BRF_SND },
	{ "igs_fixed_data.key", 0x000015, 0x9159ecbf, 7 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(genius6b)
STD_ROM_FN(genius6b)

static INT32 Genius6bInit()
{
	INT32 ret = common_init(PROFILE_GENIUS6, decrypt_iqblocka);
	if (ret) return ret;

	DrvBitswapValXor = 0x15d6;
	DrvBitswapMFBits[0] = 2; DrvBitswapMFBits[1] = 7; DrvBitswapMFBits[2] = 9; DrvBitswapMFBits[3] = 13;
	DrvBitswapM3Bits[0][0] = (UINT8)~5;  DrvBitswapM3Bits[0][1] = 6;         DrvBitswapM3Bits[0][2] = (UINT8)~7;  DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = 1;         DrvBitswapM3Bits[1][1] = (UINT8)~6;  DrvBitswapM3Bits[1][2] = (UINT8)~9;  DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = 4;         DrvBitswapM3Bits[2][1] = (UINT8)~8;  DrvBitswapM3Bits[2][2] = (UINT8)~12; DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = 3;         DrvBitswapM3Bits[3][1] = (UINT8)~5;  DrvBitswapM3Bits[3][2] = (UINT8)~6;  DrvBitswapM3Bits[3][3] = (UINT8)~15;

	return 0;
}

struct BurnDriver BurnDrvGenius6b = {
	"genius6b", "genius6", NULL, NULL, "1997",
	"Genius 6 (V132F)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_PUZZLE, 0,
	ROM_NAME(genius6b), INPUT_FN(Iqblockz), DIP_FN(Genius6),
	Genius6bInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tjsbRomDesc[] = {
	{ "p0700.u16",      0x040000, 0x1b2a50df, 1 | BRF_PRG | BRF_ESS },
	{ "a0701.u3",       0x400000, 0x27502a0a, 2 | BRF_GRA },
	{ "text.u6",        0x080000, 0x3be886b8, 3 | BRF_GRA },
	{ "s0703.u15",      0x080000, 0xc6f94d29, 4 | BRF_SND },
	{ "tjsb_string.key",0x0000ec, 0x412c83a0, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tjsb)
STD_ROM_FN(tjsb)

static INT32 TjsbInit() { return common_init(PROFILE_TJSB, decrypt_tjsb); }

struct BurnDriver BurnDrvTjsb = {
	"tjsb", NULL, NULL, NULL, "1997",
	"Tian Jiang Shen Bing (China, V137C)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MISC, 0,
	ROM_NAME(tjsb), INPUT_FN(Tjsb), DIP_FN(Tjsb),
	TjsbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tarzancRomDesc[] = {
	{ "u19",                0x040000, 0xe6c552a5, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u15", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u5",  0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u14", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "tarzan_string.key",  0x0000ec, 0x595fe40c, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzanc)
STD_ROM_FN(tarzanc)

static INT32 TarzancInit() { return common_init(PROFILE_TARZAN, decrypt_tarzanc); }

struct BurnDriver BurnDrvTarzanc = {
	"tarzanc", NULL, NULL, NULL, "1999",
	"Tarzan Chuang Tian Guan (China, V109C, set 1)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzanc), INPUT_FN(Tarzan), DIP_FN(Tarzan),
	TarzancInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tarzanbRomDesc[] = {
	{ "t.z._v110.u19",      0x040000, 0x16026d12, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2103_cg_v100f.u15", 0x200000, 0xafe56ed5, 2 | BRF_GRA },
	{ "t.z._text_u5.u5",    0x080000, 0x1724e039, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u14", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "tarzanb_string.key", 0x0000ec, 0x595fe40c, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzanb)
STD_ROM_FN(tarzanb)

static INT32 TarzanbInit() { return common_init(PROFILE_TARZAN, decrypt_tarzanc); }

struct BurnDriver BurnDrvTarzanb = {
	"tarzanb", "tarzanc", NULL, NULL, "1999",
	"Tarzan Chuang Tian Guan (China, V110)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzanb), INPUT_FN(Tarzan), DIP_FN(Tarzan),
	TarzanbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tarzanRomDesc[] = {
	{ "0228-u16.bin",       0x040000, 0xe6c552a5, 1 | BRF_PRG | BRF_ESS },
	{ "sprites.u15",        0x080000, 0x00000000, 2 | BRF_GRA | BRF_NODUMP },
	{ "0228-u6.bin",        0x080000, 0x55e94832, 3 | BRF_GRA },
	{ "sound.u14",          0x040000, 0x00000000, 4 | BRF_SND | BRF_NODUMP },
	{ "tarzan_string.key",  0x0000ec, 0x595fe40c, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzan)
STD_ROM_FN(tarzan)

static INT32 TarzanInit() { return common_init(PROFILE_TARZAN, decrypt_tarzan); }

struct BurnDriver BurnDrvTarzan = {
	"tarzan", "tarzanc", NULL, NULL, "1999",
	"Tarzan Chuang Tian Guan (China, V109C, set 2)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzan), INPUT_FN(Tarzan), DIP_FN(Tarzan),
	TarzanInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tarzanaRomDesc[] = {
	{ "0228-u21.bin",       0x040000, 0x80aaece4, 1 | BRF_PRG | BRF_ESS },
	{ "sprites.u17",        0x080000, 0x00000000, 2 | BRF_GRA | BRF_NODUMP },
	{ "0228-u6.bin",        0x080000, 0x55e94832, 3 | BRF_GRA },
	{ "sound.u16",          0x040000, 0x00000000, 4 | BRF_SND | BRF_NODUMP },
	{ "tarzan_string.key",  0x0000ec, 0x595fe40c, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzana)
STD_ROM_FN(tarzana)

static INT32 TarzanaInit() { return common_init(PROFILE_TARZAN, decrypt_tarzana); }

struct BurnDriver BurnDrvTarzana = {
	"tarzana", "tarzanc", NULL, NULL, "1999",
	"Tarzan (V107)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzana), INPUT_FN(Tarzan), DIP_FN(Tarzan),
	TarzanaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo starzanRomDesc[] = {
	{ "sp_tarzan_v100i.u9", 0x040000, 0x64180bff, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "sp_tarzan_cg.u2",    0x080000, 0x884f95f5, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u8", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "starzan_string.key", 0x0000ec, 0xb33f5050, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(starzan)
STD_ROM_FN(starzan)

static INT32 StarzanInit() { return common_init(PROFILE_STARZAN, decrypt_starzan); }

struct BurnDriver BurnDrvStarzan = {
	"starzan", NULL, NULL, NULL, "2000",
	"Super Tarzan (Italy, V100I)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(starzan), INPUT_FN(Starzan), DIP_FN(Starzan),
	StarzanInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static INT32 Jking200prInit();
static INT32 Jking103aInit();

static struct BurnRomInfo tarzan201faRomDesc[] = {
	{ "u9",                 0x040000, 0x455ac41f, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u8", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "tarzan201fa_string.key", 0x0000ec, 0xb33f5050, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzan201fa)
STD_ROM_FN(tarzan201fa)

static INT32 Tarzan201faInit() { return common_init(PROFILE_STARZAN, decrypt_jking200pr); }

struct BurnDriver BurnDrvTarzan201fa = {
	"tarzan201fa", "tarzanc", NULL, NULL, "1999",
	"Tarzan (V201FA)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzan201fa), INPUT_FN(Starzan), DIP_FN(Tarzan202fa),
	Tarzan201faInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tarzan202faRomDesc[] = {
	{ "tarzan_v102f.u9",    0x040000, 0xb099baaa, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u8", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "tarzan202fa_string.key", 0x0000ec, 0xb33f5050, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzan202fa)
STD_ROM_FN(tarzan202fa)

static INT32 Tarzan202faInit() { return common_init(PROFILE_STARZAN, decrypt_jking103a); }

struct BurnDriver BurnDrvTarzan202fa = {
	"tarzan202fa", "tarzanc", NULL, NULL, "1999",
	"Tarzan (V202FA)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzan202fa), INPUT_FN(Starzan), DIP_FN(Tarzan202fa),
	Tarzan202faInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo jking200prRomDesc[] = {
	{ "u9",                 0x040000, 0x8c40c920, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u8", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "jking200pr_string.key", 0x0000ec, 0xb33f5050, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(jking200pr)
STD_ROM_FN(jking200pr)

static INT32 Jking200prInit() { return common_init(PROFILE_STARZAN, decrypt_jking200pr); }

struct BurnDriver BurnDrvJking200pr = {
	"jking200pr", "starzan", NULL, NULL, "1999",
	"Jungle King (V200PR)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(jking200pr), INPUT_FN(Starzan), DIP_FN(Tarzan202fa),
	Jking200prInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo jking103aRomDesc[] = {
	{ "jungleking_v103a.u9", 0x040000, 0xacd23f7e, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u8", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "jking103a_string.key", 0x0000ec, 0x8d288f5e, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(jking103a)
STD_ROM_FN(jking103a)

static INT32 Jking103aInit() { return common_init(PROFILE_STARZAN, decrypt_jking103a); }

struct BurnDriver BurnDrvJking103a = {
	"jking103a", "starzan", NULL, NULL, "2000",
	"Jungle King (V103A)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(jking103a), INPUT_FN(Starzan), DIP_FN(Starzan),
	Jking103aInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo jking105usRomDesc[] = {
	{ "j_k_v-105us.u9",     0x040000, 0x96dac1e9, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102.u8",       0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "jking105us_string.key", 0x0000ec, 0x8d288f5e, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(jking105us)
STD_ROM_FN(jking105us)

static INT32 Jking105usInit() { return common_init(PROFILE_STARZAN, decrypt_jking103a); }

struct BurnDriver BurnDrvJking105us = {
	"jking105us", "starzan", NULL, NULL, "2000",
	"Jungle King (V105US)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(jking105us), INPUT_FN(Starzan), DIP_FN(Tarzan202fa),
	Jking105usInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tarzan103mRomDesc[] = {
	{ "tms27c040.u9",       0x040000, 0x19e3811d, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102_sp_v102.u8", 0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "tarzan103m_string.key", 0x0000ec, 0xb33f5050, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzan103m)
STD_ROM_FN(tarzan103m)

static INT32 Tarzan103mInit() { return common_init(PROFILE_TARZAN103M, decrypt_starzan); }

struct BurnDriver BurnDrvTarzan103m = {
	"tarzan103m", "tarzanc", NULL, NULL, "1999",
	"Tarzan (V103M)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzan103m), INPUT_FN(Tarzan103m), DIP_FN(Tarzan103m),
	Tarzan103mInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo tarzan106faRomDesc[] = {
	{ "t_z_v-106fa.u9",     0x040000, 0x588caceb, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2104_cg_v110.u3", 0x400000, 0xdcbff16f, 2 | BRF_GRA },
	{ "igs_t2105_cg_v110.u11", 0x080000, 0x1d4be260, 3 | BRF_GRA },
	{ "igs_s2102.u8",       0x080000, 0x90dda82d, 4 | BRF_SND },
	{ "tarzan106fa_string.key", 0x0000ec, 0x8d288f5e, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(tarzan106fa)
STD_ROM_FN(tarzan106fa)

static INT32 Tarzan106faInit() { return common_init(PROFILE_STARZAN, decrypt_jking103a); }

struct BurnDriver BurnDrvTarzan106fa = {
	"tarzan106fa", "tarzanc", NULL, NULL, "1999",
	"Tarzan (V106FA)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tarzan106fa), INPUT_FN(Starzan), DIP_FN(Tarzan202fa),
	Tarzan106faInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo happysklRomDesc[] = {
	{ "v611.u9",            0x040000, 0x1fb3da98, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2701_cg_v100.u3", 0x400000, 0xf3756a51, 2 | BRF_GRA },
	{ "happyskill_cg.u2",   0x080000, 0x297a1893, 2 | BRF_GRA },
	{ "happyskill_text.u11",0x080000, 0xc6f51041, 3 | BRF_GRA },
	{ "igs_s2702_sp_v100.u8", 0x080000, 0x0ec9b1b5, 4 | BRF_SND },
};
STD_ROM_PICK(happyskl)
STD_ROM_FN(happyskl)

static INT32 HappysklInit() { return common_init(PROFILE_HAPPYSKL, decrypt_happyskl); }

struct BurnDriver BurnDrvHappyskl = {
	"happyskl", NULL, NULL, NULL, "2000",
	"Happy Skill (Italy, V611IT)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(happyskl), INPUT_FN(Happyskl), DIP_FN(Happyskl),
	HappysklInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo cpoker2RomDesc[] = {
	{ "champion_2_v100a.u9", 0x040000, 0x8d79eb4d, 1 | BRF_PRG | BRF_ESS },
	{ "igs_a2701_cg_v100.u3", 0x400000, 0xf3756a51, 2 | BRF_GRA },
	{ "champion_2_text.u11", 0x080000, 0x34475c83, 3 | BRF_GRA },
	{ "igs_s2702_sp_v100.u8", 0x080000, 0x0ec9b1b5, 4 | BRF_SND },
};
STD_ROM_PICK(cpoker2)
STD_ROM_FN(cpoker2)

static INT32 Cpoker2Init() { return common_init(PROFILE_CPOKER2, decrypt_cpoker2); }

struct BurnDriver BurnDrvCpoker2 = {
	"cpoker2", NULL, NULL, NULL, "2000",
	"Champion Poker 2 (V100A)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cpoker2), INPUT_FN(Cpoker2), DIP_FN(Cpoker2),
	Cpoker2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo spkrformRomDesc[] = {
	{ "super2in1_v100xd03.u29", 0x040000, 0xe8f7476c, 1 | BRF_PRG | BRF_ESS },
	{ "super2in1.u26",      0x080000, 0xaf3b1d9d, 2 | BRF_GRA },
	{ "super2in1.u25",      0x080000, 0x7ebaf0a0, 2 | BRF_GRA },
	{ "super2in1.u24",      0x040000, 0x54d68c49, 3 | BRF_GRA },
	{ "super2in1_sp.u28",   0x040000, 0x33e6089d, 4 | BRF_SND },
	{ "spkrform_string.key",0x0000ec, 0x17a9021a, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(spkrform)
STD_ROM_FN(spkrform)

static INT32 SpkrformInit() {
	int ret = common_init(PROFILE_SPKRFORM, decrypt_spkrform);
	if (ret == 0)
	{
		DrvZ180ROM[0x32ea9] = 0; // enable poker ($e9be = 0)
		DrvZ180ROM[0x32ef9] = 0; // start with poker ($e9bf = 0)

		DrvOpcodeROM[0x32ea9] = 0; // enable poker ($e9be = 0)
		DrvOpcodeROM[0x32ef9] = 0; // start with poker ($e9bf = 0)
	}
	return ret;
}

struct BurnDriver BurnDrvSpkrform = {
	"spkrform", NULL, NULL, NULL, "2000",
	"Super Poker (V100xD03) / Formosa\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(spkrform), INPUT_FN(Spkrform), DIP_FN(Spkrform),
	SpkrformInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

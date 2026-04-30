#include "tiles_generic.h"
#include "m68000_intf.h"
#include "msm6295.h"
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
	PROFILE_MGDH = 0,
	PROFILE_SDMG2,
	PROFILE_SDMG2P,
	PROFILE_MGCS,
	PROFILE_MGCSA,
	PROFILE_MGCSB,
	PROFILE_LHZB2,
	PROFILE_LHZB2A,
	PROFILE_LHZB2B,
	PROFILE_LHZB2C,
	PROFILE_SLQZ2,
};

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *Drv68KROM;
static UINT8 *DrvTileROM;
static UINT8 *DrvSpriteROM;
static UINT8 *DrvSndROM;
static UINT8 *DrvStringROM;
static UINT8 *DrvProtROM;
static UINT8 *DrvNVRAM;
static UINT8 *DrvProtRAM;
static UINT8 *DrvFixedData;

static INT32 Drv68KLen;
static INT32 DrvTileLen;
static INT32 DrvSpriteLen;
static INT32 DrvSndLen;
static INT32 DrvStringLen;
static INT32 DrvProtLen;
static INT32 DrvNVRAMLen;
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
static UINT8 DrvHasBootRead;
static UINT8 DrvBootPtr;
static UINT8 DrvIncDecVal;
static UINT8 DrvIncAltVal;
static UINT8 DrvNmiLine;
static UINT8 DrvDswSelect;
static UINT8 DrvScrambleData;
static UINT8 DrvIgs029SendData;
static UINT8 DrvIgs029RecvData;
static UINT8 DrvIgs029SendBuf[256];
static UINT8 DrvIgs029RecvBuf[256];
static INT32 DrvIgs029SendLen;
static INT32 DrvIgs029RecvLen;
static UINT32 DrvIgs029Reg[0x60];
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
static UINT8 DrvIgs022Latch;
static UINT32 DrvIgs022Regs[0x300];
static UINT32 DrvIgs022Stack[0x100];
static UINT8 DrvIgs022StackPtr;

static UINT8 string_result_r();
static void string_bitswap_w(UINT8 offset, UINT8 data);
static UINT8 string_advance_r();
static UINT8 bitswap_result_r();
static void bitswap_word_w(UINT8 data);
static void bitswap_mode_f_w();
static void bitswap_mode_3_w();
static void bitswap_exec_w(UINT8 offset, UINT8 data);
static void mgcs_keys_hopper_igs029_w(UINT8 data);
static void mgcs_scramble_data_w(UINT8 data);
static UINT8 mgcs_scramble_data_r();
static UINT8 mgcs_igs029_data_r();
static void mgcs_igs029_data_w(UINT8 data);
static void mgcsb_counter_w(UINT8 data);
static void lhzb2_keys_hopper_w(UINT8 data);
static UINT8 lhzb2_keys_r();
static UINT8 lhzb2_scramble_data_r();
static UINT8 lhzb2b_scramble_data_r();
static void lhzb2b_igs029_scramble_irq_w(UINT8 data);
static void lhzb2_igs022_execute_w(UINT8 data);
static UINT8 slqz2_coins_r();
static void slqz2_sound_hopper_w(UINT8 data);
static UINT8 slqz2_scramble_data_r();
static UINT8 slqz2_player1_r();
static UINT8 slqz2_player2_r();
static UINT16 __fastcall lhzb2_incdec_read_word(UINT32 address);
static UINT8 __fastcall lhzb2_incdec_read_byte(UINT32 address);
static void __fastcall lhzb2_incdec_write_word(UINT32 address, UINT16 data);
static void __fastcall lhzb2_incdec_write_byte(UINT32 address, UINT8 data);
template<UINT8 Bit, UINT8 WarnMask> static void igs022_execute_w(UINT8 data);

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

	if (DrvJoy1[0] && !DrvCoin1Prev) {
		DrvCoin1Pulse = 5;
	}
	DrvCoin1Prev = DrvJoy1[0] ? 1 : 0;

	DrvInputs[0] |= 0x01;
	if (DrvCoin1Pulse) {
		DrvInputs[0] &= ~0x01;
		DrvCoin1Pulse--;
	}

	pack_mahjong_inputs();
}

static inline UINT8 read_dip_a() { return DrvDips[0]; }
static inline UINT8 read_dip_b() { return DrvDips[1]; }
static inline UINT8 read_dip_c() { return DrvDips[2]; }
static UINT8 read_ff() { return 0xff; }

static void apply_default_dips_if_invalid()
{
	if (DrvDips[0] == 0x00 && DrvDips[1] == 0x00 && DrvDips[2] == 0x00) {
		DrvDips[0] = 0xff;
		DrvDips[1] = 0xff;
		DrvDips[2] = 0xff;

		switch (DrvProfile)
		{
			case PROFILE_MGDH:
				DrvDips[1] &= ~0x02; // default to joystick controls
			break;

			case PROFILE_SDMG2:
			case PROFILE_SDMG2P:
				DrvDips[1] &= ~0x40; // default to joystick controls
			break;

			case PROFILE_MGCS:
			case PROFILE_MGCSA:
			case PROFILE_MGCSB:
				DrvDips[1] &= ~0x10; // default to joystick controls
			break;
		}
	}
}

static UINT16 mgcs_palette_bitswap(UINT16 bgr)
{
	bgr = (bgr >> 8) | (bgr << 8);
	return BITSWAP16(bgr, 7,8,9,2,14,3,13,15,12,11,10,0,1,4,5,6);
}

static UINT16 lhzb2a_palette_bitswap(UINT16 bgr)
{
	return BITSWAP16(bgr, 15,9,13,12,11,5,4,8,7,6,0,14,3,2,1,10);
}

static UINT16 slqz2_palette_bitswap(UINT16 bgr)
{
	return BITSWAP16(bgr, 15,14,9,4,11,10,12,3,7,6,5,8,13,2,1,0);
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

static inline INT32 controls_mahjong()
{
	if (DrvProfile == PROFILE_MGDH) {
		return (DrvDips[1] & 0x02) ? 1 : 0;
	}

	if (DrvProfile == PROFILE_MGCS || DrvProfile == PROFILE_MGCSA || DrvProfile == PROFILE_MGCSB) {
		return (DrvDips[1] & 0x10) ? 1 : 0;
	}

	if (DrvProfile == PROFILE_LHZB2 || DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2B || DrvProfile == PROFILE_LHZB2C) {
		return 1;
	}

	return (DrvDips[1] & 0x40) ? 1 : 0;
}

static inline INT32 credit_mode_coin()
{
	if (DrvProfile == PROFILE_MGDH) {
		return (DrvDips[0] & 0x10) ? 1 : 0;
	}

	return (DrvDips[0] & 0x20) ? 1 : 0;
}

static inline INT32 payout_mode_hopper()
{
	if (DrvProfile == PROFILE_MGDH) {
		return (DrvDips[0] & 0x20) ? 1 : 0;
	}

	return (DrvDips[0] & 0x40) ? 1 : 0;
}

static UINT8 mgdh_matrix_read()
{
	if (controls_mahjong()) {
		return mahjong_matrix_read(2, 0);
	}

	UINT8 ret = 0xff;

	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x04) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x20) == 0) ret &= ~0x20;

	return ret;
}

static UINT8 mgdh_buttons_read()
{
	UINT8 ret = 0xff;

	if (!controls_mahjong()) {
		if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x02;
		if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x80;
	}

	return ret;
}

static UINT8 mgdh_coins_read()
{
	UINT8 ret = 0xff;

	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x04;
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x08;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x08;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x10;
	}
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x20;

	return ret;
}

static UINT8 sdmg2_matrix_read()
{
	if (controls_mahjong()) {
		return mahjong_matrix_read(0, 0);
	}

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

static UINT8 sdmg2_coins_read()
{
	UINT8 ret = 0xff;

	if (hopper_line() == 0) ret &= ~0x01;
	if (controls_mahjong()) {
		if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x02;
	}
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x08;
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x10;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x20;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x20;
	}
	if (controls_mahjong()) {
		if ((DrvInputs[0] & 0x80) == 0) ret &= ~0x40;
	} else {
		if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x40;
	}

	return ret;
}

static UINT8 sdmg2p_joy_read()
{
	UINT8 ret = 0xff;

	if (!controls_mahjong()) {
		if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x04;
		if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x08;
		if ((DrvInputs[1] & 0x04) == 0) ret &= ~0x10;
		if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x20;
		if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x40;
		if ((DrvInputs[1] & 0x20) == 0) ret &= ~0x80;
	}

	return ret;
}

static UINT8 sdmg2p_buttons_read()
{
	UINT8 ret = 0xff;

	if (controls_mahjong()) {
		if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x02;
	} else {
		if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x01;
	}

	if (hopper_line() == 0) ret &= ~0x80;

	return ret;
}

static UINT8 sdmg2p_coins_read()
{
	UINT8 ret = 0xff;

	if (!controls_mahjong()) {
		if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x02;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x04;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x04;
	}
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x08;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x08;
	}
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x20;

	return ret;
}

static void set_oki_bank(INT32 bank)
{
	DrvOkiBank = bank & 1;
	MSM6295SetBank(0, DrvSndROM + (DrvOkiBank * 0x40000), 0x00000, 0x3ffff);
}

static void mux_data_write(UINT8 data)
{
	if (DrvMuxAddr >= 0x20 && DrvMuxAddr <= 0x27 && DrvStringLen > 0) {
		string_bitswap_w(DrvMuxAddr, data);
		return;
	}

	if (DrvProfile == PROFILE_MGCSB || DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2C) {
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
		case PROFILE_MGDH:
			switch (DrvMuxAddr)
			{
				case 0x01:
					DrvInputSelect = data;
					DrvHopperMotor = data & 0x01;
				return;

				case 0x03:
					set_oki_bank((data >> 6) & 1);
				return;
			}
		break;

		case PROFILE_SDMG2:
			switch (DrvMuxAddr)
			{
				case 0x01:
					DrvInputSelect = data & 0x1f;
					DrvHopperMotor = (data >> 7) & 1;
				return;

				case 0x02:
					set_oki_bank((data >> 7) & 1);
				return;
			}
		break;

		case PROFILE_SDMG2P:
			switch (DrvMuxAddr)
			{
				case 0x02:
					DrvInputSelect = (data >> 2) & 0x1f;
				return;

				case 0x03:
					DrvHopperMotor = (data >> 6) & 1;
					set_oki_bank((data >> 7) & 1);
				return;
			}
		break;

		case PROFILE_MGCS:
		case PROFILE_MGCSA:
			switch (DrvMuxAddr)
			{
				case 0x00:
					mgcs_keys_hopper_igs029_w(data);
				return;

				case 0x01:
					mgcs_scramble_data_w(data);
				return;

				case 0x03:
					mgcs_igs029_data_w(data);
				return;
			}
		break;

		case PROFILE_MGCSB:
			switch (DrvMuxAddr)
			{
				case 0x00:
					mgcsb_counter_w(data);
				return;

				case 0x01:
					set_oki_bank(BIT(data, 0));
				return;

				case 0x02:
					DrvInputSelect = data;
				return;
			}
		break;

		case PROFILE_LHZB2B:
			switch (DrvMuxAddr)
			{
				case 0x00:
					lhzb2_keys_hopper_w(data);
				return;

				case 0x01:
					set_oki_bank(BIT(data, 7));
				return;

				case 0x02:
					mgcs_igs029_data_w(data);
				return;

				case 0x03:
					lhzb2b_igs029_scramble_irq_w(data);
				return;
			}
		break;

		case PROFILE_LHZB2:
			switch (DrvMuxAddr)
			{
				case 0x00:
					lhzb2_keys_hopper_w(data);
				return;

				case 0x01:
					lhzb2_igs022_execute_w(data);
				return;

				case 0x03:
					mgcs_scramble_data_w(data);
				return;
			}
		break;

		case PROFILE_LHZB2C:
			switch (DrvMuxAddr)
			{
				case 0x00:
					lhzb2_keys_hopper_w(data);
				return;

				case 0x01:
					set_oki_bank(BIT(data, 0));
				return;

				case 0x02:
					DrvInputSelect = data;
				return;
			}
		break;

		case PROFILE_SLQZ2:
			switch (DrvMuxAddr)
			{
				case 0x00:
					slqz2_sound_hopper_w(data);
				return;

				case 0x01:
					igs022_execute_w<6, 0xbf>(data);
				return;

				case 0x03:
					mgcs_scramble_data_w(data);
				return;
			}
		break;

	}
}

static UINT8 mux_data_read()
{
	if (DrvMuxAddr == 0x05 && DrvStringLen > 0) {
		return string_result_r();
	}

	if (DrvMuxAddr == 0x40 && DrvStringLen > 0) {
		return string_advance_r();
	}

	if ((DrvProfile == PROFILE_MGCSB || DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2C) && DrvMuxAddr == 0x03) {
		return bitswap_result_r();
	}

	switch (DrvProfile)
	{
		case PROFILE_MGDH:
			switch (DrvMuxAddr)
			{
				case 0x00: return mgdh_matrix_read();
				case 0x01: return mgdh_buttons_read();
				case 0x02: return BITSWAP08(DrvDips[1], 0, 1, 2, 3, 4, 5, 6, 7);
				case 0x03: return mgdh_coins_read();
				case 0x05:
				{
					static const UINT8 boot_table[4] = { 0, 1, 2, 0 };
					UINT8 ret = DrvHasBootRead ? boot_table[DrvBootPtr & 3] : 0xff;
					if (DrvHasBootRead) DrvBootPtr = (DrvBootPtr + 1) & 3;
					return ret;
				}
			}
		break;

		case PROFILE_SDMG2:
			switch (DrvMuxAddr)
			{
				case 0x00: return sdmg2_coins_read();
				case 0x02: return sdmg2_matrix_read();
			}
		break;

		case PROFILE_SDMG2P:
			switch (DrvMuxAddr)
			{
				case 0x00: return mahjong_matrix_read(0, 2);
				case 0x01: return sdmg2p_joy_read();
				case 0x02: return sdmg2p_buttons_read();
				case 0x03: return sdmg2p_coins_read();
			}
		break;

		case PROFILE_MGCS:
		case PROFILE_MGCSA:
			switch (DrvMuxAddr)
			{
				case 0x00: return DrvInputSelect | 0x02;
				case 0x01: return mgcs_scramble_data_r();
				case 0x02: return mgcs_igs029_data_r();
			}
		break;

		case PROFILE_MGCSB:
			break;

		case PROFILE_LHZB2B:
			switch (DrvMuxAddr)
			{
				case 0x01: return lhzb2_keys_r();
				case 0x02: return lhzb2b_scramble_data_r();
			}
		break;

		case PROFILE_LHZB2:
			switch (DrvMuxAddr)
			{
				case 0x01: return lhzb2_keys_r();
				case 0x02: return lhzb2_scramble_data_r();
			}
		break;

		case PROFILE_LHZB2C:
			break;

		case PROFILE_SLQZ2:
			switch (DrvMuxAddr)
			{
				case 0x00: return slqz2_player2_r();
				case 0x01: return slqz2_player1_r();
				case 0x02: return slqz2_scramble_data_r();
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

static inline UINT32 get_u32le_local(const UINT8 *src)
{
	return src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
}

static inline void put_u32be_local(UINT8 *dst, UINT32 data)
{
	dst[0] = (data >> 24) & 0xff;
	dst[1] = (data >> 16) & 0xff;
	dst[2] = (data >> 8) & 0xff;
	dst[3] = (data >> 0) & 0xff;
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

static inline UINT16 prot_read16(UINT32 offset)
{
	return ((UINT16*)DrvProtROM)[offset];
}

static inline UINT16 sharedprot_read16(UINT32 offset)
{
	return ((UINT16*)DrvProtRAM)[offset];
}

static inline void sharedprot_write16(UINT32 offset, UINT16 data)
{
	((UINT16*)DrvProtRAM)[offset] = data;
}

static void igs022_dma(UINT16 src, UINT16 dst, UINT16 size, UINT16 mode)
{
	UINT16 param = mode >> 8;
	mode &= 0x7;

	switch (mode)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			for (INT32 x = 0; x < size; x++) {
				UINT16 dat = prot_read16(src + x);
				UINT8 extraoffset = param & 0xff;
				UINT8 *dectable = DrvProtROM;
				UINT8 taboff = ((x * 2) + extraoffset) & 0xff;
				UINT16 extraxor = (dectable[taboff + 1] << 8) | dectable[taboff + 0];

				switch (mode)
				{
					case 1: dat -= extraxor; break;
					case 2: dat += extraxor; break;
					case 3: dat ^= extraxor; break;
					case 4:
						extraxor = 0;
						if ((x & 0x003) == 0x000) extraxor |= 0x0049;
						if ((x & 0x003) == 0x001) extraxor |= 0x0047;
						if ((x & 0x003) == 0x002) extraxor |= 0x0053;
						if ((x & 0x003) == 0x003) extraxor |= 0x0020;
						if ((x & 0x300) == 0x000) extraxor |= 0x4900;
						if ((x & 0x300) == 0x100) extraxor |= 0x4700;
						if ((x & 0x300) == 0x200) extraxor |= 0x5300;
						if ((x & 0x300) == 0x300) extraxor |= 0x2000;
						dat -= extraxor;
					break;
				}

				sharedprot_write16(dst + x, dat);
			}
		break;

		case 5:
			for (INT32 x = 0; x < size; x++) {
				UINT16 dat = prot_read16(src + x);
				sharedprot_write16(dst + x, BURN_ENDIAN_SWAP_INT16(dat));
			}
		break;

		case 6:
			for (INT32 x = 0; x < size; x++) {
				UINT16 dat = prot_read16(src + x);
				dat = ((dat & 0xf0f0) >> 4) | ((dat & 0x0f0f) << 4);
				sharedprot_write16(dst + x, dat);
			}
		break;
	}
}

static void igs022_push(UINT32 data)
{
	if (DrvIgs022StackPtr < 0xff) DrvIgs022StackPtr++;
	DrvIgs022Stack[DrvIgs022StackPtr] = data;
}

static UINT32 igs022_pop()
{
	UINT32 data = DrvIgs022Stack[DrvIgs022StackPtr];
	if (DrvIgs022StackPtr > 0) DrvIgs022StackPtr--;
	return data;
}

static UINT32 igs022_read_reg(UINT16 offset)
{
	if (offset < 0x300) return DrvIgs022Regs[offset];
	if (offset == 0x400) return igs022_pop();
	return 0;
}

static void igs022_write_reg(UINT16 offset, UINT32 data)
{
	if (offset < 0x300) {
		DrvIgs022Regs[offset] = data;
	} else if (offset == 0x300) {
		igs022_push(data);
	}
}

static void igs022_handle_incomplete(UINT16, UINT16 res)
{
	sharedprot_write16(0x202 / 2, res);
}

static void igs022_handle_6d()
{
	UINT32 p1 = (sharedprot_read16(0x298 / 2) << 16) | sharedprot_read16(0x29a / 2);
	UINT32 p2 = (sharedprot_read16(0x29c / 2) << 16) | sharedprot_read16(0x29e / 2);

	switch (p2 & 0xffff)
	{
		case 0x0:
			igs022_write_reg(p2 >> 16, igs022_read_reg(p1 >> 16) + igs022_read_reg(p1 & 0xffff));
		break;

		case 0x1:
			igs022_write_reg(p2 >> 16, igs022_read_reg(p1 >> 16) - igs022_read_reg(p1 & 0xffff));
		break;

		case 0x6:
			igs022_write_reg(p2 >> 16, igs022_read_reg(p1 & 0xffff) - igs022_read_reg(p1 >> 16));
		break;

		case 0x9:
			igs022_write_reg(p2 >> 16, p1);
		break;

		case 0xa:
		{
			UINT32 data = igs022_read_reg(p1 >> 16);
			sharedprot_write16(0x29c / 2, (data >> 16) & 0xffff);
			sharedprot_write16(0x29e / 2, data & 0xffff);
		}
		break;
	}

	sharedprot_write16(0x202 / 2, 0x7c);
}

static void igs022_handle_command()
{
	UINT16 cmd = sharedprot_read16(0x200 / 2);

	switch (cmd)
	{
		case 0x12:
			igs022_push((sharedprot_read16(0x288 / 2) << 16) | sharedprot_read16(0x28a / 2));
			sharedprot_write16(0x202 / 2, 0x23);
		break;

		case 0x2d:
			igs022_handle_incomplete(cmd, 0x3c);
		break;

		case 0x45:
		{
			UINT32 data = igs022_pop();
			sharedprot_write16(0x28c / 2, (data >> 16) & 0xffff);
			sharedprot_write16(0x28e / 2, data & 0xffff);
			sharedprot_write16(0x202 / 2, 0x56);
		}
		break;

		case 0x4f:
		{
			UINT16 src = sharedprot_read16(0x290 / 2) >> 1;
			UINT32 dst = sharedprot_read16(0x292 / 2);
			UINT16 size = sharedprot_read16(0x294 / 2);
			UINT16 mode = sharedprot_read16(0x296 / 2);
			igs022_dma(src, dst, size, mode);
			sharedprot_write16(0x202 / 2, 0x5e);
		}
		break;

		case 0x5a:
			igs022_handle_incomplete(cmd, 0x4b);
		break;

		case 0x6d:
			igs022_handle_6d();
		break;
	}
}

static void igs022_reset()
{
	memset(DrvIgs022Regs, 0, sizeof(DrvIgs022Regs));
	memset(DrvIgs022Stack, 0, sizeof(DrvIgs022Stack));
	DrvIgs022StackPtr = 0;

	for (INT32 i = 0; i < 0x4000 / 2; i++) {
		sharedprot_write16(i, 0xa55a);
	}

	if (DrvProtLen >= 0x116) {
		UINT16 src = prot_read16(0x100 / 2);
		UINT32 dst = prot_read16(0x102 / 2);
		UINT16 size = prot_read16(0x104 / 2);
		UINT16 mode = BURN_ENDIAN_SWAP_INT16(prot_read16(0x106 / 2));
		src >>= 1;
		igs022_dma(src, dst, size, mode);
		sharedprot_write16(0x2a2 / 2, prot_read16(0x114 / 2));
	}
}

template<UINT8 Bit, UINT8 WarnMask>
static void igs022_execute_w(UINT8 data)
{
	if (((DrvIgs022Latch & (1 << Bit)) == 0) && (data & (1 << Bit))) {
		igs022_handle_command();
		DrvScrambleData = (DrvScrambleData + 1) & 0x7f;
	}
	DrvIgs022Latch = data;
}

static void igs029_get_reg(UINT8 reg_offset)
{
	put_u32be_local(DrvIgs029RecvBuf, DrvIgs029Reg[reg_offset]);
	DrvIgs029RecvLen = 4;
	DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x05;
}

static void igs029_get_ret(UINT32 ret)
{
	put_u32be_local(DrvIgs029RecvBuf, ret);
	DrvIgs029RecvLen = 4;
	DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x05;
}

static void igs029_set_reg(UINT8 reg_offset, UINT8 data_offset)
{
	DrvIgs029Reg[reg_offset] = get_u32le_local(&DrvIgs029SendBuf[data_offset]);
	DrvIgs029RecvLen = 0;
	DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
}

static void common_igs029_run()
{
	if (DrvIgs029SendBuf[0] == 0x03 && DrvIgs029SendBuf[1] == 0x04) {
		igs029_get_reg(0x50);
	} else if (DrvIgs029SendBuf[0] == 0x03 && DrvIgs029SendBuf[1] == 0x0f) {
		igs029_get_reg(0x10);
	} else if (DrvIgs029SendBuf[0] == 0x03 && DrvIgs029SendBuf[1] == 0x19) {
		igs029_get_reg(0x55);
	} else if (DrvIgs029SendBuf[0] == 0x03 && (DrvIgs029SendBuf[1] == 0x1c || DrvIgs029SendBuf[1] == 0x31)) {
		DrvIgs029RecvLen = 0;
		DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
	} else if (DrvIgs029SendBuf[0] == 0x03 && DrvIgs029SendBuf[1] == 0x39) {
		UINT8 ret = 0xff;
		if (!BIT(DrvDswSelect, 0)) ret &= DrvDips[0];
		if (!BIT(DrvDswSelect, 1)) ret &= DrvDips[1];
		DrvIgs029RecvLen = 0;
		DrvIgs029RecvBuf[DrvIgs029RecvLen++] = ret;
		DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x02;
	} else if (DrvIgs029SendBuf[0] == 0x03 && DrvIgs029SendBuf[1] == 0x3c) {
		igs029_get_reg(0x58);
	} else if (DrvIgs029SendBuf[0] == 0x03 && DrvIgs029SendBuf[1] == 0x55) {
		igs029_get_reg(0x20);
	} else if (DrvIgs029SendBuf[0] == 0x04 && (DrvIgs029SendBuf[1] == 0x02 || DrvIgs029SendBuf[1] == 0x48)) {
		DrvIgs029RecvLen = 0;
		DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
	} else if (DrvIgs029SendBuf[0] == 0x04 && DrvIgs029SendBuf[1] == 0x58) {
		igs029_get_reg(DrvIgs029SendBuf[2]);
	} else if (DrvIgs029SendBuf[0] == 0x07 && DrvIgs029SendBuf[1] == 0x15) {
		igs029_set_reg(0x50, 2);
	} else if (DrvIgs029SendBuf[0] == 0x07 && DrvIgs029SendBuf[1] == 0x21) {
		igs029_set_reg(0x10, 2);
	} else if (DrvIgs029SendBuf[0] == 0x07 && DrvIgs029SendBuf[1] == 0x2c) {
		INT32 no_ret = 1;

		switch (DrvIgs029SendBuf[2])
		{
			case 0x03:
				DrvIgs029Reg[DrvIgs029SendBuf[5]] = DrvIgs029Reg[DrvIgs029SendBuf[3]] & DrvIgs029Reg[DrvIgs029SendBuf[4]];
			break;

			case 0x15:
				igs029_get_ret(DrvIgs029Reg[DrvIgs029SendBuf[3]] + DrvIgs029Reg[DrvIgs029SendBuf[4]]);
				no_ret = 0;
			break;

			case 0x1e:
				igs029_get_ret(DrvIgs029Reg[DrvIgs029SendBuf[4]] - DrvIgs029Reg[DrvIgs029SendBuf[3]]);
				no_ret = 0;
			break;

			case 0x28:
				DrvIgs029Reg[DrvIgs029SendBuf[5]] = DrvIgs029Reg[DrvIgs029SendBuf[3]] - 1;
			break;

			case 0x43:
				igs029_get_ret(DrvIgs029Reg[DrvIgs029SendBuf[5]] + 1);
				no_ret = 0;
			break;

			case 0x67:
				DrvIgs029Reg[DrvIgs029SendBuf[4]] = DrvIgs029Reg[DrvIgs029SendBuf[5]];
			break;

			case 0xc2:
				DrvIgs029Reg[DrvIgs029SendBuf[3]] = DrvIgs029Reg[DrvIgs029SendBuf[4]] + DrvIgs029Reg[DrvIgs029SendBuf[5]];
			break;

			case 0xe8:
				DrvIgs029Reg[DrvIgs029SendBuf[4]] = DrvIgs029Reg[DrvIgs029SendBuf[3]] - DrvIgs029Reg[DrvIgs029SendBuf[5]];
			break;

			case 0xf2:
				DrvIgs029Reg[DrvIgs029SendBuf[5]] = DrvIgs029Reg[DrvIgs029SendBuf[4]];
			break;
		}

		if (no_ret) {
			DrvIgs029RecvLen = 0;
			DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
		}
	} else if (DrvIgs029SendBuf[0] == 0x07 && DrvIgs029SendBuf[1] == 0x3b) {
		INT32 no_ret = 1;

		switch (DrvIgs029SendBuf[2])
		{
			case 0x00:
				DrvIgs029Reg[DrvIgs029SendBuf[3]] = DrvIgs029Reg[DrvIgs029SendBuf[5]] >> DrvIgs029Reg[DrvIgs029SendBuf[4]];
			break;

			case 0x34:
				DrvIgs029Reg[DrvIgs029SendBuf[5]] = DrvIgs029Reg[DrvIgs029SendBuf[4]] - 1;
				igs029_get_ret(DrvIgs029Reg[DrvIgs029SendBuf[5]]);
				no_ret = 0;
			break;

			case 0x71:
				DrvIgs029Reg[DrvIgs029SendBuf[3]] = DrvIgs029Reg[DrvIgs029SendBuf[5]] + 1;
			break;

			case 0x9c:
				igs029_get_ret(DrvIgs029Reg[DrvIgs029SendBuf[4]] + 1);
				no_ret = 0;
			break;

			case 0xa5:
				DrvIgs029Reg[DrvIgs029SendBuf[4]] = DrvIgs029Reg[DrvIgs029SendBuf[5]] + 1;
			break;

			case 0xb6:
				DrvIgs029Reg[DrvIgs029SendBuf[5]] = DrvIgs029Reg[DrvIgs029SendBuf[3]] * DrvIgs029Reg[DrvIgs029SendBuf[4]];
			break;
		}

		if (no_ret) {
			DrvIgs029RecvLen = 0;
			DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
		}
	} else if (DrvIgs029SendBuf[0] == 0x07 && DrvIgs029SendBuf[1] == 0x44) {
		igs029_set_reg(0x20, 2);
	} else if (DrvIgs029SendBuf[0] == 0x07 && DrvIgs029SendBuf[1] == 0x50) {
		igs029_set_reg(0x55, 2);
	} else if (DrvIgs029SendBuf[0] == 0x07 && DrvIgs029SendBuf[1] == 0x64) {
		igs029_set_reg(0x58, 2);
	} else if (DrvIgs029SendBuf[0] == 0x08 && DrvIgs029SendBuf[1] == 0x23) {
		igs029_set_reg(DrvIgs029SendBuf[6], 2);
	} else if (DrvIgs029SendBuf[0] == 0x08 && DrvIgs029SendBuf[1] == 0x73) {
		igs029_set_reg(DrvIgs029SendBuf[2] + 0x30, 3);
	} else {
		DrvIgs029RecvLen = 0;
		DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
	}
}

static void mgcs_igs029_run()
{
	if (DrvIgs029SendBuf[0] == 0x05 && DrvIgs029SendBuf[1] == 0x5a) {
		UINT8 data = DrvIgs029SendBuf[2];
		UINT8 port = DrvIgs029SendBuf[3];

		switch (port)
		{
			case 0x01:
				set_oki_bank(BIT(data, 4));
			break;

			case 0x03:
				DrvDswSelect = data;
			break;
		}

		DrvIgs029RecvLen = 0;
		DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
	} else {
		common_igs029_run();
	}

	DrvIgs029SendLen = 0;
}

static UINT8 mgcs_joy_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x04) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x20) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x40;
	if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x80;
	return ret;
}

static UINT8 mgcsb_joy_r()
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

static UINT8 mgcsb_joy_raw_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x04) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x20) == 0) ret &= ~0x20;
	if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x40;
	return ret;
}

static UINT8 mgcs_coins_r()
{
	UINT8 ret = 0xff;

	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x04;
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x08;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x08;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x10;
	}
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x20;

	if (!controls_mahjong()) {
		if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x40;
		if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x80;
	}

	return ret;
}

static UINT8 mgcsb_coins_raw_r()
{
	UINT8 ret = 0xff;

	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x04;
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x08;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x08;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x10;
	}
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x20;

	if (!controls_mahjong()) {
		if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x40;
	}

	return ret;
}

static UINT8 lhzb2a_coins_raw_r()
{
	UINT8 ret = 0xff;

	if (hopper_line() == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x04;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x08;
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x10;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x20;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x20;
	}
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x40;

	return ret;
}

static UINT8 mgcs_keys_joy_r()
{
	UINT8 ret = 0xff;

	if ((DrvInputSelect & 0x07) == 0x00) {
		ret &= (mgcs_joy_r() | 0x3f);
	}
	if ((DrvInputSelect & 0x08) == 0) ret &= DrvMahjongInputs[0];
	if ((DrvInputSelect & 0x10) == 0) ret &= DrvMahjongInputs[1];
	if ((DrvInputSelect & 0x20) == 0) ret &= DrvMahjongInputs[2];
	if ((DrvInputSelect & 0x40) == 0) ret &= DrvMahjongInputs[3];
	if ((DrvInputSelect & 0x80) == 0) ret &= DrvMahjongInputs[4];

	return ret;
}

static void mgcs_keys_hopper_igs029_w(UINT8 data)
{
	INT32 igs029_irq = ((DrvInputSelect & 0x04) == 0) && (data & 0x04);
	DrvInputSelect = data;
	DrvHopperMotor = data & 1;

	if (igs029_irq) {
		if (!DrvIgs029RecvLen) {
			if (DrvIgs029SendLen < (INT32)sizeof(DrvIgs029SendBuf)) {
				DrvIgs029SendBuf[DrvIgs029SendLen++] = DrvIgs029SendData;
			}

			if (DrvIgs029SendBuf[0] == DrvIgs029SendLen) {
				mgcs_igs029_run();
			}
		}

		if (DrvIgs029RecvLen) {
			DrvIgs029RecvLen--;
			DrvIgs029RecvData = DrvIgs029RecvBuf[DrvIgs029RecvLen];
		}
	}
}

static void mgcs_scramble_data_w(UINT8 data)
{
	DrvScrambleData = data;
}

static UINT8 mgcs_scramble_data_r()
{
	UINT8 tmp = BITSWAP08(DrvScrambleData, 0,1,2,3,4,5,6,7);
	return BITSWAP08((tmp + 1) & 3, 4,5,6,7,0,1,2,3);
}

static UINT8 mgcs_igs029_data_r()
{
	return DrvIgs029RecvData;
}

static void mgcs_igs029_data_w(UINT8 data)
{
	DrvIgs029SendData = data;
}

static UINT8 lhzb2_keys_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputSelect & 0x01) == 0) ret &= DrvMahjongInputs[0];
	if ((DrvInputSelect & 0x02) == 0) ret &= DrvMahjongInputs[1];
	if ((DrvInputSelect & 0x04) == 0) ret &= DrvMahjongInputs[2];
	if ((DrvInputSelect & 0x08) == 0) ret &= DrvMahjongInputs[3];
	if ((DrvInputSelect & 0x10) == 0) ret &= DrvMahjongInputs[4];
	return ret;
}

static UINT8 lhzb2_coins_r()
{
	UINT8 ret = 0xff;
	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x04;
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x08;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x08;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x10;
	}
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x20;
	return ret;
}

static void lhzb2_keys_hopper_w(UINT8 data)
{
	DrvInputSelect = data & 0x1f;
	DrvHopperMotor = (data >> 5) & 1;
	if (data & 0x40) {
		/* coin out counter */
	}
	if (data & 0x80) {
		/* coin in counter */
	}
}

static UINT8 lhzb2_scramble_data_r()
{
	return BITSWAP08(DrvScrambleData, 0,1,2,3,4,5,6,7);
}

static UINT8 lhzb2b_scramble_data_r()
{
	return (BITSWAP08(DrvScrambleData, 0,1,2,3,4,5,6,7) + 1) & 3;
}

static void lhzb2_igs022_execute_w(UINT8 data)
{
	set_oki_bank(BIT(data, 7));
	igs022_execute_w<6, 0x3f>(data);
}

static UINT8 slqz2_player1_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[1] & 0x01) == 0) ret &= ~0x01;
	if ((DrvInputs[1] & 0x02) == 0) ret &= ~0x02;
	if ((DrvInputs[1] & 0x04) == 0) ret &= ~0x04;
	if ((DrvInputs[1] & 0x08) == 0) ret &= ~0x08;
	if ((DrvInputs[1] & 0x10) == 0) ret &= ~0x10;
	if ((DrvInputs[1] & 0x20) == 0) ret &= ~0x20;
	return ret;
}

static UINT8 slqz2_player2_r()
{
	UINT8 ret = 0xff;
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x02;
	return ret;
}

static UINT8 slqz2_coins_r()
{
	UINT8 ret = 0xff;
	if (hopper_line() == 0) ret &= ~0x01;
	if ((DrvInputs[0] & 0x20) == 0) ret &= ~0x02;
	if ((DrvInputs[0] & 0x10) == 0) ret &= ~0x04;
	if (credit_mode_coin()) {
		if ((DrvInputs[0] & 0x01) == 0) ret &= ~0x08;
	} else {
		if ((DrvInputs[0] & 0x02) == 0) ret &= ~0x08;
	}
	if (payout_mode_hopper()) {
		if ((DrvInputs[0] & 0x08) == 0) ret &= ~0x10;
	} else {
		if ((DrvInputs[0] & 0x04) == 0) ret &= ~0x10;
	}
	if ((DrvInputs[0] & 0x40) == 0) ret &= ~0x20;
	if ((DrvInputs[1] & 0x40) == 0) ret &= ~0x40;
	if ((DrvInputs[1] & 0x80) == 0) ret &= ~0x80;
	return ret;
}

static void slqz2_sound_hopper_w(UINT8 data)
{
	set_oki_bank(BIT(data, 0));
	DrvHopperMotor = (data >> 5) & 1;
}

static UINT8 slqz2_scramble_data_r()
{
	return DrvScrambleData;
}

static void lhzb2b_igs029_run()
{
	if (DrvIgs029SendBuf[0] == 0x05 && DrvIgs029SendBuf[1] == 0x5a) {
		UINT8 data = DrvIgs029SendBuf[2];
		UINT8 port = DrvIgs029SendBuf[3];

		if (port == 0x01) {
			DrvDswSelect = data;
		}

		DrvIgs029RecvLen = 0;
		DrvIgs029RecvBuf[DrvIgs029RecvLen++] = 0x01;
	} else {
		common_igs029_run();
	}

	DrvIgs029SendLen = 0;
}

static void lhzb2b_igs029_scramble_irq_w(UINT8 data)
{
	INT32 igs029_irq = BIT(data, 3);
	DrvScrambleData = data;

	if (igs029_irq) {
		if (!DrvIgs029RecvLen) {
			if (DrvIgs029SendLen < (INT32)sizeof(DrvIgs029SendBuf)) {
				DrvIgs029SendBuf[DrvIgs029SendLen++] = DrvIgs029SendData;
			}

			if (DrvIgs029SendBuf[0] == DrvIgs029SendLen) {
				lhzb2b_igs029_run();
			}
		}

		if (DrvIgs029RecvLen) {
			DrvIgs029RecvLen--;
			DrvIgs029RecvData = DrvIgs029RecvBuf[DrvIgs029RecvLen];
		}
	}
}

static UINT16 lhzb2a_input_r(UINT32 address)
{
	switch (address & 0x0006)
	{
		case 0x0000:
			return (lhzb2_keys_r() << 8) | 0xff;

		case 0x0002:
			return (DrvDips[0] << 8) | lhzb2a_coins_raw_r();

		case 0x0004:
			return 0xff00 | DrvDips[1];
	}

	return 0xffff;
}

static UINT8 __fastcall lhzb2a_input_read_byte(UINT32 address)
{
	UINT16 data = lhzb2a_input_r(address);
	return (data >> ((~address & 1) * 8)) & 0xff;
}

static UINT16 __fastcall lhzb2a_input_read_word(UINT32 address)
{
	return lhzb2a_input_r(address);
}

static void __fastcall lhzb2a_keys_hopper_write_word(UINT32, UINT16 data)
{
	DrvInputSelect = data & 0x1f;
	DrvHopperMotor = (data >> 5) & 1;
	set_oki_bank((data >> 8) & 1);
}

static void __fastcall lhzb2a_keys_hopper_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 1) == 0) {
		DrvInputSelect = data & 0x1f;
		DrvHopperMotor = (data >> 5) & 1;
	} else {
		set_oki_bank(data & 1);
	}
}

static void mgcsb_counter_w(UINT8 data)
{
	DrvHopperMotor = (data >> 5) & 1;
}

static UINT16 mgcsb_input_r(UINT32 address)
{
	switch (address & 0x0006)
	{
		case 0x0000:
			return (DrvDips[1] << 8) | 0xff;

		case 0x0002:
			return (mgcsb_joy_raw_r() << 8) | mgcsb_joy_raw_r();

		case 0x0004:
			return 0xff00 | mgcsb_coins_raw_r();

		case 0x0006:
			return 0xff00 | DrvDips[0];
	}

	return 0xffff;
}

static UINT8 __fastcall mgcsb_remap_read_byte(UINT32 address)
{
	UINT32 base = (DrvRemapAddr & 0xff) << 16;

	if (address == (base + 0x4003)) {
		UINT8 ret = mux_data_read();
		return ret;
	}
	if ((DrvProfile == PROFILE_MGCSB || DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2C) &&
		address >= (base + 0x8000) && address <= (base + 0x8007)) {
		if (DrvProfile == PROFILE_MGCSB) {
			UINT8 ret = (mgcsb_input_r(address) >> ((~address & 1) * 8)) & 0xff;
			return ret;
		}
		UINT8 ret = lhzb2a_input_read_byte(address);
		return ret;
	}

	if (DrvProfile == PROFILE_LHZB2C && address == (base + 0x4001)) {
		return 0xff;
	}

	if (address >= (base + 0x8000) && address <= (base + 0x8007) && DrvProfile == PROFILE_MGCSB) {
		UINT8 ret = (mgcsb_input_r(address) >> ((~address & 1) * 8)) & 0xff;
		return ret;
	}

	return 0xff;
}

static UINT16 __fastcall mgcsb_remap_read_word(UINT32 address)
{
	UINT32 base = (DrvRemapAddr & 0xff) << 16;

	if (address == (base + 0x4000) || address == (base + 0x4002)) {
		UINT16 ret = 0xff00 | mux_data_read();
		return ret;
	}

	if (DrvProfile == PROFILE_MGCSB &&
		(address == (base + 0x8000) || address == (base + 0x8002) || address == (base + 0x8004) || address == (base + 0x8006))) {
		UINT16 ret = mgcsb_input_r(address);
		return ret;
	}

	if ((DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2C) &&
		(address == (base + 0x8000) || address == (base + 0x8002) || address == (base + 0x8004))) {
		UINT16 ret = lhzb2a_input_read_word(address);
		return ret;
	}

	return 0xffff;
}

static void __fastcall mgcsb_remap_write_byte(UINT32 address, UINT8 data)
{
	UINT32 base = (DrvRemapAddr & 0xff) << 16;

	if (address == (base + 0x4001)) {
		DrvMuxAddr = data;
		return;
	}

	if (address == (base + 0x4003)) {
		mux_data_write(data);
		return;
	}

	if ((DrvProfile == PROFILE_MGCSB || DrvProfile == PROFILE_LHZB2A) && address == (base + 0xc001)) {
		DrvRemapAddr = data & 0xff;
		return;
	}
}

static void __fastcall mgcsb_remap_write_word(UINT32 address, UINT16 data)
{
	UINT32 base = (DrvRemapAddr & 0xff) << 16;

	if (address == (base + 0x4000)) {
		DrvMuxAddr = data & 0xff;
		return;
	}

	if (address == (base + 0x4002)) {
		mux_data_write(data & 0xff);
		return;
	}

	if ((DrvProfile == PROFILE_MGCSB || DrvProfile == PROFILE_LHZB2A) && address == (base + 0xc000)) {
		DrvRemapAddr = data & 0xff;
		return;
	}

	mgcsb_remap_write_byte(address, data & 0xff);
}

static UINT8 video_read(UINT32 address, UINT32 base)
{
	return igs017_igs031_read((address - base) >> 1);
}

static void video_write(UINT32 address, UINT32 base, UINT8 data)
{
	igs017_igs031_write((address - base) >> 1, data);
}

static UINT16 __fastcall igs017_video_read_word(UINT32 address)
{
	UINT32 base = 0xa00000;
	if (DrvProfile == PROFILE_SDMG2P) base = 0xb00000;
	if (DrvProfile == PROFILE_SDMG2) base = 0x200000;
	if (DrvProfile == PROFILE_MGCSA) base = 0x900000;
	if (DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2B || DrvProfile == PROFILE_LHZB2C) base = 0xb00000;
	return 0xff00 | video_read(address, base);
}

static UINT8 __fastcall igs017_video_read_byte(UINT32 address)
{
	UINT32 base = 0xa00000;
	if (DrvProfile == PROFILE_SDMG2P) base = 0xb00000;
	if (DrvProfile == PROFILE_SDMG2) base = 0x200000;
	if (DrvProfile == PROFILE_MGCSA) base = 0x900000;
	if (DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2B || DrvProfile == PROFILE_LHZB2C) base = 0xb00000;
	return video_read(address, base);
}

static void __fastcall igs017_video_write_word(UINT32 address, UINT16 data)
{
	UINT32 base = 0xa00000;
	if (DrvProfile == PROFILE_SDMG2P) base = 0xb00000;
	if (DrvProfile == PROFILE_SDMG2) base = 0x200000;
	if (DrvProfile == PROFILE_MGCSA) base = 0x900000;
	if (DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2B || DrvProfile == PROFILE_LHZB2C) base = 0xb00000;
	video_write(address, base, data & 0xff);
}

static void __fastcall igs017_video_write_byte(UINT32 address, UINT8 data)
{
	UINT32 base = 0xa00000;
	if (DrvProfile == PROFILE_SDMG2P) base = 0xb00000;
	if (DrvProfile == PROFILE_SDMG2) base = 0x200000;
	if (DrvProfile == PROFILE_MGCSA) base = 0x900000;
	if (DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2B || DrvProfile == PROFILE_LHZB2C) base = 0xb00000;
	video_write(address, base, data);
}

static UINT16 __fastcall igs017_oki_read_word(UINT32)
{
	return 0xff00 | MSM6295Read(0);
}

static UINT8 __fastcall igs017_oki_read_byte(UINT32)
{
	return MSM6295Read(0);
}

static void __fastcall igs017_oki_write_word(UINT32, UINT16 data)
{
	MSM6295Write(0, data & 0xff);
}

static void __fastcall igs017_oki_write_byte(UINT32, UINT8 data)
{
	MSM6295Write(0, data);
}

static UINT16 __fastcall igs017_mux_read_word(UINT32)
{
	return 0xff00 | mux_data_read();
}

static UINT8 __fastcall igs017_mux_read_byte(UINT32)
{
	return mux_data_read();
}

static void __fastcall igs017_mux_write_word(UINT32 address, UINT16 data)
{
	if ((address & 2) == 0) {
		DrvMuxAddr = data & 0xff;
	} else {
		mux_data_write(data & 0xff);
	}
}

static void __fastcall igs017_mux_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 2) == 0) {
		DrvMuxAddr = data;
	} else {
		mux_data_write(data);
	}
}

static UINT8 rom_fallback_read_byte(UINT32 address)
{
	address &= 0xffffff;
	if (address >= (UINT32)Drv68KLen) return 0xff;
	return Drv68KROM[address ^ 1];
}

static UINT16 rom_fallback_read_word(UINT32 address)
{
	address &= 0xffffff;

	if (address & 1) {
		return (rom_fallback_read_byte(address + 0) << 8) | rom_fallback_read_byte(address + 1);
	}

	if ((address + 1) >= (UINT32)Drv68KLen) return 0xffff;
	return BURN_ENDIAN_SWAP_INT16(*((UINT16*)(Drv68KROM + address)));
}

static UINT16 __fastcall sdmg2_incdec_read_word(UINT32 address)
{
	UINT16 ret = (address == 0x00200a) ? (0xff00 | incdec_result()) : rom_fallback_read_word(address);
	return ret;
}

static UINT8 __fastcall sdmg2_incdec_read_byte(UINT32 address)
{
	UINT8 ret = (address == 0x00200b) ? incdec_result() : rom_fallback_read_byte(address);
	return ret;
}

static void __fastcall sdmg2_incdec_write_word(UINT32 address, UINT16)
{
	switch (address)
	{
		case 0x002000: DrvIncDecVal = 0; break;
		case 0x002002: DrvIncDecVal--; break;
		case 0x002006: DrvIncDecVal++; break;
		default: return;
	}
}

static void __fastcall sdmg2_incdec_write_byte(UINT32 address, UINT8)
{
	switch (address)
	{
		case 0x002001: DrvIncDecVal = 0; break;
		case 0x002003: DrvIncDecVal--; break;
		case 0x002007: DrvIncDecVal++; break;
		default: return;
	}
}

static UINT16 __fastcall lhzb2_incdec_read_word(UINT32 address)
{
	UINT16 ret = (address == 0x00320a) ? (0xff00 | incdec_result()) : rom_fallback_read_word(address);
	return ret;
}

static UINT8 __fastcall lhzb2_incdec_read_byte(UINT32 address)
{
	UINT8 ret = (address == 0x00320b) ? incdec_result() : rom_fallback_read_byte(address);
	return ret;
}

static void __fastcall lhzb2_incdec_write_word(UINT32 address, UINT16)
{
	switch (address)
	{
		case 0x003200: DrvIncDecVal = 0; break;
		case 0x003202: DrvIncDecVal--; break;
		case 0x003206: DrvIncDecVal++; break;
		default: return;
	}
}

static void __fastcall lhzb2_incdec_write_byte(UINT32 address, UINT8)
{
	switch (address)
	{
		case 0x003201: DrvIncDecVal = 0; break;
		case 0x003203: DrvIncDecVal--; break;
		case 0x003207: DrvIncDecVal++; break;
		default: return;
	}
}

static UINT16 __fastcall sdmg2p_incdec_read_word(UINT32 address)
{
	UINT16 ret = (address == 0x00304a) ? (0xff00 | incdec_result()) : rom_fallback_read_word(address);
	return ret;
}

static UINT8 __fastcall sdmg2p_incdec_read_byte(UINT32 address)
{
	UINT8 ret = (address == 0x00304b) ? incdec_result() : rom_fallback_read_byte(address);
	return ret;
}

static void __fastcall sdmg2p_incdec_write_word(UINT32 address, UINT16)
{
	switch (address)
	{
		case 0x003040: DrvIncDecVal = 0; break;
		case 0x003042: DrvIncDecVal--; break;
		case 0x003046: DrvIncDecVal++; break;
		default: return;
	}
}

static void __fastcall sdmg2p_incdec_write_byte(UINT32 address, UINT8)
{
	switch (address)
	{
		case 0x003041: DrvIncDecVal = 0; break;
		case 0x003043: DrvIncDecVal--; break;
		case 0x003047: DrvIncDecVal++; break;
		default: return;
	}
}

static UINT16 __fastcall sdmg2_incalt_read_word(UINT32)
{
	return incalt_result();
}

static UINT8 __fastcall sdmg2_incalt_read_byte(UINT32)
{
	return incalt_result();
}

static void __fastcall sdmg2_incalt_write_word(UINT32, UINT16 data)
{
	incalt_write(data & 0xff);
}

static void __fastcall sdmg2_incalt_write_byte(UINT32, UINT8 data)
{
	incalt_write(data);
}

static INT32 MemIndex()
{
	UINT8 *Next = AllMem;

	Drv68KROM = Next; Next += Drv68KLen;
	DrvTileROM = Next; Next += DrvTileLen;
	DrvSpriteROM = Next; Next += DrvSpriteLen;
	DrvSndROM = Next; Next += DrvSndLen;
	DrvStringROM = Next; Next += DrvStringLen;
	DrvNVRAM = Next; Next += DrvNVRAMLen;
	DrvProtROM = Next; Next += DrvProtLen;
	DrvProtRAM = Next; Next += 0x4000;
	DrvFixedData = Next; Next += DrvFixedLen;

	MemEnd = Next;

	return 0;
}

static INT32 drv_get_roms(INT32 bLoad)
{
	BurnRomInfo ri;
	UINT8 *mainLoad = Drv68KROM;
	UINT8 *tileLoad = DrvTileROM;
	UINT8 *spriteLoad = DrvSpriteROM;
	UINT8 *sndLoad = DrvSndROM;
	UINT8 *stringLoad = DrvStringROM;
	UINT8 *protLoad = DrvProtROM;
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
					Drv68KLen += ri.nLen;
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

			case 6:
				if (bLoad) {
					if (BurnLoadRom(protLoad, i, 1)) return 1;
					protLoad += ri.nLen;
				} else {
					DrvProtLen += ri.nLen;
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

static void decrypt_sdmg2754ca() { igs017_crypt_sdmg2754ca(Drv68KROM, Drv68KLen); }
static void decrypt_sdmg2754cb() { igs017_crypt_sdmg2754cb(Drv68KROM, Drv68KLen); }
static void decrypt_sdmg2() { igs017_crypt_sdmg2(Drv68KROM, Drv68KLen); }
static void decrypt_sdmg2p() { igs017_crypt_sdmg2p(Drv68KROM, Drv68KLen); }
static void decrypt_mgdha() { igs017_crypt_mgdha(Drv68KROM, Drv68KLen); igs017_igs031_mgcs_flip_sprites(DrvSpriteROM, DrvSpriteLen, 0); }
static void decrypt_mgcs() { igs017_crypt_mgcs(Drv68KROM, Drv68KLen); igs017_igs031_mgcs_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_mgcs_flip_sprites(DrvSpriteROM, DrvSpriteLen, 0); }
static void decrypt_mgcsa() { igs017_crypt_mgcsa(Drv68KROM, Drv68KLen); igs017_igs031_mgcs_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_mgcs_flip_sprites(DrvSpriteROM, DrvSpriteLen, 0); }
static void decrypt_mgcsb() { igs017_crypt_mgcsb(Drv68KROM, Drv68KLen); igs017_igs031_mgcs_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_mgcs_flip_sprites(DrvSpriteROM, DrvSpriteLen, 0); }
static void decrypt_lhzb2a() { igs017_crypt_lhzb2a(Drv68KROM, Drv68KLen); igs017_igs031_lhzb2_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_lhzb2_decrypt_sprites(DrvSpriteROM, DrvSpriteLen); }
static void decrypt_lhzb2() { igs017_crypt_lhzb2(Drv68KROM, Drv68KLen); igs017_igs031_lhzb2_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_lhzb2_decrypt_sprites(DrvSpriteROM, DrvSpriteLen); }
static void decrypt_lhzb2c() { igs017_crypt_lhzb2c(Drv68KROM, Drv68KLen); igs017_igs031_lhzb2_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_lhzb2_decrypt_sprites(DrvSpriteROM, DrvSpriteLen); }
static void decrypt_lhzb2b() { igs017_crypt_lhzb2b(Drv68KROM, Drv68KLen); igs017_igs031_lhzb2_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_lhzb2_decrypt_sprites(DrvSpriteROM, DrvSpriteLen); }
static void decrypt_slqz2() { igs017_crypt_slqz2(Drv68KROM, Drv68KLen); igs017_igs031_slqz2_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_lhzb2_decrypt_sprites(DrvSpriteROM, DrvSpriteLen); }
static void decrypt_slqz2b() { igs017_crypt_slqz2b(Drv68KROM, Drv68KLen); igs017_igs031_slqz2_decrypt_tiles(DrvTileROM, DrvTileLen); igs017_igs031_lhzb2_decrypt_sprites(DrvSpriteROM, DrvSpriteLen); }
static void decrypt_program_rom(UINT8 mask, INT32 a7, INT32 a6, INT32 a5, INT32 a4, INT32 a3, INT32 a2, INT32 a1, INT32 a0) { igs017_crypt_program_rom(Drv68KROM, Drv68KLen, mask, a7, a6, a5, a4, a3, a2, a1, a0); }

static INT32 DrvDoReset()
{
	memset(DrvNVRAM, 0, DrvNVRAMLen);
	DrvMuxAddr = 0;
	DrvInputSelect = 0xff;
	DrvOkiBank = 0;
	DrvHopperMotor = 0;
	DrvCoin1Pulse = 0;
	DrvCoin1Prev = 0;
	DrvBootPtr = 0;
	DrvIncDecVal = 0;
	DrvIncAltVal = 0;
	DrvDswSelect = 0xff;
	DrvScrambleData = 0;
	DrvIgs029SendData = 0;
	DrvIgs029RecvData = 0;
	DrvIgs029SendLen = 0;
	DrvIgs029RecvLen = 0;
	DrvStringOffs = 0;
	DrvStringWord = 0;
	DrvStringValue = 0;
	DrvBitswapM3 = 0;
	DrvBitswapMF = 0;
	DrvBitswapVal = 0;
	DrvBitswapWord = 0;
	DrvIgs022Latch = 0;
	DrvRemapAddr = (DrvProfile == PROFILE_MGCSB || DrvProfile == PROFILE_LHZB2A || DrvProfile == PROFILE_LHZB2C) ? 0xf0 : -1;
	memset(DrvIgs029SendBuf, 0, sizeof(DrvIgs029SendBuf));
	memset(DrvIgs029RecvBuf, 0, sizeof(DrvIgs029RecvBuf));
	memset(DrvIgs029Reg, 0, sizeof(DrvIgs029Reg));
	memset(DrvIgs022Regs, 0, sizeof(DrvIgs022Regs));
	memset(DrvIgs022Stack, 0, sizeof(DrvIgs022Stack));
	DrvIgs022StackPtr = 0;

	if (DrvProfile == PROFILE_LHZB2 || DrvProfile == PROFILE_SLQZ2) {
		igs022_reset();
	}

	SekOpen(0);
	SekReset();
	SekClose();

	MSM6295Reset(0);
	set_oki_bank(0);
	igs017_igs031_reset();

	return 0;
}

static INT32 common_init(INT32 profile, INT32 nvram_len, UINT8 has_boot, UINT8 nmi_line, void (*decrypt)())
{
	DrvProfile = profile;
	DrvNVRAMLen = nvram_len;
	DrvHasBootRead = has_boot;
	DrvNmiLine = nmi_line;
	apply_default_dips_if_invalid();
	Drv68KLen = DrvTileLen = DrvSpriteLen = DrvSndLen = DrvStringLen = DrvProtLen = DrvFixedLen = 0;
	DrvBitswapValXor = 0;
	memset(DrvBitswapM3Bits, 0, sizeof(DrvBitswapM3Bits));
	memset(DrvBitswapMFBits, 0, sizeof(DrvBitswapMFBits));

	if (drv_get_roms(0)) return 1;

	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8*)0;
	AllMem = (UINT8*)BurnMalloc(nLen);
	if (AllMem == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();
	memset(DrvNVRAM, 0, DrvNVRAMLen);

	if (drv_get_roms(1)) return 1;
	if (decrypt) decrypt();

	igs017_igs031_set_text_reverse_bits(0);
	igs017_igs031_set_palette_scramble(NULL);
	switch (DrvProfile)
	{
		case PROFILE_MGCS:
		case PROFILE_MGCSA:
		case PROFILE_MGCSB:
			igs017_igs031_set_palette_scramble(mgcs_palette_bitswap);
		break;

		case PROFILE_LHZB2:
		case PROFILE_LHZB2A:
		case PROFILE_LHZB2B:
		case PROFILE_LHZB2C:
			igs017_igs031_set_palette_scramble(lhzb2a_palette_bitswap);
		break;

		case PROFILE_SLQZ2:
			igs017_igs031_set_palette_scramble(slqz2_palette_bitswap);
		break;
	}
	if (igs017_igs031_init(DrvTileROM, DrvTileLen, DrvSpriteROM, DrvSpriteLen)) return 1;

	switch (DrvProfile)
	{
		case PROFILE_MGDH:
			igs017_igs031_set_inputs(read_dip_a, read_ff, read_ff);
		break;

		case PROFILE_SDMG2:
			igs017_igs031_set_inputs(read_dip_a, read_dip_b, read_ff);
		break;

		case PROFILE_SDMG2P:
			igs017_igs031_set_inputs(read_dip_a, read_dip_b, read_dip_c);
		break;

		case PROFILE_MGCS:
		case PROFILE_MGCSA:
			igs017_igs031_set_inputs(mgcs_coins_r, mgcs_keys_joy_r, mgcs_joy_r);
		break;

		case PROFILE_MGCSB:
			igs017_igs031_set_inputs(read_ff, read_ff, read_ff);
		break;

		case PROFILE_LHZB2B:
			igs017_igs031_set_inputs(lhzb2_coins_r, mgcs_igs029_data_r, lhzb2b_scramble_data_r);
		break;

		case PROFILE_LHZB2:
			igs017_igs031_set_inputs(lhzb2_coins_r, read_dip_a, read_dip_b);
		break;

		case PROFILE_SLQZ2:
			igs017_igs031_set_inputs(slqz2_coins_r, read_dip_a, read_dip_b);
		break;

		case PROFILE_LHZB2A:
		case PROFILE_LHZB2C:
			igs017_igs031_set_inputs(read_ff, read_ff, read_ff);
		break;
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM, 0x000000, Drv68KLen - 1, MAP_ROM);

	if (DrvProfile == PROFILE_MGDH) {
		SekMapMemory(DrvNVRAM, 0x600000, 0x600000 + DrvNVRAMLen - 1, MAP_RAM);
		SekMapHandler(1, 0x876000, 0x876003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, igs017_mux_read_word);
		SekSetReadByteHandler(1, igs017_mux_read_byte);
		SekSetWriteWordHandler(1, igs017_mux_write_word);
		SekSetWriteByteHandler(1, igs017_mux_write_byte);

		SekMapHandler(2, 0xa00000, 0xa0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_video_read_word);
		SekSetReadByteHandler(2, igs017_video_read_byte);
		SekSetWriteWordHandler(2, igs017_video_write_word);
		SekSetWriteByteHandler(2, igs017_video_write_byte);

		SekMapHandler(3, 0xa10000, 0xa10001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_oki_read_word);
		SekSetReadByteHandler(3, igs017_oki_read_byte);
		SekSetWriteWordHandler(3, igs017_oki_write_word);
		SekSetWriteByteHandler(3, igs017_oki_write_byte);
	} else if (DrvProfile == PROFILE_SDMG2) {
		SekMapMemory(DrvNVRAM, 0x1f0000, 0x1f0000 + DrvNVRAMLen - 1, MAP_RAM);
		SekMapHandler(1, 0x002000, 0x00200f, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, sdmg2_incdec_read_word);
		SekSetReadByteHandler(1, sdmg2_incdec_read_byte);
		SekSetWriteWordHandler(1, sdmg2_incdec_write_word);
		SekSetWriteByteHandler(1, sdmg2_incdec_write_byte);

		SekMapHandler(2, 0x010000, 0x0107ff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, sdmg2_incalt_read_word);
		SekSetReadByteHandler(2, sdmg2_incalt_read_byte);
		SekSetWriteWordHandler(2, sdmg2_incalt_write_word);
		SekSetWriteByteHandler(2, sdmg2_incalt_write_byte);

		SekMapHandler(3, 0x200000, 0x20ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_video_read_word);
		SekSetReadByteHandler(3, igs017_video_read_byte);
		SekSetWriteWordHandler(3, igs017_video_write_word);
		SekSetWriteByteHandler(3, igs017_video_write_byte);

		SekMapHandler(4, 0x210000, 0x210001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(4, igs017_oki_read_word);
		SekSetReadByteHandler(4, igs017_oki_read_byte);
		SekSetWriteWordHandler(4, igs017_oki_write_word);
		SekSetWriteByteHandler(4, igs017_oki_write_byte);

		SekMapHandler(5, 0x300000, 0x300003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(5, igs017_mux_read_word);
		SekSetReadByteHandler(5, igs017_mux_read_byte);
		SekSetWriteWordHandler(5, igs017_mux_write_word);
		SekSetWriteByteHandler(5, igs017_mux_write_byte);
	} else if (DrvProfile == PROFILE_SDMG2P) {
		SekMapMemory(DrvNVRAM, 0x100000, 0x100000 + DrvNVRAMLen - 1, MAP_RAM);
		SekMapHandler(1, 0x003040, 0x00304b, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, sdmg2p_incdec_read_word);
		SekSetReadByteHandler(1, sdmg2p_incdec_read_byte);
		SekSetWriteWordHandler(1, sdmg2p_incdec_write_word);
		SekSetWriteByteHandler(1, sdmg2p_incdec_write_byte);

		SekMapHandler(2, 0x38d000, 0x38d003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_mux_read_word);
		SekSetReadByteHandler(2, igs017_mux_read_byte);
		SekSetWriteWordHandler(2, igs017_mux_write_word);
		SekSetWriteByteHandler(2, igs017_mux_write_byte);

		SekMapHandler(3, 0xb00000, 0xb0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_video_read_word);
		SekSetReadByteHandler(3, igs017_video_read_byte);
		SekSetWriteWordHandler(3, igs017_video_write_word);
		SekSetWriteByteHandler(3, igs017_video_write_byte);

		SekMapHandler(4, 0xb10000, 0xb10001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(4, igs017_oki_read_word);
		SekSetReadByteHandler(4, igs017_oki_read_byte);
		SekSetWriteWordHandler(4, igs017_oki_write_word);
		SekSetWriteByteHandler(4, igs017_oki_write_byte);
	} else if (DrvProfile == PROFILE_MGCS || DrvProfile == PROFILE_MGCSA) {
		UINT32 nvram_base = (DrvProfile == PROFILE_MGCSA) ? 0x100000 : 0x300000;
		UINT32 video_base = (DrvProfile == PROFILE_MGCSA) ? 0x900000 : 0xa00000;
		UINT32 oki_base = (DrvProfile == PROFILE_MGCSA) ? 0x912000 : 0xa12000;

		SekMapMemory(DrvNVRAM, nvram_base, nvram_base + DrvNVRAMLen - 1, MAP_RAM);

		SekMapHandler(1, 0x49c000, 0x49c003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, igs017_mux_read_word);
		SekSetReadByteHandler(1, igs017_mux_read_byte);
		SekSetWriteWordHandler(1, igs017_mux_write_word);
		SekSetWriteByteHandler(1, igs017_mux_write_byte);

		SekMapHandler(2, video_base, video_base + 0x0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_video_read_word);
		SekSetReadByteHandler(2, igs017_video_read_byte);
		SekSetWriteWordHandler(2, igs017_video_write_word);
		SekSetWriteByteHandler(2, igs017_video_write_byte);

		SekMapHandler(3, oki_base, oki_base + 1, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_oki_read_word);
		SekSetReadByteHandler(3, igs017_oki_read_byte);
		SekSetWriteWordHandler(3, igs017_oki_write_word);
		SekSetWriteByteHandler(3, igs017_oki_write_byte);
	} else if (DrvProfile == PROFILE_MGCSB) {
		SekMapMemory(DrvNVRAM, 0x300000, 0x300000 + DrvNVRAMLen - 1, MAP_RAM);

		SekMapHandler(1, 0x002000, 0x00200f, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, sdmg2_incdec_read_word);
		SekSetReadByteHandler(1, sdmg2_incdec_read_byte);
		SekSetWriteWordHandler(1, sdmg2_incdec_write_word);
		SekSetWriteByteHandler(1, sdmg2_incdec_write_byte);

		SekMapHandler(2, 0x49c000, 0x49c003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_mux_read_word);
		SekSetReadByteHandler(2, igs017_mux_read_byte);
		SekSetWriteWordHandler(2, igs017_mux_write_word);
		SekSetWriteByteHandler(2, igs017_mux_write_byte);

		SekMapHandler(3, 0xa00000, 0xa0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_video_read_word);
		SekSetReadByteHandler(3, igs017_video_read_byte);
		SekSetWriteWordHandler(3, igs017_video_write_word);
		SekSetWriteByteHandler(3, igs017_video_write_byte);

		SekMapHandler(4, 0xa10000, 0xa10001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(4, igs017_oki_read_word);
		SekSetReadByteHandler(4, igs017_oki_read_byte);
		SekSetWriteWordHandler(4, igs017_oki_write_word);
		SekSetWriteByteHandler(4, igs017_oki_write_byte);

		SekSetReadWordHandler(0, mgcsb_remap_read_word);
		SekSetReadByteHandler(0, mgcsb_remap_read_byte);
		SekSetWriteWordHandler(0, mgcsb_remap_write_word);
		SekSetWriteByteHandler(0, mgcsb_remap_write_byte);
	} else if (DrvProfile == PROFILE_LHZB2A) {
		SekMapMemory(DrvNVRAM, 0x500000, 0x500000 + DrvNVRAMLen - 1, MAP_RAM);

		SekMapHandler(1, 0x003200, 0x00320f, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, lhzb2_incdec_read_word);
		SekSetReadByteHandler(1, lhzb2_incdec_read_byte);
		SekSetWriteWordHandler(1, lhzb2_incdec_write_word);
		SekSetWriteByteHandler(1, lhzb2_incdec_write_byte);

		SekMapHandler(2, 0xb00000, 0xb0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_video_read_word);
		SekSetReadByteHandler(2, igs017_video_read_byte);
		SekSetWriteWordHandler(2, igs017_video_write_word);
		SekSetWriteByteHandler(2, igs017_video_write_byte);

		SekMapHandler(3, 0xb10000, 0xb10001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_oki_read_word);
		SekSetReadByteHandler(3, igs017_oki_read_byte);
		SekSetWriteWordHandler(3, igs017_oki_write_word);
		SekSetWriteByteHandler(3, igs017_oki_write_byte);

		SekMapHandler(4, 0xb12000, 0xb12001, MAP_WRITE);
		SekSetWriteWordHandler(4, lhzb2a_keys_hopper_write_word);
		SekSetWriteByteHandler(4, lhzb2a_keys_hopper_write_byte);

		SekSetReadWordHandler(0, mgcsb_remap_read_word);
		SekSetReadByteHandler(0, mgcsb_remap_read_byte);
		SekSetWriteWordHandler(0, mgcsb_remap_write_word);
		SekSetWriteByteHandler(0, mgcsb_remap_write_byte);
	} else if (DrvProfile == PROFILE_LHZB2B) {
		SekMapMemory(DrvNVRAM, 0x500000, 0x500000 + DrvNVRAMLen - 1, MAP_RAM);

		SekMapHandler(1, 0x003200, 0x00320f, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, lhzb2_incdec_read_word);
		SekSetReadByteHandler(1, lhzb2_incdec_read_byte);
		SekSetWriteWordHandler(1, lhzb2_incdec_write_word);
		SekSetWriteByteHandler(1, lhzb2_incdec_write_byte);

		SekMapHandler(2, 0x910000, 0x910003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_mux_read_word);
		SekSetReadByteHandler(2, igs017_mux_read_byte);
		SekSetWriteWordHandler(2, igs017_mux_write_word);
		SekSetWriteByteHandler(2, igs017_mux_write_byte);

		SekMapHandler(3, 0xb00000, 0xb0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_video_read_word);
		SekSetReadByteHandler(3, igs017_video_read_byte);
		SekSetWriteWordHandler(3, igs017_video_write_word);
		SekSetWriteByteHandler(3, igs017_video_write_byte);

		SekMapHandler(4, 0xb10000, 0xb10001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(4, igs017_oki_read_word);
		SekSetReadByteHandler(4, igs017_oki_read_byte);
		SekSetWriteWordHandler(4, igs017_oki_write_word);
		SekSetWriteByteHandler(4, igs017_oki_write_byte);
	} else if (DrvProfile == PROFILE_LHZB2C) {
		SekMapMemory(DrvNVRAM, 0x500000, 0x500000 + DrvNVRAMLen - 1, MAP_RAM);

		SekMapHandler(1, 0x003200, 0x00320f, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, lhzb2_incdec_read_word);
		SekSetReadByteHandler(1, lhzb2_incdec_read_byte);
		SekSetWriteWordHandler(1, lhzb2_incdec_write_word);
		SekSetWriteByteHandler(1, lhzb2_incdec_write_byte);

		SekMapHandler(2, 0x910000, 0x910003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_mux_read_word);
		SekSetReadByteHandler(2, igs017_mux_read_byte);
		SekSetWriteWordHandler(2, igs017_mux_write_word);
		SekSetWriteByteHandler(2, igs017_mux_write_byte);

		SekMapHandler(3, 0xb00000, 0xb0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_video_read_word);
		SekSetReadByteHandler(3, igs017_video_read_byte);
		SekSetWriteWordHandler(3, igs017_video_write_word);
		SekSetWriteByteHandler(3, igs017_video_write_byte);

		SekMapHandler(4, 0xb10000, 0xb10001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(4, igs017_oki_read_word);
		SekSetReadByteHandler(4, igs017_oki_read_byte);
		SekSetWriteWordHandler(4, igs017_oki_write_word);
		SekSetWriteByteHandler(4, igs017_oki_write_byte);

		SekSetReadWordHandler(0, mgcsb_remap_read_word);
		SekSetReadByteHandler(0, mgcsb_remap_read_byte);
		SekSetWriteWordHandler(0, mgcsb_remap_write_word);
		SekSetWriteByteHandler(0, mgcsb_remap_write_byte);
	} else if (DrvProfile == PROFILE_LHZB2) {
		SekMapMemory(DrvProtRAM, 0x100000, 0x103fff, MAP_RAM);
		SekMapMemory(DrvNVRAM, 0x500000, 0x503fff, MAP_RAM);

		SekMapHandler(1, 0x910000, 0x910003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, igs017_mux_read_word);
		SekSetReadByteHandler(1, igs017_mux_read_byte);
		SekSetWriteWordHandler(1, igs017_mux_write_word);
		SekSetWriteByteHandler(1, igs017_mux_write_byte);

		SekMapHandler(2, 0xb00000, 0xb0ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_video_read_word);
		SekSetReadByteHandler(2, igs017_video_read_byte);
		SekSetWriteWordHandler(2, igs017_video_write_word);
		SekSetWriteByteHandler(2, igs017_video_write_byte);

		SekMapHandler(3, 0xb10000, 0xb10001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_oki_read_word);
		SekSetReadByteHandler(3, igs017_oki_read_byte);
		SekSetWriteWordHandler(3, igs017_oki_write_word);
		SekSetWriteByteHandler(3, igs017_oki_write_byte);
	} else if (DrvProfile == PROFILE_SLQZ2) {
		SekMapMemory(DrvNVRAM, 0x100000, 0x103fff, MAP_RAM);
		SekMapMemory(DrvProtRAM, 0x300000, 0x303fff, MAP_RAM);

		SekMapHandler(1, 0x602000, 0x602003, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(1, igs017_mux_read_word);
		SekSetReadByteHandler(1, igs017_mux_read_byte);
		SekSetWriteWordHandler(1, igs017_mux_write_word);
		SekSetWriteByteHandler(1, igs017_mux_write_byte);

		SekMapHandler(2, 0x900000, 0x90ffff, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(2, igs017_video_read_word);
		SekSetReadByteHandler(2, igs017_video_read_byte);
		SekSetWriteWordHandler(2, igs017_video_write_word);
		SekSetWriteByteHandler(2, igs017_video_write_byte);

		SekMapHandler(3, 0x910000, 0x910001, MAP_READ | MAP_WRITE);
		SekSetReadWordHandler(3, igs017_oki_read_word);
		SekSetReadByteHandler(3, igs017_oki_read_byte);
		SekSetWriteWordHandler(3, igs017_oki_write_word);
		SekSetWriteByteHandler(3, igs017_oki_write_byte);
	}
	SekClose();

	MSM6295ROM = DrvSndROM;
	MSM6295Init(0, 1000000 / MSM6295_PIN7_HIGH, 0);
	MSM6295SetRoute(0, 1.00, BURN_SND_ROUTE_BOTH);
	DrvDoReset();
	return 0;
}

static INT32 DrvExit()
{
	igs017_igs031_exit();
	SekExit();
	MSM6295Exit(0);

	BurnFree(AllMem);
	Drv68KROM = DrvTileROM = DrvSpriteROM = DrvSndROM = DrvStringROM = DrvProtROM = DrvNVRAM = DrvProtRAM = DrvFixedData = NULL;
	Drv68KLen = DrvTileLen = DrvSpriteLen = DrvSndLen = DrvStringLen = DrvProtLen = DrvNVRAMLen = DrvFixedLen = 0;

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

	DrvMakeInputs();
	SekNewFrame();

	INT32 nInterleave = 256;
	INT32 nCyclesTotal[1] = { 11000000 / (nBurnFPS / 100) };
	INT32 nCyclesDone[1] = { 0 };

	SekOpen(0);

	for (INT32 i = 0; i < nInterleave; i++) {
		CPU_RUN(0, Sek);

		if (i == nScreenHeight && igs017_igs031_irq_enable()) {
			SekSetIRQLine(1, CPU_IRQSTATUS_AUTO);
		}

		if (i == 0 && igs017_igs031_nmi_enable()) {
			SekSetIRQLine(DrvNmiLine, CPU_IRQSTATUS_AUTO);
		}
	}

	SekClose();

	if (pBurnSoundOut) {
		MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 nvram_scan_address()
{
	switch (DrvProfile)
	{
		case PROFILE_MGDH:   return 0x600000;
		case PROFILE_SDMG2:  return 0x1f0000;
		case PROFILE_SDMG2P: return 0x100000;
		case PROFILE_MGCS:   return 0x300000;
		case PROFILE_MGCSA:  return 0x100000;
		case PROFILE_MGCSB:  return 0x300000;
		case PROFILE_LHZB2:
		case PROFILE_LHZB2A:
		case PROFILE_LHZB2B:
		case PROFILE_LHZB2C: return 0x500000;
		case PROFILE_SLQZ2:  return 0x100000;
	}

	return 0;
}

static INT32 protram_scan_address()
{
	switch (DrvProfile)
	{
		case PROFILE_LHZB2: return 0x100000;
		case PROFILE_SLQZ2: return 0x300000;
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
		ba.nAddress = nvram_scan_address();
		ba.szName = "NVRAM";
		BurnAcb(&ba);

		ba.Data = DrvProtRAM;
		ba.nLen = 0x4000;
		ba.nAddress = protram_scan_address();
		ba.szName = "IGS022 Shared RAM";
		BurnAcb(&ba);
	}

	SekScan(nAction);
	MSM6295Scan(nAction, pnMin);
	igs017_igs031_scan(nAction, pnMin);

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(DrvMuxAddr);
		SCAN_VAR(DrvInputSelect);
		SCAN_VAR(DrvOkiBank);
		SCAN_VAR(DrvHopperMotor);
		SCAN_VAR(DrvCoin1Pulse);
		SCAN_VAR(DrvCoin1Prev);
		SCAN_VAR(DrvBootPtr);
		SCAN_VAR(DrvIncDecVal);
		SCAN_VAR(DrvIncAltVal);
		SCAN_VAR(DrvDswSelect);
		SCAN_VAR(DrvScrambleData);
		SCAN_VAR(DrvIgs029SendData);
		SCAN_VAR(DrvIgs029RecvData);
		SCAN_VAR(DrvIgs029SendBuf);
		SCAN_VAR(DrvIgs029RecvBuf);
		SCAN_VAR(DrvIgs029SendLen);
		SCAN_VAR(DrvIgs029RecvLen);
		SCAN_VAR(DrvIgs029Reg);
		SCAN_VAR(DrvStringOffs);
		SCAN_VAR(DrvStringWord);
		SCAN_VAR(DrvStringValue);
		SCAN_VAR(DrvRemapAddr);
		SCAN_VAR(DrvBitswapM3);
		SCAN_VAR(DrvBitswapMF);
		SCAN_VAR(DrvBitswapVal);
		SCAN_VAR(DrvBitswapWord);
		SCAN_VAR(DrvIgs022Latch);
		SCAN_VAR(DrvIgs022Regs);
		SCAN_VAR(DrvIgs022Stack);
		SCAN_VAR(DrvIgs022StackPtr);
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

static struct BurnDIPInfo MgdhDIPList[] = {
	{0x2b, 0xff, 0xff, 0xff, NULL},
	{0x2c, 0xff, 0xff, 0xfd, NULL},
	{0x2d, 0xff, 0xff, 0xff, NULL},

	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x2b, 0x01, 0x06, 0x06, "5"},
	{0x2b, 0x01, 0x06, 0x04, "10"},
	{0x2b, 0x01, 0x06, 0x02, "50"},
	{0x2b, 0x01, 0x06, 0x00, "100"},

	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x2b, 0x01, 0x08, 0x08, "100"},
	{0x2b, 0x01, 0x08, 0x00, "500"},

	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x2b, 0x01, 0x10, 0x10, "Coin Acceptor"},
	{0x2b, 0x01, 0x10, 0x00, "Key-In"},

	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x2b, 0x01, 0x20, 0x20, "Return Coins"},
	{0x2b, 0x01, 0x20, 0x00, "Key-Out"},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x2b, 0x01, 0xc0, 0xc0, "1"},
	{0x2b, 0x01, 0xc0, 0x80, "2"},
	{0x2b, 0x01, 0xc0, 0x40, "3"},
	{0x2b, 0x01, 0xc0, 0x00, "5"},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x2c, 0x01, 0x01, 0x00, "Off"},
	{0x2c, 0x01, 0x01, 0x01, "On"},

	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x2c, 0x01, 0x02, 0x02, "Mahjong"},
	{0x2c, 0x01, 0x02, 0x00, "Joystick"},

	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x2c, 0x01, 0x04, 0x00, "Off"},
	{0x2c, 0x01, 0x04, 0x04, "On"},

	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x2c, 0x01, 0x18, 0x00, "2 Coins 1 Credit"},
	{0x2c, 0x01, 0x18, 0x18, "1 Coin 1 Credit"},
	{0x2c, 0x01, 0x18, 0x10, "1 Coin 2 Credits"},
	{0x2c, 0x01, 0x18, 0x08, "1 Coin 3 Credits"},

	{0   , 0xfe, 0   ,    8, "Coin Out Rate"},
	{0x2c, 0x01, 0xe0, 0xe0, "1 Coin 1 Credit"},
	{0x2c, 0x01, 0xe0, 0xc0, "2 Coins 1 Credit"},
	{0x2c, 0x01, 0xe0, 0xa0, "5 Coins 1 Credit"},
	{0x2c, 0x01, 0xe0, 0x80, "6 Coins 1 Credit"},
	{0x2c, 0x01, 0xe0, 0x60, "7 Coins 1 Credit"},
	{0x2c, 0x01, 0xe0, 0x40, "8 Coins 1 Credit"},
	{0x2c, 0x01, 0xe0, 0x20, "9 Coins 1 Credit"},
	{0x2c, 0x01, 0xe0, 0x00, "10 Coins 1 Credit"},
};

STDDIPINFO(Mgdh)

static struct BurnDIPInfo Sdmg2DIPList[] = {
	{0x2b, 0xff, 0xff, 0xff, NULL},
	{0x2c, 0xff, 0xff, 0xbf, NULL},
	{0x2d, 0xff, 0xff, 0xff, NULL},

	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x2b, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x2b, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x2b, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x2b, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},

	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x2b, 0x01, 0x0c, 0x0c, "10"},
	{0x2b, 0x01, 0x0c, 0x08, "20"},
	{0x2b, 0x01, 0x0c, 0x04, "50"},
	{0x2b, 0x01, 0x0c, 0x00, "100"},

	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x2b, 0x01, 0x10, 0x10, "2000"},
	{0x2b, 0x01, 0x10, 0x00, "Unlimited"},

	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x2b, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x2b, 0x01, 0x20, 0x00, "Key-In"},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x2b, 0x01, 0x80, 0x00, "Off"},
	{0x2b, 0x01, 0x80, 0x80, "On"},

	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x2c, 0x01, 0x03, 0x03, "500"},
	{0x2c, 0x01, 0x03, 0x02, "1000"},
	{0x2c, 0x01, 0x03, 0x01, "1500"},
	{0x2c, 0x01, 0x03, 0x00, "2000"},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x2c, 0x01, 0x0c, 0x0c, "1"},
	{0x2c, 0x01, 0x0c, 0x08, "2"},
	{0x2c, 0x01, 0x0c, 0x04, "3"},
	{0x2c, 0x01, 0x0c, 0x00, "5"},

	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x2c, 0x01, 0x10, 0x00, "Off"},
	{0x2c, 0x01, 0x10, 0x10, "On"},

	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x2c, 0x01, 0x20, 0x20, "Continue Play"},
	{0x2c, 0x01, 0x20, 0x00, "Double Up"},

	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x2c, 0x01, 0x40, 0x40, "Mahjong"},
	{0x2c, 0x01, 0x40, 0x00, "Joystick"},

	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x2c, 0x01, 0x80, 0x80, "Numbers"},
	{0x2c, 0x01, 0x80, 0x00, "Blocks"},
};

STDDIPINFO(Sdmg2)

static struct BurnDIPInfo Sdmg2pDIPList[] = {
	{0x2b, 0xff, 0xff, 0xff, NULL},
	{0x2c, 0xff, 0xff, 0xbf, NULL},
	{0x2d, 0xff, 0xff, 0xff, NULL},

	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x2b, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x2b, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x2b, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x2b, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},

	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x2b, 0x01, 0x0c, 0x0c, "10"},
	{0x2b, 0x01, 0x0c, 0x08, "20"},
	{0x2b, 0x01, 0x0c, 0x04, "50"},
	{0x2b, 0x01, 0x0c, 0x00, "100"},

	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x2b, 0x01, 0x10, 0x10, "2000"},
	{0x2b, 0x01, 0x10, 0x00, "Unlimited"},

	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x2b, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x2b, 0x01, 0x20, 0x00, "Key-In"},

	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x2b, 0x01, 0x80, 0x80, "Off"},
	{0x2b, 0x01, 0x80, 0x00, "On"},

	{0   , 0xfe, 0   ,    2, "Game Title"},
	{0x2c, 0x01, 0x01, 0x01, "Maque Wangchao"},
	{0x2c, 0x01, 0x01, 0x00, "Chaoji Da Manguan 2 - Jiaqiang Ban"},

	{0   , 0xfe, 0   ,    2, "Double Up Jackpot"},
	{0x2c, 0x01, 0x02, 0x02, "500"},
	{0x2c, 0x01, 0x02, 0x00, "1000"},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x2c, 0x01, 0x0c, 0x0c, "1"},
	{0x2c, 0x01, 0x0c, 0x08, "2"},
	{0x2c, 0x01, 0x0c, 0x04, "3"},
	{0x2c, 0x01, 0x0c, 0x00, "5"},

	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x2c, 0x01, 0x10, 0x00, "Off"},
	{0x2c, 0x01, 0x10, 0x10, "On"},

	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x2c, 0x01, 0x20, 0x20, "Continue Play"},
	{0x2c, 0x01, 0x20, 0x00, "Double Up"},

	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x2c, 0x01, 0x40, 0x40, "Mahjong"},
	{0x2c, 0x01, 0x40, 0x00, "Joystick"},

	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x2c, 0x01, 0x80, 0x80, "Numbers"},
	{0x2c, 0x01, 0x80, 0x00, "Blocks"},
};

STDDIPINFO(Sdmg2p)

static struct BurnDIPInfo MgcsDIPList[] = {
	{0x2b, 0xff, 0xff, 0xff, NULL},
	{0x2c, 0xff, 0xff, 0xef, NULL},
	{0x2d, 0xff, 0xff, 0xff, NULL},

	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x2b, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x2b, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x2b, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x2b, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},

	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x2b, 0x01, 0x0c, 0x0c, "10"},
	{0x2b, 0x01, 0x0c, 0x08, "20"},
	{0x2b, 0x01, 0x0c, 0x04, "50"},
	{0x2b, 0x01, 0x0c, 0x00, "100"},

	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x2b, 0x01, 0x10, 0x10, "500"},
	{0x2b, 0x01, 0x10, 0x00, "1000"},

	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x2b, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x2b, 0x01, 0x20, 0x00, "Key-In"},

	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x2b, 0x01, 0x40, 0x40, "Return Coins"},
	{0x2b, 0x01, 0x40, 0x00, "Key-Out"},

	{0   , 0xfe, 0   ,    2, "Double Up Jackpot"},
	{0x2b, 0x01, 0x80, 0x80, "1000"},
	{0x2b, 0x01, 0x80, 0x00, "2000"},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x2c, 0x01, 0x03, 0x03, "1"},
	{0x2c, 0x01, 0x03, 0x02, "2"},
	{0x2c, 0x01, 0x03, 0x01, "3"},
	{0x2c, 0x01, 0x03, 0x00, "5"},

	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x2c, 0x01, 0x04, 0x00, "Off"},
	{0x2c, 0x01, 0x04, 0x04, "On"},

	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x2c, 0x01, 0x08, 0x08, "Double Up"},
	{0x2c, 0x01, 0x08, 0x00, "Continue Play"},

	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x2c, 0x01, 0x10, 0x10, "Mahjong"},
	{0x2c, 0x01, 0x10, 0x00, "Joystick"},

	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x2c, 0x01, 0x20, 0x20, "Numbers"},
	{0x2c, 0x01, 0x20, 0x00, "Blocks"},

	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x2c, 0x01, 0x40, 0x40, "Off"},
	{0x2c, 0x01, 0x40, 0x00, "On"},
};

STDDIPINFO(Mgcs)

static struct BurnDIPInfo Lhzb2DIPList[] = {
	{0x2b, 0xff, 0xff, 0xff, NULL},
	{0x2c, 0xff, 0xff, 0xff, NULL},
	{0x2d, 0xff, 0xff, 0xff, NULL},

	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x2b, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x2b, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x2b, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x2b, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},

	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x2b, 0x01, 0x0c, 0x0c, "10"},
	{0x2b, 0x01, 0x0c, 0x08, "20"},
	{0x2b, 0x01, 0x0c, 0x04, "50"},
	{0x2b, 0x01, 0x0c, 0x00, "100"},

	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x2b, 0x01, 0x10, 0x10, "1000"},
	{0x2b, 0x01, 0x10, 0x00, "2000"},

	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x2b, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x2b, 0x01, 0x20, 0x00, "Key-In"},

	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x2b, 0x01, 0x40, 0x40, "Return Coins"},
	{0x2b, 0x01, 0x40, 0x00, "Key-Out"},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x2b, 0x01, 0x80, 0x00, "Off"},
	{0x2b, 0x01, 0x80, 0x80, "On"},

	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x2c, 0x01, 0x03, 0x03, "500"},
	{0x2c, 0x01, 0x03, 0x02, "1000"},
	{0x2c, 0x01, 0x03, 0x01, "1500"},
	{0x2c, 0x01, 0x03, 0x00, "2000"},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x2c, 0x01, 0x0c, 0x0c, "1 (1)"},
	{0x2c, 0x01, 0x0c, 0x08, "1 (2)"},
	{0x2c, 0x01, 0x0c, 0x04, "1 (3)"},
	{0x2c, 0x01, 0x0c, 0x00, "1 (4)"},

	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x2c, 0x01, 0x10, 0x00, "Off"},
	{0x2c, 0x01, 0x10, 0x10, "On"},

	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x2c, 0x01, 0x20, 0x20, "Continue Play"},
	{0x2c, 0x01, 0x20, 0x00, "Double Up"},

	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x2c, 0x01, 0x40, 0x40, "Numbers"},
	{0x2c, 0x01, 0x40, 0x00, "Blocks"},

	{0   , 0xfe, 0   ,    2, "Show Credits"},
	{0x2c, 0x01, 0x80, 0x00, "Off"},
	{0x2c, 0x01, 0x80, 0x80, "On"},
};

STDDIPINFO(Lhzb2)

static struct BurnDIPInfo LhzbDIPList[] = {
	{0x2b, 0xff, 0xff, 0xff, NULL},
	{0x2c, 0xff, 0xff, 0xff, NULL},
	{0x2d, 0xff, 0xff, 0xff, NULL},

	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x2b, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x2b, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x2b, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x2b, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},

	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x2b, 0x01, 0x0c, 0x0c, "10"},
	{0x2b, 0x01, 0x0c, 0x08, "20"},
	{0x2b, 0x01, 0x0c, 0x04, "50"},
	{0x2b, 0x01, 0x0c, 0x00, "100"},

	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x2b, 0x01, 0x10, 0x10, "1000"},
	{0x2b, 0x01, 0x10, 0x00, "2000"},

	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x2b, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x2b, 0x01, 0x20, 0x00, "Key-In"},

	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x2b, 0x01, 0x40, 0x40, "Return Coins"},
	{0x2b, 0x01, 0x40, 0x00, "Key-Out"},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x2b, 0x01, 0x80, 0x00, "Off"},
	{0x2b, 0x01, 0x80, 0x80, "On"},

	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x2c, 0x01, 0x03, 0x03, "500"},
	{0x2c, 0x01, 0x03, 0x02, "1000"},
	{0x2c, 0x01, 0x03, 0x01, "1500"},
	{0x2c, 0x01, 0x03, 0x00, "2000"},

	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x2c, 0x01, 0x0c, 0x0c, "1"},
	{0x2c, 0x01, 0x0c, 0x08, "2"},
	{0x2c, 0x01, 0x0c, 0x04, "3"},
	{0x2c, 0x01, 0x0c, 0x00, "5"},

	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x2c, 0x01, 0x10, 0x00, "Off"},
	{0x2c, 0x01, 0x10, 0x10, "On"},

	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x2c, 0x01, 0x20, 0x20, "Continue Play"},
	{0x2c, 0x01, 0x20, 0x00, "Double Up"},

	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x2c, 0x01, 0x40, 0x40, "Numbers"},
	{0x2c, 0x01, 0x40, 0x00, "Blocks"},
};

STDDIPINFO(Lhzb)

static struct BurnRomInfo mgdhaRomDesc[] = {
	{ "flash.u19",	0x080000, 0xff3aed2c, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1001.u4",	0x400000, 0x0cfb60d6, 2 | BRF_GRA },
	{ "text.u6",	0x020000, 0xdb50f8fc, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "s1002.u22",	0x080000, 0xac6b55f2, 4 | BRF_SND },
};
STD_ROM_PICK(mgdha)
STD_ROM_FN(mgdha)

static INT32 MgdhaInit() { return common_init(PROFILE_MGDH, 0x4000, 0, 3, decrypt_mgdha); }

struct BurnDriver BurnDrvMgdha = {
	"mgdha", "mgdh", NULL, NULL, "1997",
	"Manguan Daheng (Taiwan, V123T1)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgdha), INPUT_FN(Igs017), DIP_FN(Mgdh),
	MgdhaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo mgdhRomDesc[] = {
	{ "igs_f4bd.125",	0x080000, 0x8bb0b870, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1001.u4",		0x400000, 0x0cfb60d6, 2 | BRF_GRA },
	{ "igs_512e.u6",	0x020000, 0xdb50f8fc, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "ig2_8836.u14",	0x080000, 0xac1f4da8, 4 | BRF_SND },
};
STD_ROM_PICK(mgdh)
STD_ROM_FN(mgdh)

static INT32 MgdhInit() { return common_init(PROFILE_MGDH, 0x4000, 1, 3, decrypt_mgdha); }

struct BurnDriver BurnDrvMgdh = {
	"mgdh", NULL, NULL, NULL, "1997",
	"Manguan Daheng (Taiwan, V125T1)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgdh), INPUT_FN(Igs017), DIP_FN(Mgdh),
	MgdhInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo sdmg2754caRomDesc[] = {
	{ "p0900.u25",	0x080000, 0x43366f51, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m0901.u5",	0x200000, 0x9699db24, 2 | BRF_GRA },
	{ "m0902.u4",	0x080000, 0x3298b13b, 2 | BRF_GRA },
	{ "text.u6",	0x020000, 0xcb34cbc0, 3 | BRF_GRA },
	{ "s0903.u15",	0x080000, 0xae5a441c, 4 | BRF_SND },
};
STD_ROM_PICK(sdmg2754ca)
STD_ROM_FN(sdmg2754ca)

static INT32 Sdmg2754caInit() { return common_init(PROFILE_SDMG2, 0x10000, 0, 2, decrypt_sdmg2754ca); }

struct BurnDriver BurnDrvSdmg2754ca = {
	"sdmg2754ca", "sdmg2", NULL, NULL, "1997",
	"Chaoji Da Manguan II (China, V754C, set 1)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(sdmg2754ca), INPUT_FN(Igs017), DIP_FN(Sdmg2),
	Sdmg2754caInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo sdmg2754cbRomDesc[] = {
	{ "p0900.u25",	0x080000, 0x1afc95d8, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m0901.u5",	0x200000, 0x9699db24, 2 | BRF_GRA },
	{ "m0902.u4",	0x080000, 0x3298b13b, 2 | BRF_GRA },
	{ "text.u6",	0x020000, 0xcb34cbc0, 3 | BRF_GRA },
	{ "s0903.u15",	0x080000, 0xae5a441c, 4 | BRF_SND },
};
STD_ROM_PICK(sdmg2754cb)
STD_ROM_FN(sdmg2754cb)

static INT32 Sdmg2754cbInit() { return common_init(PROFILE_SDMG2, 0x10000, 0, 2, decrypt_sdmg2754cb); }

struct BurnDriver BurnDrvSdmg2754cb = {
	"sdmg2754cb", "sdmg2", NULL, NULL, "1997",
	"Chaoji Da Manguan II (China, V754C, set 2)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(sdmg2754cb), INPUT_FN(Igs017), DIP_FN(Sdmg2),
	Sdmg2754cbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo sdmg2RomDesc[] = {
	{ "rom.u16",	0x080000, 0x362800e8, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m0205.u6",	0x200000, 0x9699db24, 2 | BRF_GRA },
	{ "rom.u5",		0x080000, 0x3298b13b, 2 | BRF_GRA },
	{ "text.u7",	0x020000, 0xcb34cbc0, 3 | BRF_GRA },
	{ "s0206.u3",	0x080000, 0xae5a441c, 4 | BRF_SND },
};
STD_ROM_PICK(sdmg2)
STD_ROM_FN(sdmg2)

static INT32 Sdmg2Init() { return common_init(PROFILE_SDMG2, 0x10000, 0, 2, decrypt_sdmg2); }

struct BurnDriver BurnDrvSdmg2 = {
	"sdmg2", NULL, NULL, NULL, "1997",
	"Chaoji Da Manguan II (China, V765C)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(sdmg2), INPUT_FN(Igs017), DIP_FN(Sdmg2),
	Sdmg2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo sdmg2pRomDesc[] = {
	{ "ma.dy_v100c.u21",	0x080000, 0xc071270e, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "igs_m0906.u20",		0x400000, 0x01ea0a60, 2 | BRF_GRA },
	{ "ma.dy_text.u18",		0x080000, 0xe46a3a52, 3 | BRF_GRA },
	{ "ma.dy_sp.u14",		0x080000, 0x3c16fb8c, 4 | BRF_SND },
	{ "sdmg2p_string.key",	0x0000ec, 0xc134a304, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(sdmg2p)
STD_ROM_FN(sdmg2p)

static INT32 Sdmg2pInit() { return common_init(PROFILE_SDMG2P, 0x4000, 0, 2, decrypt_sdmg2p); }

struct BurnDriver BurnDrvSdmg2p = {
	"sdmg2p", NULL, NULL, NULL, "2000",
	"Maque Wangchao / Chaoji Da Manguan 2 - Jiaqiang Ban (China, V100C)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(sdmg2p), INPUT_FN(Igs017), DIP_FN(Sdmg2p),
	Sdmg2pInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo mgcsRomDesc[] = {
	{ "p1500.u8",	0x080000, 0xa8cb5905, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1501.u23",	0x400000, 0x96fce058, 2 | BRF_GRA },
	{ "text.u25",	0x080000, 0xa37f9613, 3 | BRF_GRA },
	{ "s1502.u10",	0x080000, 0xa8a6ba58, 4 | BRF_SND },
	{ "mgcs_string.key", 0x0000ec, 0x6cdadd19, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(mgcs)
STD_ROM_FN(mgcs)

static INT32 MgcsInit() { return common_init(PROFILE_MGCS, 0x4000, 0, 2, decrypt_mgcs); }

struct BurnDriver BurnDrvMgcs = {
	"mgcs", NULL, NULL, NULL, "1998",
	"Manguan Caishen (China, V103CS)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgcs), INPUT_FN(Igs017), DIP_FN(Mgcs),
	MgcsInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo mgcsaRomDesc[] = {
	{ "27c4096.u24",	0x080000, 0xc41b7530, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1501.u21",		0x400000, 0x96fce058, 2 | BRF_GRA },
	{ "m1503.u22",		0x080000, 0xa37f9613, 3 | BRF_GRA },
	{ "s1502.u12",		0x080000, 0xa8a6ba58, 4 | BRF_SND },
	{ "mgcs_string.key", 0x0000ec, 0x6cdadd19, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(mgcsa)
STD_ROM_FN(mgcsa)

static INT32 MgcsaInit() { return common_init(PROFILE_MGCSA, 0x4000, 0, 2, decrypt_mgcsa); }

struct BurnDriver BurnDrvMgcsa = {
	"mgcsa", "mgcs", NULL, NULL, "1998",
	"Manguan Caishen (China, V106CS)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgcsa), INPUT_FN(Igs017), DIP_FN(Mgcs),
	MgcsaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo mgcsbRomDesc[] = {
	{ "rom.u23",				0x080000, 0xefc2b198, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1501.u21",				0x400000, 0x96fce058, 2 | BRF_GRA },
	{ "m1503-text v100c.u12",	0x080000, 0xa37f9613, 3 | BRF_GRA },
	{ "s1502.u16",				0x080000, 0xa8a6ba58, 4 | BRF_SND },
	{ "mgcs_string.key",		0x0000ec, 0x6cdadd19, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(mgcsb)
STD_ROM_FN(mgcsb)

static INT32 MgcsbInit()
{
	INT32 ret = common_init(PROFILE_MGCSB, 0x4000, 0, 2, decrypt_mgcsb);
	if (ret) return ret;

	DrvBitswapValXor = 0x289a;
	DrvBitswapMFBits[0] = 4;  DrvBitswapMFBits[1] = 7;  DrvBitswapMFBits[2] = 10; DrvBitswapMFBits[3] = 13;
	DrvBitswapM3Bits[0][0] = (UINT8)~3;   DrvBitswapM3Bits[0][1] = 8;         DrvBitswapM3Bits[0][2] = (UINT8)~12; DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = (UINT8)~3;   DrvBitswapM3Bits[1][1] = (UINT8)~6;  DrvBitswapM3Bits[1][2] = (UINT8)~9;  DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = (UINT8)~3;   DrvBitswapM3Bits[2][1] = 4;         DrvBitswapM3Bits[2][2] = (UINT8)~5;  DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = (UINT8)~9;   DrvBitswapM3Bits[3][1] = (UINT8)~11; DrvBitswapM3Bits[3][2] = 12;        DrvBitswapM3Bits[3][3] = (UINT8)~15;
	return 0;
}

struct BurnDriver BurnDrvMgcsb = {
	"mgcsb", "mgcs", NULL, NULL, "1998",
	"Manguan Caishen (China, V110C)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgcsb), INPUT_FN(Igs017), DIP_FN(Mgcs),
	MgcsbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo lhzb2aRomDesc[] = {
	{ "p-4096",			0x080000, 0x41293f32, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u6",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "m1103.u8",		0x080000, 0x4d3776b4, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "s1102.u23",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "lhzb2_string.key", 0x0000ec, 0xc964dc35, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(lhzb2a)
STD_ROM_FN(lhzb2a)

static INT32 Lhzb2aInit()
{
	INT32 ret = common_init(PROFILE_LHZB2A, 0x4000, 0, 2, decrypt_lhzb2a);
	if (ret) return ret;
	DrvBitswapValXor = 0x289a;
	DrvBitswapMFBits[0] = 4;  DrvBitswapMFBits[1] = 7;  DrvBitswapMFBits[2] = 10; DrvBitswapMFBits[3] = 13;
	DrvBitswapM3Bits[0][0] = (UINT8)~3;   DrvBitswapM3Bits[0][1] = 8;         DrvBitswapM3Bits[0][2] = (UINT8)~12; DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = (UINT8)~3;   DrvBitswapM3Bits[1][1] = (UINT8)~6;  DrvBitswapM3Bits[1][2] = (UINT8)~9;  DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = (UINT8)~3;   DrvBitswapM3Bits[2][1] = 4;         DrvBitswapM3Bits[2][2] = (UINT8)~5;  DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = (UINT8)~9;   DrvBitswapM3Bits[3][1] = (UINT8)~11; DrvBitswapM3Bits[3][2] = 12;        DrvBitswapM3Bits[3][3] = (UINT8)~15;
	return 0;
}

struct BurnDriver BurnDrvLhzb2a = {
	"lhzb2a", "lhzb2", NULL, NULL, "1998",
	"Long Hu Zhengba 2 (China, VS221M)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb2a), INPUT_FN(Igs017), DIP_FN(Lhzb2),
	Lhzb2aInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo lhzb2bRomDesc[] = {
	{ "p-4096",			0x080000, 0x46bbef6e, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u6",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "m1103.u8",		0x080000, 0x4d3776b4, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "s1102.u23",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "lhzb2_string.key", 0x0000ec, 0xc964dc35, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(lhzb2b)
STD_ROM_FN(lhzb2b)

static INT32 Lhzb2bInit() { return common_init(PROFILE_LHZB2B, 0x4000, 0, 2, decrypt_lhzb2b); }

struct BurnDriver BurnDrvLhzb2b = {
	"lhzb2b", "lhzb2", NULL, NULL, "1998",
	"Long Hu Zhengba 2 (China, VS210M)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb2b), INPUT_FN(Igs017), DIP_FN(Lhzb2),
	Lhzb2bInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo lhzb2cRomDesc[] = {
	{ "janan409",		0x080000, 0xa52e954c, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u6",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "m1103.u8",		0x080000, 0x4d3776b4, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "s1102.u23",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "lhzb2_string.key", 0x0000ec, 0xc964dc35, 5 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(lhzb2c)
STD_ROM_FN(lhzb2c)

static INT32 Lhzb2cInit()
{
	INT32 ret = common_init(PROFILE_LHZB2C, 0x4000, 0, 2, decrypt_lhzb2c);
	if (ret) return ret;
	DrvBitswapValXor = 0x289a;
	DrvBitswapMFBits[0] = 4;  DrvBitswapMFBits[1] = 7;  DrvBitswapMFBits[2] = 10; DrvBitswapMFBits[3] = 13;
	DrvBitswapM3Bits[0][0] = (UINT8)~3;   DrvBitswapM3Bits[0][1] = 8;         DrvBitswapM3Bits[0][2] = (UINT8)~12; DrvBitswapM3Bits[0][3] = (UINT8)~15;
	DrvBitswapM3Bits[1][0] = (UINT8)~3;   DrvBitswapM3Bits[1][1] = (UINT8)~6;  DrvBitswapM3Bits[1][2] = (UINT8)~9;  DrvBitswapM3Bits[1][3] = (UINT8)~15;
	DrvBitswapM3Bits[2][0] = (UINT8)~3;   DrvBitswapM3Bits[2][1] = 4;         DrvBitswapM3Bits[2][2] = (UINT8)~5;  DrvBitswapM3Bits[2][3] = (UINT8)~15;
	DrvBitswapM3Bits[3][0] = (UINT8)~9;   DrvBitswapM3Bits[3][1] = (UINT8)~11; DrvBitswapM3Bits[3][2] = 12;        DrvBitswapM3Bits[3][3] = (UINT8)~15;
	return 0;
}

struct BurnDriver BurnDrvLhzb2c = {
	"lhzb2c", "lhzb2", NULL, NULL, "1998",
	"Long Hu Zhengba 2 (China, VS220M)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb2c), INPUT_FN(Igs017), DIP_FN(Lhzb2),
	Lhzb2cInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo lhzb2RomDesc[] = {
	{ "p1100.u30",		0x080000, 0x68102b25, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u6",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "m1103.u8",		0x080000, 0x4d3776b4, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "s1102.u23",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "lhzb2_string.key", 0x0000ec, 0xc964dc35, 5 | BRF_PRG | BRF_ESS },
	{ "m1104.u11",		0x010000, 0x794d0276, 6 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(lhzb2)
STD_ROM_FN(lhzb2)

static INT32 Lhzb2Init() { return common_init(PROFILE_LHZB2, 0x4000, 0, 2, decrypt_lhzb2); }

struct BurnDriver BurnDrvLhzb2 = {
	"lhzb2", NULL, NULL, NULL, "1998",
	"Long Hu Zhengba 2 (China, set 1)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb2), INPUT_FN(Igs017), DIP_FN(Lhzb2),
	Lhzb2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo lhzbRomDesc[] = {
	{ "rom.u25",		0x080000, 0x46f5df48, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u13",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "rom.u15",		0x080000, 0x5d28287b, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "rom.u22",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "lhzb2_string.key", 0x0000ec, 0xc964dc35, 5 | BRF_PRG | BRF_ESS },
	{ "rom.u12",		0x010000, 0x794d0276, 6 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(lhzb)
STD_ROM_FN(lhzb)

static INT32 LhzbInit() { return common_init(PROFILE_LHZB2, 0x4000, 0, 2, decrypt_lhzb2); }

struct BurnDriver BurnDrvLhzb = {
	"lhzb", NULL, NULL, NULL, "1998",
	"Long Hu Zhengba (China, VS105M, set 1)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb), INPUT_FN(Igs017), DIP_FN(Lhzb),
	LhzbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo lhzbaRomDesc[] = {
	{ "rom.u25",		0x080000, 0x2fd43fea, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u13",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "rom.u15",		0x080000, 0x5d28287b, 3 | BRF_GRA | IGS017_WORD_SWAP },
	{ "rom.u22",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "lhzb2_string.key", 0x0000ec, 0xc964dc35, 5 | BRF_PRG | BRF_ESS },
	{ "rom.u12",		0x010000, 0x794d0276, 6 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(lhzba)
STD_ROM_FN(lhzba)

static INT32 LhzbaInit() { return common_init(PROFILE_LHZB2, 0x4000, 0, 2, decrypt_lhzb2); }

struct BurnDriver BurnDrvLhzba = {
	"lhzba", "lhzb", NULL, NULL, "1998",
	"Long Hu Zhengba (China, VS105M, set 2)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzba), INPUT_FN(Igs017), DIP_FN(Lhzb),
	LhzbaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo slqz2RomDesc[] = {
	{ "p1100.u28",		0x080000, 0x0b8e5c9e, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u4",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "text.u6",		0x080000, 0x40d21adf, 3 | BRF_GRA },
	{ "s1102.u20",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "slqz2_string.key", 0x0000ec, 0x5ca22f9d, 5 | BRF_PRG | BRF_ESS },
	{ "m1103.u12",		0x010000, 0x9f3b8d65, 6 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(slqz2)
STD_ROM_FN(slqz2)

static INT32 Slqz2Init() { return common_init(PROFILE_SLQZ2, 0x4000, 0, 2, decrypt_slqz2); }

struct BurnDriver BurnDrvSlqz2 = {
	"slqz2", NULL, NULL, NULL, "1998",
	"Shuang Long Qiang Zhu 2 VS (China, VS203J, set 1)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(slqz2), INPUT_FN(Igs017), DIP_FN(Lhzb2),
	Slqz2Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

static struct BurnRomInfo slqz2bRomDesc[] = {
	{ "slqz2.u28",		0x080000, 0x63be02a3, 1 | BRF_PRG | BRF_ESS | IGS017_WORD_SWAP },
	{ "m1101.u4",		0x400000, 0x0114e9d1, 2 | BRF_GRA | IGS017_WORD_SWAP },
	{ "text.u6",		0x080000, 0x40d21adf, 3 | BRF_GRA },
	{ "s1102.u20",		0x080000, 0x51ffe245, 4 | BRF_SND },
	{ "slqz2_string.key", 0x0000ec, 0x5ca22f9d, 5 | BRF_PRG | BRF_ESS },
	{ "m1103.u12",		0x010000, 0x9f3b8d65, 6 | BRF_PRG | BRF_ESS },
};
STD_ROM_PICK(slqz2b)
STD_ROM_FN(slqz2b)

static INT32 Slqz2bInit() { return common_init(PROFILE_SLQZ2, 0x4000, 0, 2, decrypt_slqz2b); }

struct BurnDriver BurnDrvSlqz2b = {
	"slqz2b", "slqz2", NULL, NULL, "1998",
	"Shuang Long Qiang Zhu 2 VS (China, VS203J, set 2)\0", NULL, "IGS", "IGS017",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(slqz2b), INPUT_FN(Igs017), DIP_FN(Lhzb2),
	Slqz2bInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

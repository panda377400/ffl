#ifndef IGS_M027_H
#define IGS_M027_H

#include "burnint.h"
#include "ics2115.h"

#define M027_WORD_SWAP (1 << 16)

enum {
	M027_IN_COIN1 = 0,
	M027_IN_COIN2,
	M027_IN_START1,
	M027_IN_UP,
	M027_IN_DOWN,
	M027_IN_LEFT,
	M027_IN_RIGHT,
	M027_IN_BUTTON1,
	M027_IN_BUTTON2,
	M027_IN_BUTTON3,
	M027_IN_SERVICE,
	M027_IN_BOOK,
	M027_IN_SHOW_CREDITS,
	M027_IN_KEYIN,
	M027_IN_PAYOUT,
	M027_IN_KEYOUT,
	M027_IN_ATTENDANT,
	M027_IN_STOPALL,
	M027_IN_STOP1,
	M027_IN_STOP2,
	M027_IN_STOP3,
	M027_IN_STOP4,
	M027_IN_STOP5,
	M027_IN_TICKET,
	M027_IN_MJ_A,
	M027_IN_MJ_B,
	M027_IN_MJ_C,
	M027_IN_MJ_D,
	M027_IN_MJ_E,
	M027_IN_MJ_F,
	M027_IN_MJ_G,
	M027_IN_MJ_H,
	M027_IN_MJ_I,
	M027_IN_MJ_J,
	M027_IN_MJ_K,
	M027_IN_MJ_L,
	M027_IN_MJ_M,
	M027_IN_MJ_N,
	M027_IN_MJ_KAN,
	M027_IN_MJ_REACH,
	M027_IN_MJ_BET,
	M027_IN_MJ_CHI,
	M027_IN_MJ_RON,
	M027_IN_MJ_PON,
	M027_IN_MJ_LAST,
	M027_IN_MJ_SCORE,
	M027_IN_MJ_DOUBLE,
	M027_IN_MJ_BIG,
	M027_IN_MJ_SMALL,
	M027_IN_CLEAR_NVRAM,
	M027_INPUT_MAX
};

INT32 slqz3Init();
INT32 lhdmgInit();
INT32 lhzb3sjbInit();
INT32 lhzb3106Init();
INT32 qlgsInit();
INT32 lhzb4Init();
INT32 cjddzInit();
INT32 cjddzpInit();
INT32 cjddzlfInit();
INT32 cjtljpInit();
INT32 lthypInit();
INT32 zhongguoInit();
INT32 tct2pInit();
INT32 mgzzInit();
INT32 mgcs3Init();
INT32 jking02Init();
INT32 klxyjInit();
INT32 xypdkInit();
INT32 oceanparInit();
INT32 fruitparInit();
INT32 tripslotInit();
INT32 cclyInit();
INT32 tswxpInit();
INT32 royal5pInit();
INT32 mgfxInit();
INT32 tshsInit();

static inline UINT8 igs_m027_ics2115_read(INT32 port)
{
	if (port < 0) return 0xff;
	UINT16 xa_val;
	if ((port == 2 || port == 3) && ics2115_xa_reg_override(&xa_val)) {
		return (port == 2) ? (xa_val & 0xff) : (xa_val >> 8);
	} else {
		return ics2115read(port);
	}

	return 0xff;
}

static inline void igs_m027_ics2115_write(UINT32 port, UINT8 data)
{
	if (port < 0) return;

	if (port == 1) {
		ics2115write(port, data);
	} else {
		UINT8 reg = ics2115read(1);
		if (port == 2) {
			switch (reg) {
				case 0x00: case 0x03: case 0x05: case 0x06:
				case 0x0c: case 0x0d: case 0x0e:
				case 0x10: case 0x11: case 0x12:
					return;
			}
		}

		if (port == 3) {
			switch (reg) {
				case 0x07: case 0x08:
				case 0x40: case 0x41: case 0x42: case 0x43:
				case 0x4a: case 0x4f:
					return;
			}
		}
		
		if ((reg == 0x07 || reg == 0x08) && port == 2) {
			port = 3;
		}

		ics2115write(port, data);
	}
}
#endif

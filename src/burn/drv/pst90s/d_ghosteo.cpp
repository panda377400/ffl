// FinalBurn Neo Ghost hardware driver module
// Based on MAME driver: src/mame/eolith/ghosteo.cpp

#include "tiles_generic.h"
#include "arm9_intf.h"
#include "qs1000.h"
#include "mcs51.h"

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;

static UINT8 *DrvFlashROM;
static UINT8 *DrvQSROM;
static UINT8 *DrvSndROM;
static UINT32 *DrvPalette;

static UINT8 *DrvStepRAM;
static UINT8 *DrvSysRAM;
static UINT8 *DrvEeprom;

static UINT32 *DrvMemConRegs;
static UINT32 *DrvIrqRegs;
static UINT32 *DrvClkPowRegs;
static UINT32 *DrvLcdRegs;
static UINT32 *DrvNandRegs;
static UINT32 *DrvPwmRegs;
static UINT32 *DrvIicRegs;
static UINT32 *DrvGpioRegs;
static UINT32 *DrvRtcRegs;
static UINT32 *DrvUartRegs;
static UINT32 *DrvDummyRegs;
static UINT32 *DrvBballoonPort;

static UINT8 DrvJoy1[8];
static UINT8 DrvJoy2[8];
static UINT8 DrvJoy3[8];
static UINT8 DrvReset;
static UINT8 DrvRecalc;

static UINT32 DrvInputs[3];
static UINT8 DrvSoundLatch;
static UINT8 DrvSoundPending;
static UINT8 DrvSecurityCount;
static UINT8 DrvQs1000Bank;
static UINT8 DrvIicStarted;
static UINT8 DrvIicRw;
static UINT8 DrvIicExpectAddr;
static UINT8 DrvIicCount;
static UINT16 DrvIicAddr;
static UINT32 DrvFrameCounter;
static UINT8 DrvPalette565Mode;

static INT32 nGhostCyclesExtra;

enum
{
	PORT_A = 0,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_E,
	PORT_F,
	PORT_G,
	PORT_H,
};

enum
{
	S3C24XX_BASE_MEMCON = 0x48000000,
	S3C24XX_BASE_IRQ    = 0x4a000000,
	S3C24XX_BASE_DMA0   = 0x4b000000,
	S3C24XX_BASE_DMA1   = 0x4b000040,
	S3C24XX_BASE_DMA2   = 0x4b000080,
	S3C24XX_BASE_DMA3   = 0x4b0000c0,
	S3C24XX_BASE_CLKPOW = 0x4c000000,
	S3C24XX_BASE_LCD    = 0x4d000000,
	S3C24XX_BASE_NAND   = 0x4e000000,
	S3C24XX_BASE_UART1  = 0x50004000,
	S3C24XX_BASE_UART2  = 0x50008000,
	S3C24XX_BASE_PWM    = 0x51000000,
	S3C24XX_BASE_IIC    = 0x54000000,
	S3C24XX_BASE_GPIO   = 0x56000000,
	S3C24XX_BASE_RTC    = 0x57000000,
	S3C24XX_BASE_UART0  = 0x50000000,
};

enum
{
	S3C24XX_LCDCON1   = 0x00 / 4,
	S3C24XX_LCDCON2   = 0x04 / 4,
	S3C24XX_LCDCON3   = 0x08 / 4,
	S3C24XX_LCDCON4   = 0x0c / 4,
	S3C24XX_LCDCON5   = 0x10 / 4,
	S3C24XX_LCDSADDR1 = 0x14 / 4,
	S3C24XX_LCDSADDR2 = 0x18 / 4,
	S3C24XX_LCDSADDR3 = 0x1c / 4,
	S3C24XX_TPAL      = 0x50 / 4,
	S3C24XX_LCDINTPND = 0x54 / 4,
	S3C24XX_LCDSRCPND = 0x58 / 4,
	S3C24XX_LCDINTMSK = 0x5c / 4,
	S3C24XX_LPCSEL    = 0x60 / 4,
};

enum
{
	S3C24XX_SRCPND    = 0x00 / 4,
	S3C24XX_INTMOD    = 0x04 / 4,
	S3C24XX_INTMSK    = 0x08 / 4,
	S3C24XX_PRIORITY  = 0x0c / 4,
	S3C24XX_INTPND    = 0x10 / 4,
	S3C24XX_INTOFFSET = 0x14 / 4,
	S3C24XX_SUBSRCPND = 0x18 / 4,
	S3C24XX_INTSUBMSK = 0x1c / 4,
};

enum
{
	S3C24XX_GPACON   = 0x00 / 4,
	S3C24XX_GPADAT   = 0x04 / 4,
	S3C24XX_GPBCON   = 0x10 / 4,
	S3C24XX_GPBDAT   = 0x14 / 4,
	S3C24XX_GPCCON   = 0x20 / 4,
	S3C24XX_GPCDAT   = 0x24 / 4,
	S3C24XX_GPDCON   = 0x30 / 4,
	S3C24XX_GPDDAT   = 0x34 / 4,
	S3C24XX_GPECON   = 0x40 / 4,
	S3C24XX_GPEDAT   = 0x44 / 4,
	S3C24XX_GPFCON   = 0x50 / 4,
	S3C24XX_GPFDAT   = 0x54 / 4,
	S3C24XX_GPGCON   = 0x60 / 4,
	S3C24XX_GPGDAT   = 0x64 / 4,
	S3C24XX_GPHCON   = 0x70 / 4,
	S3C24XX_GPHDAT   = 0x74 / 4,
	S3C24XX_MISCCR   = 0x80 / 4,
	S3C24XX_EINTMASK = 0xa4 / 4,
	S3C24XX_EINTPEND = 0xa8 / 4,
	S3C24XX_GSTATUS1 = 0xb0 / 4,
	S3C24XX_GSTATUS2 = 0xb4 / 4,
};

enum
{
	S3C24XX_NFCONF = 0x00 / 4,
	S3C24XX_NFCMD  = 0x04 / 4,
	S3C24XX_NFADDR = 0x08 / 4,
	S3C24XX_NFDATA = 0x0c / 4,
	S3C24XX_NFSTAT = 0x10 / 4,
	S3C24XX_NFECC  = 0x14 / 4,
};

enum
{
	S3C24XX_ULCON   = 0x00 / 4,
	S3C24XX_UCON    = 0x04 / 4,
	S3C24XX_UFCON   = 0x08 / 4,
	S3C24XX_UMCON   = 0x0c / 4,
	S3C24XX_UTRSTAT = 0x10 / 4,
	S3C24XX_UERSTAT = 0x14 / 4,
	S3C24XX_UFSTAT  = 0x18 / 4,
	S3C24XX_UMSTAT  = 0x1c / 4,
	S3C24XX_UTXH    = 0x20 / 4,
	S3C24XX_URXH    = 0x24 / 4,
	S3C24XX_UBRDIV  = 0x28 / 4,
};

enum
{
	S3C24XX_IICCON  = 0x00 / 4,
	S3C24XX_IICSTAT = 0x04 / 4,
	S3C24XX_IICADD  = 0x08 / 4,
	S3C24XX_IICDS   = 0x0c / 4,
};

enum
{
	NAND_M_INIT = 0,
	NAND_M_READ,
};

struct ghost_nand_state
{
	UINT8 mode;
	UINT8 addr_load_ptr;
	UINT16 byte_addr;
	UINT32 page_addr;
};

static ghost_nand_state DrvNandState;

struct lcd_state
{
	UINT32 vramaddr_cur;
	UINT32 vramaddr_max;
	UINT32 offsize;
	UINT32 pagewidth_cur;
	UINT32 pagewidth_max;
	UINT32 bppmode;
	UINT32 tpal;
	UINT32 vpos_min;
	UINT32 vpos_max;
	UINT32 total_height;
	UINT8 bswp;
	UINT8 hwswp;
	INT32 width;
	INT32 height;
};

static lcd_state DrvLcdState;

static struct BurnInputInfo BballoonInputList[] = {
	{"P1 Coin",        BIT_DIGITAL, DrvJoy3 + 0, "p1 coin"   },
	{"P1 Start",       BIT_DIGITAL, DrvJoy3 + 2, "p1 start"  },
	{"P1 Up",          BIT_DIGITAL, DrvJoy1 + 0, "p1 up"     },
	{"P1 Down",        BIT_DIGITAL, DrvJoy1 + 1, "p1 down"   },
	{"P1 Left",        BIT_DIGITAL, DrvJoy1 + 2, "p1 left"   },
	{"P1 Right",       BIT_DIGITAL, DrvJoy1 + 3, "p1 right"  },
	{"P1 Button 1",    BIT_DIGITAL, DrvJoy1 + 4, "p1 fire 1" },
	{"P1 Button 2",    BIT_DIGITAL, DrvJoy1 + 5, "p1 fire 2" },

	{"P2 Coin",        BIT_DIGITAL, DrvJoy3 + 1, "p2 coin"   },
	{"P2 Start",       BIT_DIGITAL, DrvJoy3 + 3, "p2 start"  },
	{"P2 Up",          BIT_DIGITAL, DrvJoy2 + 0, "p2 up"     },
	{"P2 Down",        BIT_DIGITAL, DrvJoy2 + 1, "p2 down"   },
	{"P2 Left",        BIT_DIGITAL, DrvJoy2 + 2, "p2 left"   },
	{"P2 Right",       BIT_DIGITAL, DrvJoy2 + 3, "p2 right"  },
	{"P2 Button 1",    BIT_DIGITAL, DrvJoy2 + 4, "p2 fire 1" },
	{"P2 Button 2",    BIT_DIGITAL, DrvJoy2 + 5, "p2 fire 2" },

	{"Test",           BIT_DIGITAL, DrvJoy3 + 5, "diag"      },
	{"Service",        BIT_DIGITAL, DrvJoy3 + 7, "service"   },
	{"Reset",          BIT_DIGITAL, &DrvReset,   "reset"     },
};

STDINPUTINFO(Bballoon)

static INT32 MemIndex()
{
	UINT8 *Next = AllMem;

	DrvFlashROM     = Next; Next += 0x2000000;
	DrvQSROM        = Next; Next += 0x080000;
	DrvSndROM       = Next; Next += 0x1000000;
	DrvPalette      = (UINT32*)Next; Next += 0x10000 * sizeof(UINT32);

	AllRam          = Next;

	DrvStepRAM      = Next; Next += 0x001000;
	DrvSysRAM       = Next; Next += 0x2000000;
	DrvEeprom       = Next; Next += 0x000800;

	DrvMemConRegs   = (UINT32*)Next; Next += 0x40 * sizeof(UINT32);
	DrvIrqRegs      = (UINT32*)Next; Next += 0x20 * sizeof(UINT32);
	DrvClkPowRegs   = (UINT32*)Next; Next += 0x20 * sizeof(UINT32);
	DrvLcdRegs      = (UINT32*)Next; Next += 0x40 * sizeof(UINT32);
	DrvNandRegs     = (UINT32*)Next; Next += 0x20 * sizeof(UINT32);
	DrvPwmRegs      = (UINT32*)Next; Next += 0x20 * sizeof(UINT32);
	DrvIicRegs      = (UINT32*)Next; Next += 0x10 * sizeof(UINT32);
	DrvGpioRegs     = (UINT32*)Next; Next += 0x40 * sizeof(UINT32);
	DrvRtcRegs      = (UINT32*)Next; Next += 0x20 * sizeof(UINT32);
	DrvUartRegs     = (UINT32*)Next; Next += 0x30 * sizeof(UINT32);
	DrvDummyRegs    = (UINT32*)Next; Next += 0x100 * sizeof(UINT32);
	DrvBballoonPort = (UINT32*)Next; Next += 0x20 * sizeof(UINT32);

	RamEnd          = Next;
	MemEnd          = Next;

	return 0;
}

static INT32 DrvLoadRoms()
{
	if (BurnLoadRom(DrvFlashROM, 0, 1)) return 1;
	if (BurnLoadRom(DrvQSROM,    1, 1)) return 1;
	if (BurnLoadRom(DrvSndROM + 0x000000, 2, 1)) return 1;
	if (BurnLoadRom(DrvSndROM + 0x200000, 3, 1)) return 1;

	return 0;
}

static inline UINT8 *GhostMapAddress(UINT32 address)
{
	if (address < 0x00001000) {
		return DrvStepRAM + (address & 0x0fff);
	}

	if (address >= 0x30000000 && address < 0x34000000) {
		return DrvSysRAM + ((address - 0x30000000) & 0x01ffffff);
	}

	if (address >= 0x40000000 && address < 0x40001000) {
		return DrvStepRAM + (address & 0x0fff);
	}

	return NULL;
}

static void GhostCalcMecc(const UINT8 *data, UINT32 size, UINT8 *mecc)
{
	mecc[0] = mecc[1] = mecc[2] = mecc[3] = 0xff;

	for (UINT32 pos = 0; pos < size; pos++)
	{
		INT32 bit[8];
		UINT8 temp;
		UINT8 value = data[pos];

		for (INT32 i = 0; i < 8; i++) {
			bit[i] = (value >> i) & 1;
		}

		mecc[2] ^= ((bit[6] ^ bit[4] ^ bit[2] ^ bit[0]) << 2);
		mecc[2] ^= ((bit[7] ^ bit[5] ^ bit[3] ^ bit[1]) << 3);
		mecc[2] ^= ((bit[5] ^ bit[4] ^ bit[1] ^ bit[0]) << 4);
		mecc[2] ^= ((bit[7] ^ bit[6] ^ bit[3] ^ bit[2]) << 5);
		mecc[2] ^= ((bit[3] ^ bit[2] ^ bit[1] ^ bit[0]) << 6);
		mecc[2] ^= ((bit[7] ^ bit[6] ^ bit[5] ^ bit[4]) << 7);

		temp = bit[7] ^ bit[6] ^ bit[5] ^ bit[4] ^ bit[3] ^ bit[2] ^ bit[1] ^ bit[0];
		if (pos & 0x001) mecc[0] ^= (temp << 1); else mecc[0] ^= (temp << 0);
		if (pos & 0x002) mecc[0] ^= (temp << 3); else mecc[0] ^= (temp << 2);
		if (pos & 0x004) mecc[0] ^= (temp << 5); else mecc[0] ^= (temp << 4);
		if (pos & 0x008) mecc[0] ^= (temp << 7); else mecc[0] ^= (temp << 6);
		if (pos & 0x010) mecc[1] ^= (temp << 1); else mecc[1] ^= (temp << 0);
		if (pos & 0x020) mecc[1] ^= (temp << 3); else mecc[1] ^= (temp << 2);
		if (pos & 0x040) mecc[1] ^= (temp << 5); else mecc[1] ^= (temp << 4);
		if (pos & 0x080) mecc[1] ^= (temp << 7); else mecc[1] ^= (temp << 6);
		if (pos & 0x100) mecc[2] ^= (temp << 1); else mecc[2] ^= (temp << 0);
		if (pos & 0x200) mecc[3] ^= (temp << 5); else mecc[3] ^= (temp << 4);
		if (pos & 0x400) mecc[3] ^= (temp << 7); else mecc[3] ^= (temp << 6);
	}
}

static void GhostNandCommand(UINT8 data)
{
	switch (data)
	{
		case 0xff:
			DrvNandState.mode = NAND_M_INIT;
			DrvNandState.addr_load_ptr = 0;
			DrvNandRegs[S3C24XX_NFSTAT] |= 1;
		return;

		case 0x00:
			DrvNandState.mode = NAND_M_READ;
			DrvNandState.page_addr = 0;
			DrvNandState.byte_addr = 0;
			DrvNandState.addr_load_ptr = 0;
		return;
	}
}

static void GhostNandAddress(UINT8 data)
{
	if (DrvNandState.mode != NAND_M_READ) return;

	if (DrvNandState.addr_load_ptr == 0) {
		DrvNandState.byte_addr = data;
	} else {
		UINT32 shift = (DrvNandState.addr_load_ptr - 1) * 8;
		DrvNandState.page_addr &= ~(0xff << shift);
		DrvNandState.page_addr |= data << shift;
	}

	DrvNandState.addr_load_ptr++;

	if (DrvNandState.addr_load_ptr >= 4) {
		DrvNandRegs[S3C24XX_NFSTAT] |= 1;
	}
}

static UINT8 GhostNandRead()
{
	if (DrvNandState.mode != NAND_M_READ) {
		return 0xff;
	}

	UINT8 data = 0xff;

	if (DrvNandState.byte_addr < 0x200) {
		UINT32 flashAddr = (DrvNandState.page_addr * 0x200) + DrvNandState.byte_addr;
		if (flashAddr < 0x2000000) {
			data = DrvFlashROM[flashAddr];
		}
	} else if (DrvNandState.byte_addr >= 0x200 && DrvNandState.byte_addr < 0x204) {
		UINT8 mecc[4];
		UINT32 flashAddr = DrvNandState.page_addr * 0x200;
		if (flashAddr + 0x200 <= 0x2000000) {
			GhostCalcMecc(DrvFlashROM + flashAddr, 0x200, mecc);
			data = mecc[DrvNandState.byte_addr - 0x200];
		}
	}

	DrvNandState.byte_addr++;
	if (DrvNandState.byte_addr == 0x210) {
		DrvNandState.byte_addr = 0;
		DrvNandState.page_addr++;
	}

	return data;
}

static void GhostNandAutoBoot()
{
	for (INT32 page = 0; page < 8; page++) {
		memcpy(DrvStepRAM + (page * 0x200), DrvFlashROM + (page * 0x200), 0x200);
	}
}

static UINT32 GhostGpioPortRead(INT32 port)
{
	static const UINT8 security_data[] = { 0x01, 0xc4, 0xff, 0x22, 0xff, 0xff, 0xff, 0xff };

	UINT32 data = DrvBballoonPort[port];

	switch (port)
	{
		case PORT_F:
			data = (data & ~0xff) | security_data[DrvSecurityCount & 7];
		break;

		case PORT_G:
			data ^= 0x20;
			DrvBballoonPort[port] = data;
		break;
	}

	return data;
}

static void GhostGpioPortWrite(INT32 port, UINT32 data)
{
	UINT32 oldValue = DrvBballoonPort[port];
	DrvBballoonPort[port] = data;

	switch (port)
	{
		case PORT_F:
			if (data == 0x04) DrvSecurityCount = 0;
			if (data == 0x44) DrvSecurityCount = 2;
		break;

		case PORT_G:
			if ((data & 0x10) && ((oldValue & 0x10) == 0)) {
				DrvSecurityCount = (DrvSecurityCount + 1) & 7;
			}
		break;
	}
}

static void GhostSetSoundBank(UINT8 bank)
{
	DrvQs1000Bank = bank & 7;
	qs1000_set_bankedrom(DrvQSROM + (DrvQs1000Bank * 0x10000));
}

static void GhostSetSoundBankDirect(UINT8 bank)
{
	DrvQs1000Bank = bank & 7;
	qs1000_set_bankedrom_direct(DrvQSROM + (DrvQs1000Bank * 0x10000));
}

static inline INT32 GhostSoundCpuAcquire()
{
	INT32 active = mcs51GetActive();

	if (active == 0) {
		return 0;
	}

	if (active >= 0) {
		mcs51Close();
	}

	mcs51Open(0);

	return active;
}

static inline void GhostSoundCpuRelease(INT32 active)
{
	if (active == 0) {
		return;
	}

	mcs51Close();

	if (active > 0) {
		mcs51Open(active);
	}
}

static inline void GhostSoundSync()
{
	INT32 active = GhostSoundCpuAcquire();
	INT32 todo = (INT64)Arm9TotalCycles() * (24000000 / 12) / 200000000 - mcs51TotalCycles();

	if (todo > 0) {
		mcs51Run(todo);
	}

	GhostSoundCpuRelease(active);
}

static inline void GhostSoundSetIrq(INT32 state)
{
	INT32 active = GhostSoundCpuAcquire();
	qs1000_set_irq(state);
	GhostSoundCpuRelease(active);
}

static UINT8 GhostQs1000P1Read()
{
	return DrvSoundLatch;
}

static void GhostQs1000P3Write(UINT8 data)
{
	GhostSetSoundBank(data & 0x07);

	if ((data & 0x20) == 0) {
		DrvSoundPending = 0;
		GhostSoundSetIrq(0);
	}
}

static UINT32 GhostReadInput(INT32 which)
{
	UINT32 port = 0xffffffff;

	switch (which)
	{
		case 0:
			if (DrvJoy1[0]) port &= ~0x00000001;
			if (DrvJoy1[1]) port &= ~0x00000002;
			if (DrvJoy1[2]) port &= ~0x00000004;
			if (DrvJoy1[3]) port &= ~0x00000008;
			if (DrvJoy1[4]) port &= ~0x00000010;
			if (DrvJoy1[5]) port &= ~0x00000020;
		return port;

		case 1:
			if (DrvJoy2[0]) port &= ~0x00000001;
			if (DrvJoy2[1]) port &= ~0x00000002;
			if (DrvJoy2[2]) port &= ~0x00000004;
			if (DrvJoy2[3]) port &= ~0x00000008;
			if (DrvJoy2[4]) port &= ~0x00000010;
			if (DrvJoy2[5]) port &= ~0x00000020;
		return port;

		case 2:
			if (DrvJoy3[0]) port &= ~0x00000001;
			if (DrvJoy3[1]) port &= ~0x00000002;
			if (DrvJoy3[2]) port &= ~0x00000004;
			if (DrvJoy3[3]) port &= ~0x00000008;
			if (DrvJoy3[5]) port &= ~0x00000020;
			if (DrvJoy3[7]) port &= ~0x00000080;
		return port;
	}

	return 0xffffffff;
}

static void GhostRecalcPalette()
{
	UINT8 mode565 = (DrvLcdRegs[S3C24XX_LCDCON5] >> 11) & 1;
	if ((DrvRecalc == 0) && (mode565 == DrvPalette565Mode)) return;

	DrvPalette565Mode = mode565;
	DrvRecalc = 0;

	for (INT32 i = 0; i < 0x10000; i++)
	{
		INT32 r, g, b;

		if (mode565 == 0)
		{
			INT32 intensity = ((i >> 1) & 1) << 2;
			r = ((i >> 11) & 0x1f) << 3;
			g = ((i >>  6) & 0x1f) << 3;
			b = ((i >>  1) & 0x1f) << 3;
			DrvPalette[i] = BurnHighCol(r | intensity, g | intensity, b | intensity, 0);
		}
		else
		{
			r = ((i >> 11) & 0x1f) << 3;
			g = ((i >>  5) & 0x3f) << 2;
			b = ((i >>  0) & 0x1f) << 3;
			DrvPalette[i] = BurnHighCol(r, g, b, 0);
		}
	}
}

static void GhostLcdConfigure()
{
	UINT32 vspw    =  (DrvLcdRegs[S3C24XX_LCDCON2] >>  0) & 0x3f;
	UINT32 vbpd    =  (DrvLcdRegs[S3C24XX_LCDCON2] >> 24) & 0xff;
	UINT32 lineval =  (DrvLcdRegs[S3C24XX_LCDCON2] >> 14) & 0x3ff;
	UINT32 vfpd    =  (DrvLcdRegs[S3C24XX_LCDCON2] >>  6) & 0xff;
	UINT32 hspw    =  (DrvLcdRegs[S3C24XX_LCDCON4] >>  0) & 0xff;
	UINT32 hbpd    =  (DrvLcdRegs[S3C24XX_LCDCON3] >> 19) & 0x7f;
	UINT32 hozval  =  (DrvLcdRegs[S3C24XX_LCDCON3] >>  8) & 0x7ff;

	DrvLcdState.width  = hozval + 1;
	DrvLcdState.height = lineval + 1;
	DrvLcdState.vpos_min = (vspw + 1) + (vbpd + 1);
	DrvLcdState.vpos_max = DrvLcdState.vpos_min + DrvLcdState.height - 1;
	DrvLcdState.total_height = (vspw + 1) + (vbpd + 1) + DrvLcdState.height + (vfpd + 1);

	if (DrvLcdState.width <= 0 || DrvLcdState.width > 640) DrvLcdState.width = 320;
	if (DrvLcdState.height <= 0 || DrvLcdState.height > 480) DrvLcdState.height = 240;
	if (DrvLcdState.total_height == 0) DrvLcdState.total_height = DrvLcdState.height;
	if (DrvLcdState.vpos_max < DrvLcdState.vpos_min) {
		DrvLcdState.vpos_min = 0;
		DrvLcdState.vpos_max = DrvLcdState.height ? (DrvLcdState.height - 1) : 0;
	}
	(void)hspw;
	(void)hbpd;

	DrvLcdState.vramaddr_cur = DrvLcdRegs[S3C24XX_LCDSADDR1] << 1;
	DrvLcdState.vramaddr_max = ((DrvLcdRegs[S3C24XX_LCDSADDR1] & 0xffe00000) | DrvLcdRegs[S3C24XX_LCDSADDR2]) << 1;
	DrvLcdState.offsize      = (DrvLcdRegs[S3C24XX_LCDSADDR3] >> 11) & 0x7ff;
	DrvLcdState.pagewidth_cur = 0;
	DrvLcdState.pagewidth_max = DrvLcdRegs[S3C24XX_LCDSADDR3] & 0x7ff;
	DrvLcdState.bppmode      = (DrvLcdRegs[S3C24XX_LCDCON1] >> 1) & 0x0f;
	DrvLcdState.tpal         = DrvLcdRegs[S3C24XX_TPAL];
	DrvLcdState.bswp         = (DrvLcdRegs[S3C24XX_LCDCON5] >> 1) & 1;
	DrvLcdState.hwswp        = (DrvLcdRegs[S3C24XX_LCDCON5] >> 0) & 1;

	if (DrvLcdState.pagewidth_max == 0) {
		DrvLcdState.pagewidth_max = DrvLcdState.width;
	}
}

static UINT8 GhostLcdReadByte(UINT32 address)
{
	UINT8 *ptr = GhostMapAddress(address);
	return ptr ? *ptr : 0xff;
}

static UINT32 GhostLcdDmaRead()
{
	UINT8 data[4];

	for (INT32 i = 0; i < 2; i++)
	{
		if (DrvLcdState.hwswp == 0)
		{
			if (DrvLcdState.bswp == 0)
			{
				if ((DrvLcdState.vramaddr_cur & 2) == 0)
				{
					data[i * 2 + 0] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 3);
					data[i * 2 + 1] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 2);
				}
				else
				{
					data[i * 2 + 0] = GhostLcdReadByte(DrvLcdState.vramaddr_cur - 1);
					data[i * 2 + 1] = GhostLcdReadByte(DrvLcdState.vramaddr_cur - 2);
				}
			}
			else
			{
				data[i * 2 + 0] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 0);
				data[i * 2 + 1] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 1);
			}
		}
		else
		{
			if (DrvLcdState.bswp == 0)
			{
				data[i * 2 + 0] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 1);
				data[i * 2 + 1] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 0);
			}
			else
			{
				if ((DrvLcdState.vramaddr_cur & 2) == 0)
				{
					data[i * 2 + 0] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 2);
					data[i * 2 + 1] = GhostLcdReadByte(DrvLcdState.vramaddr_cur + 3);
				}
				else
				{
					data[i * 2 + 0] = GhostLcdReadByte(DrvLcdState.vramaddr_cur - 2);
					data[i * 2 + 1] = GhostLcdReadByte(DrvLcdState.vramaddr_cur - 1);
				}
			}
		}

		DrvLcdState.vramaddr_cur += 2;
		DrvLcdState.pagewidth_cur++;

		if (DrvLcdState.pagewidth_cur >= DrvLcdState.pagewidth_max)
		{
			DrvLcdState.vramaddr_cur += DrvLcdState.offsize << 1;
			DrvLcdState.pagewidth_cur = 0;
		}
	}

	return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

static void GhostIicStart()
{
	UINT8 control = DrvIicRegs[S3C24XX_IICDS] & 0xff;
	UINT8 mode = (DrvIicRegs[S3C24XX_IICSTAT] >> 6) & 0x03;

	DrvIicStarted = 1;
	DrvIicCount = 1;

	// The S3C2410 IIC controller decides transfer direction from IICSTAT
	// mode selection (master receive/transmit), not from a latched software
	// flag. For random reads the control byte changes between phases while the
	// EEPROM address pointer must be preserved across the repeated start.
	if (mode == 3)
	{
		DrvIicRw = 0;
		DrvIicAddr = ((control >> 1) & 0x07) << 8;
		DrvIicExpectAddr = 1;
	}
	else if (mode == 2)
	{
		DrvIicRw = 1;
		DrvIicExpectAddr = 0;
	}

	// The S3C2410 raises IIC pending asynchronously after start/resume completes.
	// We model the completed step directly so the game-side polling state machine
	// can observe the transaction progress.
	if (DrvIicRegs[S3C24XX_IICCON] & 0x20) {
		DrvIicRegs[S3C24XX_IICCON] |= 0x10;
	}
}

static void GhostIicResume()
{
	if (DrvIicStarted == 0) {
		GhostIicStart();
		return;
	}

	UINT8 mode = (DrvIicRegs[S3C24XX_IICSTAT] >> 6) & 0x03;

	if (mode == 2)
	{
		DrvIicRw = 1;
		DrvIicRegs[S3C24XX_IICDS] = DrvEeprom[DrvIicAddr & 0x7ff];
		DrvIicAddr = (DrvIicAddr + 1) & 0x7ff;
	}
	else if (mode == 3)
	{
		DrvIicRw = 0;
		UINT8 data = DrvIicRegs[S3C24XX_IICDS] & 0xff;
		if (DrvIicExpectAddr)
		{
			DrvIicAddr = (DrvIicAddr & 0x700) | data;
			DrvIicExpectAddr = 0;
		}
		else
		{
			DrvEeprom[DrvIicAddr & 0x7ff] = data;
			DrvIicAddr = (DrvIicAddr + 1) & 0x7ff;
		}
	}

	DrvIicCount++;

	if (DrvIicRegs[S3C24XX_IICCON] & 0x20) {
		DrvIicRegs[S3C24XX_IICCON] |= 0x10;
	}
}

static void GhostIicStop()
{
	DrvIicStarted = 0;
	DrvIicCount = 0;
}

static UINT32 GhostLcdGetVpos()
{
	if (DrvLcdState.total_height == 0) return 0;

	INT32 cycles = Arm9TotalCycles();
	if (cycles < 0) cycles = 0;

	UINT32 nCyclesTotal = 200000000 / 60;
	UINT32 vpos = (UINT64)cycles * DrvLcdState.total_height / nCyclesTotal;
	if (vpos >= DrvLcdState.total_height) {
		vpos = DrvLcdState.total_height - 1;
	}

	return vpos;
}

static void GhostCombineReg(UINT32 *regs, UINT32 index, UINT32 data, UINT32 mask)
{
	regs[index] = (regs[index] & ~mask) | (data & mask);
}

static UINT32 GhostLcdReadReg(UINT32 address)
{
	UINT32 index = (address - S3C24XX_BASE_LCD) >> 2;
	UINT32 data = DrvLcdRegs[index];

	if (index == S3C24XX_LCDCON1)
	{
		UINT32 vpos = GhostLcdGetVpos();
		if (vpos < DrvLcdState.vpos_min) vpos = DrvLcdState.vpos_min;
		if (vpos > DrvLcdState.vpos_max) vpos = DrvLcdState.vpos_max;
		data = (data & ~0xfffc0000) | (((DrvLcdState.vpos_max - vpos) & 0x3fff) << 18);
	}
	else if (index == S3C24XX_LCDCON5)
	{
		UINT32 vpos = GhostLcdGetVpos();
		data &= ~0x00018000;
		if (vpos > DrvLcdState.vpos_max) {
			data |= 0x00018000;
		}

		UINT32 pc = Arm9DbgGetPC();
		if (pc == 0x3001c0d8 || pc == 0x3001c0e0 || pc == 0x3001c0e4 || pc == 0x3001c0ec) {
			Arm9BurnCycles(4000);
		}
	}

	return data;
}

static INT32 GhostUartGetIndex(UINT32 address)
{
	UINT32 channel = 0;
	UINT32 offset = 0;

	if (address >= S3C24XX_BASE_UART0 && address < S3C24XX_BASE_UART0 + 0x30) {
		channel = 0;
		offset = address - S3C24XX_BASE_UART0;
	} else
	if (address >= S3C24XX_BASE_UART1 && address < S3C24XX_BASE_UART1 + 0x30) {
		channel = 1;
		offset = address - S3C24XX_BASE_UART1;
	} else
	if (address >= S3C24XX_BASE_UART2 && address < S3C24XX_BASE_UART2 + 0x30) {
		channel = 2;
		offset = address - S3C24XX_BASE_UART2;
	} else {
		return -1;
	}

	return (channel * 0x10) + (offset >> 2);
}

static UINT32 GhostUartReadReg(UINT32 address)
{
	INT32 index = GhostUartGetIndex(address);
	if (index < 0) return 0;

	UINT32 reg = index & 0x0f;
	UINT32 data = DrvUartRegs[index];

	switch (reg)
	{
		case S3C24XX_UTRSTAT:
			data = (data & ~0x00000006) | 0x00000006;
		return data;

		case S3C24XX_URXH:
			DrvUartRegs[(index & ~0x0f) + S3C24XX_UTRSTAT] &= ~1;
		return data & 0xff;

		default:
		return data;
	}
}

static void GhostUartWriteReg(UINT32 address, UINT32 data, UINT32 mask)
{
	INT32 index = GhostUartGetIndex(address);
	if (index < 0) return;

	UINT32 reg = index & 0x0f;
	GhostCombineReg(DrvUartRegs, index, data, mask);

	switch (reg)
	{
		case S3C24XX_UFCON:
			DrvUartRegs[index] &= ~((1 << 2) | (1 << 1));
		return;

		case S3C24XX_UTXH:
		return;
	}
}

static UINT32 GhostReadLong(UINT32 address)
{
	if (address == 0x10000000) return DrvInputs[0];
	if (address == 0x10100000) return DrvInputs[1];
	if (address == 0x10200000) return DrvInputs[2];

	if (address >= S3C24XX_BASE_MEMCON && address < S3C24XX_BASE_MEMCON + 0x80) {
		return DrvMemConRegs[(address - S3C24XX_BASE_MEMCON) >> 2];
	}

	if (address >= S3C24XX_BASE_IRQ && address < S3C24XX_BASE_IRQ + 0x20) {
		return DrvIrqRegs[(address - S3C24XX_BASE_IRQ) >> 2];
	}

	if (address >= S3C24XX_BASE_DMA0 && address < S3C24XX_BASE_DMA3 + 0x40) {
		UINT32 index = (address - S3C24XX_BASE_DMA0) >> 2;
		return DrvDummyRegs[index & 0x3f];
	}

	if (address >= S3C24XX_BASE_CLKPOW && address < S3C24XX_BASE_CLKPOW + 0x20) {
		return DrvClkPowRegs[(address - S3C24XX_BASE_CLKPOW) >> 2];
	}

	if (address >= S3C24XX_BASE_LCD && address < S3C24XX_BASE_LCD + 0x80) {
		return GhostLcdReadReg(address);
	}

	if (address >= S3C24XX_BASE_NAND && address < S3C24XX_BASE_NAND + 0x20)
	{
		UINT32 index = (address - S3C24XX_BASE_NAND) >> 2;
		if (index == S3C24XX_NFDATA) return GhostNandRead();
		if (index == S3C24XX_NFSTAT) return DrvNandRegs[S3C24XX_NFSTAT] | 1;
		if (index == S3C24XX_NFECC) {
			UINT8 mecc[4];
			UINT32 flashAddr = DrvNandState.page_addr * 0x200;
			memset(mecc, 0xff, sizeof(mecc));
			if (flashAddr + 0x200 <= 0x2000000) {
				GhostCalcMecc(DrvFlashROM + flashAddr, 0x200, mecc);
			}
			return mecc[0] | (mecc[1] << 8) | (mecc[2] << 16) | (mecc[3] << 24);
		}
		return DrvNandRegs[index];
	}

	if (address >= S3C24XX_BASE_PWM && address < S3C24XX_BASE_PWM + 0x80) {
		return DrvPwmRegs[(address - S3C24XX_BASE_PWM) >> 2];
	}

	if (address >= S3C24XX_BASE_IIC && address < S3C24XX_BASE_IIC + 0x20)
	{
		UINT32 index = (address - S3C24XX_BASE_IIC) >> 2;
		UINT32 data = DrvIicRegs[index];
		if (index == S3C24XX_IICSTAT) data &= ~0x0f;
		return data;
	}

	if (address >= S3C24XX_BASE_GPIO && address < S3C24XX_BASE_GPIO + 0xc0)
	{
		UINT32 index = (address - S3C24XX_BASE_GPIO) >> 2;
		switch (index)
		{
			case S3C24XX_GPADAT: return GhostGpioPortRead(PORT_A) & 0x007fffff;
			case S3C24XX_GPBDAT: return GhostGpioPortRead(PORT_B) & 0x000007ff;
			case S3C24XX_GPCDAT: return GhostGpioPortRead(PORT_C) & 0x0000ffff;
			case S3C24XX_GPDDAT: return GhostGpioPortRead(PORT_D) & 0x0000ffff;
			case S3C24XX_GPEDAT: return GhostGpioPortRead(PORT_E) & 0x0000ffff;
			case S3C24XX_GPFDAT: return GhostGpioPortRead(PORT_F) & 0x000000ff;
			case S3C24XX_GPGDAT: return GhostGpioPortRead(PORT_G) & 0x0000ffff;
			case S3C24XX_GPHDAT: return GhostGpioPortRead(PORT_H) & 0x000007ff;
		}
		return DrvGpioRegs[index];
	}

	if (address >= S3C24XX_BASE_RTC && address < S3C24XX_BASE_RTC + 0x80) {
		return DrvRtcRegs[(address - S3C24XX_BASE_RTC) >> 2];
	}

	if (GhostUartGetIndex(address) >= 0) {
		return GhostUartReadReg(address);
	}

	if (address >= 0x50000000 && address < 0x5c000000) {
		return DrvDummyRegs[((address >> 2) & 0xff)];
	}
	return 0;
}

static void GhostWriteLongMasked(UINT32 address, UINT32 data, UINT32 mask)
{
	if (address == 0x10300000)
	{
		UINT32 value = GhostReadLong(address);
		value = (value & ~mask) | (data & mask);
		GhostSoundSync();
		DrvSoundLatch = value & 0xff;
		DrvSoundPending = 1;
		GhostSoundSetIrq(1);
		return;
	}

	if (address >= S3C24XX_BASE_MEMCON && address < S3C24XX_BASE_MEMCON + 0x80) {
		GhostCombineReg(DrvMemConRegs, (address - S3C24XX_BASE_MEMCON) >> 2, data, mask);
		return;
	}

	if (address >= S3C24XX_BASE_IRQ && address < S3C24XX_BASE_IRQ + 0x20)
	{
		UINT32 index = (address - S3C24XX_BASE_IRQ) >> 2;
		UINT32 oldValue = DrvIrqRegs[index];
		GhostCombineReg(DrvIrqRegs, index, data, mask);

		if (index == S3C24XX_SRCPND || index == S3C24XX_INTPND || index == S3C24XX_SUBSRCPND) {
			DrvIrqRegs[index] = oldValue & ~(data & mask);
			DrvIrqRegs[S3C24XX_INTOFFSET] = 0;
		}
		return;
	}

	if (address >= S3C24XX_BASE_DMA0 && address < S3C24XX_BASE_DMA3 + 0x40)
	{
		UINT32 index = (address - S3C24XX_BASE_DMA0) >> 2;
		GhostCombineReg(DrvDummyRegs, index & 0x3f, data, mask);
		return;
	}

	if (address >= S3C24XX_BASE_CLKPOW && address < S3C24XX_BASE_CLKPOW + 0x20) {
		GhostCombineReg(DrvClkPowRegs, (address - S3C24XX_BASE_CLKPOW) >> 2, data, mask);
		return;
	}

	if (address >= S3C24XX_BASE_LCD && address < S3C24XX_BASE_LCD + 0x80)
	{
		GhostCombineReg(DrvLcdRegs, (address - S3C24XX_BASE_LCD) >> 2, data, mask);
		return;
	}

	if (address >= S3C24XX_BASE_NAND && address < S3C24XX_BASE_NAND + 0x20)
	{
		UINT32 index = (address - S3C24XX_BASE_NAND) >> 2;
		GhostCombineReg(DrvNandRegs, index, data, mask);

		if ((mask & 0x000000ff) == 0) return;

		switch (index)
		{
			case S3C24XX_NFCMD:
				GhostNandCommand(data & 0xff);
			return;

			case S3C24XX_NFADDR:
				GhostNandAddress(data & 0xff);
			return;
		}
		return;
	}

	if (address >= S3C24XX_BASE_PWM && address < S3C24XX_BASE_PWM + 0x80) {
		GhostCombineReg(DrvPwmRegs, (address - S3C24XX_BASE_PWM) >> 2, data, mask);
		return;
	}

	if (address >= S3C24XX_BASE_IIC && address < S3C24XX_BASE_IIC + 0x20)
	{
		UINT32 index = (address - S3C24XX_BASE_IIC) >> 2;
		UINT32 oldCon = DrvIicRegs[S3C24XX_IICCON];
		GhostCombineReg(DrvIicRegs, index, data, mask);

		if (index == S3C24XX_IICCON)
		{
			INT32 oldPending = (oldCon >> 4) & 1;
			INT32 newPending = (DrvIicRegs[S3C24XX_IICCON] >> 4) & 1;
			if (oldPending && !newPending)
			{
				if (DrvIicRegs[S3C24XX_IICSTAT] & 0x20) {
					if (DrvIicCount == 0) GhostIicStart(); else GhostIicResume();
				} else {
					GhostIicStop();
				}
			}
		}
		else if (index == S3C24XX_IICSTAT)
		{
			DrvIicCount = 0;
			if (((DrvIicRegs[S3C24XX_IICCON] >> 4) & 1) == 0)
			{
				if (DrvIicRegs[S3C24XX_IICSTAT] & 0x20) {
					GhostIicStart();
				} else {
					GhostIicStop();
				}
			}
		}
		return;
	}

	if (address >= S3C24XX_BASE_GPIO && address < S3C24XX_BASE_GPIO + 0xc0)
	{
		UINT32 index = (address - S3C24XX_BASE_GPIO) >> 2;
		UINT32 oldValue = DrvGpioRegs[index];
		GhostCombineReg(DrvGpioRegs, index, data, mask);

		switch (index)
		{
			case S3C24XX_GPADAT: GhostGpioPortWrite(PORT_A, DrvGpioRegs[index] & 0x007fffff); return;
			case S3C24XX_GPBDAT: GhostGpioPortWrite(PORT_B, DrvGpioRegs[index] & 0x000007ff); return;
			case S3C24XX_GPCDAT: GhostGpioPortWrite(PORT_C, DrvGpioRegs[index] & 0x0000ffff); return;
			case S3C24XX_GPDDAT: GhostGpioPortWrite(PORT_D, DrvGpioRegs[index] & 0x0000ffff); return;
			case S3C24XX_GPEDAT: GhostGpioPortWrite(PORT_E, DrvGpioRegs[index] & 0x0000ffff); return;
			case S3C24XX_GPFDAT: GhostGpioPortWrite(PORT_F, DrvGpioRegs[index] & 0x000000ff); return;
			case S3C24XX_GPGDAT: GhostGpioPortWrite(PORT_G, DrvGpioRegs[index] & 0x0000ffff); return;
			case S3C24XX_GPHDAT: GhostGpioPortWrite(PORT_H, DrvGpioRegs[index] & 0x000007ff); return;

			case S3C24XX_EINTPEND:
				DrvGpioRegs[index] = oldValue & ~(data & mask);
			return;

			case S3C24XX_GSTATUS2:
				DrvGpioRegs[index] = (oldValue & ~(data & mask)) & 7;
			return;
		}
		return;
	}

	if (address >= S3C24XX_BASE_RTC && address < S3C24XX_BASE_RTC + 0x80) {
		GhostCombineReg(DrvRtcRegs, (address - S3C24XX_BASE_RTC) >> 2, data, mask);
		return;
	}

	if (GhostUartGetIndex(address) >= 0) {
		GhostUartWriteReg(address, data, mask);
		return;
	}

	if (address >= 0x50000000 && address < 0x5c000000) {
		GhostCombineReg(DrvDummyRegs, ((address >> 2) & 0xff), data, mask);
		return;
	}
}

static UINT8 GhostReadByte(UINT32 address)
{
	UINT32 data = GhostReadLong(address & ~3);
	return (data >> ((address & 3) * 8)) & 0xff;
}

static UINT16 GhostReadWord(UINT32 address)
{
	UINT32 data = GhostReadLong(address & ~3);
	return (data >> ((address & 2) * 8)) & 0xffff;
}

static void GhostWriteByte(UINT32 address, UINT8 data)
{
	GhostWriteLongMasked(address & ~3, (UINT32)data << ((address & 3) * 8), 0xff << ((address & 3) * 8));
}

static void GhostWriteWord(UINT32 address, UINT16 data)
{
	GhostWriteLongMasked(address & ~3, (UINT32)data << ((address & 2) * 8), 0xffff << ((address & 2) * 8));
}

static void GhostWriteLong(UINT32 address, UINT32 data)
{
	GhostWriteLongMasked(address, data, 0xffffffff);
}

static void GhostResetRegs()
{
	memset(DrvMemConRegs, 0, (UINT8*)RamEnd - (UINT8*)DrvMemConRegs);

	DrvIrqRegs[S3C24XX_INTMSK]    = 0xffffffff;
	DrvIrqRegs[S3C24XX_PRIORITY]  = 0x0000007f;
	DrvIrqRegs[S3C24XX_INTSUBMSK] = 0x000007ff;

	DrvClkPowRegs[0] = 0x00ffffff;
	DrvClkPowRegs[1] = 0x0005c080;
	DrvClkPowRegs[2] = 0x00028080;
	DrvClkPowRegs[3] = 0x0007fff0;
	DrvClkPowRegs[4] = 0x00000004;

	DrvLcdRegs[S3C24XX_LCDINTMSK] = 3;
	DrvLcdRegs[S3C24XX_LPCSEL] = 4;

	DrvNandRegs[S3C24XX_NFSTAT] = 1;

	for (INT32 channel = 0; channel < 3; channel++) {
		DrvUartRegs[channel * 0x10 + S3C24XX_UTRSTAT] = 6;
	}

	DrvGpioRegs[S3C24XX_GPACON]   = 0x007fffff;
	DrvGpioRegs[S3C24XX_MISCCR]   = 0x00010330;
	DrvGpioRegs[S3C24XX_EINTMASK] = 0x00fffff0;
	DrvGpioRegs[S3C24XX_GSTATUS1] = 0x32410002;
	DrvGpioRegs[S3C24XX_GSTATUS2] = 0x00000001;
	DrvGpioRegs[(0x18 / 4)]       = 0x0000f000; // GPBUP
	DrvGpioRegs[(0x68 / 4)]       = 0x0000f800; // GPGUP

	memset(DrvEeprom, 0xff, 0x800);
	memset(DrvBballoonPort, 0, 0x20 * sizeof(UINT32));

	DrvSecurityCount = 0;
	DrvSoundLatch = 0;
	DrvSoundPending = 0;
	DrvQs1000Bank = 0;
	DrvIicStarted = 0;
	DrvIicRw = 0;
	DrvIicExpectAddr = 0;
	DrvIicCount = 0;
	DrvIicAddr = 0;
	DrvPalette565Mode = 0xff;

	memset(&DrvNandState, 0, sizeof(DrvNandState));
	memset(&DrvLcdState, 0, sizeof(DrvLcdState));
}

static INT32 DrvDoReset()
{
	memset(AllRam, 0, RamEnd - AllRam);
	GhostResetRegs();
	GhostNandAutoBoot();

	Arm9Open(0);
	Arm9Reset();
	Arm9Close();

	qs1000_reset();
	GhostSetSoundBankDirect(0);
	GhostSoundSetIrq(0);

	nGhostCyclesExtra = 0;

	HiscoreReset();
	return 0;
}

static INT32 BballoonInit()
{
	BurnAllocMemIndex();

	if (DrvLoadRoms()) {
		BalloonExit:
		BurnFreeMemIndex();
		return 1;
	}

	GenericTilesInit();

	Arm9Init(0);
	Arm9Open(0);
	Arm9MapMemory(DrvStepRAM, 0x00000000, 0x00000fff, MAP_RAM);
	Arm9MapMemory(DrvSysRAM,  0x30000000, 0x31ffffff, MAP_RAM);
	Arm9MapMemory(DrvSysRAM,  0x32000000, 0x33ffffff, MAP_RAM);
	Arm9MapMemory(DrvStepRAM, 0x40000000, 0x40000fff, MAP_RAM);
	Arm9SetReadByteHandler(GhostReadByte);
	Arm9SetReadWordHandler(GhostReadWord);
	Arm9SetReadLongHandler(GhostReadLong);
	Arm9SetWriteByteHandler(GhostWriteByte);
	Arm9SetWriteWordHandler(GhostWriteWord);
	Arm9SetWriteLongHandler(GhostWriteLong);
	Arm9Close();

	qs1000_init(DrvQSROM, DrvSndROM, 0x1000000);
	qs1000_set_read_handler(1, GhostQs1000P1Read);
	qs1000_set_write_handler(3, GhostQs1000P3Write);
	qs1000_set_volume(1.00);
	GhostSetSoundBankDirect(0);

	if (DrvDoReset()) goto BalloonExit;

	return 0;
}

static INT32 BballoonExit()
{
	GenericTilesExit();
	Arm9Exit();
	qs1000_exit();
	BurnFreeMemIndex();

	AllMem = NULL;

	return 0;
}

static INT32 BballoonDraw()
{
	if (pBurnDraw == NULL) return 0;

	GhostLcdConfigure();
	GhostRecalcPalette();
	BurnTransferClear();

	if ((DrvLcdRegs[S3C24XX_LCDCON1] & 1) && DrvLcdState.bppmode == 0x0c)
	{
		INT32 width = DrvLcdState.width;
		INT32 height = DrvLcdState.height;

		if (width > nScreenWidth) width = nScreenWidth;
		if (height > nScreenHeight) height = nScreenHeight;

		for (INT32 y = 0; y < height; y++)
		{
			UINT16 *dst = pTransDraw + (y * nScreenWidth);

			for (INT32 x = 0; x < width; x += 2)
			{
				UINT32 data = GhostLcdDmaRead();
				dst[x + 0] = (data >> 16) & 0xffff;
				if (x + 1 < width) {
					dst[x + 1] = data & 0xffff;
				}
			}
		}
	}

	BurnTransferCopy(DrvPalette);
	return 0;
}

static INT32 BballoonFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	DrvInputs[0] = GhostReadInput(0);
	DrvInputs[1] = GhostReadInput(1);
	DrvInputs[2] = GhostReadInput(2);

	Arm9NewFrame();
	mcs51NewFrame();

	INT32 nInterleave = 100;
	INT32 nCyclesTotal[2] = { 200000000 / 60, (24000000 / 12) / 60 };
	INT32 nCyclesDone[2] = { nGhostCyclesExtra, 0 };

	Arm9Open(0);
	mcs51Open(0);
	for (INT32 i = 0; i < nInterleave; i++)
	{
		nCyclesDone[0] += Arm9Run((((i + 1) * nCyclesTotal[0]) / nInterleave) - nCyclesDone[0]);

		nCyclesDone[1] += mcs51Run((((i + 1) * nCyclesTotal[1]) / nInterleave) - mcs51TotalCycles());
	}
	mcs51Close();
	Arm9Close();

	nGhostCyclesExtra = nCyclesDone[0] - nCyclesTotal[0];
	DrvFrameCounter++;

	if (pBurnSoundOut) {
		qs1000_update(pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		BballoonDraw();
	}

	return 0;
}

static INT32 BballoonScan(INT32 nAction, INT32 *pnMin)
{
	if (pnMin) {
		*pnMin = 0x029719;
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
		Arm9Scan(nAction);
		mcs51_scan(nAction);
		qs1000_scan(nAction, pnMin);

		SCAN_VAR(DrvInputs);
		SCAN_VAR(DrvSoundLatch);
		SCAN_VAR(DrvSoundPending);
		SCAN_VAR(DrvSecurityCount);
		SCAN_VAR(DrvQs1000Bank);
		SCAN_VAR(DrvIicStarted);
		SCAN_VAR(DrvIicRw);
		SCAN_VAR(DrvIicExpectAddr);
		SCAN_VAR(DrvIicCount);
		SCAN_VAR(DrvIicAddr);
		SCAN_VAR(DrvFrameCounter);
		SCAN_VAR(DrvPalette565Mode);
		SCAN_VAR(DrvNandState);
		SCAN_VAR(DrvLcdState);
		SCAN_VAR(nGhostCyclesExtra);
	}

	if (nAction & ACB_WRITE)
	{
		GhostSetSoundBank(DrvQs1000Bank);
	}

	return 0;
}

static struct BurnRomInfo BballoonRomDesc[] = {
	{ "flash.u1",    0x2000000, 0x73285634, BRF_PRG | BRF_ESS },
	{ "b2.u20",      0x080000,  0x0a12334c, BRF_PRG | BRF_ESS },
	{ "b1.u16",      0x100000,  0xc42c1c85, BRF_SND },
	{ "qs1001a.u17", 0x080000,  0xd13c6407, BRF_SND },
};

STD_ROM_PICK(Bballoon)
STD_ROM_FN(Bballoon)

struct BurnDriver BurnDrvBballoon = {
	"bballoon", NULL, NULL, NULL, "2003",
	"BnB Arcade (V1.0005 World)\0", NULL, "Eolith", "Ghost Hardware",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
	NULL, BballoonRomInfo, BballoonRomName, NULL, NULL, NULL, NULL, BballoonInputInfo, NULL,
	BballoonInit, BballoonExit, BballoonFrame, BballoonDraw, BballoonScan, &DrvRecalc, 0x10000,
	320, 240, 4, 3
};

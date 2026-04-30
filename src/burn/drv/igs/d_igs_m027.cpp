#include "igs_m027.h"
#include "igs027a.h"
#include "8255ppi.h"
#include "tiles_generic.h"

#ifndef ROM_NAME
#define ROM_NAME(name) NULL, name##RomInfo, name##RomName, NULL, NULL, NULL, NULL
#endif

#ifndef INPUT_FN
#define INPUT_FN(name) name##InputInfo
#endif

#ifndef DIP_FN
#define DIP_FN(name) name##DIPInfo
#endif

static UINT8 DrvInputPort[M027_INPUT_MAX] = {0};
static UINT8 DrvDips[3] = {0,0,0};
static UINT8 DrvReset = 0;
static UINT8 DrvRecalc = 0;
static UINT8 DrvInputState[M027_INPUT_MAX] = {0};
static UINT8 DrvCoinPulse[2] = {0, 0};
static UINT8 DrvCoinPrev[2] = {0, 0};

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;

static UINT8 *DrvArmROM;
static UINT8 *DrvUserROM;
static UINT8 *DrvUserMap;
static UINT8 *DrvTileROM;
static UINT8 *DrvSpriteROM;
static UINT8 *DrvOkiROM;
static UINT8 *DrvNVRAM;
static UINT8 *DrvArmRAM;

static INT32 DrvArmLen;
static INT32 DrvUserLen;
static INT32 DrvTileLen;
static INT32 DrvSpriteLen;
static INT32 DrvOkiLen;

static UINT8 DrvXorTable[0x100];
static UINT8 DrvIoSelect[2];
static UINT8 DrvHopperMotor;
static UINT8 DrvTicketMotor;
static UINT8 DrvMisc;
static INT32 nCyclesDone[1];
static INT32 nCyclesTotal[1];

static INT32 DrvOkiClock;
static INT32 DrvOkiDivisor;
static INT32 DrvUseSplitBank;
static INT32 DrvUseHopper;
static INT32 DrvUseTicket;
static INT32 DrvNumPpi;
static INT32 DrvControlsBank;
static UINT8 DrvControlsMask;
static UINT8 DrvControlsValue;

static UINT32 (*pDrvMainInCb)();
static void (*DrvMainOutCb)(UINT8);
static void (*pDrvPostloadCb)();
static void (*pDrvDecryptCb)();
static void (*pDrvGfxSetupCb)();

static struct BurnInputInfo M027MahjongInputList[] = {
	{"Reset",            BIT_DIGITAL,   &DrvReset,                      "reset"     },
	{"Dip A",            BIT_DIPSWITCH, DrvDips + 0,                   "dip"       },
	{"Dip B",            BIT_DIPSWITCH, DrvDips + 1,                   "dip"       },
	{"Dip C",            BIT_DIPSWITCH, DrvDips + 2,                   "dip"       },
	{"Coin 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN1,  "p1 coin"   },
	{"Coin 2",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN2,  "p2 coin"   },
	{"Start",            BIT_DIGITAL,   DrvInputPort + M027_IN_START1, "p1 start"  },
	{"Up",               BIT_DIGITAL,   DrvInputPort + M027_IN_UP,     "p1 up"     },
	{"Down",             BIT_DIGITAL,   DrvInputPort + M027_IN_DOWN,   "p1 down"   },
	{"Left",             BIT_DIGITAL,   DrvInputPort + M027_IN_LEFT,   "p1 left"   },
	{"Right",            BIT_DIGITAL,   DrvInputPort + M027_IN_RIGHT,  "p1 right"  },
	{"Button 1",         BIT_DIGITAL,   DrvInputPort + M027_IN_BUTTON1,"p1 fire 1" },
	{"Button 2",         BIT_DIGITAL,   DrvInputPort + M027_IN_BUTTON2,"p1 fire 2" },
	{"Button 3",         BIT_DIGITAL,   DrvInputPort + M027_IN_BUTTON3,"p1 fire 3" },
	{"Service",          BIT_DIGITAL,   DrvInputPort + M027_IN_SERVICE,"service"   },
	{"Book",             BIT_DIGITAL,   DrvInputPort + M027_IN_BOOK,   "diag"      },
	{"Show Credits",     BIT_DIGITAL,   DrvInputPort + M027_IN_SHOW_CREDITS, "service1" },
	{"Key In",           BIT_DIGITAL,   DrvInputPort + M027_IN_KEYIN,  "service2"  },
	{"Payout",           BIT_DIGITAL,   DrvInputPort + M027_IN_PAYOUT, "p1 fire 4" },
	{"Key Out",          BIT_DIGITAL,   DrvInputPort + M027_IN_KEYOUT, "p1 fire 5" },
	{"Mahjong A",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_A,   "mah a"     },
	{"Mahjong B",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_B,   "mah b"     },
	{"Mahjong C",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_C,   "mah c"     },
	{"Mahjong D",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_D,   "mah d"     },
	{"Mahjong E",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_E,   "mah e"     },
	{"Mahjong F",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_F,   "mah f"     },
	{"Mahjong G",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_G,   "mah g"     },
	{"Mahjong H",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_H,   "mah h"     },
	{"Mahjong I",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_I,   "mah i"     },
	{"Mahjong J",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_J,   "mah j"     },
	{"Mahjong K",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_K,   "mah k"     },
	{"Mahjong L",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_L,   "mah l"     },
	{"Mahjong M",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_M,   "mah m"     },
	{"Mahjong N",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_N,   "mah n"     },
	{"Mahjong Kan",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_KAN, "mah kan"   },
	{"Mahjong Reach",    BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_REACH,"mah reach"},
	{"Mahjong Bet",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BET, "mah bet"   },
	{"Mahjong Chi",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_CHI, "mah chi"   },
	{"Mahjong Ron",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_RON, "mah ron"   },
	{"Mahjong Pon",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_PON, "mah pon"   },
	{"Mahjong Last",     BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_LAST,"mah last"   },
	{"Mahjong Score",    BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SCORE,"mah score" },
	{"Mahjong Double",   BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_DOUBLE,"mah double"},
	{"Mahjong Big",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BIG, "mah big"   },
	{"Mahjong Small",    BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SMALL,"mah small" },
	{"Clear NVRAM",      BIT_DIGITAL,   DrvInputPort + M027_IN_CLEAR_NVRAM, "service3"},
};

STDINPUTINFO(M027Mahjong)

static struct BurnInputInfo M027DdzInputList[] = {
	{"Reset",            BIT_DIGITAL,   &DrvReset,                      "reset"     },
	{"Dip A",            BIT_DIPSWITCH, DrvDips + 0,                   "dip"       },
	{"Dip B",            BIT_DIPSWITCH, DrvDips + 1,                   "dip"       },
	{"Dip C",            BIT_DIPSWITCH, DrvDips + 2,                   "dip"       },
	{"Coin 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN1,  "p1 coin"   },
	{"Start",            BIT_DIGITAL,   DrvInputPort + M027_IN_START1, "p1 start"  },
	{"Up",               BIT_DIGITAL,   DrvInputPort + M027_IN_UP,     "p1 up"     },
	{"Down",             BIT_DIGITAL,   DrvInputPort + M027_IN_DOWN,   "p1 down"   },
	{"Left",             BIT_DIGITAL,   DrvInputPort + M027_IN_LEFT,   "p1 left"   },
	{"Right",            BIT_DIGITAL,   DrvInputPort + M027_IN_RIGHT,  "p1 right"  },
	{"Button 1",         BIT_DIGITAL,   DrvInputPort + M027_IN_BUTTON1,"p1 fire 1" },
	{"Button 2",         BIT_DIGITAL,   DrvInputPort + M027_IN_BUTTON2,"p1 fire 2" },
	{"Button 3",         BIT_DIGITAL,   DrvInputPort + M027_IN_BUTTON3,"p1 fire 3" },
	{"Key In",           BIT_DIGITAL,   DrvInputPort + M027_IN_KEYIN,  "service2"  },
	{"Payout",           BIT_DIGITAL,   DrvInputPort + M027_IN_PAYOUT, "p1 fire 4" },
	{"Key Out",          BIT_DIGITAL,   DrvInputPort + M027_IN_KEYOUT, "p1 fire 5" },
	{"Service",          BIT_DIGITAL,   DrvInputPort + M027_IN_SERVICE,"service"   },
	{"Book",             BIT_DIGITAL,   DrvInputPort + M027_IN_BOOK,   "diag"      },
	{"Mahjong A",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_A,   "mah a"     },
	{"Mahjong B",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_B,   "mah b"     },
	{"Mahjong C",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_C,   "mah c"     },
	{"Mahjong D",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_D,   "mah d"     },
	{"Mahjong E",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_E,   "mah e"     },
	{"Mahjong F",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_F,   "mah f"     },
	{"Mahjong G",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_G,   "mah g"     },
	{"Mahjong H",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_H,   "mah h"     },
	{"Mahjong I",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_I,   "mah i"     },
	{"Mahjong J",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_J,   "mah j"     },
	{"Mahjong K",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_K,   "mah k"     },
	{"Mahjong L",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_L,   "mah l"     },
	{"Mahjong M",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_M,   "mah m"     },
	{"Mahjong N",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_N,   "mah n"     },
	{"Mahjong Kan",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_KAN, "mah kan"   },
	{"Mahjong Reach",    BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_REACH,"mah reach"},
	{"Mahjong Bet",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BET, "mah bet"   },
	{"Mahjong Chi",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_CHI, "mah chi"   },
	{"Mahjong Ron",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_RON, "mah ron"   },
	{"Mahjong Pon",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_PON, "mah pon"   },
	{"Mahjong Last",     BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_LAST,"mah last"   },
	{"Mahjong Score",    BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SCORE,"mah score" },
	{"Mahjong Double",   BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_DOUBLE,"mah double"},
	{"Mahjong Big",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BIG, "mah big"   },
	{"Mahjong Small",    BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SMALL,"mah small" },
	{"Clear NVRAM",      BIT_DIGITAL,   DrvInputPort + M027_IN_CLEAR_NVRAM, "service3"},
};

STDINPUTINFO(M027Ddz)

static struct BurnInputInfo M027Tct2pInputList[] = {
	{"Reset",            BIT_DIGITAL,   &DrvReset,                      "reset"     },
	{"Dip A",            BIT_DIPSWITCH, DrvDips + 0,                   "dip"       },
	{"Dip B",            BIT_DIPSWITCH, DrvDips + 1,                   "dip"       },
	{"Dip C",            BIT_DIPSWITCH, DrvDips + 2,                   "dip"       },
	{"Coin 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN1,  "p1 coin"   },
	{"Start",            BIT_DIGITAL,   DrvInputPort + M027_IN_START1, "p1 start"  },
	{"Bet",              BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BET, "p1 fire 1" },
	{"Big",              BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BIG, "p1 up"     },
	{"Small",            BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SMALL,"p1 down"  },
	{"Stop 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP1,  "p1 fire 2" },
	{"Stop 2",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP2,  "p1 fire 3" },
	{"Stop 3",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP3,  "p1 fire 4" },
	{"Stop 4",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP4,  "p1 fire 5" },
	{"Stop 5",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP5,  "p1 fire 6" },
	{"Payout",           BIT_DIGITAL,   DrvInputPort + M027_IN_PAYOUT, "service1"  },
	{"Key In",           BIT_DIGITAL,   DrvInputPort + M027_IN_KEYIN,  "service2"  },
	{"Key Out",          BIT_DIGITAL,   DrvInputPort + M027_IN_KEYOUT, "service3"  },
	{"Service",          BIT_DIGITAL,   DrvInputPort + M027_IN_SERVICE,"service"   },
	{"Book",             BIT_DIGITAL,   DrvInputPort + M027_IN_BOOK,   "diag"      },
	{"Mahjong A",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_A,   "mah a"     },
	{"Mahjong B",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_B,   "mah b"     },
	{"Mahjong C",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_C,   "mah c"     },
	{"Mahjong D",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_D,   "mah d"     },
	{"Mahjong E",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_E,   "mah e"     },
	{"Mahjong F",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_F,   "mah f"     },
	{"Mahjong G",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_G,   "mah g"     },
	{"Mahjong H",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_H,   "mah h"     },
	{"Mahjong I",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_I,   "mah i"     },
	{"Mahjong J",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_J,   "mah j"     },
	{"Mahjong K",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_K,   "mah k"     },
	{"Mahjong L",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_L,   "mah l"     },
	{"Mahjong M",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_M,   "mah m"     },
	{"Mahjong N",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_N,   "mah n"     },
	{"Mahjong Kan",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_KAN, "mah kan"   },
	{"Mahjong Reach",    BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_REACH,"mah reach"},
	{"Mahjong Chi",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_CHI, "mah chi"   },
	{"Mahjong Ron",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_RON, "mah ron"   },
	{"Mahjong Pon",      BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_PON, "mah pon"   },
	{"Mahjong Last",     BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_LAST,"mah last"  },
	{"Mahjong Double",   BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_DOUBLE,"mah double"},
	{"Clear NVRAM",      BIT_DIGITAL,   DrvInputPort + M027_IN_CLEAR_NVRAM, "service4"},
};

STDINPUTINFO(M027Tct2p)

static struct BurnInputInfo M027SlotInputList[] = {
	{"Reset",            BIT_DIGITAL,   &DrvReset,                      "reset"     },
	{"Dip A",            BIT_DIPSWITCH, DrvDips + 0,                   "dip"       },
	{"Dip B",            BIT_DIPSWITCH, DrvDips + 1,                   "dip"       },
	{"Dip C",            BIT_DIPSWITCH, DrvDips + 2,                   "dip"       },
	{"Coin 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN1,  "p1 coin"   },
	{"Coin 2",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN2,  "p2 coin"   },
	{"Start",            BIT_DIGITAL,   DrvInputPort + M027_IN_START1, "p1 start"  },
	{"Bet",              BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BET, "p1 fire 1" },
	{"Stop All",         BIT_DIGITAL,   DrvInputPort + M027_IN_STOPALL,"p1 fire 2" },
	{"Stop 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP1,  "p1 fire 3" },
	{"Stop 2",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP2,  "p1 fire 4" },
	{"Stop 3",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP3,  "p1 fire 5" },
	{"Stop 4",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP4,  "p1 fire 6" },
	{"Stop 5",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP5,  "p1 fire 7" },
	{"Payout",           BIT_DIGITAL,   DrvInputPort + M027_IN_PAYOUT, "service1"  },
	{"Key In",           BIT_DIGITAL,   DrvInputPort + M027_IN_KEYIN,  "service2"  },
	{"Key Out",          BIT_DIGITAL,   DrvInputPort + M027_IN_KEYOUT, "service3"  },
	{"Call Attendant",   BIT_DIGITAL,   DrvInputPort + M027_IN_ATTENDANT, "service4"},
	{"Ticket",           BIT_DIGITAL,   DrvInputPort + M027_IN_TICKET, "service5"  },
	{"Service",          BIT_DIGITAL,   DrvInputPort + M027_IN_SERVICE,"service"   },
	{"Book",             BIT_DIGITAL,   DrvInputPort + M027_IN_BOOK,   "diag"      },
	{"Clear NVRAM",      BIT_DIGITAL,   DrvInputPort + M027_IN_CLEAR_NVRAM, "service6"},
};

STDINPUTINFO(M027Slot)

static struct BurnInputInfo M027CclyInputList[] = {
	{"Reset",            BIT_DIGITAL,   &DrvReset,                      "reset"     },
	{"Dip A",            BIT_DIPSWITCH, DrvDips + 0,                   "dip"       },
	{"Dip B",            BIT_DIPSWITCH, DrvDips + 1,                   "dip"       },
	{"Dip C",            BIT_DIPSWITCH, DrvDips + 2,                   "dip"       },
	{"Coin 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN1,  "p1 coin"   },
	{"Coin 2",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN2,  "p2 coin"   },
	{"Start",            BIT_DIGITAL,   DrvInputPort + M027_IN_START1, "p1 start"  },
	{"Bet",              BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BET, "p1 fire 1" },
	{"Stop All",         BIT_DIGITAL,   DrvInputPort + M027_IN_STOPALL,"p1 fire 2" },
	{"Stop 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP1,  "p1 fire 3" },
	{"Stop 2",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP2,  "p1 fire 4" },
	{"Stop 3",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP3,  "p1 fire 5" },
	{"Big",              BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BIG, "p1 up"     },
	{"Small",            BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SMALL,"p1 down"  },
	{"Take Score",       BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SCORE,"p1 left"  },
	{"Double Up",        BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_DOUBLE,"p1 right"},
	{"Payout",           BIT_DIGITAL,   DrvInputPort + M027_IN_PAYOUT, "service1"  },
	{"Key In",           BIT_DIGITAL,   DrvInputPort + M027_IN_KEYIN,  "service2"  },
	{"Key Out",          BIT_DIGITAL,   DrvInputPort + M027_IN_KEYOUT, "service3"  },
	{"Service",          BIT_DIGITAL,   DrvInputPort + M027_IN_SERVICE,"service"   },
	{"Book",             BIT_DIGITAL,   DrvInputPort + M027_IN_BOOK,   "diag"      },
	{"Clear NVRAM",      BIT_DIGITAL,   DrvInputPort + M027_IN_CLEAR_NVRAM, "service4"},
};

STDINPUTINFO(M027Ccly)

static struct BurnInputInfo M027PokerInputList[] = {
	{"Reset",            BIT_DIGITAL,   &DrvReset,                      "reset"     },
	{"Dip A",            BIT_DIPSWITCH, DrvDips + 0,                   "dip"       },
	{"Dip B",            BIT_DIPSWITCH, DrvDips + 1,                   "dip"       },
	{"Dip C",            BIT_DIPSWITCH, DrvDips + 2,                   "dip"       },
	{"Coin 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_COIN1,  "p1 coin"   },
	{"Start",            BIT_DIGITAL,   DrvInputPort + M027_IN_START1, "p1 start"  },
	{"Bet",              BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BET, "p1 fire 1" },
	{"Hold 1",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP1,  "p1 fire 2" },
	{"Hold 2",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP2,  "p1 fire 3" },
	{"Hold 3",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP3,  "p1 fire 4" },
	{"Hold 4",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP4,  "p1 fire 5" },
	{"Hold 5",           BIT_DIGITAL,   DrvInputPort + M027_IN_STOP5,  "p1 fire 6" },
	{"Payout",           BIT_DIGITAL,   DrvInputPort + M027_IN_PAYOUT, "service1"  },
	{"Key In",           BIT_DIGITAL,   DrvInputPort + M027_IN_KEYIN,  "service2"  },
	{"Key Out",          BIT_DIGITAL,   DrvInputPort + M027_IN_KEYOUT, "service3"  },
	{"High",             BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_BIG, "p1 up"     },
	{"Low",              BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_SMALL,"p1 down"   },
	{"Double",           BIT_DIGITAL,   DrvInputPort + M027_IN_MJ_DOUBLE,"p1 left"  },
	{"Service",          BIT_DIGITAL,   DrvInputPort + M027_IN_SERVICE,"service"   },
	{"Book",             BIT_DIGITAL,   DrvInputPort + M027_IN_BOOK,   "diag"      },
	{"Clear NVRAM",      BIT_DIGITAL,   DrvInputPort + M027_IN_CLEAR_NVRAM, "service4"},
};

STDINPUTINFO(M027Poker)

#define DIP_UNKNOWN_LOC(offset, mask, label) \
	{0   , 0xfe, 0   ,    2, label}, \
	{offset, 0x01, mask, mask, "Off"}, \
	{offset, 0x01, mask, 0x00, "On"}

static struct BurnDIPInfo CjddzDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x01, 0x00, "Off"},
	{0x01, 0x01, 0x01, 0x01, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x01, 0x01, 0x02, 0x02, "Joystick"},
	{0x01, 0x01, 0x02, 0x00, "Mahjong"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x01, 0x01, 0x04, 0x00, "Off"},
	{0x01, 0x01, 0x04, 0x04, "On"},
	{0   , 0xfe, 0   ,    2, "Siren Sound"},
	{0x01, 0x01, 0x08, 0x00, "Off"},
	{0x01, 0x01, 0x08, 0x08, "On"},
	{0   , 0xfe, 0   ,    2, "Auto Pass"},
	{0x01, 0x01, 0x10, 0x00, "Off"},
	{0x01, 0x01, 0x10, 0x10, "On"},
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	DIP_UNKNOWN_LOC(0x02, 0x01, "SW2:1"),
	DIP_UNKNOWN_LOC(0x02, 0x02, "SW2:2"),
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Cjddz)

static struct BurnDIPInfo Lhzb4DIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x01, 0x01, 0x01, 0x01, "Joystick"},
	{0x01, 0x01, 0x01, 0x00, "Mahjong"},
	DIP_UNKNOWN_LOC(0x01, 0x02, "SW1:2"),
	DIP_UNKNOWN_LOC(0x01, 0x04, "SW1:3"),
	DIP_UNKNOWN_LOC(0x01, 0x08, "SW1:4"),
	DIP_UNKNOWN_LOC(0x01, 0x10, "SW1:5"),
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	DIP_UNKNOWN_LOC(0x02, 0x01, "SW2:1"),
	DIP_UNKNOWN_LOC(0x02, 0x02, "SW2:2"),
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Lhzb4)

static struct BurnDIPInfo CjddzpDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,   32, "Satellite Machine No."},
	{0x01, 0x01, 0x1f, 0x1f, "1"},
	{0x01, 0x01, 0x1f, 0x1e, "2"},
	{0x01, 0x01, 0x1f, 0x1d, "3"},
	{0x01, 0x01, 0x1f, 0x1c, "4"},
	{0x01, 0x01, 0x1f, 0x1b, "5"},
	{0x01, 0x01, 0x1f, 0x1a, "6"},
	{0x01, 0x01, 0x1f, 0x19, "7"},
	{0x01, 0x01, 0x1f, 0x18, "8"},
	{0x01, 0x01, 0x1f, 0x17, "9"},
	{0x01, 0x01, 0x1f, 0x16, "10"},
	{0x01, 0x01, 0x1f, 0x15, "11"},
	{0x01, 0x01, 0x1f, 0x14, "12"},
	{0x01, 0x01, 0x1f, 0x13, "13"},
	{0x01, 0x01, 0x1f, 0x12, "14"},
	{0x01, 0x01, 0x1f, 0x11, "15"},
	{0x01, 0x01, 0x1f, 0x10, "16"},
	{0x01, 0x01, 0x1f, 0x0f, "17"},
	{0x01, 0x01, 0x1f, 0x0e, "18"},
	{0x01, 0x01, 0x1f, 0x0d, "19"},
	{0x01, 0x01, 0x1f, 0x0c, "20"},
	{0x01, 0x01, 0x1f, 0x0b, "20 (dup 1)"},
	{0x01, 0x01, 0x1f, 0x0a, "20 (dup 2)"},
	{0x01, 0x01, 0x1f, 0x09, "20 (dup 3)"},
	{0x01, 0x01, 0x1f, 0x08, "20 (dup 4)"},
	{0x01, 0x01, 0x1f, 0x07, "20 (dup 5)"},
	{0x01, 0x01, 0x1f, 0x06, "20 (dup 6)"},
	{0x01, 0x01, 0x1f, 0x05, "20 (dup 7)"},
	{0x01, 0x01, 0x1f, 0x04, "20 (dup 8)"},
	{0x01, 0x01, 0x1f, 0x03, "20 (dup 9)"},
	{0x01, 0x01, 0x1f, 0x02, "20 (dup 10)"},
	{0x01, 0x01, 0x1f, 0x01, "20 (dup 11)"},
	{0x01, 0x01, 0x1f, 0x00, "20 (dup 12)"},
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	{0   , 0xfe, 0   ,    2, "Single Machine"},
	{0x01, 0x01, 0x80, 0x80, "On"},
	{0x01, 0x01, 0x80, 0x00, "Off"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x02, 0x01, 0x01, 0x00, "Off"},
	{0x02, 0x01, 0x01, 0x01, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x02, 0x02, "Joystick"},
	{0x02, 0x01, 0x02, 0x00, "Mahjong"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x02, 0x01, 0x04, 0x00, "Off"},
	{0x02, 0x01, 0x04, 0x04, "On"},
	{0   , 0xfe, 0   ,    2, "Siren Sound"},
	{0x02, 0x01, 0x08, 0x00, "Off"},
	{0x02, 0x01, 0x08, 0x08, "On"},
	{0   , 0xfe, 0   ,    2, "Auto Pass"},
	{0x02, 0x01, 0x10, 0x00, "Off"},
	{0x02, 0x01, 0x10, 0x10, "On"},
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Cjddzp)

static struct BurnDIPInfo Tct2pDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xfb, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,   32, "Satellite Machine No."},
	{0x01, 0x01, 0x1f, 0x1f, "1"},
	{0x01, 0x01, 0x1f, 0x1e, "2"},
	{0x01, 0x01, 0x1f, 0x1d, "3"},
	{0x01, 0x01, 0x1f, 0x1c, "4"},
	{0x01, 0x01, 0x1f, 0x1b, "5"},
	{0x01, 0x01, 0x1f, 0x1a, "6"},
	{0x01, 0x01, 0x1f, 0x19, "7"},
	{0x01, 0x01, 0x1f, 0x18, "8"},
	{0x01, 0x01, 0x1f, 0x17, "9"},
	{0x01, 0x01, 0x1f, 0x16, "10"},
	{0x01, 0x01, 0x1f, 0x15, "11"},
	{0x01, 0x01, 0x1f, 0x14, "12"},
	{0x01, 0x01, 0x1f, 0x13, "13"},
	{0x01, 0x01, 0x1f, 0x12, "14"},
	{0x01, 0x01, 0x1f, 0x11, "15"},
	{0x01, 0x01, 0x1f, 0x10, "16"},
	{0x01, 0x01, 0x1f, 0x0f, "17"},
	{0x01, 0x01, 0x1f, 0x0e, "18"},
	{0x01, 0x01, 0x1f, 0x0d, "19"},
	{0x01, 0x01, 0x1f, 0x0c, "20"},
	{0x01, 0x01, 0x1f, 0x0b, "20 (dup 1)"},
	{0x01, 0x01, 0x1f, 0x0a, "20 (dup 2)"},
	{0x01, 0x01, 0x1f, 0x09, "20 (dup 3)"},
	{0x01, 0x01, 0x1f, 0x08, "20 (dup 4)"},
	{0x01, 0x01, 0x1f, 0x07, "20 (dup 5)"},
	{0x01, 0x01, 0x1f, 0x06, "20 (dup 6)"},
	{0x01, 0x01, 0x1f, 0x05, "20 (dup 7)"},
	{0x01, 0x01, 0x1f, 0x04, "20 (dup 8)"},
	{0x01, 0x01, 0x1f, 0x03, "20 (dup 9)"},
	{0x01, 0x01, 0x1f, 0x02, "20 (dup 10)"},
	{0x01, 0x01, 0x1f, 0x01, "20 (dup 11)"},
	{0x01, 0x01, 0x1f, 0x00, "20 (dup 12)"},
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	{0   , 0xfe, 0   ,    2, "Link Mode"},
	{0x01, 0x01, 0x80, 0x80, "Off"},
	{0x01, 0x01, 0x80, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x01, 0x01, "Joystick"},
	{0x02, 0x01, 0x01, 0x00, "Mahjong"},
	DIP_UNKNOWN_LOC(0x02, 0x02, "SW2:2"),
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Tct2p)

static struct BurnDIPInfo TswxpDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x01, 0x01, 0x01, 0x01, "Joystick"},
	{0x01, 0x01, 0x01, 0x00, "Mahjong"},
	DIP_UNKNOWN_LOC(0x01, 0x02, "SW1:2"),
	DIP_UNKNOWN_LOC(0x01, 0x04, "SW1:3"),
	DIP_UNKNOWN_LOC(0x01, 0x08, "SW1:4"),
	DIP_UNKNOWN_LOC(0x01, 0x10, "SW1:5"),
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	DIP_UNKNOWN_LOC(0x02, 0x01, "SW2:1"),
	DIP_UNKNOWN_LOC(0x02, 0x02, "SW2:2"),
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Tswxp)

static struct BurnDIPInfo QlgsDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xfa, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,   32, "Satellite Machine No."},
	{0x01, 0x01, 0x1f, 0x1f, "1"},
	{0x01, 0x01, 0x1f, 0x1e, "2"},
	{0x01, 0x01, 0x1f, 0x1d, "3"},
	{0x01, 0x01, 0x1f, 0x1c, "4"},
	{0x01, 0x01, 0x1f, 0x1b, "5"},
	{0x01, 0x01, 0x1f, 0x1a, "6"},
	{0x01, 0x01, 0x1f, 0x19, "7"},
	{0x01, 0x01, 0x1f, 0x18, "8"},
	{0x01, 0x01, 0x1f, 0x17, "9"},
	{0x01, 0x01, 0x1f, 0x16, "10"},
	{0x01, 0x01, 0x1f, 0x15, "11"},
	{0x01, 0x01, 0x1f, 0x14, "12"},
	{0x01, 0x01, 0x1f, 0x13, "13"},
	{0x01, 0x01, 0x1f, 0x12, "14"},
	{0x01, 0x01, 0x1f, 0x11, "15"},
	{0x01, 0x01, 0x1f, 0x10, "16"},
	{0x01, 0x01, 0x1f, 0x0f, "17"},
	{0x01, 0x01, 0x1f, 0x0e, "18"},
	{0x01, 0x01, 0x1f, 0x0d, "19"},
	{0x01, 0x01, 0x1f, 0x0c, "20"},
	{0x01, 0x01, 0x1f, 0x0b, "20 (dup 1)"},
	{0x01, 0x01, 0x1f, 0x0a, "20 (dup 2)"},
	{0x01, 0x01, 0x1f, 0x09, "20 (dup 3)"},
	{0x01, 0x01, 0x1f, 0x08, "20 (dup 4)"},
	{0x01, 0x01, 0x1f, 0x07, "20 (dup 5)"},
	{0x01, 0x01, 0x1f, 0x06, "20 (dup 6)"},
	{0x01, 0x01, 0x1f, 0x05, "20 (dup 7)"},
	{0x01, 0x01, 0x1f, 0x04, "20 (dup 8)"},
	{0x01, 0x01, 0x1f, 0x03, "20 (dup 9)"},
	{0x01, 0x01, 0x1f, 0x02, "20 (dup 10)"},
	{0x01, 0x01, 0x1f, 0x01, "20 (dup 11)"},
	{0x01, 0x01, 0x1f, 0x00, "20 (dup 12)"},
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x01, 0x00, "Joystick"},
	{0x02, 0x01, 0x01, 0x01, "Mahjong"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x02, 0x01, 0x02, 0x00, "Off"},
	{0x02, 0x01, 0x02, 0x02, "On"},
	{0   , 0xfe, 0   ,    2, "Link Mode"},
	{0x02, 0x01, 0x04, 0x00, "Offline Version"},
	{0x02, 0x01, 0x04, 0x04, "Online Version"},
	{0   , 0xfe, 0   ,    2, "Show Title"},
	{0x02, 0x01, 0x08, 0x00, "Off"},
	{0x02, 0x01, 0x08, 0x08, "On"},
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
};

STDDIPINFO(Qlgs)

static struct BurnDIPInfo LthypDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xf6, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,   32, "Satellite Machine No."},
	{0x01, 0x01, 0x1f, 0x1f, "1"},
	{0x01, 0x01, 0x1f, 0x1e, "2"},
	{0x01, 0x01, 0x1f, 0x1d, "3"},
	{0x01, 0x01, 0x1f, 0x1c, "4"},
	{0x01, 0x01, 0x1f, 0x1b, "5"},
	{0x01, 0x01, 0x1f, 0x1a, "6"},
	{0x01, 0x01, 0x1f, 0x19, "7"},
	{0x01, 0x01, 0x1f, 0x18, "8"},
	{0x01, 0x01, 0x1f, 0x17, "9"},
	{0x01, 0x01, 0x1f, 0x16, "10"},
	{0x01, 0x01, 0x1f, 0x15, "11"},
	{0x01, 0x01, 0x1f, 0x14, "12"},
	{0x01, 0x01, 0x1f, 0x13, "13"},
	{0x01, 0x01, 0x1f, 0x12, "14"},
	{0x01, 0x01, 0x1f, 0x11, "15"},
	{0x01, 0x01, 0x1f, 0x10, "16"},
	{0x01, 0x01, 0x1f, 0x0f, "17"},
	{0x01, 0x01, 0x1f, 0x0e, "18"},
	{0x01, 0x01, 0x1f, 0x0d, "19"},
	{0x01, 0x01, 0x1f, 0x0c, "20"},
	{0x01, 0x01, 0x1f, 0x0b, "20 (dup 1)"},
	{0x01, 0x01, 0x1f, 0x0a, "20 (dup 2)"},
	{0x01, 0x01, 0x1f, 0x09, "20 (dup 3)"},
	{0x01, 0x01, 0x1f, 0x08, "20 (dup 4)"},
	{0x01, 0x01, 0x1f, 0x07, "20 (dup 5)"},
	{0x01, 0x01, 0x1f, 0x06, "20 (dup 6)"},
	{0x01, 0x01, 0x1f, 0x05, "20 (dup 7)"},
	{0x01, 0x01, 0x1f, 0x04, "20 (dup 8)"},
	{0x01, 0x01, 0x1f, 0x03, "20 (dup 9)"},
	{0x01, 0x01, 0x1f, 0x02, "20 (dup 10)"},
	{0x01, 0x01, 0x1f, 0x01, "20 (dup 11)"},
	{0x01, 0x01, 0x1f, 0x00, "20 (dup 12)"},
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x01, 0x00, "Joystick"},
	{0x02, 0x01, 0x01, 0x01, "Mahjong"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x02, 0x01, 0x02, 0x00, "Off"},
	{0x02, 0x01, 0x02, 0x02, "On"},
	{0   , 0xfe, 0   ,    2, "Show Title"},
	{0x02, 0x01, 0x04, 0x00, "Off"},
	{0x02, 0x01, 0x04, 0x04, "On"},
	{0   , 0xfe, 0   ,    2, "Link Feature"},
	{0x02, 0x01, 0x08, 0x00, "Off"},
	{0x02, 0x01, 0x08, 0x08, "On"},
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
};

STDDIPINFO(Lthyp)

static struct BurnDIPInfo Mgcs3DIPList[] = {
	{0x01, 0xff, 0xff, 0xfe, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x01, 0x01, "Off"},
	{0x01, 0x01, 0x01, 0x00, "On"},
	DIP_UNKNOWN_LOC(0x01, 0x02, "SW1:2"),
	DIP_UNKNOWN_LOC(0x01, 0x04, "SW1:3"),
	DIP_UNKNOWN_LOC(0x01, 0x08, "SW1:4"),
	DIP_UNKNOWN_LOC(0x01, 0x10, "SW1:5"),
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x01, 0x01, "Joystick"},
	{0x02, 0x01, 0x01, 0x00, "Mahjong"},
	DIP_UNKNOWN_LOC(0x02, 0x02, "SW2:2"),
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Mgcs3)

static struct BurnDIPInfo ZhongguoDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xfe, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x01, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x01, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x01, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x01, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x01, 0x01, 0x0c, 0x0c, "10"},
	{0x01, 0x01, 0x0c, 0x08, "20"},
	{0x01, 0x01, 0x0c, 0x04, "50"},
	{0x01, 0x01, 0x0c, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x01, 0x01, 0x10, 0x10, "1000"},
	{0x01, 0x01, 0x10, 0x00, "2000"},
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x01, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x01, 0x01, 0x20, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x01, 0x01, 0x40, 0x40, "Return Coins"},
	{0x01, 0x01, 0x40, 0x00, "Key-Out"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x80, 0x00, "Off"},
	{0x01, 0x01, 0x80, 0x80, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x01, 0x01, "Mahjong"},
	{0x02, 0x01, 0x01, 0x00, "Joystick"},
	{0   , 0xfe, 0   ,    4, "Card Display"},
	{0x02, 0x01, 0x06, 0x06, "Small Cards"},
	{0x02, 0x01, 0x06, 0x04, "Cards"},
	{0x02, 0x01, 0x06, 0x02, "Alternate"},
	{0x02, 0x01, 0x06, 0x00, "Small Cards (dup)"},
	{0   , 0xfe, 0   ,    2, "Double Up Jackpot"},
	{0x02, 0x01, 0x08, 0x08, "1000"},
	{0x02, 0x01, 0x08, 0x00, "2000"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x02, 0x01, 0x10, 0x00, "Off"},
	{0x02, 0x01, 0x10, 0x10, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x02, 0x01, 0x20, 0x20, "Double Up"},
	{0x02, 0x01, 0x20, 0x00, "Continue Play"},
	{0   , 0xfe, 0   ,    2, "Credit Display"},
	{0x02, 0x01, 0x40, 0x40, "Numbers"},
	{0x02, 0x01, 0x40, 0x00, "Circle Tiles"},
	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x02, 0x01, 0x80, 0x80, "Off"},
	{0x02, 0x01, 0x80, 0x00, "On"},
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Zhongguo)

static struct BurnDIPInfo Jking02DIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xf1, NULL},
	{0x03, 0xff, 0xff, 0xe7, NULL},
	{0   , 0xfe, 0   ,   32, "ID Number"},
	{0x01, 0x01, 0x1f, 0x1f, "1"},
	{0x01, 0x01, 0x1f, 0x1e, "2"},
	{0x01, 0x01, 0x1f, 0x1d, "3"},
	{0x01, 0x01, 0x1f, 0x1c, "4"},
	{0x01, 0x01, 0x1f, 0x1b, "5"},
	{0x01, 0x01, 0x1f, 0x1a, "6"},
	{0x01, 0x01, 0x1f, 0x19, "7"},
	{0x01, 0x01, 0x1f, 0x18, "8"},
	{0x01, 0x01, 0x1f, 0x17, "9"},
	{0x01, 0x01, 0x1f, 0x16, "10"},
	{0x01, 0x01, 0x1f, 0x15, "11"},
	{0x01, 0x01, 0x1f, 0x14, "12"},
	{0x01, 0x01, 0x1f, 0x13, "13"},
	{0x01, 0x01, 0x1f, 0x12, "14"},
	{0x01, 0x01, 0x1f, 0x11, "15"},
	{0x01, 0x01, 0x1f, 0x10, "16"},
	{0x01, 0x01, 0x1f, 0x0f, "17"},
	{0x01, 0x01, 0x1f, 0x0e, "18"},
	{0x01, 0x01, 0x1f, 0x0d, "19"},
	{0x01, 0x01, 0x1f, 0x0c, "20"},
	{0x01, 0x01, 0x1f, 0x0b, "20 (dup 1)"},
	{0x01, 0x01, 0x1f, 0x0a, "20 (dup 2)"},
	{0x01, 0x01, 0x1f, 0x09, "20 (dup 3)"},
	{0x01, 0x01, 0x1f, 0x08, "20 (dup 4)"},
	{0x01, 0x01, 0x1f, 0x07, "20 (dup 5)"},
	{0x01, 0x01, 0x1f, 0x06, "20 (dup 6)"},
	{0x01, 0x01, 0x1f, 0x05, "20 (dup 7)"},
	{0x01, 0x01, 0x1f, 0x04, "20 (dup 8)"},
	{0x01, 0x01, 0x1f, 0x03, "20 (dup 9)"},
	{0x01, 0x01, 0x1f, 0x02, "20 (dup 10)"},
	{0x01, 0x01, 0x1f, 0x01, "20 (dup 11)"},
	{0x01, 0x01, 0x1f, 0x00, "20 (dup 12)"},
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	{0   , 0xfe, 0   ,    2, "PC Board Mode"},
	{0x01, 0x01, 0x80, 0x80, "Single"},
	{0x01, 0x01, 0x80, 0x00, "Linking"},
	{0   , 0xfe, 0   ,    2, "Wiring Diagram"},
	{0x02, 0x01, 0x01, 0x01, "36+10"},
	{0x02, 0x01, 0x01, 0x00, "28-pin"},
	{0   , 0xfe, 0   ,    2, "Odds Table"},
	{0x02, 0x01, 0x02, 0x02, "No"},
	{0x02, 0x01, 0x02, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x02, 0x01, 0x04, 0x04, "Off"},
	{0x02, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Back Color"},
	{0x02, 0x01, 0x08, 0x08, "Black"},
	{0x02, 0x01, 0x08, 0x00, "Color"},
	{0   , 0xfe, 0   ,    2, "Password"},
	{0x02, 0x01, 0x10, 0x10, "No"},
	{0x02, 0x01, 0x10, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x02, 0x01, 0x20, 0x20, "Off"},
	{0x02, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Auto Stop"},
	{0x02, 0x01, 0x40, 0x40, "No"},
	{0x02, 0x01, 0x40, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x02, 0x01, 0x80, 0x80, "Normal"},
	{0x02, 0x01, 0x80, 0x00, "Auto"},
	{0   , 0xfe, 0   ,    4, "Score Box"},
	{0x03, 0x01, 0x03, 0x03, "Off"},
	{0x03, 0x01, 0x03, 0x02, "On"},
	{0x03, 0x01, 0x03, 0x01, "10 Times"},
	{0x03, 0x01, 0x03, 0x00, "10 Times (dup)"},
	{0   , 0xfe, 0   ,    2, "Play Score"},
	{0x03, 0x01, 0x04, 0x04, "Off"},
	{0x03, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Show Title"},
	{0x03, 0x01, 0x08, 0x08, "Off"},
	{0x03, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    4, "Symbols"},
	{0x03, 0x01, 0x30, 0x30, "Fruit"},
	{0x03, 0x01, 0x30, 0x20, "Legend"},
	{0x03, 0x01, 0x30, 0x10, "Both"},
	{0x03, 0x01, 0x30, 0x00, "Both (dup)"},
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Jking02)

static struct BurnDIPInfo CjsxpDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Operation Mode"},
	{0x01, 0x01, 0x01, 0x01, "Amusement"},
	{0x01, 0x01, 0x01, 0x00, "Poker"},
	{0   , 0xfe, 0   ,    2, "Playing Card Display"},
	{0x01, 0x01, 0x02, 0x02, "Soccer Balls / Playing Cards"},
	{0x01, 0x01, 0x02, 0x00, "Soccer Jerseys / Cubes"},
	{0   , 0xfe, 0   ,    2, "Game Title"},
	{0x01, 0x01, 0x04, 0x04, "Huangpai Zuqiu Plus"},
	{0x01, 0x01, 0x04, 0x00, "Chaoji Shuangxing Plus"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x01, 0x01, 0x08, 0x00, "Off"},
	{0x01, 0x01, 0x08, 0x08, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x01, 0x01, 0x10, 0x10, "Double Up"},
	{0x01, 0x01, 0x10, 0x00, "Continue Play"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x20, 0x00, "Off"},
	{0x01, 0x01, 0x20, 0x20, "On"},
	{0   , 0xfe, 0   ,    2, "Face Card Display"},
	{0x01, 0x01, 0x40, 0x40, "Numbers"},
	{0x01, 0x01, 0x40, 0x00, "Letters"},
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x02, 0x01, 0x01, 0x01, "Coin Acceptor"},
	{0x02, 0x01, 0x01, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x02, 0x01, 0x02, 0x02, "Return Coins"},
	{0x02, 0x01, 0x02, 0x00, "Key-Out"},
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Cjsxp)

static struct BurnDIPInfo OceanparDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x01, 0x00, "Off"},
	{0x01, 0x01, 0x01, 0x01, "On"},
	{0   , 0xfe, 0   ,    2, "Non-Stop"},
	{0x01, 0x01, 0x02, 0x02, "No"},
	{0x01, 0x01, 0x02, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "Record Password"},
	{0x01, 0x01, 0x04, 0x00, "No"},
	{0x01, 0x01, 0x04, 0x04, "Yes"},
	{0   , 0xfe, 0   ,    2, "Odds Table"},
	{0x01, 0x01, 0x08, 0x00, "No"},
	{0x01, 0x01, 0x08, 0x08, "Yes"},
	{0   , 0xfe, 0   ,    2, "Auto Take"},
	{0x01, 0x01, 0x10, 0x10, "No"},
	{0x01, 0x01, 0x10, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x01, 0x01, 0x20, 0x00, "Off"},
	{0x01, 0x01, 0x20, 0x20, "On"},
	{0   , 0xfe, 0   ,    4, "Double Up Game Type"},
	{0x01, 0x01, 0xc0, 0xc0, "Poker 1"},
	{0x01, 0x01, 0xc0, 0x80, "Poker 2"},
	{0x01, 0x01, 0xc0, 0x40, "Symbol"},
	{0x01, 0x01, 0xc0, 0x00, "Symbol (dup)"},
	{0   , 0xfe, 0   ,    2, "Chance Level"},
	{0x02, 0x01, 0x01, 0x01, "Low"},
	{0x02, 0x01, 0x01, 0x00, "High"},
	DIP_UNKNOWN_LOC(0x02, 0x02, "SW2:2"),
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Oceanpar)

static struct BurnDIPInfo Oceanpar105DIPList[] = {
	{0   , 0xfe, 0   ,    4, "Score Box"},
	{0x02, 0x01, 0x06, 0x06, "Off"},
	{0x02, 0x01, 0x06, 0x04, "On"},
	{0x02, 0x01, 0x06, 0x02, "10 Times"},
	{0x02, 0x01, 0x06, 0x00, "10 Times (dup)"},
	{0   , 0xfe, 0   ,    2, "Play Score"},
	{0x02, 0x01, 0x08, 0x08, "No"},
	{0x02, 0x01, 0x08, 0x00, "Yes"},
};

STDDIPINFOEXT(Oceanpar105, Oceanpar, Oceanpar105)

static struct BurnDIPInfo Fruitpar206DIPList[] = {
	{0   , 0xfe, 0   ,    2, "Score Box"},
	{0x02, 0x01, 0x02, 0x02, "Off"},
	{0x02, 0x01, 0x02, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Play Score"},
	{0x02, 0x01, 0x04, 0x04, "No"},
	{0x02, 0x01, 0x04, 0x00, "Yes"},
};

STDDIPINFOEXT(Fruitpar206, Oceanpar, Fruitpar206)

static struct BurnDIPInfo MgzzDIPList[] = {
	{0x01, 0xff, 0xff, 0xfd, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x01, 0x01, 0x01, 0x00, "Off"},
	{0x01, 0x01, 0x01, 0x01, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x01, 0x01, 0x02, 0x02, "Mahjong"},
	{0x01, 0x01, 0x02, 0x00, "Joystick"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x04, 0x00, "Off"},
	{0x01, 0x01, 0x04, 0x04, "On"},
	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x01, 0x01, 0x18, 0x00, "1 Coin 2 Credits"},
	{0x01, 0x01, 0x18, 0x08, "1 Coin 2 Credits (dup)"},
	{0x01, 0x01, 0x18, 0x10, "1 Coin 5 Credits"},
	{0x01, 0x01, 0x18, 0x18, "1 Coin 10 Credits"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x01, 0x01, 0x60, 0x60, "50"},
	{0x01, 0x01, 0x60, 0x40, "100"},
	{0x01, 0x01, 0x60, 0x20, "200"},
	{0x01, 0x01, 0x60, 0x00, "500"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x01, 0x01, 0x80, 0x80, "5000"},
	{0x01, 0x01, 0x80, 0x00, "10000"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x02, 0x01, 0x03, 0x03, "10"},
	{0x02, 0x01, 0x03, 0x02, "20"},
	{0x02, 0x01, 0x03, 0x01, "30"},
	{0x02, 0x01, 0x03, 0x00, "50"},
	{0   , 0xfe, 0   ,    2, "Initial Double Up"},
	{0x02, 0x01, 0x04, 0x04, "Off"},
	{0x02, 0x01, 0x04, 0x00, "On"},
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
};

STDDIPINFO(Mgzz)

static struct BurnDIPInfo CjtljpDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x01, 0x01, 0x01, 0x01, "Joystick"},
	{0x01, 0x01, 0x01, 0x00, "Mahjong"},
};

STDDIPINFO(Cjtljp)

static struct BurnDIPInfo CclyDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Operation Mode"},
	{0x02, 0x01, 0x01, 0x01, "Amusement"},
	{0x02, 0x01, 0x01, 0x00, "Fruit Machine"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x02, 0x01, 0x02, 0x00, "Off"},
	{0x02, 0x01, 0x02, 0x02, "On"},
	DIP_UNKNOWN_LOC(0x01, 0x01, "SW1:1"),
	DIP_UNKNOWN_LOC(0x01, 0x02, "SW1:2"),
	DIP_UNKNOWN_LOC(0x01, 0x04, "SW1:3"),
	DIP_UNKNOWN_LOC(0x01, 0x08, "SW1:4"),
	DIP_UNKNOWN_LOC(0x01, 0x10, "SW1:5"),
	DIP_UNKNOWN_LOC(0x01, 0x20, "SW1:6"),
	DIP_UNKNOWN_LOC(0x01, 0x40, "SW1:7"),
	DIP_UNKNOWN_LOC(0x01, 0x80, "SW1:8"),
	DIP_UNKNOWN_LOC(0x02, 0x04, "SW2:3"),
	DIP_UNKNOWN_LOC(0x02, 0x08, "SW2:4"),
	DIP_UNKNOWN_LOC(0x02, 0x10, "SW2:5"),
	DIP_UNKNOWN_LOC(0x02, 0x20, "SW2:6"),
	DIP_UNKNOWN_LOC(0x02, 0x40, "SW2:7"),
	DIP_UNKNOWN_LOC(0x02, 0x80, "SW2:8"),
	DIP_UNKNOWN_LOC(0x03, 0x01, "SW3:1"),
	DIP_UNKNOWN_LOC(0x03, 0x02, "SW3:2"),
	DIP_UNKNOWN_LOC(0x03, 0x04, "SW3:3"),
	DIP_UNKNOWN_LOC(0x03, 0x08, "SW3:4"),
	DIP_UNKNOWN_LOC(0x03, 0x10, "SW3:5"),
	DIP_UNKNOWN_LOC(0x03, 0x20, "SW3:6"),
	DIP_UNKNOWN_LOC(0x03, 0x40, "SW3:7"),
	DIP_UNKNOWN_LOC(0x03, 0x80, "SW3:8"),
};

STDDIPINFO(Ccly)

static struct BurnDIPInfo Slqz3DIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x01, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x01, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x01, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x01, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x01, 0x01, 0x0c, 0x0c, "10"},
	{0x01, 0x01, 0x0c, 0x08, "20"},
	{0x01, 0x01, 0x0c, 0x04, "50"},
	{0x01, 0x01, 0x0c, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x01, 0x01, 0x10, 0x10, "1000"},
	{0x01, 0x01, 0x10, 0x00, "2000"},
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x01, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x01, 0x01, 0x20, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x01, 0x01, 0x40, 0x40, "Return Coins"},
	{0x01, 0x01, 0x40, 0x00, "Key-Out"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x80, 0x00, "Off"},
	{0x01, 0x01, 0x80, 0x80, "On"},
	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x02, 0x01, 0x03, 0x03, "500"},
	{0x02, 0x01, 0x03, 0x02, "1000"},
	{0x02, 0x01, 0x03, 0x01, "1500"},
	{0x02, 0x01, 0x03, 0x00, "2000"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x02, 0x01, 0x0c, 0x0c, "1"},
	{0x02, 0x01, 0x0c, 0x08, "1"},
	{0x02, 0x01, 0x0c, 0x04, "1"},
	{0x02, 0x01, 0x0c, 0x00, "1"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x02, 0x01, 0x10, 0x00, "Off"},
	{0x02, 0x01, 0x10, 0x10, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x02, 0x01, 0x20, 0x20, "Double Up"},
	{0x02, 0x01, 0x20, 0x00, "Continue Play"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x02, 0x01, 0x40, 0x40, "Numbers"},
	{0x02, 0x01, 0x40, 0x00, "Blocks"},
	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x02, 0x01, 0x80, 0x80, "Off"},
	{0x02, 0x01, 0x80, 0x00, "On"},
};

STDDIPINFO(Slqz3)

static struct BurnDIPInfo Lhzb3DIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x01, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x01, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x01, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x01, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x01, 0x01, 0x0c, 0x0c, "10"},
	{0x01, 0x01, 0x0c, 0x08, "20"},
	{0x01, 0x01, 0x0c, 0x04, "50"},
	{0x01, 0x01, 0x0c, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x01, 0x01, 0x10, 0x10, "1000"},
	{0x01, 0x01, 0x10, 0x00, "2000"},
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x01, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x01, 0x01, 0x20, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x01, 0x01, 0x40, 0x40, "Return Coins"},
	{0x01, 0x01, 0x40, 0x00, "Key-Out"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x80, 0x00, "Off"},
	{0x01, 0x01, 0x80, 0x80, "On"},
	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x02, 0x01, 0x03, 0x03, "500"},
	{0x02, 0x01, 0x03, 0x02, "1000"},
	{0x02, 0x01, 0x03, 0x01, "1500"},
	{0x02, 0x01, 0x03, 0x00, "2000"},
	{0   , 0xfe, 0   ,    2, "Show Title"},
	{0x02, 0x01, 0x04, 0x00, "Off"},
	{0x02, 0x01, 0x04, 0x04, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:4"},
	{0x02, 0x01, 0x08, 0x08, "Off"},
	{0x02, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x02, 0x01, 0x10, 0x00, "Off"},
	{0x02, 0x01, 0x10, 0x10, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x02, 0x01, 0x20, 0x20, "Double Up"},
	{0x02, 0x01, 0x20, 0x00, "Continue Play"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x02, 0x01, 0x40, 0x40, "Numbers"},
	{0x02, 0x01, 0x40, 0x00, "Blocks"},
	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x02, 0x01, 0x80, 0x80, "Off"},
	{0x02, 0x01, 0x80, 0x00, "On"},
};

STDDIPINFO(Lhzb3)

static struct BurnDIPInfo Lhzb3sjbDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xf7, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x01, 0x01, 0x03, 0x03, "1 Coin 1 Credit"},
	{0x01, 0x01, 0x03, 0x02, "1 Coin 2 Credits"},
	{0x01, 0x01, 0x03, 0x01, "1 Coin 3 Credits"},
	{0x01, 0x01, 0x03, 0x00, "1 Coin 5 Credits"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x01, 0x01, 0x0c, 0x0c, "10"},
	{0x01, 0x01, 0x0c, 0x08, "20"},
	{0x01, 0x01, 0x0c, 0x04, "50"},
	{0x01, 0x01, 0x0c, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x01, 0x01, 0x10, 0x10, "1000"},
	{0x01, 0x01, 0x10, 0x00, "2000"},
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x01, 0x01, 0x20, 0x20, "Coin Acceptor"},
	{0x01, 0x01, 0x20, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x01, 0x01, 0x40, 0x40, "Return Coins"},
	{0x01, 0x01, 0x40, 0x00, "Key-Out"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x80, 0x00, "Off"},
	{0x01, 0x01, 0x80, 0x80, "On"},
	{0   , 0xfe, 0   ,    4, "Double Up Jackpot"},
	{0x02, 0x01, 0x03, 0x03, "500"},
	{0x02, 0x01, 0x03, 0x02, "1000"},
	{0x02, 0x01, 0x03, 0x01, "1500"},
	{0x02, 0x01, 0x03, 0x00, "2000"},
	{0   , 0xfe, 0   ,    2, "Show Title"},
	{0x02, 0x01, 0x04, 0x00, "Off"},
	{0x02, 0x01, 0x04, 0x04, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x08, 0x08, "Mahjong"},
	{0x02, 0x01, 0x08, 0x00, "Joystick"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x02, 0x01, 0x10, 0x00, "Off"},
	{0x02, 0x01, 0x10, 0x10, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x02, 0x01, 0x20, 0x20, "Double Up"},
	{0x02, 0x01, 0x20, 0x00, "Continue Play"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x02, 0x01, 0x40, 0x40, "Numbers"},
	{0x02, 0x01, 0x40, 0x00, "Blocks"},
	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x02, 0x01, 0x80, 0x80, "Off"},
	{0x02, 0x01, 0x80, 0x00, "On"},
};

STDDIPINFO(Lhzb3sjb)

static struct BurnDIPInfo KlxyjDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,   32, "Satellite Machine No."},
	{0x01, 0x01, 0x1f, 0x1f, "1"},
	{0x01, 0x01, 0x1f, 0x1e, "2"},
	{0x01, 0x01, 0x1f, 0x1d, "3"},
	{0x01, 0x01, 0x1f, 0x1c, "4"},
	{0x01, 0x01, 0x1f, 0x1b, "5"},
	{0x01, 0x01, 0x1f, 0x1a, "6"},
	{0x01, 0x01, 0x1f, 0x19, "7"},
	{0x01, 0x01, 0x1f, 0x18, "8"},
	{0x01, 0x01, 0x1f, 0x17, "9"},
	{0x01, 0x01, 0x1f, 0x16, "10"},
	{0x01, 0x01, 0x1f, 0x15, "11"},
	{0x01, 0x01, 0x1f, 0x14, "12"},
	{0x01, 0x01, 0x1f, 0x13, "13"},
	{0x01, 0x01, 0x1f, 0x12, "14"},
	{0x01, 0x01, 0x1f, 0x11, "15"},
	{0x01, 0x01, 0x1f, 0x10, "16"},
	{0x01, 0x01, 0x1f, 0x0f, "17"},
	{0x01, 0x01, 0x1f, 0x0e, "18"},
	{0x01, 0x01, 0x1f, 0x0d, "19"},
	{0x01, 0x01, 0x1f, 0x0c, "20"},
	{0x01, 0x01, 0x1f, 0x0b, "20 (dup 1)"},
	{0x01, 0x01, 0x1f, 0x0a, "20 (dup 2)"},
	{0x01, 0x01, 0x1f, 0x09, "20 (dup 3)"},
	{0x01, 0x01, 0x1f, 0x08, "20 (dup 4)"},
	{0x01, 0x01, 0x1f, 0x07, "20 (dup 5)"},
	{0x01, 0x01, 0x1f, 0x06, "20 (dup 6)"},
	{0x01, 0x01, 0x1f, 0x05, "20 (dup 7)"},
	{0x01, 0x01, 0x1f, 0x04, "20 (dup 8)"},
	{0x01, 0x01, 0x1f, 0x03, "20 (dup 9)"},
	{0x01, 0x01, 0x1f, 0x02, "20 (dup 10)"},
	{0x01, 0x01, 0x1f, 0x01, "20 (dup 11)"},
	{0x01, 0x01, 0x1f, 0x00, "20 (dup 12)"},
	{0   , 0xfe, 0   ,    2, "SW1:6"},
	{0x01, 0x01, 0x20, 0x20, "Off"},
	{0x01, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW1:7"},
	{0x01, 0x01, 0x40, 0x40, "Off"},
	{0x01, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Link Mode"},
	{0x01, 0x01, 0x80, 0x80, "Off"},
	{0x01, 0x01, 0x80, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x01, 0x01, "Joystick"},
	{0x02, 0x01, 0x01, 0x00, "Mahjong"},
	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x02, 0x01, 0x02, 0x02, "Off"},
	{0x02, 0x01, 0x02, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x02, 0x01, 0x04, 0x04, "Numbers"},
	{0x02, 0x01, 0x04, 0x00, "Circle Tiles"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x02, 0x01, 0x08, 0x08, "Off"},
	{0x02, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Background Color Mode"},
	{0x02, 0x01, 0x10, 0x10, "Monochrome"},
	{0x02, 0x01, 0x10, 0x00, "Color"},
	{0   , 0xfe, 0   ,    2, "SW2:6"},
	{0x02, 0x01, 0x20, 0x20, "Off"},
	{0x02, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:7"},
	{0x02, 0x01, 0x40, 0x40, "Off"},
	{0x02, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:8"},
	{0x02, 0x01, 0x80, 0x80, "Off"},
	{0x02, 0x01, 0x80, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:1"},
	{0x03, 0x01, 0x01, 0x01, "Off"},
	{0x03, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:2"},
	{0x03, 0x01, 0x02, 0x02, "Off"},
	{0x03, 0x01, 0x02, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:3"},
	{0x03, 0x01, 0x04, 0x04, "Off"},
	{0x03, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:4"},
	{0x03, 0x01, 0x08, 0x08, "Off"},
	{0x03, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:5"},
	{0x03, 0x01, 0x10, 0x10, "Off"},
	{0x03, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:6"},
	{0x03, 0x01, 0x20, 0x20, "Off"},
	{0x03, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:7"},
	{0x03, 0x01, 0x40, 0x40, "Off"},
	{0x03, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:8"},
	{0x03, 0x01, 0x80, 0x80, "Off"},
	{0x03, 0x01, 0x80, 0x00, "On"},
};

STDDIPINFO(Klxyj)

static struct BurnDIPInfo TshsDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x01, 0x01, 0x01, 0x01, "Coin Acceptor"},
	{0x01, 0x01, 0x01, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x01, 0x01, 0x02, 0x02, "Return Coins"},
	{0x01, 0x01, 0x02, 0x00, "Key-Out"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x01, 0x01, 0x04, 0x04, "Joystick"},
	{0x01, 0x01, 0x04, 0x00, "Mahjong"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x01, 0x01, 0x08, 0x00, "Off"},
	{0x01, 0x01, 0x08, 0x08, "On"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Name"},
	{0x01, 0x01, 0x10, 0x10, "Double Up"},
	{0x01, 0x01, 0x10, 0x00, "Continue Play"},
	{0   , 0xfe, 0   ,    2, "Number Type"},
	{0x01, 0x01, 0x20, 0x20, "Numbers"},
	{0x01, 0x01, 0x20, 0x00, "Circle Tiles"},
	{0   , 0xfe, 0   ,    2, "Hide Credits"},
	{0x01, 0x01, 0x40, 0x40, "Off"},
	{0x01, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x80, 0x00, "Off"},
	{0x01, 0x01, 0x80, 0x80, "On"},
	{0   , 0xfe, 0   ,    4, "Playing Card Display"},
	{0x02, 0x01, 0x03, 0x03, "Normal"},
	{0x02, 0x01, 0x03, 0x02, "Half-variation"},
	{0x02, 0x01, 0x03, 0x01, "Full-variation"},
	{0x02, 0x01, 0x03, 0x00, "Full-variation (dup)"},
	{0   , 0xfe, 0   ,    2, "SW2:3"},
	{0x02, 0x01, 0x04, 0x04, "Off"},
	{0x02, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:4"},
	{0x02, 0x01, 0x08, 0x08, "Off"},
	{0x02, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:5"},
	{0x02, 0x01, 0x10, 0x10, "Off"},
	{0x02, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:6"},
	{0x02, 0x01, 0x20, 0x20, "Off"},
	{0x02, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:7"},
	{0x02, 0x01, 0x40, 0x40, "Off"},
	{0x02, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:8"},
	{0x02, 0x01, 0x80, 0x80, "Off"},
	{0x02, 0x01, 0x80, 0x00, "On"},
};

STDDIPINFO(Tshs)

static struct BurnDIPInfo MgfxDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xfd, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "SW1:1"},
	{0x01, 0x01, 0x01, 0x01, "Off"},
	{0x01, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x01, 0x01, 0x06, 0x06, "5"},
	{0x01, 0x01, 0x06, 0x04, "10"},
	{0x01, 0x01, 0x06, 0x02, "50"},
	{0x01, 0x01, 0x06, 0x00, "100"},
	{0   , 0xfe, 0   ,    2, "Credit Limit"},
	{0x01, 0x01, 0x08, 0x08, "100"},
	{0x01, 0x01, 0x08, 0x00, "500"},
	{0   , 0xfe, 0   ,    2, "Credit Mode"},
	{0x01, 0x01, 0x10, 0x10, "Coin Acceptor"},
	{0x01, 0x01, 0x10, 0x00, "Key-In"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x01, 0x01, 0x20, 0x20, "Return Coins"},
	{0x01, 0x01, 0x20, 0x00, "Key-Out"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x01, 0x01, 0xc0, 0xc0, "1"},
	{0x01, 0x01, 0xc0, 0x80, "2"},
	{0x01, 0x01, 0xc0, 0x40, "3"},
	{0x01, 0x01, 0xc0, 0x00, "5"},
	{0   , 0xfe, 0   ,    2, "SW2:1"},
	{0x02, 0x01, 0x01, 0x01, "Off"},
	{0x02, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Controls"},
	{0x02, 0x01, 0x02, 0x00, "Joystick"},
	{0x02, 0x01, 0x02, 0x02, "Mahjong"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x02, 0x01, 0x04, 0x00, "Off"},
	{0x02, 0x01, 0x04, 0x04, "On"},
	{0   , 0xfe, 0   ,    4, "Coinage"},
	{0x02, 0x01, 0x18, 0x00, "2 Coins 1 Credit"},
	{0x02, 0x01, 0x18, 0x18, "1 Coin 1 Credit"},
	{0x02, 0x01, 0x18, 0x10, "1 Coin 2 Credits"},
	{0x02, 0x01, 0x18, 0x08, "1 Coin 3 Credits"},
	{0   , 0xfe, 0   ,    8, "Lottery Rate"},
	{0x02, 0x01, 0xe0, 0xe0, "1:1"},
	{0x02, 0x01, 0xe0, 0xc0, "1:2"},
	{0x02, 0x01, 0xe0, 0xa0, "1:5"},
	{0x02, 0x01, 0xe0, 0x80, "1:6"},
	{0x02, 0x01, 0xe0, 0x60, "1:7"},
	{0x02, 0x01, 0xe0, 0x40, "1:8"},
	{0x02, 0x01, 0xe0, 0x20, "1:9"},
	{0x02, 0x01, 0xe0, 0x00, "1:10"},
};

STDDIPINFO(Mgfx)

static struct BurnDIPInfo Royal5pDIPList[] = {
	{0x01, 0xff, 0xff, 0xf9, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Symbols"},
	{0x01, 0x01, 0x01, 0x01, "Fruit"},
	{0x01, 0x01, 0x01, 0x00, "Christmas"},
	{0   , 0xfe, 0   ,    2, "Demo Sounds"},
	{0x01, 0x01, 0x02, 0x02, "Off"},
	{0x01, 0x01, 0x02, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "Odds Table"},
	{0x01, 0x01, 0x04, 0x04, "No"},
	{0x01, 0x01, 0x04, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "System Limit"},
	{0x01, 0x01, 0x08, 0x00, "No"},
	{0x01, 0x01, 0x08, 0x08, "Yes"},
	{0   , 0xfe, 0   ,    2, "Double Up Game"},
	{0x01, 0x01, 0x10, 0x00, "Off"},
	{0x01, 0x01, 0x10, 0x10, "On"},
	{0   , 0xfe, 0   ,    2, "Payout Mode"},
	{0x01, 0x01, 0x20, 0x20, "Normal"},
	{0x01, 0x01, 0x20, 0x00, "Auto"},
	{0   , 0xfe, 0   ,    2, "Non Stop"},
	{0x01, 0x01, 0x40, 0x40, "No"},
	{0x01, 0x01, 0x40, 0x00, "Yes"},
	{0   , 0xfe, 0   ,    2, "SW1:8"},
	{0x01, 0x01, 0x80, 0x80, "Off"},
	{0x01, 0x01, 0x80, 0x00, "On"},
	{0   , 0xfe, 0   ,    4, "Maximum Bet"},
	{0x02, 0x01, 0x03, 0x03, "10"},
	{0x02, 0x01, 0x03, 0x02, "50"},
	{0x02, 0x01, 0x03, 0x01, "100"},
	{0x02, 0x01, 0x03, 0x00, "300"},
	{0   , 0xfe, 0   ,    4, "Minimum Bet"},
	{0x02, 0x01, 0x0c, 0x0c, "1"},
	{0x02, 0x01, 0x0c, 0x08, "10"},
	{0x02, 0x01, 0x0c, 0x04, "50"},
	{0x02, 0x01, 0x0c, 0x00, "100"},
	{0   , 0xfe, 0   ,    4, "Key-In Rate"},
	{0x02, 0x01, 0x30, 0x30, "1"},
	{0x02, 0x01, 0x30, 0x20, "10"},
	{0x02, 0x01, 0x30, 0x10, "50"},
	{0x02, 0x01, 0x30, 0x00, "100"},
	{0   , 0xfe, 0   ,    4, "Key-Out Rate"},
	{0x02, 0x01, 0xc0, 0xc0, "1"},
	{0x02, 0x01, 0xc0, 0x80, "10"},
	{0x02, 0x01, 0xc0, 0x40, "75"},
	{0x02, 0x01, 0xc0, 0x00, "100"},
	{0   , 0xfe, 0   ,    8, "Coinage"},
	{0x03, 0x01, 0x07, 0x07, "1 Coin 1 Credit"},
	{0x03, 0x01, 0x07, 0x06, "1 Coin 4 Credits"},
	{0x03, 0x01, 0x07, 0x05, "1 Coin 5 Credits"},
	{0x03, 0x01, 0x07, 0x04, "1 Coin 10 Credits"},
	{0x03, 0x01, 0x07, 0x03, "1 Coin 25 Credits"},
	{0x03, 0x01, 0x07, 0x02, "1 Coin 50 Credits"},
	{0x03, 0x01, 0x07, 0x01, "1 Coin 100 Credits"},
	{0x03, 0x01, 0x07, 0x00, "1 Coin 100 Credits (dup)"},
	{0   , 0xfe, 0   ,    8, "Payout Rate"},
	{0x03, 0x01, 0x38, 0x00, "35%"},
	{0x03, 0x01, 0x38, 0x08, "40%"},
	{0x03, 0x01, 0x38, 0x10, "45%"},
	{0x03, 0x01, 0x38, 0x18, "50%"},
	{0x03, 0x01, 0x38, 0x20, "55%"},
	{0x03, 0x01, 0x38, 0x28, "60%"},
	{0x03, 0x01, 0x38, 0x30, "70%"},
	{0x03, 0x01, 0x38, 0x38, "80%"},
	{0   , 0xfe, 0   ,    2, "Double Up Game Payout Rate"},
	{0x03, 0x01, 0x40, 0x00, "80%"},
	{0x03, 0x01, 0x40, 0x40, "90%"},
	{0   , 0xfe, 0   ,    2, "Language"},
	{0x03, 0x01, 0x80, 0x80, "English"},
	{0x03, 0x01, 0x80, 0x00, "Spanish"},
};

STDDIPINFO(Royal5p)

static struct BurnDIPInfo TripslotDIPList[] = {
	{0x01, 0xff, 0xff, 0xff, NULL},
	{0x02, 0xff, 0xff, 0xff, NULL},
	{0x03, 0xff, 0xff, 0xff, NULL},
	{0   , 0xfe, 0   ,    2, "Language"},
	{0x01, 0x01, 0x01, 0x01, "English"},
	{0x01, 0x01, 0x01, 0x00, "Spanish"},
	{0   , 0xfe, 0   ,    2, "Wiring Diagram"},
	{0x01, 0x01, 0x02, 0x02, "36+10"},
	{0x01, 0x01, 0x02, 0x00, "28-pin"},
	{0   , 0xfe, 0   ,    2, "SW1:3"},
	{0x01, 0x01, 0x04, 0x04, "Off"},
	{0x01, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW1:4"},
	{0x01, 0x01, 0x08, 0x08, "Off"},
	{0x01, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW1:5"},
	{0x01, 0x01, 0x10, 0x10, "Off"},
	{0x01, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW1:6"},
	{0x01, 0x01, 0x20, 0x20, "Off"},
	{0x01, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW1:7"},
	{0x01, 0x01, 0x40, 0x40, "Off"},
	{0x01, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW1:8"},
	{0x01, 0x01, 0x80, 0x80, "Off"},
	{0x01, 0x01, 0x80, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:1"},
	{0x02, 0x01, 0x01, 0x01, "Off"},
	{0x02, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:2"},
	{0x02, 0x01, 0x02, 0x02, "Off"},
	{0x02, 0x01, 0x02, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:3"},
	{0x02, 0x01, 0x04, 0x04, "Off"},
	{0x02, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:4"},
	{0x02, 0x01, 0x08, 0x08, "Off"},
	{0x02, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:5"},
	{0x02, 0x01, 0x10, 0x10, "Off"},
	{0x02, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:6"},
	{0x02, 0x01, 0x20, 0x20, "Off"},
	{0x02, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:7"},
	{0x02, 0x01, 0x40, 0x40, "Off"},
	{0x02, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW2:8"},
	{0x02, 0x01, 0x80, 0x80, "Off"},
	{0x02, 0x01, 0x80, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:1"},
	{0x03, 0x01, 0x01, 0x01, "Off"},
	{0x03, 0x01, 0x01, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:2"},
	{0x03, 0x01, 0x02, 0x02, "Off"},
	{0x03, 0x01, 0x02, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:3"},
	{0x03, 0x01, 0x04, 0x04, "Off"},
	{0x03, 0x01, 0x04, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:4"},
	{0x03, 0x01, 0x08, 0x08, "Off"},
	{0x03, 0x01, 0x08, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:5"},
	{0x03, 0x01, 0x10, 0x10, "Off"},
	{0x03, 0x01, 0x10, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:6"},
	{0x03, 0x01, 0x20, 0x20, "Off"},
	{0x03, 0x01, 0x20, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:7"},
	{0x03, 0x01, 0x40, 0x40, "Off"},
	{0x03, 0x01, 0x40, 0x00, "On"},
	{0   , 0xfe, 0   ,    2, "SW3:8"},
	{0x03, 0x01, 0x80, 0x80, "Off"},
	{0x03, 0x01, 0x80, 0x00, "On"},
};

STDDIPINFO(Tripslot)


#include "bitswap.h"
#include "arm7_intf.h"
#include "8255ppi.h"
#include "msm6295.h"
#include "igs017_igs031.h"

#define IGSM027_FREQ					22000000
#define IGSM027_CYCS_PER_FRAME			((IGSM027_FREQ) / (nBurnFPS/100))

void drv_crypt_set_region(UINT8 *user1, INT32 user1_len);
void zhongguo_decrypt();
void cjddz_decrypt();
void cjddzp_decrypt();
void lhzb4_decrypt();
void slqz3_decrypt();
void fruitpar_decrypt();
void oceanpar_decrypt();
void mgcs3_decrypt();
void mgzz_decrypt();
void qlgs_decrypt();
void tct2p_decrypt();
void xypdk_decrypt();
void crzybugs_decrypt();
void tripslot_decrypt();
void cjddzlf_decrypt();
void cjtljp_decrypt();
void tswxp_decrypt();
void royal5p_decrypt();
void mgfx_decrypt();

static void drv_refresh_user_map();

enum {
	CFG_SLQZ3 = 0,
	CFG_LHDMG,
	CFG_LHDMG106,
	CFG_LHZB3SJB,
	CFG_CJDDZ,
	CFG_LHZB4,
	CFG_MGCS3,
	CFG_CJTLJP,
	CFG_LTHYP,
	CFG_JKING02,
	CFG_TCT2P,
	CFG_TSWXP,
	CFG_MGZZ,
	CFG_OCEANPAR,
	CFG_ROYAL5P,
	CFG_TRIPSLOT,
	CFG_CCLY,
	CFG_CJSXP,
	CFG_QLGS
};

static INT32 DrvConfig;

static inline UINT8 drv_input_active(INT32 idx)
{
	return DrvInputState[idx] & 1;
}

static void drv_make_inputs()
{
	memcpy(DrvInputState, DrvInputPort, sizeof(DrvInputState));

	if (DrvInputState[M027_IN_UP] && DrvInputState[M027_IN_DOWN]) {
		DrvInputState[M027_IN_UP] = 0;
		DrvInputState[M027_IN_DOWN] = 0;
	}

	if (DrvInputState[M027_IN_LEFT] && DrvInputState[M027_IN_RIGHT]) {
		DrvInputState[M027_IN_LEFT] = 0;
		DrvInputState[M027_IN_RIGHT] = 0;
	}

	for (INT32 i = 0; i < 2; i++) {
		const INT32 input = (i == 0) ? M027_IN_COIN1 : M027_IN_COIN2;
		const UINT8 raw = DrvInputPort[input] ? 1 : 0;

		if (raw && !DrvCoinPrev[i]) {
			DrvCoinPulse[i] = 5;
		}
		DrvCoinPrev[i] = raw;
		DrvInputState[input] = (DrvCoinPulse[i] != 0) ? 1 : 0;

		if (DrvCoinPulse[i]) {
			DrvCoinPulse[i]--;
		}
	}
}

static UINT8 drv_hopper_line()
{
	if (!DrvUseHopper) return 1;
	return (DrvHopperMotor && ((GetCurrentFrame() % 10) == 0)) ? 0 : 1;
}

static UINT8 drv_ticket_line()
{
	if (!DrvUseTicket) return 1;
	return (DrvTicketMotor && ((GetCurrentFrame() % 20) == 0)) ? 0 : 1;
}

static INT32 drv_controls_joystick()
{
	if (DrvControlsMask == 0) return 1;
	return (DrvDips[DrvControlsBank] & DrvControlsMask) == DrvControlsValue;
}

static INT32 drv_jking02_36pin()
{
	return (DrvDips[1] & 0x01) == 0x01;
}

static INT32 drv_tripslot_36pin()
{
	return (DrvDips[0] & 0x02) == 0x02;
}

static INT32 drv_lhzb3sjb_mahjong()
{
	return (DrvDips[1] & 0x08) == 0x08;
}

static INT32 drv_qlgs_mahjong()
{
	return (DrvDips[1] & 0x01) == 0x01;
}

static INT32 drv_cjsxp_amusement()
{
	return (DrvDips[0] & 0x01) == 0x01;
}

static INT32 drv_ccly_amusement()
{
	return (DrvDips[1] & 0x01) == 0x01;
}

static UINT8 drv_key_group(INT32 group)
{
	UINT8 data = 0xff;

	switch (group) {
		case 0:
			if (drv_input_active(M027_IN_MJ_A)) data &= ~0x01;
			if (drv_input_active(M027_IN_MJ_E)) data &= ~0x02;
			if (drv_input_active(M027_IN_MJ_I)) data &= ~0x04;
			if (drv_input_active(M027_IN_MJ_M)) data &= ~0x08;
			if (drv_input_active(M027_IN_MJ_KAN)) data &= ~0x10;
			if (drv_input_active(M027_IN_START1)) data &= ~0x20;
		return data;

		case 1:
			if (drv_input_active(M027_IN_MJ_B)) data &= ~0x01;
			if (drv_input_active(M027_IN_MJ_F)) data &= ~0x02;
			if (drv_input_active(M027_IN_MJ_J)) data &= ~0x04;
			if (drv_input_active(M027_IN_MJ_N)) data &= ~0x08;
			if (drv_input_active(M027_IN_MJ_REACH)) data &= ~0x10;
			if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x20;
		return data;

		case 2:
			if (drv_input_active(M027_IN_MJ_C)) data &= ~0x01;
			if (drv_input_active(M027_IN_MJ_G)) data &= ~0x02;
			if (drv_input_active(M027_IN_MJ_K)) data &= ~0x04;
			if (drv_input_active(M027_IN_MJ_CHI)) data &= ~0x08;
			if (drv_input_active(M027_IN_MJ_RON)) data &= ~0x10;
		return data;

		case 3:
			if (drv_input_active(M027_IN_MJ_D)) data &= ~0x01;
			if (drv_input_active(M027_IN_MJ_H)) data &= ~0x02;
			if (drv_input_active(M027_IN_MJ_L)) data &= ~0x04;
			if (drv_input_active(M027_IN_MJ_PON)) data &= ~0x08;
		return data;

		case 4:
			if (drv_input_active(M027_IN_MJ_LAST)) data &= ~0x01;
			if (drv_input_active(M027_IN_MJ_SCORE)) data &= ~0x02;
			if (drv_input_active(M027_IN_MJ_DOUBLE)) data &= ~0x04;
			if (drv_input_active(M027_IN_MJ_BIG)) data &= ~0x10;
			if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x20;
		return data;
	}

	return data;
}

static UINT8 drv_dsw_r(INT32 select, INT32 first, INT32 shift)
{
	UINT8 data = 0xff;

	for (INT32 i = first; i < 3; i++) {
		if ((DrvIoSelect[select] & (1 << (i - first + shift))) == 0) {
			data &= DrvDips[i];
		}
	}

	return data;
}

static UINT8 drv_kbd_r(INT32 select, INT32 shift, INT32 rotate)
{
	UINT8 data = 0xff;

	for (INT32 i = 0; i < 5; i++) {
		if ((DrvIoSelect[select] & (1 << (i + shift))) == 0) {
			data &= drv_key_group(i);
		}
	}

	if (rotate == 0) return data;
	return (data << rotate) | (data >> (8 - rotate));
}

static UINT8 port_test_mahjong()
{
	UINT8 data = 0xff;
	if (drv_hopper_line() == 0) data &= ~0x01;
	if (drv_input_active(M027_IN_SHOW_CREDITS)) data &= ~0x02;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
	if (drv_input_active(M027_IN_PAYOUT) || drv_input_active(M027_IN_KEYOUT)) data &= ~0x20;
	if (drv_input_active(M027_IN_BUTTON3)) data &= ~0x40;
	if (drv_input_active(M027_IN_BUTTON2)) data &= ~0x80;
	return data;
}

static UINT8 port_test_slqz3()
{
	UINT8 data = 0xff;
	if (drv_hopper_line() == 0) data &= ~0x01;
	if (drv_input_active(M027_IN_SHOW_CREDITS)) data &= ~0x02;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
	if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20;
	if (drv_input_active(M027_IN_BUTTON3)) data &= ~0x40;
	if (drv_input_active(M027_IN_BUTTON2)) data &= ~0x80;
	return data;
}

static UINT8 port_joy_mahjong()
{
	UINT8 data = 0xff;
	if (drv_input_active(M027_IN_START1)) data &= ~0x01;
	if (drv_input_active(M027_IN_UP)) data &= ~0x02;
	if (drv_input_active(M027_IN_DOWN)) data &= ~0x04;
	if (drv_input_active(M027_IN_LEFT)) data &= ~0x08;
	if (drv_input_active(M027_IN_RIGHT)) data &= ~0x10;
	if (drv_input_active(M027_IN_BUTTON1)) data &= ~0x20;
	return data;
}

static UINT8 port_test_lhzb3sjb()
{
	UINT8 data = 0xff;
	if (drv_hopper_line() == 0) data &= ~0x01;
	if (drv_input_active(M027_IN_SHOW_CREDITS)) data &= ~0x02;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
	if (drv_input_active(M027_IN_PAYOUT) || drv_input_active(M027_IN_KEYOUT)) data &= ~0x20;
	if (!drv_lhzb3sjb_mahjong() && drv_input_active(M027_IN_BUTTON3)) data &= ~0x40;
	if (!drv_lhzb3sjb_mahjong() && drv_input_active(M027_IN_BUTTON2)) data &= ~0x80;
	return data;
}

static UINT8 port_joy_slqz3()
{
	UINT8 data = 0xff;
	if (drv_input_active(M027_IN_START1)) data &= ~0x04;
	if (drv_input_active(M027_IN_UP)) data &= ~0x08;
	if (drv_input_active(M027_IN_DOWN)) data &= ~0x10;
	if (drv_input_active(M027_IN_LEFT)) data &= ~0x20;
	if (drv_input_active(M027_IN_RIGHT)) data &= ~0x40;
	if (drv_input_active(M027_IN_BUTTON1)) data &= ~0x80;
	return data;
}

static UINT8 port_joy_lhzb3sjb()
{
	if (drv_lhzb3sjb_mahjong()) {
		return 0x03 | ((drv_kbd_r(0, 0, 0) & 0x3f) << 2);
	}

	return port_joy_slqz3();
}

static UINT8 port_test_ddz()
{
	UINT8 data = 0xff;
	INT32 joystick = drv_controls_joystick();
	if (joystick && drv_input_active(M027_IN_DOWN)) data &= ~0x02;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (joystick && drv_input_active(M027_IN_COIN1)) data &= ~0x10;
	if (joystick && drv_input_active(M027_IN_UP)) data &= ~0x40;
	if (joystick && drv_input_active(M027_IN_LEFT)) data &= ~0x80;
	return data;
}

static UINT8 port_joy_ddz()
{
	UINT8 data = 0xff;
	INT32 joystick = drv_controls_joystick();
	if (joystick && drv_input_active(M027_IN_BUTTON3)) data &= ~0x01;
	if (joystick && drv_input_active(M027_IN_BUTTON2)) data &= ~0x04;
	if (joystick && drv_input_active(M027_IN_BUTTON1)) data &= ~0x08;
	if (joystick && drv_input_active(M027_IN_START1)) data &= ~0x10;
	if (drv_hopper_line() == 0) data &= ~0x20;
	if (!joystick && drv_input_active(M027_IN_COIN1)) data &= ~0x40;
	if (joystick && drv_input_active(M027_IN_RIGHT)) data &= ~0x80;
	return data;
}

static UINT32 player_ddz()
{
	UINT32 data = 0xfffff;
	INT32 joystick = drv_controls_joystick();

	data = (data & ~0x3f) | (drv_kbd_r(0, 0, 0) & 0x3f);
	if (!joystick && drv_hopper_line() == 0) data &= ~0x00040;
	if (drv_input_active(M027_IN_KEYIN)) data &= ~0x00100;
	if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x10000;
	if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x20000;

	return data;
}

static UINT8 port_test_qlgs()
{
	UINT8 data = 0xff;
	INT32 mahjong = drv_qlgs_mahjong();
	if (!mahjong && drv_input_active(M027_IN_DOWN)) data &= ~0x02;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (mahjong && drv_input_active(M027_IN_SHOW_CREDITS)) data &= ~0x20;
	if (!mahjong && drv_input_active(M027_IN_COIN1)) data &= ~0x10;
	if (!mahjong && drv_input_active(M027_IN_UP)) data &= ~0x40;
	if (!mahjong && drv_input_active(M027_IN_LEFT)) data &= ~0x80;
	return data;
}

static UINT8 port_joy_qlgs()
{
	UINT8 data = 0xff;
	INT32 mahjong = drv_qlgs_mahjong();
	if (mahjong) {
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x40;
	} else {
		if (drv_input_active(M027_IN_BUTTON3)) data &= ~0x01;
		if (drv_input_active(M027_IN_BUTTON2)) data &= ~0x04;
		if (drv_input_active(M027_IN_BUTTON1)) data &= ~0x08;
		if (drv_input_active(M027_IN_START1)) data &= ~0x10;
		if (drv_hopper_line() == 0) data &= ~0x20;
		if (drv_input_active(M027_IN_RIGHT)) data &= ~0x80;
	}
	return data;
}

static UINT32 player_qlgs()
{
	UINT32 data = 0xfffff;
	data = (data & ~0x3f) | (drv_kbd_r(0, 0, 0) & 0x3f);
	if (drv_qlgs_mahjong() && drv_hopper_line() == 0) data &= ~0x00040;
	if (!drv_qlgs_mahjong() && drv_input_active(M027_IN_SHOW_CREDITS)) data &= ~0x10000;
	if (drv_input_active(M027_IN_ATTENDANT)) data &= ~0x10000;
	if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20000;
	return data;
}

static UINT8 portb_tct2p()
{
	UINT8 data = 0xff;

	if (drv_controls_joystick()) {
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x01;
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x02;
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x40;
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x80;
	} else {
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	}

	return data;
}

static UINT8 portc_tct2p()
{
	UINT8 data = 0xff;

	if (drv_controls_joystick()) {
		if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x01;
		if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x04;
		if (drv_input_active(M027_IN_MJ_BIG)) data &= ~0x08;
		if (drv_input_active(M027_IN_START1)) data &= ~0x10;
		if (drv_hopper_line() == 0) data &= ~0x20;
		if (drv_input_active(M027_IN_STOP4)) data &= ~0x80;
	} else {
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x40;
	}

	return data;
}

static UINT32 player_tct2p()
{
	UINT32 data = 0xfffff;

	if (drv_controls_joystick()) {
		if (drv_input_active(M027_IN_KEYIN)) data &= ~0x00100;
		if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x20000;
	} else {
		data = (data & ~0x3f) | (drv_kbd_r(0, 0, 0) & 0x3f);
		if (drv_hopper_line() == 0) data &= ~0x00040;
		if (drv_input_active(M027_IN_KEYIN)) data &= ~0x01000;
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x02000;
		if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x20000;
	}

	return data;
}

static UINT8 portb_tswxp()
{
	UINT8 data = 0xff;

	if (drv_controls_joystick()) {
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x02;
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x40;
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x80;
	} else {
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	}

	return data;
}

static UINT8 portc_tswxp()
{
	UINT8 data = 0xff;

	if (drv_controls_joystick()) {
		if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x01;
		if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x04;
		if (drv_input_active(M027_IN_STOP5) || drv_input_active(M027_IN_MJ_BIG)) data &= ~0x08;
		if (drv_input_active(M027_IN_START1)) data &= ~0x10;
		if (drv_hopper_line() == 0) data &= ~0x20;
		if (drv_input_active(M027_IN_STOP4)) data &= ~0x80;
	} else {
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x40;
	}

	return data;
}

static UINT32 player_tswxp()
{
	UINT32 data = 0xfffff;

	if (drv_controls_joystick()) {
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20000;
	} else {
		data = (data & ~0x3f) | (drv_kbd_r(0, 0, 0) & 0x3f);
		if (drv_hopper_line() == 0) data &= ~0x00040;
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20000;
	}

	return data;
}

static UINT8 portb_three_reel()
{
	UINT8 data = 0xff;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20;
	if (drv_input_active(M027_IN_ATTENDANT)) data &= ~0x40;
	return data;
}

static UINT8 portc_three_reel()
{
	UINT8 data = 0xff;
	if (drv_input_active(M027_IN_KEYIN)) data &= ~0x01;
	if (drv_hopper_line() == 0) data &= ~0x20;
	if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x40;
	return data;
}

static UINT32 player_three_reel()
{
	UINT32 data = 0xfffff;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x00001;
	if (drv_input_active(M027_IN_START1)) data &= ~0x00004;
	if (drv_input_active(M027_IN_STOPALL)) data &= ~0x00008;
	if (drv_input_active(M027_IN_STOP1)) data &= ~0x00010;
	if (drv_input_active(M027_IN_STOP2)) data &= ~0x00020;
	if (drv_input_active(M027_IN_STOP3)) data &= ~0x00040;
	if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x00080;
	if (drv_input_active(M027_IN_TICKET)) data &= ~0x00800;
	if (drv_ticket_line() == 0) data &= ~0x01000;
	if (drv_input_active(M027_IN_COIN2)) data &= ~0x02000;
	return data;
}

static UINT8 portb_jking02()
{
	UINT8 data = 0xff;

	if (drv_jking02_36pin()) {
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
		return data;
	}

	if (drv_input_active(M027_IN_STOP4)) data &= ~0x02;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x04;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x08;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
	if (drv_input_active(M027_IN_STOP3)) data &= ~0x40;
	if (drv_input_active(M027_IN_STOP2)) data &= ~0x80;
	return data;
}

static UINT8 portc_jking02()
{
	UINT8 data = 0xff;

	if (drv_jking02_36pin()) {
		if (drv_input_active(M027_IN_KEYIN)) data &= ~0x01;
		if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x40;
		return data;
	}

	if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x01;
	if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x02;
	if (drv_input_active(M027_IN_START1)) data &= ~0x04;
	if (drv_input_active(M027_IN_STOP1)) data &= ~0x80;
	return data;
}

static UINT32 player_jking02()
{
	UINT32 data = 0xfffff;

	if (drv_jking02_36pin()) {
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x00001;
		if (drv_input_active(M027_IN_COIN2)) data &= ~0x00002;
		if (drv_input_active(M027_IN_STOP4) || drv_input_active(M027_IN_START1)) data &= ~0x00004;
		if (drv_input_active(M027_IN_STOPALL)) data &= ~0x00008;
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x00010;
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x00020;
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x00040;
		if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x00080;
	} else {
		if (drv_input_active(M027_IN_KEYIN)) data &= ~0x10000;
	}

	return data;
}

static UINT8 portb_royal5p()
{
	UINT8 data = 0xff;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20;
	return data;
}

static UINT32 player_royal5p()
{
	UINT32 data = 0xfffff;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x00001;
	if (drv_input_active(M027_IN_COIN2)) data &= ~0x00002;
	if (drv_input_active(M027_IN_START1) || drv_input_active(M027_IN_STOPALL)) data &= ~0x00004;
	if (drv_input_active(M027_IN_STOP2)) data &= ~0x00008;
	if (drv_input_active(M027_IN_STOP4)) data &= ~0x00010;
	if (drv_input_active(M027_IN_STOP3)) data &= ~0x00020;
	if (drv_input_active(M027_IN_STOP1)) data &= ~0x00040;
	if (drv_input_active(M027_IN_STOP5) || drv_input_active(M027_IN_MJ_BET)) data &= ~0x00080;
	return data;
}

static UINT8 portb_tripslot()
{
	UINT8 data = 0xff;

	if (drv_tripslot_36pin()) {
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20;
		return data;
	}

	if (drv_hopper_line() == 0) data &= ~0x01;
	if (drv_input_active(M027_IN_STOP4)) data &= ~0x02;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x04;
	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x08;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
	if (drv_input_active(M027_IN_STOP3)) data &= ~0x40;
	if (drv_input_active(M027_IN_STOP2)) data &= ~0x80;
	return data;
}

static UINT8 portc_tripslot()
{
	UINT8 data = 0xff;

	if (drv_tripslot_36pin()) {
		if (drv_input_active(M027_IN_KEYIN)) {
			data &= ~0x01;
			data &= ~0x80;
		}
		if (drv_hopper_line() == 0) data &= ~0x20;
		if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x40;
		return data;
	}

	if (drv_input_active(M027_IN_STOP5) || drv_input_active(M027_IN_MJ_BET)) data &= ~0x01;
	if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x02;
	if (drv_input_active(M027_IN_START1) || drv_input_active(M027_IN_STOPALL)) data &= ~0x04;
	if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x08;
	if (drv_input_active(M027_IN_STOP1)) data &= ~0x80;
	return data;
}

static UINT32 player_tripslot()
{
	UINT32 data = 0xfffff;

	if (drv_tripslot_36pin()) {
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x00001;
		if (drv_input_active(M027_IN_COIN2)) data &= ~0x00002;
		if (drv_input_active(M027_IN_START1) || drv_input_active(M027_IN_STOPALL)) data &= ~0x00004;
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x00008;
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x00010;
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x00020;
		if (drv_input_active(M027_IN_STOP4)) data &= ~0x00040;
		if (drv_input_active(M027_IN_STOP5) || drv_input_active(M027_IN_MJ_BET)) data &= ~0x00080;
	} else {
		if (drv_input_active(M027_IN_KEYIN)) data &= ~0x10000;
	}

	return data;
}

static UINT8 portb_cjsxp()
{
	UINT8 data = 0xff;
	INT32 amusement = drv_cjsxp_amusement();

	if (amusement) {
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x02;
	} else {
		if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x02;
	}

	if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
	if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
	if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;

	if (amusement) {
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x40;
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x80;
	} else {
		if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x20;
		if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x40;
	}

	return data;
}

static UINT8 portc_cjsxp()
{
	UINT8 data = 0xff;
	INT32 amusement = drv_cjsxp_amusement();

	if (amusement) {
		if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x01;
		if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x04;
		if (drv_input_active(M027_IN_STOP5)) data &= ~0x08;
		if (drv_input_active(M027_IN_START1)) data &= ~0x10;
		if (drv_input_active(M027_IN_STOP4)) data &= ~0x80;
	} else {
		if (drv_input_active(M027_IN_KEYIN)) data &= ~0x40;
		if (drv_input_active(M027_IN_START1)) data &= ~0x04;
	}

	if (drv_hopper_line() == 0) data &= ~0x20;
	return data;
}

static UINT32 player_cjsxp()
{
	UINT32 data = 0xfffff;

	if (drv_cjsxp_amusement()) {
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x00002;
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x00004;
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20000;
	} else {
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x00001;
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x00002;
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x00004;
		if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x00008;
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x00010;
		if (drv_input_active(M027_IN_MJ_DOUBLE)) data &= ~0x00020;
		data &= ~0x00100;
	}

	return data;
}

static UINT8 portb_ccly()
{
	UINT8 data = 0xff;

	if (drv_ccly_amusement()) {
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x02;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x04;
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x08;
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
		if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x20;
		if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x80;
	} else {
		if (drv_hopper_line() == 0) data &= ~0x01;
		if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x02;
		if (drv_input_active(M027_IN_SERVICE)) data &= ~0x04;
		if (drv_input_active(M027_IN_BOOK)) data &= ~0x08;
		if (drv_input_active(M027_IN_COIN1)) data &= ~0x10;
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x20;
		if (drv_input_active(M027_IN_MJ_BIG)) data &= ~0x80;
	}

	return data;
}

static UINT8 portc_ccly()
{
	UINT8 data = 0xff;

	if (drv_ccly_amusement()) {
		if (drv_input_active(M027_IN_KEYIN)) data &= ~0x01;
		if (drv_input_active(M027_IN_MJ_BET)) data &= ~0x04;
		if (drv_input_active(M027_IN_MJ_BIG)) data &= ~0x08;
		if (drv_input_active(M027_IN_START1)) data &= ~0x10;
		if (drv_hopper_line() == 0) data &= ~0x20;
		if (drv_input_active(M027_IN_PAYOUT)) data &= ~0x40;
	} else {
		if (drv_input_active(M027_IN_KEYOUT)) data &= ~0x02;
		if (drv_input_active(M027_IN_START1)) data &= ~0x04;
		if (drv_input_active(M027_IN_STOPALL)) data &= ~0x08;
		if (drv_input_active(M027_IN_COIN2)) data &= ~0x80;
	}

	return data;
}

static UINT32 player_ccly()
{
	UINT32 data = 0xfffff;

	if (drv_ccly_amusement()) {
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x00002;
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x00004;
	} else {
		if (drv_input_active(M027_IN_STOP1)) data &= ~0x00001;
		if (drv_input_active(M027_IN_STOP2)) data &= ~0x00002;
		if (drv_input_active(M027_IN_STOP3)) data &= ~0x00004;
		if (drv_input_active(M027_IN_MJ_SMALL)) data &= ~0x00008;
		if (drv_input_active(M027_IN_MJ_SCORE)) data &= ~0x00010;
		if (drv_input_active(M027_IN_MJ_DOUBLE)) data &= ~0x00020;
	}

	// MAME notes this bit must be low or most inputs are ignored.
	data &= ~0x00100;
	return data;
}

static UINT8 portc_const_ff() { return 0xff; }
static UINT8 dsw1_sel10() { return drv_dsw_r(1, 0, 0); }
static UINT8 dsw2_sel10() { return DrvDips[1]; }
static UINT8 dsw3_sel10() { return DrvDips[2]; }
static UINT8 kbd_sel0_3_0() { return drv_kbd_r(0, 3, 0); }
static UINT8 kbd_sel1_0_2() { return drv_kbd_r(1, 0, 2); }

static UINT32 gpio_slqz3()
{
	return (DrvIoSelect[1] & 1) ? 0x000fffff : 0x000ffffd;
}

static UINT32 gpio_lhdmg()
{
	return (DrvIoSelect[1] & 1) ? 0x000fffff : (0x000fffff ^ 0x00080000);
}

static UINT32 gpio_lhzb3106()
{
	return (DrvIoSelect[1] & 1) ? 0x000fffff : (0x000fffff ^ 0x00008000);
}

static UINT32 gpio_all_high()
{
	return 0x000fffff;
}

static void drv_set_oki_full_bank(INT32 bank)
{
	if (DrvOkiLen <= 0) return;
	INT32 banks = DrvOkiLen / 0x40000;
	if (banks <= 0) banks = 1;
	if (bank >= banks) bank %= banks;
	MSM6295SetBank(0, DrvOkiROM + (bank * 0x40000), 0x00000, 0x3ffff);
}

static void drv_set_oki_split_bank(INT32 which, INT32 bank)
{
	if (DrvOkiLen <= 0) return;
	INT32 banks = DrvOkiLen / 0x20000;
	if (banks <= 0) banks = 1;
	if (bank >= banks) bank %= banks;

	if (which == 0) {
		MSM6295SetBank(0, DrvOkiROM + (bank * 0x20000), 0x00000, 0x1ffff);
	} else {
		MSM6295SetBank(0, DrvOkiROM + (bank * 0x20000), 0x20000, 0x3ffff);
	}
}

static void main_out_base(UINT8 data)
{
	DrvIoSelect[1] = data;
}

static void main_out_oki_bank(UINT8 data)
{
	main_out_base(data);
	drv_set_oki_full_bank((data >> 3) & 0x03);
}

static void main_out_tripslot(UINT8 data)
{
	main_out_base(data);
	DrvIoSelect[0] = (DrvIoSelect[0] & 0x04) | ((data >> 3) & 0x03);
	drv_set_oki_full_bank(DrvIoSelect[0] & 0x07);
}

static void ppi0_write_mahjong(UINT8 data)
{
	if (DrvUseHopper) DrvHopperMotor = (data >> 2) & 1;
	drv_set_oki_full_bank((data >> 6) & 0x03);
}

static void ppi0_write_ioselect0(UINT8 data)
{
	DrvIoSelect[0] = data;
}

static void ppi0_write_lhzb3sjb(UINT8 data)
{
	UINT8 sel = 0;
	sel |= ((data >> 3) & 1) << 2;
	sel |= ((data >> 1) & 1) << 1;
	sel |= ((data >> 0) & 1) << 0;
	DrvIoSelect[0] = ~(UINT8)(1 << sel);
	if (DrvUseHopper) DrvHopperMotor = (data >> 2) & 1;
	drv_set_oki_full_bank((data >> 6) & 0x03);
}

static void ppi0_write_jking02_a(UINT8) { }
static void ppi0_write_jking02_b(UINT8) { }
static void ppi0_write_jking02_c(UINT8) { }

static void ppi0_write_oceanpar_a(UINT8 data)
{
	DrvTicketMotor = (data >> 7) & 1;
}

static void ppi0_write_oceanpar_b(UINT8 data)
{
	if (DrvUseHopper) DrvHopperMotor = (data >> 7) & 1;
}

static void ppi0_write_oceanpar_c(UINT8) { }
static void ppi0_write_royal5p_b(UINT8) { }
static void ppi0_write_royal5p_c(UINT8) { }
static void ppi0_write_tripslot_a(UINT8) { }
static void ppi0_write_tripslot_b(UINT8) { }
static void ppi0_write_tripslot_c(UINT8) { }
static void ppi0_write_ccly_b(UINT8) { }
static void ppi0_write_ccly_c(UINT8) { }

static void ppi0_write_cjddz_b(UINT8 data)
{
	if (!drv_controls_joystick() && DrvUseHopper) {
		DrvHopperMotor = (data >> 4) & 1;
	}
}

static void ppi0_write_cjddz_c(UINT8 data)
{
	DrvIoSelect[0] = data & 0x1f;
	if (drv_controls_joystick() && DrvUseHopper) {
		DrvHopperMotor = (data >> 7) & 1;
	}
}

static void ppi0_write_tct2p_b(UINT8 data)
{
	if (!drv_controls_joystick() && DrvUseHopper) {
		DrvHopperMotor = (data >> 4) & 1;
	}
}

static void ppi0_write_tct2p_c(UINT8 data)
{
	DrvIoSelect[0] = data;
	if (drv_controls_joystick() && DrvUseHopper) {
		DrvHopperMotor = (data >> 7) & 1;
	}
}

static void ppi0_write_cjsxp_b(UINT8 data)
{
	DrvHopperMotor = ((data & 0x10) || (data & 0x80)) ? 1 : 0;
}

static UINT8 ppi_unused_read() { return 0xff; }
static void ppi_unused_write(UINT8) { }

static PPIPortRead DrvPPIRead[2][3];
static PPIPortWrite DrvPPIWrite[2][3];

static void drv_assign_ppi_defaults()
{
	for (INT32 chip = 0; chip < 2; chip++) {
		for (INT32 port = 0; port < 3; port++) {
			DrvPPIRead[chip][port] = ppi_unused_read;
			DrvPPIWrite[chip][port] = ppi_unused_write;
		}
	}
}

static void drv_apply_config(INT32 config)
{
	drv_assign_ppi_defaults();
	DrvUseHopper = 0;
	DrvUseTicket = 0;
	DrvUseSplitBank = 0;
	DrvNumPpi = 1;
	DrvControlsBank = 0;
	DrvControlsMask = 0;
	DrvControlsValue = 0;
	pDrvMainInCb = gpio_all_high;
	DrvMainOutCb = main_out_base;
	igs017_igs031_set_inputs(dsw1_sel10, dsw2_sel10, portc_const_ff);

	switch (config) {
		case CFG_SLQZ3:
			DrvUseHopper = 1;
			pDrvMainInCb = gpio_slqz3;
			DrvPPIRead[0][0] = port_test_slqz3;
			DrvPPIRead[0][1] = port_joy_slqz3;
			DrvPPIWrite[0][2] = ppi0_write_mahjong;
			igs017_igs031_set_inputs(dsw1_sel10, dsw2_sel10, portc_const_ff);
		break;

		case CFG_LHDMG:
			DrvUseHopper = 1;
			pDrvMainInCb = gpio_lhdmg;
			DrvPPIRead[0][0] = port_test_mahjong;
			DrvPPIWrite[0][1] = ppi0_write_ioselect0;
			DrvPPIWrite[0][2] = ppi0_write_mahjong;
			igs017_igs031_set_inputs(dsw1_sel10, dsw2_sel10, kbd_sel0_3_0);
		break;

		case CFG_LHDMG106:
			DrvUseHopper = 1;
			pDrvMainInCb = gpio_lhzb3106;
			DrvPPIRead[0][0] = port_test_mahjong;
			DrvPPIWrite[0][1] = ppi0_write_ioselect0;
			DrvPPIWrite[0][2] = ppi0_write_mahjong;
			igs017_igs031_set_inputs(dsw1_sel10, dsw2_sel10, kbd_sel0_3_0);
		break;

		case CFG_LHZB3SJB:
			DrvUseHopper = 1;
			pDrvMainInCb = gpio_slqz3;
			DrvPPIRead[0][0] = port_test_lhzb3sjb;
			DrvPPIRead[0][1] = port_joy_lhzb3sjb;
			DrvPPIWrite[0][2] = ppi0_write_lhzb3sjb;
			igs017_igs031_set_inputs(dsw1_sel10, dsw2_sel10, portc_const_ff);
		break;

		case CFG_CJDDZ:
			DrvUseHopper = 1;
			DrvUseSplitBank = 1;
			pDrvMainInCb = player_ddz;
			DrvControlsBank = 0;
			DrvControlsMask = 0x02;
			DrvControlsValue = 0x02;
			DrvPPIWrite[0][1] = ppi0_write_cjddz_b;
			DrvPPIWrite[0][2] = ppi0_write_cjddz_c;
			igs017_igs031_set_inputs(dsw1_sel10, port_test_ddz, port_joy_ddz);
		break;

		case CFG_LHZB4:
			DrvUseHopper = 1;
			DrvUseSplitBank = 1;
			pDrvMainInCb = player_ddz;
			DrvControlsBank = 0;
			// lhzb4 uses SW1:1 for control mode, unlike cjddz on SW1:2.
			DrvControlsMask = 0x01;
			DrvControlsValue = 0x01;
			DrvPPIWrite[0][1] = ppi0_write_cjddz_b;
			DrvPPIWrite[0][2] = ppi0_write_cjddz_c;
			igs017_igs031_set_inputs(dsw1_sel10, port_test_ddz, port_joy_ddz);
		break;

		case CFG_MGCS3:
			DrvUseHopper = 1;
			DrvUseSplitBank = 1;
			pDrvMainInCb = player_ddz;
			DrvControlsBank = 1;
			DrvControlsMask = 0x01;
			DrvControlsValue = 0x01;
			DrvPPIWrite[0][1] = ppi0_write_cjddz_b;
			DrvPPIWrite[0][2] = ppi0_write_cjddz_c;
			igs017_igs031_set_inputs(dsw1_sel10, port_test_ddz, port_joy_ddz);
		break;

		case CFG_CJTLJP:
			DrvUseHopper = 1;
			DrvUseSplitBank = 1;
			pDrvMainInCb = player_ddz;
			DrvControlsBank = 0;
			DrvControlsMask = 0x01;
			DrvControlsValue = 0x01;
			DrvPPIWrite[0][1] = ppi0_write_cjddz_b;
			DrvPPIWrite[0][2] = ppi0_write_cjddz_c;
			igs017_igs031_set_inputs(dsw1_sel10, port_test_ddz, port_joy_ddz);
		break;

		case CFG_LTHYP:
			DrvUseHopper = 1;
			DrvPPIRead[0][0] = port_test_mahjong;
			DrvPPIRead[0][1] = kbd_sel1_0_2;
			DrvPPIWrite[0][2] = ppi0_write_mahjong;
			igs017_igs031_set_inputs(dsw1_sel10, dsw2_sel10, port_joy_mahjong);
		break;

		case CFG_JKING02:
			pDrvMainInCb = player_jking02;
			DrvMainOutCb = main_out_oki_bank;
			DrvPPIWrite[0][0] = ppi0_write_jking02_a;
			DrvPPIWrite[0][1] = ppi0_write_jking02_b;
			DrvPPIWrite[0][2] = ppi0_write_jking02_c;
			igs017_igs031_set_inputs(dsw1_sel10, portb_jking02, portc_jking02);
		break;

		case CFG_TCT2P:
			DrvUseHopper = 1;
			DrvControlsBank = 1;
			DrvControlsMask = 0x01;
			DrvControlsValue = 0x01;
			pDrvMainInCb = player_tct2p;
			DrvMainOutCb = main_out_oki_bank;
			DrvPPIWrite[0][1] = ppi0_write_tct2p_b;
			DrvPPIWrite[0][2] = ppi0_write_tct2p_c;
			igs017_igs031_set_inputs(dsw1_sel10, portb_tct2p, portc_tct2p);
		break;

		case CFG_TSWXP:
			DrvUseHopper = 1;
			DrvControlsBank = 0;
			DrvControlsMask = 0x01;
			DrvControlsValue = 0x01;
			pDrvMainInCb = player_tswxp;
			DrvMainOutCb = main_out_oki_bank;
			DrvPPIWrite[0][1] = ppi0_write_tct2p_b;
			DrvPPIWrite[0][2] = ppi0_write_tct2p_c;
			igs017_igs031_set_inputs(dsw1_sel10, portb_tswxp, portc_tswxp);
		break;

		case CFG_MGZZ:
			DrvUseHopper = 1;
			DrvPPIWrite[0][0] = ppi0_write_mahjong;
			DrvPPIRead[0][1] = port_test_mahjong;
			DrvPPIRead[0][2] = kbd_sel1_0_2;
			igs017_igs031_set_inputs(dsw1_sel10, dsw2_sel10, port_joy_mahjong);
		break;

		case CFG_OCEANPAR:
			DrvUseHopper = 1;
			DrvUseTicket = 1;
			pDrvMainInCb = player_three_reel;
			DrvMainOutCb = main_out_oki_bank;
			DrvPPIWrite[0][0] = ppi0_write_oceanpar_a;
			DrvPPIWrite[0][1] = ppi0_write_oceanpar_b;
			DrvPPIWrite[0][2] = ppi0_write_oceanpar_c;
			igs017_igs031_set_inputs(dsw1_sel10, portb_three_reel, portc_three_reel);
		break;

		case CFG_ROYAL5P:
			DrvUseHopper = 1;
			pDrvMainInCb = player_royal5p;
			DrvMainOutCb = main_out_oki_bank;
			DrvPPIWrite[0][1] = ppi0_write_royal5p_b;
			DrvPPIWrite[0][2] = ppi0_write_royal5p_c;
			igs017_igs031_set_inputs(dsw1_sel10, portb_royal5p, portc_three_reel);
		break;

		case CFG_TRIPSLOT:
			DrvUseHopper = 1;
			pDrvMainInCb = player_tripslot;
			DrvMainOutCb = main_out_tripslot;
			DrvPPIWrite[0][0] = ppi0_write_tripslot_a;
			DrvPPIWrite[0][1] = ppi0_write_tripslot_b;
			DrvPPIWrite[0][2] = ppi0_write_tripslot_c;
			igs017_igs031_set_inputs(dsw1_sel10, portb_tripslot, portc_tripslot);
		break;

		case CFG_CCLY:
			DrvUseHopper = 1;
			DrvOkiClock = 2000000;
			pDrvMainInCb = player_ccly;
			DrvPPIWrite[0][1] = ppi0_write_ccly_b;
			DrvPPIWrite[0][2] = ppi0_write_ccly_c;
			igs017_igs031_set_inputs(dsw1_sel10, portb_ccly, portc_ccly);
		break;

		case CFG_CJSXP:
			DrvUseHopper = 1;
			pDrvMainInCb = player_cjsxp;
			DrvMainOutCb = main_out_oki_bank;
			DrvPPIWrite[0][1] = ppi0_write_cjsxp_b;
			DrvPPIRead[0][2] = portc_cjsxp;
			igs017_igs031_set_inputs(dsw1_sel10, portb_cjsxp, portc_cjsxp);
		break;

		case CFG_QLGS:
			pDrvMainInCb = player_qlgs;
			DrvMainOutCb = main_out_oki_bank;
			DrvPPIWrite[0][2] = ppi0_write_cjddz_c;
			igs017_igs031_set_inputs(dsw1_sel10, port_test_qlgs, port_joy_qlgs);
		break;
	}

	ppi8255_set_read_ports(0, DrvPPIRead[0][0], DrvPPIRead[0][1], DrvPPIRead[0][2]);
	ppi8255_set_write_ports(0, DrvPPIWrite[0][0], DrvPPIWrite[0][1], DrvPPIWrite[0][2]);

	if (DrvNumPpi > 1) {
		ppi8255_set_read_ports(1, DrvPPIRead[1][0], DrvPPIRead[1][1], DrvPPIRead[1][2]);
		ppi8255_set_write_ports(1, DrvPPIWrite[1][0], DrvPPIWrite[1][1], DrvPPIWrite[1][2]);
	}
}

static UINT32 drv_main_port_value()
{
	UINT32 port = pDrvMainInCb ? pDrvMainInCb() : 0x000fffff;
	return 0xff800000 | ((port & 0x000fffff) << 3) | 0x00000007;
}

static UINT8 __fastcall m027_default_read_byte(UINT32)
{
	return 0xff;
}

static UINT16 __fastcall m027_default_read_word(UINT32)
{
	return 0xffff;
}

static UINT32 __fastcall m027_default_read_long(UINT32)
{
	return 0xffffffff;
}

static UINT8 __fastcall m027_video_read_byte(UINT32 address)
{
	return igs017_igs031_read(address - 0x38000000);
}

static UINT16 __fastcall m027_video_read_word(UINT32 address)
{
	UINT32 base = address - 0x38000000;
	return igs017_igs031_read(base + 0) | (igs017_igs031_read(base + 1) << 8);
}

static UINT32 __fastcall m027_video_read_long(UINT32 address)
{
	UINT32 base = address - 0x38000000;
	return igs017_igs031_read(base + 0) | (igs017_igs031_read(base + 1) << 8) | (igs017_igs031_read(base + 2) << 16) | (igs017_igs031_read(base + 3) << 24);
}

static void __fastcall m027_video_write_byte(UINT32 address, UINT8 data)
{
	igs017_igs031_write(address - 0x38000000, data);
}

static void __fastcall m027_video_write_word(UINT32 address, UINT16 data)
{
	UINT32 base = address - 0x38000000;
	igs017_igs031_write(base + 0, data & 0xff);
	igs017_igs031_write(base + 1, (data >> 8) & 0xff);
}

static void __fastcall m027_video_write_long(UINT32 address, UINT32 data)
{
	UINT32 base = address - 0x38000000;
	igs017_igs031_write(base + 0, data & 0xff);
	igs017_igs031_write(base + 1, (data >> 8) & 0xff);
	igs017_igs031_write(base + 2, (data >> 16) & 0xff);
	igs017_igs031_write(base + 3, (data >> 24) & 0xff);
}

static inline void m027_write_380_misc(UINT8 low)
{
	if (DrvConfig == CFG_ROYAL5P) {
		DrvHopperMotor = (low >> 3) & 1;
	} else if (DrvConfig == CFG_TRIPSLOT) {
		DrvMisc = low;
		DrvIoSelect[0] = ((low & 0x20) ? 0x04 : 0x00) | (DrvIoSelect[0] & 0x03);
		drv_set_oki_full_bank(DrvIoSelect[0] & 0x07);
	} else if (DrvConfig == CFG_CCLY) {
		drv_set_oki_full_bank(low & 0x07);
	}
}

static UINT8 __fastcall m027_io_read_byte(UINT32 address)
{
	if (address >= 0x38008000 && address <= 0x38008003) {
		return MSM6295Read(0) & 0xff;
	}

	if (address >= 0x38009000 && address <= 0x38009003) {
		return ppi8255_r(0, address & 3);
	}

	if (address >= 0x3800a000 && address <= 0x3800a003 && DrvNumPpi > 1) {
		return ppi8255_r(1, address & 3);
	}

	return 0xff;
}

static UINT16 __fastcall m027_io_read_word(UINT32 address)
{
	if (address >= 0x38008000 && address <= 0x38008003) return 0xff00 | (MSM6295Read(0) & 0xff);
	if (address >= 0x38009000 && address <= 0x38009003) return 0xff00 | ppi8255_r(0, address & 3);
	if (address >= 0x3800a000 && address <= 0x3800a003 && DrvNumPpi > 1) return 0xff00 | ppi8255_r(1, address & 3);

	return 0xffff;
}

static UINT32 __fastcall m027_io_read_long(UINT32 address)
{
	if (address >= 0x38008000 && address <= 0x38008003) return 0xffffff00 | (MSM6295Read(0) & 0xff);
	if (address >= 0x38009000 && address <= 0x38009003) return 0xffffff00 | ppi8255_r(0, address & 3);
	if (address >= 0x3800a000 && address <= 0x3800a003 && DrvNumPpi > 1) return 0xffffff00 | ppi8255_r(1, address & 3);

	return 0xffffffff;
}

static void __fastcall m027_io_write_byte(UINT32 address, UINT8 data)
{
	if (address >= 0x38008000 && address <= 0x38008003) { MSM6295Write(0, data); return; }
	if (address >= 0x38009000 && address <= 0x38009003) { ppi8255_w(0, address & 3, data); return; }
	if (address >= 0x3800a000 && address <= 0x3800a003 && DrvNumPpi > 1) { ppi8255_w(1, address & 3, data); return; }
	if (address == 0x3800b000 && DrvUseSplitBank) {
		drv_set_oki_split_bank(0, (data >> 0) & 0x0f);
		drv_set_oki_split_bank(1, (data >> 4) & 0x0f);
		return;
	}
	if (address == 0x3800c000) m027_write_380_misc(data);
}

static void __fastcall m027_io_write_word(UINT32 address, UINT16 data)
{
	if (address >= 0x38008000 && address <= 0x38008003) { MSM6295Write(0, data & 0xff); return; }
	if (address >= 0x38009000 && address <= 0x38009003) { ppi8255_w(0, address & 3, data & 0xff); return; }
	if (address >= 0x3800a000 && address <= 0x3800a003 && DrvNumPpi > 1) { ppi8255_w(1, address & 3, data & 0xff); return; }
	if (address >= 0x3800b000 && address <= 0x3800b003 && DrvUseSplitBank) {
		drv_set_oki_split_bank(0, (data >> 0) & 0x0f);
		drv_set_oki_split_bank(1, (data >> 4) & 0x0f);
		return;
	}
	if (address >= 0x3800c000 && address <= 0x3800c003) m027_write_380_misc(data & 0xff);
}

static void __fastcall m027_io_write_long(UINT32 address, UINT32 data)
{
	if (address >= 0x38008000 && address <= 0x38008003) { MSM6295Write(0, data & 0xff); return; }
	if (address >= 0x38009000 && address <= 0x38009003) { ppi8255_w(0, address & 3, data & 0xff); return; }
	if (address >= 0x3800a000 && address <= 0x3800a003 && DrvNumPpi > 1) { ppi8255_w(1, address & 3, data & 0xff); return; }
	if (address >= 0x3800b000 && address <= 0x3800b003 && DrvUseSplitBank) {
		drv_set_oki_split_bank(0, (data >> 0) & 0x0f);
		drv_set_oki_split_bank(1, (data >> 4) & 0x0f);
		return;
	}
	if (address >= 0x3800c000 && address <= 0x3800c003) m027_write_380_misc(data & 0xff);
}

static UINT8 __fastcall m027_mainio_read_byte(UINT32 address)
{
	if (address >= 0x4000000c && address <= 0x4000000f) {
		return (drv_main_port_value() >> ((address & 3) * 8)) & 0xff;
	}

	return 0xff;
}

static UINT16 __fastcall m027_mainio_read_word(UINT32 address)
{
	if (address >= 0x4000000c && address <= 0x4000000e) {
		return (drv_main_port_value() >> ((address & 2) * 8)) & 0xffff;
	}

	return 0xffff;
}

static UINT32 __fastcall m027_mainio_read_long(UINT32 address)
{
	if (address == 0x4000000c) {
		return drv_main_port_value();
	}

	return 0xffffffff;
}

static void __fastcall m027_mainio_write_byte(UINT32 address, UINT8 data)
{
	if (address >= 0x40000014 && address <= 0x40000017) {
		igs027a_fiq_enable_w(data);
		return;
	}

	if (address >= 0x40000018 && address <= 0x4000001b && DrvMainOutCb) {
		DrvMainOutCb(data & 0x1f);
	}
}

static void __fastcall m027_mainio_write_word(UINT32 address, UINT16 data)
{
	if (address >= 0x40000014 && address <= 0x40000016) {
		igs027a_fiq_enable_w(data & 0xff);
		return;
	}

	if (address >= 0x40000018 && address <= 0x4000001a && DrvMainOutCb) {
		DrvMainOutCb(data & 0x1f);
	}
}

static void __fastcall m027_mainio_write_long(UINT32 address, UINT32 data)
{
	if (address >= 0x40000014 && address <= 0x40000017) {
		igs027a_fiq_enable_w(data & 0xff);
		return;
	}

	if (address >= 0x40000018 && address <= 0x4000001b && DrvMainOutCb) {
		DrvMainOutCb(data & 0x1f);
	}
}

static UINT8 __fastcall m027_xortab_read_byte(UINT32)
{
	return 0xff;
}

static UINT16 __fastcall m027_xortab_read_word(UINT32)
{
	return 0xffff;
}

static UINT32 __fastcall m027_xortab_read_long(UINT32)
{
	return 0xffffffff;
}

static void __fastcall m027_xortab_write_byte(UINT32 address, UINT8 data)
{
	UINT32 index = ((address - 0x50000000) >> 2) & 0xff;
	DrvXorTable[index] = data;
	drv_refresh_user_map();
}

static void __fastcall m027_xortab_write_word(UINT32 address, UINT16 data)
{
	UINT32 index = ((address - 0x50000000) >> 2) & 0xff;
	DrvXorTable[index] = data & 0xff;
	drv_refresh_user_map();
}

static void __fastcall m027_xortab_write_long(UINT32 address, UINT32 data)
{
	UINT32 index = ((address - 0x50000000) >> 2) & 0xff;
	DrvXorTable[index] = data & 0xff;
	drv_refresh_user_map();
}

static UINT8 __fastcall m027_timerirq_read_byte(UINT32 address)
{
	if (address >= 0x70000200 && address <= 0x70000203) {
		return igs027a_irq_pending_r();
	}

	return 0xff;
}

static UINT16 __fastcall m027_timerirq_read_word(UINT32 address)
{
	if (address >= 0x70000200 && address <= 0x70000202) {
		return 0xff00 | igs027a_irq_pending_r();
	}

	return 0xffff;
}

static UINT32 __fastcall m027_timerirq_read_long(UINT32 address)
{
	if (address == 0x70000200) {
		return 0xffffff00 | igs027a_irq_pending_r();
	}

	return 0xffffffff;
}

static void __fastcall m027_timerirq_write_byte(UINT32 address, UINT8 data)
{
	if (address >= 0x70000100 && address <= 0x70000107) {
		INT32 which = (address >> 2) & 1;
		igs027a_timer_w(which, data);
		return;
	}

	if (address >= 0x70000200 && address <= 0x70000203) {
		igs027a_irq_enable_w(data);
	}
}

static void __fastcall m027_timerirq_write_word(UINT32 address, UINT16 data)
{
	if (address >= 0x70000100 && address <= 0x70000106) {
		INT32 which = (address >> 2) & 1;
		igs027a_timer_w(which, data & 0xff);
		return;
	}

	if (address >= 0x70000200 && address <= 0x70000202) {
		igs027a_irq_enable_w(data & 0xff);
	}
}

static void __fastcall m027_timerirq_write_long(UINT32 address, UINT32 data)
{
	if (address >= 0x70000100 && address <= 0x70000104) {
		INT32 which = (address >> 2) & 1;
		igs027a_timer_w(which, data & 0xff);
		return;
	}

	if (address >= 0x70000200 && address <= 0x70000202) {
		igs027a_irq_enable_w(data & 0xff);
	}
}

static UINT8 m027_read_byte(UINT32 address)
{
	if (address >= 0x38000000 && address <= 0x38007fff) return m027_video_read_byte(address);
	if (address >= 0x38008000 && address <= 0x3800c003) return m027_io_read_byte(address);
	if (address >= 0x4000000c && address <= 0x4000001b) return m027_mainio_read_byte(address);
	if (address >= 0x50000000 && address <= 0x500003ff) return m027_xortab_read_byte(address);
	if (address >= 0x70000100 && address <= 0x70000203) return m027_timerirq_read_byte(address);

	return m027_default_read_byte(address);
}

static UINT16 m027_read_word(UINT32 address)
{
	if (address >= 0x38000000 && address <= 0x38007fff) return m027_video_read_word(address);
	if (address >= 0x38008000 && address <= 0x3800c003) return m027_io_read_word(address);
	if (address >= 0x4000000c && address <= 0x4000001b) return m027_mainio_read_word(address);
	if (address >= 0x50000000 && address <= 0x500003ff) return m027_xortab_read_word(address);
	if (address >= 0x70000100 && address <= 0x70000203) return m027_timerirq_read_word(address);

	return m027_default_read_word(address);
}

static UINT32 m027_read_long(UINT32 address)
{
	if (address >= 0x38000000 && address <= 0x38007fff) return m027_video_read_long(address);
	if (address >= 0x38008000 && address <= 0x3800c003) return m027_io_read_long(address);
	if (address >= 0x4000000c && address <= 0x4000001b) return m027_mainio_read_long(address);
	if (address >= 0x50000000 && address <= 0x500003ff) return m027_xortab_read_long(address);
	if (address >= 0x70000100 && address <= 0x70000203) return m027_timerirq_read_long(address);

	return m027_default_read_long(address);
}

static void m027_write_byte(UINT32 address, UINT8 data)
{
	if (address >= 0x38000000 && address <= 0x38007fff) { m027_video_write_byte(address, data); return; }
	if (address >= 0x38008000 && address <= 0x3800c003) { m027_io_write_byte(address, data); return; }
	if (address >= 0x4000000c && address <= 0x4000001b) { m027_mainio_write_byte(address, data); return; }
	if (address >= 0x50000000 && address <= 0x500003ff) { m027_xortab_write_byte(address, data); return; }
	if (address >= 0x70000100 && address <= 0x70000203) { m027_timerirq_write_byte(address, data); return; }
}

static void m027_write_word(UINT32 address, UINT16 data)
{
	if (address >= 0x38000000 && address <= 0x38007fff) { m027_video_write_word(address, data); return; }
	if (address >= 0x38008000 && address <= 0x3800c003) { m027_io_write_word(address, data); return; }
	if (address >= 0x4000000c && address <= 0x4000001b) { m027_mainio_write_word(address, data); return; }
	if (address >= 0x50000000 && address <= 0x500003ff) { m027_xortab_write_word(address, data); return; }
	if (address >= 0x70000100 && address <= 0x70000203) { m027_timerirq_write_word(address, data); return; }
}

static void m027_write_long(UINT32 address, UINT32 data)
{
	if (address >= 0x38000000 && address <= 0x38007fff) { m027_video_write_long(address, data); return; }
	if (address >= 0x38008000 && address <= 0x3800c003) { m027_io_write_long(address, data); return; }
	if (address >= 0x4000000c && address <= 0x4000001b) { m027_mainio_write_long(address, data); return; }
	if (address >= 0x50000000 && address <= 0x500003ff) { m027_xortab_write_long(address, data); return; }
	if (address >= 0x70000100 && address <= 0x70000203) { m027_timerirq_write_long(address, data); return; }
}

static INT32 drv_get_roms(bool bLoad)
{
	char *pRomName;
	BurnRomInfo ri;
	INT32 prgIndex = 0;
	INT32 graIndex = 0;

	UINT8 *armLoad = DrvArmROM;
	UINT8 *userLoad = DrvUserROM;
	UINT8 *tileLoad = DrvTileROM;
	UINT8 *spriteLoad = DrvSpriteROM;
	UINT8 *okiLoad = DrvOkiROM;

	for (INT32 i = 0; !BurnDrvGetRomName(&pRomName, i, 0); i++) {
		BurnDrvGetRomInfo(&ri, i);

		if (ri.nType & BRF_PRG) {
			if (prgIndex == 0) {
				if (bLoad) {
					if (BurnLoadRom(armLoad, i, 1)) return 1;
					armLoad += ri.nLen;
				} else {
					DrvArmLen += ri.nLen;
				}
			} else {
				if (bLoad) {
					if (BurnLoadRom(userLoad, i, 1)) return 1;
					userLoad += ri.nLen;
				} else {
					DrvUserLen += ri.nLen;
				}
			}
			prgIndex++;
			continue;
		}

		if (ri.nType & BRF_GRA) {
			if (graIndex == 0) {
				if (bLoad) {
					if (BurnLoadRom(tileLoad, i, 1)) return 1;
					if (ri.nType & M027_WORD_SWAP) {
						BurnByteswap(tileLoad, ri.nLen);
					}
					tileLoad += ri.nLen;
				} else {
					DrvTileLen += ri.nLen;
				}
			} else {
				if (bLoad) {
					if (BurnLoadRom(spriteLoad, i, 1)) return 1;
					spriteLoad += ri.nLen;
				} else {
					DrvSpriteLen += ri.nLen;
				}
			}
			graIndex++;
			continue;
		}

		if (ri.nType & BRF_SND) {
			if (bLoad) {
				if (BurnLoadRom(okiLoad, i, 1)) return 1;
				okiLoad += ri.nLen;
			} else {
				DrvOkiLen += ri.nLen;
			}
		}
	}

	return 0;
}

static INT32 MemIndex()
{
	UINT8 *Next = AllMem;

	DrvArmROM = Next; Next += DrvArmLen;
	DrvUserROM = Next; Next += DrvUserLen;
	DrvUserMap = Next; Next += DrvUserLen;
	DrvTileROM = Next; Next += DrvTileLen;
	DrvSpriteROM = Next; Next += DrvSpriteLen;
	DrvOkiROM = Next; Next += DrvOkiLen;

	AllRam = Next;
	DrvNVRAM = Next; Next += 0x8000;
	DrvArmRAM = Next; Next += 0x0400;

	RamEnd = Next;
	MemEnd = Next;

	return 0;
}

static void drv_refresh_user_map()
{
	if (DrvUserROM == NULL || DrvUserMap == NULL) return;

	memcpy(DrvUserMap, DrvUserROM, DrvUserLen);

	for (INT32 i = 0; i + 3 < DrvUserLen; i += 4) {
		UINT8 x = DrvXorTable[(i >> 2) & 0xff];
		DrvUserMap[i + 1] ^= x;
		DrvUserMap[i + 3] ^= x;
	}
}

static INT32 DrvDoReset()
{
	DrvIoSelect[0] = 0xff;
	DrvIoSelect[1] = 0xff;
	igs027a_reset();
	DrvHopperMotor = 0;
	DrvTicketMotor = 0;
	DrvMisc = 0;
	DrvCoinPulse[0] = DrvCoinPulse[1] = 0;
	DrvCoinPrev[0] = DrvCoinPrev[1] = 0;
	nCyclesDone[0] = 0;

	memset(DrvNVRAM, 0, 0x8000);

	Arm7Open(0);
	Arm7Reset();
	Arm7Close();

	ppi8255_exit();
	ppi8255_init(DrvNumPpi);
	drv_apply_config(DrvConfig);

	MSM6295Reset(0);
	if (DrvUseSplitBank) {
		drv_set_oki_split_bank(0, 0);
		drv_set_oki_split_bank(1, 0);
	} else {
		drv_set_oki_full_bank(0);
	}

	igs017_igs031_reset();

	return 0;
}

static void gfx_none() { }
static void gfx_slqz3() { igs017_igs031_set_text_reverse_bits(0); }
static void gfx_zhongguo() { igs017_igs031_set_text_reverse_bits(0); }
static void gfx_mgfx() { igs017_igs031_set_text_reverse_bits(0); }

static void gfx_sdwx_tarzan0()
{
	igs017_igs031_sdwx_gfx_decrypt(DrvTileROM, DrvTileLen);
	igs017_igs031_tarzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0, 0);
}

static void gfx_sdwx_tarzan400()
{
	igs017_igs031_sdwx_gfx_decrypt(DrvTileROM, DrvTileLen);
	igs017_igs031_tarzan_decrypt_sprites(DrvSpriteROM, DrvSpriteLen, 0x400000, 0x400000);
}

static void m027_cjddz_bitswap_user()
{
	UINT8* buffer = (UINT8*)BurnMalloc(0x100000);
	if (buffer == NULL) return;

	memcpy(buffer, DrvUserROM, 0x100000);

	for (INT32 i = 0; i < 0x100000; i++) {
		DrvUserROM[i] = buffer[BITSWAP24(i, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 11, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)];
	}

	BurnFree(buffer);
}

static void m027_cjddzs_prepare_user()
{
	UINT8* raw = (UINT8*)BurnMalloc(DrvUserLen);
	if (raw == NULL) return;

	memcpy(raw, DrvUserROM, DrvUserLen);
	memset(DrvUserROM, 0xff, DrvUserLen);

	if (DrvUserLen >= 0x180000) {
		memcpy(DrvUserROM + 0x000000, raw + 0x000000, 0x080000);
		memcpy(DrvUserROM + 0x080000, raw + 0x100000, 0x080000);
	}

	BurnFree(raw);
}

static void m027_cjddzsa_prepare_user()
{
	UINT8* raw = (UINT8*)BurnMalloc(DrvUserLen);
	if (raw == NULL) return;

	memcpy(raw, DrvUserROM, DrvUserLen);
	memset(DrvUserROM, 0xff, DrvUserLen);

	if (DrvUserLen >= 0x300000) {
		memcpy(DrvUserROM, raw + 0x200000, 0x100000);
	}

	BurnFree(raw);
}

static void m027_cjddzps_prepare_user()
{
	UINT8* raw = (UINT8*)BurnMalloc(DrvUserLen);
	if (raw == NULL) return;

	memcpy(raw, DrvUserROM, DrvUserLen);
	memset(DrvUserROM, 0xff, DrvUserLen);

	if (DrvUserLen >= 0x300000) {
		memcpy(DrvUserROM, raw + 0x200000, 0x100000);
	}

	BurnFree(raw);
}

static void cjddzs_decrypt()
{
	m027_cjddzs_prepare_user();
	DrvUserLen = 0x100000;
	m027_cjddz_bitswap_user();
	drv_crypt_set_region(DrvUserROM, DrvUserLen);
	cjddz_decrypt();
}

static void cjddzsa_decrypt()
{
	m027_cjddzsa_prepare_user();
	DrvUserLen = 0x100000;
	m027_cjddz_bitswap_user();
	drv_crypt_set_region(DrvUserROM, DrvUserLen);
	cjddz_decrypt();
}

static void cjddzps_decrypt()
{
	m027_cjddzps_prepare_user();
	DrvUserLen = 0x100000;
	m027_cjddz_bitswap_user();
	drv_crypt_set_region(DrvUserROM, DrvUserLen);
	cjddzp_decrypt();
}

static void post_tswxp()
{
	memcpy(DrvSpriteROM + 0x200000, DrvSpriteROM, 0x200000);
}

static INT32 common_init(INT32 config, void (*decrypt)(), void (*gfx_setup)(), void (*postload)(), INT32 oki_clock, INT32 oki_divisor)
{
	AllMem = NULL;

	DrvConfig = config;
	pDrvDecryptCb = decrypt;
	pDrvGfxSetupCb = gfx_setup;
	pDrvPostloadCb = postload;
	DrvOkiClock = oki_clock;
	DrvOkiDivisor = oki_divisor;

	drv_get_roms(0);

	MemIndex();
	INT32 nLen = MemEnd - (UINT8*)0;
	AllMem = (UINT8*)BurnMalloc(nLen);
	if (AllMem == NULL) return 1;
	memset(AllMem, 0, nLen);

	MemIndex();

	drv_get_roms(true);

	drv_crypt_set_region(DrvUserROM, DrvUserLen);
	if (pDrvDecryptCb) pDrvDecryptCb();
	if (pDrvPostloadCb) pDrvPostloadCb();

	igs017_igs031_set_text_reverse_bits(1);
	if (pDrvGfxSetupCb) pDrvGfxSetupCb();
	if (igs017_igs031_init(DrvTileROM, DrvTileLen, DrvSpriteROM, DrvSpriteLen)) return 1;

	Arm7Init(0);
	igs027a_init();
	Arm7Open(0);
	Arm7SetThumbStackAlignFix(1);
	Arm7MapMemory(DrvArmROM, 0x00000000, DrvArmLen - 1, MAP_ROM);
	Arm7MapMemory(DrvUserMap, 0x08000000, 0x08000000 + DrvUserLen - 1, MAP_ROM);
	Arm7MapMemory(DrvArmRAM, 0x10000000, 0x100003ff, MAP_RAM);
	for (UINT32 i = 0; i < 0x100000; i += 0x8000) { // NVRAM + Mirrors
		Arm7MapMemory(DrvNVRAM, 0x18000000 | i, 0x18007fff | i, MAP_RAM);
	}

	Arm7SetReadByteHandler(m027_read_byte);
	Arm7SetReadWordHandler(m027_read_word);
	Arm7SetReadLongHandler(m027_read_long);
	Arm7SetWriteByteHandler(m027_write_byte);
	Arm7SetWriteWordHandler(m027_write_word);
	Arm7SetWriteLongHandler(m027_write_long);
	Arm7Close();

	ppi8255_init(DrvNumPpi);
	drv_apply_config(DrvConfig);
	MSM6295ROM = DrvOkiROM;
	MSM6295Init(0, DrvOkiClock / DrvOkiDivisor, 0);
	MSM6295SetRoute(0, 1.00, BURN_SND_ROUTE_BOTH);

	if (DrvUseSplitBank) {
		drv_set_oki_split_bank(0, 0);
		drv_set_oki_split_bank(1, 0);
	}
	else {
		drv_set_oki_full_bank(0);
	}

	DrvDoReset();

	return 0;
}

INT32 slqz3Init()    { return common_init(CFG_SLQZ3,   slqz3_decrypt,    gfx_slqz3,          NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 lhdmgInit()    { return common_init(CFG_LHDMG,   slqz3_decrypt,    gfx_slqz3,          NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 lhzb3sjbInit() { return common_init(CFG_LHZB3SJB,slqz3_decrypt,    gfx_slqz3,          NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 lhzb3106Init() { return common_init(CFG_LHDMG106,slqz3_decrypt,    gfx_slqz3,          NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 qlgsInit()     { return common_init(CFG_QLGS,    qlgs_decrypt,     gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 lhzb4Init()    { return common_init(CFG_LHZB4,   lhzb4_decrypt,    gfx_sdwx_tarzan0,   NULL,       2000000, MSM6295_PIN7_HIGH); }
INT32 cjddzInit()    { return common_init(CFG_CJDDZ,   cjddz_decrypt,    gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 cjddzsInit()   { return common_init(CFG_CJDDZ,   cjddzs_decrypt,   gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 cjddzsaInit()  { return common_init(CFG_CJDDZ,   cjddzsa_decrypt,  gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 cjddzpInit()
{
	INT32 ret = common_init(CFG_CJDDZ, cjddzp_decrypt, gfx_sdwx_tarzan400, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 1;
		DrvControlsMask = 0x02;
		DrvControlsValue = 0x02;
	}
	return ret;
}
INT32 cjddzpsInit()
{
	INT32 ret = common_init(CFG_CJDDZ, cjddzps_decrypt, gfx_sdwx_tarzan400, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 1;
		DrvControlsMask = 0x02;
		DrvControlsValue = 0x02;
	}
	return ret;
}
INT32 cjddzlfInit()  { return common_init(CFG_CJDDZ,   cjddzlf_decrypt,  gfx_sdwx_tarzan400, NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 cjtljpInit()   { return common_init(CFG_CJTLJP,  cjtljp_decrypt,   gfx_sdwx_tarzan0,   NULL,       2000000, MSM6295_PIN7_LOW); }
INT32 lthypInit()
{
	INT32 ret = common_init(CFG_LTHYP, slqz3_decrypt, gfx_slqz3, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 1;
		DrvControlsMask = 0x01;
		DrvControlsValue = 0x00;
	}
	return ret;
}
INT32 zhongguoInit()
{
	INT32 ret = common_init(CFG_LTHYP, zhongguo_decrypt, gfx_zhongguo, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 1;
		DrvControlsMask = 0x01;
		DrvControlsValue = 0x00;
	}
	return ret;
}
INT32 tct2pInit()    { return common_init(CFG_TCT2P,   tct2p_decrypt,    gfx_sdwx_tarzan400, NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 mgzzInit()
{
	INT32 ret = common_init(CFG_MGZZ, mgzz_decrypt, gfx_mgfx, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 0;
		DrvControlsMask = 0x02;
		DrvControlsValue = 0x00;
	}
	return ret;
}
INT32 mgcs3Init()    { return common_init(CFG_MGCS3,   mgcs3_decrypt,    gfx_sdwx_tarzan0,   NULL,       2000000, MSM6295_PIN7_HIGH); }
INT32 jking02Init()  { return common_init(CFG_JKING02, zhongguo_decrypt, gfx_sdwx_tarzan400, NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 klxyjInit()
{
	INT32 ret = common_init(CFG_CJSXP, slqz3_decrypt, gfx_sdwx_tarzan0, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 1;
		DrvControlsMask = 0x01;
		DrvControlsValue = 0x01;
	}
	return ret;
}
INT32 xypdkInit()    { return common_init(CFG_CJDDZ,   xypdk_decrypt,    gfx_sdwx_tarzan0,   NULL,       2000000, MSM6295_PIN7_LOW); }
INT32 oceanparInit() { return common_init(CFG_OCEANPAR,oceanpar_decrypt, gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 fruitparInit() { return common_init(CFG_OCEANPAR,fruitpar_decrypt, gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 tripslotInit() { return common_init(CFG_TRIPSLOT,tripslot_decrypt, gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 cclyInit()     { return common_init(CFG_CCLY,    crzybugs_decrypt, gfx_sdwx_tarzan0,   NULL,       2000000, MSM6295_PIN7_HIGH); }
INT32 tswxpInit()    { return common_init(CFG_TSWXP,   tswxp_decrypt,    gfx_sdwx_tarzan0,   post_tswxp, 1000000, MSM6295_PIN7_HIGH); }
INT32 royal5pInit()  { return common_init(CFG_ROYAL5P, royal5p_decrypt,  gfx_sdwx_tarzan0,   NULL,       1000000, MSM6295_PIN7_HIGH); }
INT32 mgfxInit()
{
	INT32 ret = common_init(CFG_MGZZ, mgfx_decrypt, gfx_mgfx, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 1;
		DrvControlsMask = 0x02;
		DrvControlsValue = 0x00;
	}
	return ret;
}
INT32 tshsInit()
{
	INT32 ret = common_init(CFG_LTHYP, slqz3_decrypt, gfx_slqz3, NULL, 1000000, MSM6295_PIN7_HIGH);
	if (ret == 0) {
		DrvControlsBank = 0;
		DrvControlsMask = 0x04;
		DrvControlsValue = 0x04;
	}
	return ret;
}

static INT32 DrvExit()
{
	igs017_igs031_exit();
	igs027a_exit();
	Arm7Exit();
	MSM6295Exit(0);
	ppi8255_exit();

	BurnFree(AllMem);

	DrvArmROM = NULL;
	DrvUserROM = NULL;
	DrvUserMap = NULL;
	DrvTileROM = NULL;
	DrvSpriteROM = NULL;
	DrvOkiROM = NULL;

	DrvArmLen = 0;
	DrvUserLen = 0;
	DrvTileLen = 0;
	DrvSpriteLen = 0;
	DrvOkiLen = 0;

	pDrvMainInCb = NULL;
	pDrvPostloadCb = NULL;
	pDrvDecryptCb = NULL;
	pDrvGfxSetupCb = NULL;

	nCyclesDone[0] = 0;
	nCyclesTotal[0] = 0;

	DrvIoSelect[0] = 0xff;
	DrvIoSelect[1] = 0xff;
	DrvHopperMotor = 0;
	DrvTicketMotor = 0;
	DrvMisc = 0;
	DrvCoinPulse[0] = DrvCoinPulse[1] = 0;
	DrvCoinPrev[0] = DrvCoinPrev[1] = 0;

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
	if (DrvReset) DrvDoReset();

	Arm7NewFrame();
	drv_make_inputs();

	INT32 nInterleave = 256;
	INT32 nPrevCycles = 0;
	nCyclesTotal[0] = IGSM027_CYCS_PER_FRAME;

	for (INT32 i = 0; i < nInterleave; i++) {
		CPU_RUN(0, Arm7);

		INT32 executed = Arm7TotalCycles() - nPrevCycles;
		nPrevCycles = Arm7TotalCycles();
		igs027a_timer_update(executed);

		if (i == nScreenHeight && igs017_igs031_irq_enable()) {
			igs027a_set_pending_irq(3);
		}

		if (i == 0 && igs017_igs031_nmi_enable()) {
			igs027a_external_fiq(1);
			igs027a_external_fiq(0);
		}
	}

	nCyclesDone[0] = Arm7TotalCycles() - nCyclesTotal[0];

	if (pBurnSoundOut) {
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

	if (pnMin) *pnMin = 0x029743;

	DrvRecalc = 1;

	if (nAction & ACB_MEMORY_RAM) {
		ba.Data = DrvArmRAM;
		ba.nLen = 0x0400;
		ba.nAddress = 0x10000000;
		ba.szName = "ARM RAM";
		BurnAcb(&ba);

		ba.Data = DrvNVRAM;
		ba.nLen = 0x8000;
		ba.nAddress = 0x18000000;
		ba.szName = "NVRAM";
		BurnAcb(&ba);
	}

	Arm7Scan(nAction);
	MSM6295Scan(nAction, pnMin);
	ppi8255_scan();
	igs017_igs031_scan(nAction, pnMin);
	igs027a_scan(nAction);

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(DrvXorTable);
		SCAN_VAR(DrvIoSelect);
		SCAN_VAR(DrvHopperMotor);
		SCAN_VAR(DrvTicketMotor);
		SCAN_VAR(DrvMisc);
		SCAN_VAR(DrvCoinPulse);
		SCAN_VAR(DrvCoinPrev);
	}

	if ((nAction & ACB_DRIVER_DATA) && (nAction & ACB_WRITE)) {
		drv_refresh_user_map();
	}

	return 0;
}

static struct BurnRomInfo jking02RomDesc[] = {
	{ "j6_027a.bin",                 0x004000, 0x69e241f0, 1 | BRF_PRG | BRF_ESS },
	{ "j_k_2002_v-209us.u23",        0x080000, 0xef6b652b, 1 | BRF_PRG | BRF_ESS },
	{ "jungle_king_02_u12_text.u12", 0x080000, 0x22dcebd0, 2 | BRF_GRA           },
	{ "jungle_king_02_u13_a4202.u13",0x400000, 0x97a68f85, 3 | BRF_GRA           },
	{ "igs_w4201_speech_v103.u28",   0x200000, 0xfb72d4b5, 4 | BRF_SND           },
};

STD_ROM_PICK(jking02)
STD_ROM_FN(jking02)

static struct BurnRomInfo slqz3RomDesc[] = {
	{ "s11_027a.bin", 0x004000, 0xabb8ef8b, 1 | BRF_PRG | BRF_ESS },
	{ "u29",          0x200000, 0x215fed1e, 1 | BRF_PRG | BRF_ESS },
	{ "u9",           0x080000, 0xa82398a9, 2 | BRF_GRA           },
	{ "u18",          0x400000, 0x81428f18, 3 | BRF_GRA           },
	{ "u26",          0x200000, 0x84bc2f3e, 4 | BRF_SND           },
};

STD_ROM_PICK(slqz3)
STD_ROM_FN(slqz3)

static struct BurnRomInfo lhdmgRomDesc[] = {
	{ "b6_igs027a",  0x004000, 0x75645f8c, 1 | BRF_PRG | BRF_ESS },
	{ "lhdmg_prg.u9",0x080000, 0x3b3a77ac, 1 | BRF_PRG | BRF_ESS },
	{ "m2403.u17",   0x080000, 0xa82398a9, 2 | BRF_GRA           },
	{ "m2401.u18",   0x400000, 0x81428f18, 3 | BRF_GRA           },
	{ "s2402.u14",   0x100000, 0x56083fe2, 4 | BRF_SND           },
};

STD_ROM_PICK(lhdmg)
STD_ROM_FN(lhdmg)

static struct BurnRomInfo lhdmgpRomDesc[] = {
	{ "b4_igs027a",       0x004000, 0x6fd48959, 1 | BRF_PRG | BRF_ESS },
	{ "lhdmg_plus_prg.u9",0x080000, 0x77dd7855, 1 | BRF_PRG | BRF_ESS },
	{ "m2403.u17",        0x080000, 0xa82398a9, 2 | BRF_GRA           },
	{ "m2401.u18",        0x400000, 0x81428f18, 3 | BRF_GRA           },
	{ "s2402.u14",        0x100000, 0x56083fe2, 4 | BRF_SND           },
};

STD_ROM_PICK(lhdmgp)
STD_ROM_FN(lhdmgp)

static struct BurnRomInfo lhdmgp200c3mRomDesc[] = {
	{ "b4_igs027a",       0x004000, 0x75645f8c, 1 | BRF_PRG | BRF_ESS },
	{ "lhdmg_plus_prg.u9",0x080000, 0xc94cd4ea, 1 | BRF_PRG | BRF_ESS },
	{ "m2403.u17",        0x080000, 0xa82398a9, 2 | BRF_GRA           },
	{ "m2401.u18",        0x400000, 0x81428f18, 3 | BRF_GRA           },
	{ "s2402.u14",        0x100000, 0x56083fe2, 4 | BRF_SND           },
};

STD_ROM_PICK(lhdmgp200c3m)
STD_ROM_FN(lhdmgp200c3m)

static struct BurnRomInfo lhzb3RomDesc[] = {
	{ "b6_igs027a",   0x004000, 0x75645f8c, 1 | BRF_PRG | BRF_ESS },
	{ "lhzb3_104.u9", 0x080000, 0x70d61846, 1 | BRF_PRG | BRF_ESS },
	{ "lhzb3_text.u17",0x080000,0xa82398a9, 2 | BRF_GRA           },
	{ "m2401.u18",    0x400000, 0x81428f18, 3 | BRF_GRA           },
	{ "s2402.u14",    0x100000, 0x56083fe2, 4 | BRF_SND           },
};

STD_ROM_PICK(lhzb3)
STD_ROM_FN(lhzb3)

static struct BurnRomInfo lhzb3106c5mRomDesc[] = {
	{ "u10_027a.bin", 0x004000, 0x19df9d4f, 1 | BRF_PRG | BRF_ESS },
	{ "106c5n.u27",   0x200000, 0x86031487, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m2403.u17",0x080000, 0xa82398a9, 2 | BRF_GRA           },
	{ "igs_m2401.u29",0x400000, 0x81428f18, 3 | BRF_GRA           },
	{ "igs_s2402.u26",0x200000, 0x84bc2f3e, 4 | BRF_SND           },
};

STD_ROM_PICK(lhzb3106c5m)
STD_ROM_FN(lhzb3106c5m)

static struct BurnRomInfo lthypRomDesc[] = {
	{ "d6_igs027a",  0x004000, 0xb29ee32b, 1 | BRF_PRG | BRF_ESS },
	{ "27c4096.u10", 0x080000, 0xbd04f2e9, 1 | BRF_PRG | BRF_ESS },
	{ "27c4096.u9",  0x080000, 0xa82398a9, 2 | BRF_GRA           },
	{ "m2401.u17",   0x400000, 0x81428f18, 3 | BRF_GRA           },
	{ "s2402.u14",   0x100000, 0x56083fe2, 4 | BRF_SND           },
};

STD_ROM_PICK(lthyp)
STD_ROM_FN(lthyp)

static struct BurnRomInfo zhongguoRomDesc[] = {
	{ "j8_igs027a", 0x004000, 0x95c51462, 1 | BRF_PRG | BRF_ESS },
	{ "p2600.u10",  0x080000, 0x9ad34135, 1 | BRF_PRG | BRF_ESS },
	{ "t2604.u9",   0x080000, 0x5401a52d, 2 | BRF_GRA           },
	{ "m2601.u17",  0x400000, 0x89736e3f, 3 | BRF_GRA           },
	{ "m2603.u18",  0x080000, 0xfb2e91a8, 3 | BRF_GRA           },
	{ "s2602.u14",  0x100000, 0xf137028c, 4 | BRF_SND           },
};

STD_ROM_PICK(zhongguo)
STD_ROM_FN(zhongguo)

static struct BurnRomInfo tshsRomDesc[] = {
	{ "s2_027a.u23",             0x004000, 0x1755418b, 1 | BRF_PRG | BRF_ESS },
	{ "v102cn.u10",              0x080000, 0x11e69b48, 1 | BRF_PRG | BRF_ESS },
	{ "igs_t3702_text_v100.u9",  0x080000, 0x07294995, 2 | BRF_GRA           },
	{ "igs_a3701_anim_v100.u17", 0x400000, 0xbed56f35, 3 | BRF_GRA           },
	{ "u18",                     0x080000, 0xfb2e91a8, 3 | BRF_GRA           },
	{ "v102cn.u14",              0x080000, 0xfc055df1, 4 | BRF_SND           },
};

STD_ROM_PICK(tshs)
STD_ROM_FN(tshs)

static struct BurnRomInfo qlgsRomDesc[] = {
	{ "c3_igs027a.bin", 0x004000, 0x7b107594, 1 | BRF_PRG | BRF_ESS },
	{ "s-501cn.u17",    0x200000, 0xc80b61c0, 1 | BRF_PRG | BRF_ESS },
	{ "text_u26.u26",   0x200000, 0x4cd44ba8, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "cg_u28.u28",     0x400000, 0xb34e22a0, 3 | BRF_GRA           },
	{ "sp_u5.u5",       0x200000, 0x6049b892, 4 | BRF_SND           },
};

STD_ROM_PICK(qlgs)
STD_ROM_FN(qlgs)

static struct BurnRomInfo cclyRomDesc[] = {
	{ "b5_027a.u30", 0x004000, 0x5007bbf1, 1 | BRF_PRG | BRF_ESS },
	{ "rom.u21",     0x080000, 0xf5fd7279, 1 | BRF_PRG | BRF_ESS },
	{ "rom.u9",      0x080000, 0x776f198c, 2 | BRF_GRA           },
	{ "rom.u8",      0x400000, 0x5b9863ba, 3 | BRF_GRA           },
	{ "rom.u18",     0x200000, 0x2cbfc5aa, 4 | BRF_SND           },
};

STD_ROM_PICK(ccly)
STD_ROM_FN(ccly)

static struct BurnRomInfo cjsxpRomDesc[] = {
	{ "s6_027a.u26", 0x004000, 0x56749e61, 1 | BRF_PRG | BRF_ESS },
	{ "v103cn.u18",  0x080000, 0x9fb75727, 1 | BRF_PRG | BRF_ESS },
	{ "text.u15",    0x080000, 0x3ac39fa3, 2 | BRF_GRA           },
	{ "cg.u14",      0x200000, 0xa6b52a44, 3 | BRF_GRA           },
	{ "sp.u17",      0x080000, 0xc7d10e13, 4 | BRF_SND           },
};

STD_ROM_PICK(cjsxp)
STD_ROM_FN(cjsxp)

static struct BurnRomInfo oceanparRomDesc[] = {
	{ "b1_027a.bin",               0x004000, 0xe64a01a0, 1 | BRF_PRG | BRF_ESS },
	{ "ocean_paradise_v105us.u23", 0x080000, 0xe6eb66c3, 1 | BRF_PRG | BRF_ESS },
	{ "ocean_paradise_text.u12",   0x080000, 0xbdaa4407, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "igs_m4101.u13",             0x400000, 0x84899398, 3 | BRF_GRA           },
	{ "igs_w4102.u28",             0x080000, 0x558cab25, 4 | BRF_SND           },
};

STD_ROM_PICK(oceanpar)
STD_ROM_FN(oceanpar)

static struct BurnRomInfo oceanpar101usRomDesc[] = {
	{ "b1_027a.bin",               0x004000, 0xe64a01a0, 1 | BRF_PRG | BRF_ESS },
	{ "ocean_paradise_v101us.u23", 0x080000, 0x4f2bf87a, 1 | BRF_PRG | BRF_ESS },
	{ "ocean_paradise_text.u12",   0x080000, 0xbdaa4407, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "igs_m4101.u13",             0x400000, 0x84899398, 3 | BRF_GRA           },
	{ "igs_w4102.u28",             0x080000, 0x558cab25, 4 | BRF_SND           },
};

STD_ROM_PICK(oceanpar101us)
STD_ROM_FN(oceanpar101us)

static struct BurnRomInfo gonefshRomDesc[] = {
	{ "b1_027a.bin",               0x004000, 0xe64a01a0, 1 | BRF_PRG | BRF_ESS },
	{ "gonefishin_v-602us.u23",    0x080000, 0xf47b673c, 1 | BRF_PRG | BRF_ESS },
	{ "gonefishin_text.u12",       0x080000, 0xa0ece4b7, 2 | BRF_GRA           },
	{ "gonefishin_cg1_u13.u13",    0x200000, 0x9e6cea86, 3 | BRF_GRA           },
	{ "gonefishin_cg2_u11.u11",    0x080000, 0x3baeb7e6, 3 | BRF_GRA           },
	{ "gonefishin_sp.u28",         0x080000, 0xe8db1fd1, 4 | BRF_SND           },
};

STD_ROM_PICK(gonefsh)
STD_ROM_FN(gonefsh)

static struct BurnRomInfo luckycrsRomDesc[] = {
	{ "v21_igs027a",               0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "luckycross_v-106sa.u23",    0x080000, 0x5716de00, 1 | BRF_PRG | BRF_ESS              },
	{ "luckycross_text_u12.u12",   0x080000, 0xc03aa300, 2 | BRF_GRA                        },
	{ "igs_k4101_u13.u13",         0x400000, 0x84899398, 3 | BRF_GRA                        },
	{ "luckycross_ext_cg_u11.u11", 0x080000, 0x997cb3cb, 3 | BRF_GRA                        },
	{ "igs_w4102_u37.u37",         0x200000, 0x114b44ee, 4 | BRF_SND                        },
};

STD_ROM_PICK(luckycrs)
STD_ROM_FN(luckycrs)

static struct BurnRomInfo amazoniaRomDesc[] = {
	{ "amazonia_igs027a",    0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "amazonia_v-104br.u23",0x080000, 0x103d465e, 1 | BRF_PRG | BRF_ESS              },
	{ "igs_t2105_cg_v110.u12",0x080000,0x1d4be260, 2 | BRF_GRA                        },
	{ "igs_a2107_cg_v110.u13",0x400000,0xd8dadfd7, 3 | BRF_GRA                        },
	{ "amazonia_cg.u11",      0x080000, 0x2ac2cfd1, 3 | BRF_GRA                        },
	{ "igs_s2102.u28",        0x080000, 0x90dda82d, 4 | BRF_SND                        },
};

STD_ROM_PICK(amazonia)
STD_ROM_FN(amazonia)

static struct BurnRomInfo amazonkpRomDesc[] = {
	{ "amazonia_igs027a", 0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "ak_plus_v-204br.u23", 0x080000, 0xe71f6272, 1 | BRF_PRG | BRF_ESS              },
	{ "igs_t2105.u12",       0x080000, 0x1d4be260, 2 | BRF_GRA                        },
	{ "igs_a2107.u13",       0x400000, 0xd8dadfd7, 3 | BRF_GRA                        },
	{ "ak_plus_ext_cg.u11",  0x080000, 0x26796bc0, 3 | BRF_GRA                        },
	{ "igs_s2102.u37",       0x080000, 0x90dda82d, 4 | BRF_SND                        },
};

STD_ROM_PICK(amazonkp)
STD_ROM_FN(amazonkp)

static struct BurnRomInfo amazoni2RomDesc[] = {
	{ "p9_igs027a",               0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "27c4096_akii_b-202br.u23", 0x080000, 0x7147b43c, 1 | BRF_PRG | BRF_ESS              },
	{ "akii_text.u12",            0x080000, 0x60b415ac, 2 | BRF_GRA                        },
	{ "u13_27c160_akii_cg.u13",   0x200000, 0x254bd84f, 3 | BRF_GRA                        },
	{ "akii_sp.u28",              0x080000, 0x216b5418, 4 | BRF_SND                        },
};

STD_ROM_PICK(amazoni2)
STD_ROM_FN(amazoni2)

static struct BurnRomInfo olympic5RomDesc[] = {
	{ "o2_igs027a",          0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "olympic_5_v-112us.u23",0x080000,0x27743107, 1 | BRF_PRG | BRF_ESS              },
	{ "olympic_5_text.u12",  0x080000, 0x60b415ac, 2 | BRF_GRA                        },
	{ "olympic_5_cg.u13",    0x200000, 0x6803f95b, 3 | BRF_GRA                        },
	{ "olympic_5_sp.u28",    0x200000, 0x7a2b5441, 4 | BRF_SND                        },
};

STD_ROM_PICK(olympic5)
STD_ROM_FN(olympic5)

static struct BurnRomInfo olympic5107usRomDesc[] = {
	{ "o2_igs027a",          0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "olympic_5_v-107us.u23",0x080000,0x3bcd4dd9, 1 | BRF_PRG | BRF_ESS              },
	{ "olympic_5_text.u12",  0x080000, 0x60b415ac, 2 | BRF_GRA                        },
	{ "olympic_5_cg.u13",    0x200000, 0x6803f95b, 3 | BRF_GRA                        },
	{ "olympic_5_sp.u28",    0x080000, 0x216b5418, 4 | BRF_SND                        },
};

STD_ROM_PICK(olympic5107us)
STD_ROM_FN(olympic5107us)

static struct BurnRomInfo extradrwRomDesc[] = {
	{ "e1_027a.bin",          0x004000, 0xebbf4922, 1 | BRF_PRG | BRF_ESS },
	{ "extradraw_v100ve.u21", 0x040000, 0xd83c1975, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m3004.u4",         0x080000, 0xd161f8f7, 2 | BRF_GRA           },
	{ "igs_m3001.u12",        0x100000, 0x642247fb, 3 | BRF_GRA           },
	{ "h2_and_cg.u3",         0x080000, 0x97227767, 3 | BRF_GRA           },
	{ "igs s3002.u18",        0x200000, 0x48601c32, 4 | BRF_SND           },
};

STD_ROM_PICK(extradrw)
STD_ROM_FN(extradrw)

static struct BurnRomInfo cjdh6thRomDesc[] = {
	{ "igs027a.u24",  0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "u5",           0x040000, 0x76db4f1c, 1 | BRF_PRG | BRF_ESS              },
	{ "igs_m3004.u9", 0x080000, 0xd161f8f7, 2 | BRF_GRA                        },
	{ "igs_m3001.u19",0x200000, 0x642247fb, 3 | BRF_GRA                        },
	{ "igs s3002.u6", 0x080000, 0x74b64969, 4 | BRF_SND                        },
};

STD_ROM_PICK(cjdh6th)
STD_ROM_FN(cjdh6th)

static struct BurnRomInfo cjdh6thaRomDesc[] = {
	{ "igs027a.u24",  0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "u5",           0x040000, 0xf5a2d878, 1 | BRF_PRG | BRF_ESS              },
	{ "igs_m3004.u9", 0x080000, 0xd161f8f7, 2 | BRF_GRA                        },
	{ "igs_m3001.u19",0x200000, 0x642247fb, 3 | BRF_GRA                        },
	{ "s3002.u6",     0x200000, 0x35f856aa, 4 | BRF_SND                        },
};

STD_ROM_PICK(cjdh6tha)
STD_ROM_FN(cjdh6tha)

static struct BurnRomInfo gonefsh2RomDesc[] = {
	{ "027a.bin",         0x004000, 0x0ef83d8b, 1 | BRF_PRG | BRF_ESS },
	{ "gfii_v-904uso.u12",0x080000, 0xef0f6735, 1 | BRF_PRG | BRF_ESS },
	{ "gfii_text.u15",    0x080000, 0xb48118fd, 2 | BRF_GRA           },
	{ "gfii_cg.u17",      0x200000, 0x2568359c, 3 | BRF_GRA           },
	{ "gfii_sp.u13",      0x080000, 0x61da1d58, 4 | BRF_SND           },
};

STD_ROM_PICK(gonefsh2)
STD_ROM_FN(gonefsh2)

static struct BurnRomInfo chessc2RomDesc[] = {
	{ "c8_027a.bin",      0x004000, 0x0ef83d8b, 1 | BRF_PRG | BRF_ESS },
	{ "ccii_v-707uso.u12",0x080000, 0x5937b67b, 1 | BRF_PRG | BRF_ESS },
	{ "ccii_text.u15",    0x080000, 0x25fed033, 2 | BRF_GRA           },
	{ "ccii_cg.u17",      0x200000, 0x47e45157, 3 | BRF_GRA           },
	{ "ccii_sp.u13",      0x080000, 0x220a7b71, 4 | BRF_SND           },
};

STD_ROM_PICK(chessc2)
STD_ROM_FN(chessc2)

static struct BurnRomInfo cjddzsRomDesc[] = {
	{ "b1_igs027a", 0x004000, 0x124f4bee, 1 | BRF_PRG | BRF_ESS },
	{ "v-302cn.u1", 0x400000, 0xfc525368, 1 | BRF_PRG | BRF_ESS },
	{ "text.u27",   0x080000, 0x520dc392, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "ani_cg.u28", 0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "sp-1.u5",    0x200000, 0x7ef65d95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddzs)
STD_ROM_FN(cjddzs)

static struct BurnRomInfo cjddzsaRomDesc[] = {
	{ "igs027a",    0x004000, 0x124f4bee, 1 | BRF_PRG | BRF_ESS },
	{ "v-219gn.u1", 0x400000, 0x33ded435, 1 | BRF_PRG | BRF_ESS },
	{ "text.u27",   0x080000, 0x520dc392, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "ani_cg.u28", 0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "sp-1.u5",    0x200000, 0x64fbba95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddzsa)
STD_ROM_FN(cjddzsa)

static struct BurnRomInfo cjddzpsRomDesc[] = {
	{ "igs027a", 0x004000, 0x6cf26c3d, 1 | BRF_PRG | BRF_ESS },
	{ "rom.u1",  0x400000, 0x969f7d09, 1 | BRF_PRG | BRF_ESS },
	{ "rom.u27", 0x200000, 0xc4daedd6, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "rom.u26", 0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "rom.u29", 0x200000, 0xb0447269, 3 | BRF_GRA           },
	{ "rom.u11", 0x200000, 0x7ef65d95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddzps)
STD_ROM_FN(cjddzps)

static struct BurnRomInfo tshs101RomDesc[] = {
	{ "s2_027a.u23",               0x004000, 0x4fb222e6, 1 | BRF_PRG | BRF_ESS },
	{ "u4",                        0x080000, 0x2588e70a, 1 | BRF_PRG | BRF_ESS },
	{ "igs_t3702_text_v100.u6",    0x080000, 0x07294995, 2 | BRF_GRA           },
	{ "igs_a3701_anim_v100.u8",    0x400000, 0xbed56f35, 3 | BRF_GRA           },
	{ "igs_s3703_speech_v100.u25", 0x200000, 0x41313c68, 4 | BRF_SND           },
};

STD_ROM_PICK(tshs101)
STD_ROM_FN(tshs101)

static struct BurnRomInfo sdwxRomDesc[] = {
	{ "sdwx_igs027a", 0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "prg.u16",      0x080000, 0xc94ef6a8, 1 | BRF_PRG | BRF_ESS              },
	{ "text.u24",     0x080000, 0x60b415ac, 2 | BRF_GRA                        },
	{ "cg.u25",       0x200000, 0x709b9a42, 3 | BRF_GRA                        },
	{ "sp.u2",        0x080000, 0x216b5418, 4 | BRF_SND                        },
};

STD_ROM_PICK(sdwx)
STD_ROM_FN(sdwx)

static struct BurnRomInfo fruitparRomDesc[] = {
	{ "q5_027a.bin",          0x004000, 0xdf756ac3, 1 | BRF_PRG | BRF_ESS },
	{ "fruit_paradise_v214.u23", 0x080000, 0xe37bc4e0, 1 | BRF_PRG | BRF_ESS },
	{ "paradise_text.u12",    0x080000, 0xbdaa4407, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "igs_m4101.u13",        0x400000, 0x84899398, 3 | BRF_GRA           },
	{ "igs_w4102.u28",        0x080000, 0x558cab25, 4 | BRF_SND           },
};

STD_ROM_PICK(fruitpar)
STD_ROM_FN(fruitpar)

static struct BurnRomInfo fruitpar206usRomDesc[] = {
	{ "q5_027a.bin",            0x004000, 0xdf756ac3, 1 | BRF_PRG | BRF_ESS },
	{ "f paradise v-206us.u23", 0x080000, 0xee2fa627, 1 | BRF_PRG | BRF_ESS },
	{ "paradise_text.u12",      0x080000, 0xbdaa4407, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "igs_m4101.u13",          0x400000, 0x84899398, 3 | BRF_GRA           },
	{ "igs_w4102.u28",          0x080000, 0x558cab25, 4 | BRF_SND           },
};

STD_ROM_PICK(fruitpar206us)
STD_ROM_FN(fruitpar206us)

static struct BurnRomInfo royal5pRomDesc[] = {
	{ "v21_027a.u32",   0x004000, 0x90d44b53, 1 | BRF_PRG | BRF_ESS },
	{ "r5+_v-101us.u23",0x080000, 0x99e83ba0, 1 | BRF_PRG | BRF_ESS },
	{ "r5+_text_u12.u12",0x080000,0xf0e0d113, 2 | BRF_GRA           },
	{ "r5+_cg_u13.u13", 0x400000, 0x16e9a946, 3 | BRF_GRA           },
	{ "r5+_sp_u37.u37", 0x200000, 0x030ffdb4, 4 | BRF_SND           },
};

STD_ROM_PICK(royal5p)
STD_ROM_FN(royal5p)

static struct BurnRomInfo tripslotRomDesc[] = {
	{ "v21_027a.bin",         0x004000, 0xdebf0400, 1 | BRF_PRG | BRF_ESS },
	{ "tripleslot_v-200ve.u23",0x080000,0xc1a1ff26, 1 | BRF_PRG | BRF_ESS },
	{ "tripleslot_text.u12",  0x080000, 0xc2537d18, 2 | BRF_GRA           },
	{ "tripleslot_ani_cg.u13",0x400000, 0x83fc100e, 3 | BRF_GRA           },
	{ "tripleslot_sp.u37",    0x200000, 0x98b9cafd, 4 | BRF_SND           },
};

STD_ROM_PICK(tripslot)
STD_ROM_FN(tripslot)

static struct BurnRomInfo jhg3dRomDesc[] = {
	{ "g5_027a.u23", 0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "v-445cn.u8",  0x080000, 0x9d503a1f, 1 | BRF_PRG | BRF_ESS              },
	{ "u16",         0x020000, 0x29f75a96, 2 | BRF_GRA                        },
	{ "cg_u21.u21",  0x200000, 0x1e1c243a, 3 | BRF_GRA                        },
	{ "v-445cn.u20", 0x040000, 0xe6aac74d, 4 | BRF_SND                        },
};

STD_ROM_PICK(jhg3d)
STD_ROM_FN(jhg3d)

static struct BurnRomInfo tarzan2RomDesc[] = {
	{ "t2_027a.u32",               0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "tarzan_ii_v-101xb.u23",     0x080000, 0x46400562, 1 | BRF_PRG | BRF_ESS              },
	{ "tarzan_ii_textu12.u12",     0x080000, 0x22dcebd0, 2 | BRF_GRA                        },
	{ "tarzan_ii_a4202_u13.u13",   0x400000, 0x97a68f85, 3 | BRF_GRA                        },
	{ "tarzan_ii_cg_u11.u11",      0x080000, 0x9e101ef3, 3 | BRF_GRA                        },
	{ "igs_w4201_speech_v103.u28", 0x200000, 0xfb72d4b5, 4 | BRF_SND                        },
};

STD_ROM_PICK(tarzan2)
STD_ROM_FN(tarzan2)

static struct BurnRomInfo magtreeRomDesc[] = {
	{ "m10_027a.u32", 0x004000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "27c4002.u23",  0x080000, 0xe61ee1d8, 1 | BRF_PRG | BRF_ESS              },
	{ "27c4002.u12",  0x080000, 0x3e95242e, 2 | BRF_GRA                        },
	{ "m27c160.u13",  0x200000, 0xf7da0faf, 3 | BRF_GRA                        },
	{ "m27c160.u37",  0x200000, 0xf97cf8c9, 4 | BRF_SND                        },
};

STD_ROM_PICK(magtree)
STD_ROM_FN(magtree)

static struct BurnRomInfo lhzb3sjbRomDesc[] = {
	{ "lhzb3unk2_igs027a", 0x004000, 0xc713e8c6, 1 | BRF_PRG | BRF_ESS },
	{ "igs_p2401.u10",     0x080000, 0x47d26b39, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m2403.u9",      0x080000, 0xa82398a9, 2 | BRF_GRA           },
	{ "igs_m2404.u18",     0x400000, 0x2379cdf3, 3 | BRF_GRA           },
	{ "igs_s2402.u26",     0x200000, 0x84bc2f3e, 4 | BRF_SND           },
};

STD_ROM_PICK(lhzb3sjb)
STD_ROM_FN(lhzb3sjb)

static struct BurnRomInfo tct2pRomDesc[] = {
	{ "t8_027a.bin", 0x004000, 0xa5f0be90, 1 | BRF_PRG | BRF_ESS },
	{ "v-306cn.u17", 0x080000, 0xc479e5ac, 1 | BRF_PRG | BRF_ESS },
	{ "text.u27",    0x080000, 0xd0e20214, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "a4202.u28",   0x400000, 0x97a68f85, 3 | BRF_GRA           },
	{ "cg.u31",      0x080000, 0x808b38d1, 3 | BRF_GRA           },
	{ "sp.u5",       0x200000, 0x4b04e89e, 4 | BRF_SND           },
};

STD_ROM_PICK(tct2p)
STD_ROM_FN(tct2p)

static struct BurnRomInfo tct2paRomDesc[] = {
	{ "t8_027a.bin", 0x004000, 0xa5f0be90, 1 | BRF_PRG | BRF_ESS },
	{ "v-306cn.u17", 0x080000, 0xc479e5ac, 1 | BRF_PRG | BRF_ESS },
	{ "text.u27",    0x080000, 0xd0e20214, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "a4202.u28",   0x400000, 0x97a68f85, 3 | BRF_GRA           },
	{ "cg.u31",      0x080000, 0xc3f18d6c, 3 | BRF_GRA           },
	{ "sp.u5",       0x200000, 0x4b04e89e, 4 | BRF_SND           },
};

STD_ROM_PICK(tct2pa)
STD_ROM_FN(tct2pa)

static struct BurnRomInfo klxyj104cnRomDesc[] = {
	{ "klxyj_igs027a", 0x004000, 0x19505c43, 1 | BRF_PRG | BRF_ESS },
	{ "klxyj_104.u16", 0x080000, 0x8cb9bdc2, 1 | BRF_PRG | BRF_ESS },
	{ "klxyj_text.u24",0x080000, 0x22dcebd0, 2 | BRF_GRA           },
	{ "a4202.u25",     0x400000, 0x97a68f85, 3 | BRF_GRA           },
	{ "w4201.u2",      0x100000, 0x464f11ab, 4 | BRF_SND           },
};

STD_ROM_PICK(klxyj104cn)
STD_ROM_FN(klxyj104cn)

static struct BurnRomInfo klxyj102cnRomDesc[] = {
	{ "igs027a.u28",               0x004000, 0x19505c43, 1 | BRF_PRG | BRF_ESS },
	{ "u16",                       0x080000, 0x82699cd9, 1 | BRF_PRG | BRF_ESS },
	{ "u24",                       0x080000, 0x22dcebd0, 2 | BRF_GRA           },
	{ "igs_a4202_acg_v100.u25",    0x400000, 0x97a68f85, 3 | BRF_GRA           },
	{ "igs_w4201_speech_v100.u2",  0x100000, 0xb0fc9d5d, 4 | BRF_SND           },
};

STD_ROM_PICK(klxyj102cn)
STD_ROM_FN(klxyj102cn)

static struct BurnRomInfo mgzzRomDesc[] = {
	{ "f1_027a.bin", 0x004000, 0x4b270edb, 1 | BRF_PRG | BRF_ESS },
	{ "mgfx_101.u10",0x080000, 0x897c88a1, 1 | BRF_PRG | BRF_ESS },
	{ "mgfx_text.u9",0x080000, 0xe41e7768, 2 | BRF_GRA           },
	{ "mgfx_ani.u17",0x400000, 0x9fc75f4d, 3 | BRF_GRA           },
	{ "mgfx_sp.u14", 0x100000, 0x9bb28fc8, 4 | BRF_SND           },
};

STD_ROM_PICK(mgzz)
STD_ROM_FN(mgzz)

static struct BurnRomInfo mgzz100cnRomDesc[] = {
	{ "f1_027a.u23", 0x004000, 0x4b270edb, 1 | BRF_PRG | BRF_ESS },
	{ "v-100cn.u10", 0x080000, 0x278964f7, 1 | BRF_PRG | BRF_ESS },
	{ "text.u9",     0x080000, 0x10792638, 2 | BRF_GRA           },
	{ "cg.u17",      0x400000, 0x1643fa78, 3 | BRF_GRA           },
	{ "sp.u14",      0x080000, 0xf037952e, 4 | BRF_SND           },
};

STD_ROM_PICK(mgzz100cn)
STD_ROM_FN(mgzz100cn)

static struct BurnRomInfo mgfxRomDesc[] = {
	{ "w6_027a.u23", 0x004000, 0x7a8a2ea6, 1 | BRF_PRG | BRF_ESS },
	{ "v-104t.u10",  0x080000, 0xe6e304f7, 1 | BRF_PRG | BRF_ESS },
	{ "igs_t3402.u9",0x080000, 0x19b37f83, 2 | BRF_GRA           },
	{ "igs_a3401.u17",0x400000,0xc031f069, 3 | BRF_GRA           },
	{ "igs_s3403.u14",0x100000,0xd8dc252a, 4 | BRF_SND           },
};

STD_ROM_PICK(mgfx)
STD_ROM_FN(mgfx)

static struct BurnRomInfo mgcs3RomDesc[] = {
	{ "m9_027a.bin",      0x004000, 0xf40b3202, 1 | BRF_PRG | BRF_ESS },
	{ "m2401_v101cn.u17", 0x200000, 0xd0d78fb6, 1 | BRF_PRG | BRF_ESS },
	{ "text.u26",         0x200000, 0x14881df6, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "cg1.u28",          0x400000, 0x59522cf8, 3 | BRF_GRA           },
	{ "sp.u5",            0x200000, 0xeb27b166, 4 | BRF_SND           },
};

STD_ROM_PICK(mgcs3)
STD_ROM_FN(mgcs3)

static struct BurnRomInfo lhzb4RomDesc[] = {
	{ "lhzb4_igs027a", 0x004000, 0xde12c918, 1 | BRF_PRG | BRF_ESS },
	{ "lhzb4_104.u17", 0x080000, 0x6f349bbb, 1 | BRF_PRG | BRF_ESS },
	{ "lhzb4_text.u27",0x080000, 0x8488b039, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "a05501.u28",    0x400000, 0xf78b3714, 3 | BRF_GRA           },
	{ "w05502.u5",     0x200000, 0x467f677e, 4 | BRF_SND           },
};

STD_ROM_PICK(lhzb4)
STD_ROM_FN(lhzb4)

static struct BurnRomInfo lhzb4dhbRomDesc[] = {
	{ "f12_igs027a",                  0x004000, 0xde12c918, 1 | BRF_PRG | BRF_ESS },
	{ "v-203cn.u46",                  0x200000, 0x96d0cb19, 1 | BRF_PRG | BRF_ESS },
	{ "text_u26.u26",                 0x200000, 0x39d10d8f, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "igs_a05501w032_d07e_01e0.u28", 0x400000, 0xf78b3714, 3 | BRF_GRA           },
	{ "igs_w05502b016_45d1_756d.u5",  0x200000, 0x467f677e, 4 | BRF_SND           },
};

STD_ROM_PICK(lhzb4dhb)
STD_ROM_FN(lhzb4dhb)

static struct BurnRomInfo cjddzRomDesc[] = {
	{ "b6_igs027a", 0x004000, 0x124f4bee, 1 | BRF_PRG | BRF_ESS },
	{ "p1.u17",     0x080000, 0x81bc6ff6, 1 | BRF_PRG | BRF_ESS },
	{ "m3.u27",     0x080000, 0x520dc392, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "m4.u28",     0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "s2.u5",      0x200000, 0x64fbba95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddz)
STD_ROM_FN(cjddz)

static struct BurnRomInfo cjddz217cnRomDesc[] = {
	{ "s12_igs027a",               0x004000, 0x124f4bee, 1 | BRF_PRG | BRF_ESS },
	{ "v-217cn.u17",               0x080000, 0x42df949c, 1 | BRF_PRG | BRF_ESS },
	{ "text.u27",                  0x080000, 0x520dc392, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "igs_w05005w32m_f9ce_1d10.u28", 0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "igs_w05004b16m_8fa9_427d.u5",  0x200000, 0x64fbba95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddz217cn)
STD_ROM_FN(cjddz217cn)

static struct BurnRomInfo cjddz215cnRomDesc[] = {
	{ "cjddz215cn_igs027a", 0x004000, 0x124f4bee, 1 | BRF_PRG | BRF_ESS },
	{ "ddz_218cn.u17",      0x080000, 0x3cfe38d5, 1 | BRF_PRG | BRF_ESS },
	{ "ddz_text.u27",       0x080000, 0x520dc392, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "ddz_ani.u28",        0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "ddz_sp.u4",          0x200000, 0x7ef65d95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddz215cn)
STD_ROM_FN(cjddz215cn)

static struct BurnRomInfo cjddz213cnRomDesc[] = {
	{ "q1_igs027a", 0x004000, 0x124f4bee, 1 | BRF_PRG | BRF_ESS },
	{ "213cn.u17",  0x080000, 0x00000000, 1 | BRF_PRG | BRF_ESS | BRF_NODUMP },
	{ "text.u27",   0x080000, 0x520dc392, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "cg.u28",     0x400000, 0x168720a0, 3 | BRF_GRA           },
	{ "sp-1.u5",    0x200000, 0x7ef65d95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddz213cn)
STD_ROM_FN(cjddz213cn)

static struct BurnRomInfo cjddzpRomDesc[] = {
	{ "e10_igs027a.rom", 0x004000, 0x6cf26c3d, 1 | BRF_PRG | BRF_ESS },
	{ "cjddzp_s300cn.u17", 0x080000, 0x5c1501ee, 1 | BRF_PRG | BRF_ESS },
	{ "cjddzp_text.u27", 0x200000, 0xc4daedd6, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "cjddzp_anicg.u28",0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "cjddzp_extcg.u29",0x200000, 0xb0447269, 3 | BRF_GRA           },
	{ "cjddzp_sp-1.u4",  0x200000, 0x7ef65d95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddzp)
STD_ROM_FN(cjddzp)

static struct BurnRomInfo cjddzp206cnRomDesc[] = {
	{ "e9_igs027a.rom",   0x004000, 0x6cf26c3d, 1 | BRF_PRG | BRF_ESS },
	{ "cjddzp_s206cn.u17",0x080000, 0x4f828edc, 1 | BRF_PRG | BRF_ESS },
	{ "cjddzp_text.u27",  0x200000, 0xc4daedd6, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "cjddzp_anicg.u28", 0x400000, 0x72487508, 3 | BRF_GRA           },
	{ "cjddzp_extcg.u29", 0x200000, 0xb0447269, 3 | BRF_GRA           },
	{ "cjddzp_sp-1.u4",   0x200000, 0x7ef65d95, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddzp206cn)
STD_ROM_FN(cjddzp206cn)

static struct BurnRomInfo cjddzlfRomDesc[] = {
	{ "j1_igs027a",   0x004000, 0x9f6e0207, 1 | BRF_PRG | BRF_ESS },
	{ "igs_l2405.u17",0x080000, 0xec310408, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m2401.u27",0x080000, 0x670aa9f3, 2 | BRF_GRA           },
	{ "igs_s2402.u28",0x400000, 0x78d9c5a4, 3 | BRF_GRA           },
	{ "igs_m2403.u29",0x400000, 0xa6d09c16, 3 | BRF_GRA           },
	{ "igs_l2404.u5", 0x200000, 0x55f6dfac, 4 | BRF_SND           },
};

STD_ROM_PICK(cjddzlf)
STD_ROM_FN(cjddzlf)

static struct BurnRomInfo xypdkRomDesc[] = {
	{ "i8_027a.bin", 0x004000, 0xaf7889a5, 1 | BRF_PRG | BRF_ESS },
	{ "v-306cn.u17", 0x080000, 0xf78d2a7c, 1 | BRF_PRG | BRF_ESS },
	{ "text.u27",    0x080000, 0xb2b20a55, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "cg.u28",      0x400000, 0xb0835036, 3 | BRF_GRA           },
	{ "sp.u5",       0x200000, 0xe5ba3abe, 4 | BRF_SND           },
};

STD_ROM_PICK(xypdk)
STD_ROM_FN(xypdk)

static struct BurnRomInfo cjtljpRomDesc[] = {
	{ "t5_igs027a",    0x004000, 0x351d8658, 1 | BRF_PRG | BRF_ESS },
	{ "igs_p2401.u17", 0x080000, 0xe72d28c0, 1 | BRF_PRG | BRF_ESS },
	{ "igs_m2403.u27", 0x080000, 0xbb1ec620, 2 | BRF_GRA | M027_WORD_SWAP },
	{ "igs_m2404.u28", 0x400000, 0xc48f7cab, 3 | BRF_GRA           },
	{ "igs_s2402.u5",  0x200000, 0xde294c8d, 4 | BRF_SND           },
};

STD_ROM_PICK(cjtljp)
STD_ROM_FN(cjtljp)

static struct BurnRomInfo tswxpRomDesc[] = {
	{ "j2_027a.u20", 0x004000, 0x17ec400b, 1 | BRF_PRG | BRF_ESS },
	{ "u17",         0x200000, 0x49457ac2, 1 | BRF_PRG | BRF_ESS },
	{ "u23",         0x200000, 0x0bc3c59c, 2 | BRF_GRA           },
	{ "u29",         0x200000, 0x660c7219, 3 | BRF_GRA           },
	{ "u29",         0x200000, 0x660c7219, 3 | BRF_GRA           },
	{ "u4",          0x200000, 0x2e392e51, 4 | BRF_SND           },
};

STD_ROM_PICK(tswxp)
STD_ROM_FN(tswxp)

struct BurnDriver BurnDrvslqz3 = {
	"slqz3", NULL, NULL, NULL, "1999",
	"Shuang Long Qiang Zhu 3 (China, VS107C)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(slqz3), INPUT_FN(M027Mahjong), DIP_FN(Slqz3),
	slqz3Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvQlgs = {
	"qlgs", NULL, NULL, NULL, "1999",
	"Que Long Gaoshou (S501CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(qlgs), INPUT_FN(M027Mahjong), DIP_FN(Qlgs),
	qlgsInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhdmg = {
	"lhdmg", NULL, NULL, NULL, "1999",
	"Long Hu Da Manguan (V102C3M)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhdmg), INPUT_FN(M027Mahjong), DIP_FN(Slqz3),
	lhdmgInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhdmgp = {
	"lhdmgp", NULL, NULL, NULL, "1999",
	"Long Hu Da Manguan Duizhan Jiaqiang Ban (V400C3M)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhdmgp), INPUT_FN(M027Mahjong), DIP_FN(Slqz3),
	lhdmgInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvlhdmgp200c3m = {
	"lhdmgp200c3m", "lhdmgp", NULL, NULL, "1999",
	"Long Hu Da Manguan Duizhan Jiaqiang Ban (V200C3M)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhdmgp200c3m), INPUT_FN(M027Mahjong), DIP_FN(Slqz3),
	lhdmgInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhzb3 = {
	"lhzb3", NULL, NULL, NULL, "1999",
	"Long Hu Zhengba III (V400CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb3), INPUT_FN(M027Mahjong), DIP_FN(Lhzb3),
	lhdmgInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvlhzb3106c5m = {
	"lhzb3106c5m", "lhzb3", NULL, NULL, "1999",
	"Long Hu Zhengba III (V106C5M)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb3106c5m), INPUT_FN(M027Mahjong), DIP_FN(Lhzb3),
	lhzb3106Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhzb3sjb = {
	"lhzb3sjb", NULL, NULL, NULL, "1999",
	"Long Hu Zhengba III Shengji Ban (V300C5)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lhzb3sjb), INPUT_FN(M027Mahjong), DIP_FN(Lhzb3sjb),
	lhzb3sjbInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLhzb4 = {
	"lhzb4", NULL, NULL, NULL, "2004",
	"Long Hu Zhengba 4 (V104CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(lhzb4), INPUT_FN(M027Ddz), DIP_FN(Lhzb4),
	lhzb4Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvlhzb4dhb = {
	"lhzb4dhb", NULL, NULL, NULL, "2004",
	"Long Hu Zhengba 4 Dui Hua Ban (V203CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(lhzb4dhb), INPUT_FN(M027Ddz), DIP_FN(Lhzb4),
	lhzb4Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvLthyp = {
	"lthyp", NULL, NULL, NULL, "1999",
	"Long Teng Hu Yao Duizhan Jiaqiang Ban (S104CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(lthyp), INPUT_FN(M027Mahjong), DIP_FN(Lthyp),
	lthypInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvZhongguo = {
	"zhongguo", NULL, NULL, NULL, "2000",
	"Zhongguo Chu Da D (V102C)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(zhongguo), INPUT_FN(M027Mahjong), DIP_FN(Zhongguo),
	zhongguoInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvJking02 = {
	"jking02", NULL, NULL, NULL, "2001",
	"Jungle King 2002 (V209US)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(jking02), INPUT_FN(M027Slot), DIP_FN(Jking02),
	jking02Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvKlxyj104cn = {
	"klxyj104cn", "jking02", NULL, NULL, "2001",
	"Kuaile Xiyou Ji (V104CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(klxyj104cn), INPUT_FN(M027Poker), DIP_FN(Klxyj),
	klxyjInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvKlxyj102cn = {
	"klxyj102cn", "jking02", NULL, NULL, "2001",
	"Kuaile Xiyou Ji (V102CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(klxyj102cn), INPUT_FN(M027Poker), DIP_FN(Klxyj),
	klxyjInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvTct2p = {
	"tct2p", NULL, NULL, NULL, "2003",
	"Tarzan Chuang Tian Guan 2 Jiaqiang Ban (V306CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tct2p), INPUT_FN(M027Tct2p), DIP_FN(Tct2p),
	tct2pInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvtct2pa = {
	"tct2pa", "tct2p", NULL, NULL, "2003",
	"Tarzan Chuang Tian Guan 2 Jiaqiang Ban (V306CN, alternate GFX)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tct2pa), INPUT_FN(M027Tct2p), DIP_FN(Tct2p),
	tct2pInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvMgzz = {
	"mgzz", NULL, NULL, NULL, "2003",
	"Manguan Zhizun (V101CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgzz), INPUT_FN(M027Mahjong), DIP_FN(Mgzz),
	mgzzInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvmgzz100cn = {
	"mgzz100cn", "mgzz", NULL, NULL, "2003",
	"Manguan Zhizun (V100CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgzz100cn), INPUT_FN(M027Mahjong), DIP_FN(Mgzz),
	mgzzInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvMgcs3 = {
	"mgcs3", NULL, NULL, NULL, "2007",
	"Manguan Caishen 3 (V101CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(mgcs3), INPUT_FN(M027Ddz), DIP_FN(Mgcs3),
	mgcs3Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvOceanpar = {
	"oceanpar", NULL, NULL, NULL, "1999",
	"Ocean Paradise (V105US)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(oceanpar), INPUT_FN(M027Slot), DIP_FN(Oceanpar105),
	oceanparInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvoceanpar101us = {
	"oceanpar101us", "oceanpar", NULL, NULL, "1999",
	"Ocean Paradise (V101US)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(oceanpar101us), INPUT_FN(M027Slot), DIP_FN(Oceanpar),
	oceanparInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvFruitpar = {
	"fruitpar", NULL, NULL, NULL, "1999",
	"Fruit Paradise (V214US)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(fruitpar), INPUT_FN(M027Slot), DIP_FN(Oceanpar),
	fruitparInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvfruitpar206us = {
	"fruitpar206us", "fruitpar", NULL, NULL, "1999",
	"Fruit Paradise (V206US)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(fruitpar206us), INPUT_FN(M027Slot), DIP_FN(Fruitpar206),
	fruitparInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvCjddz = {
	"cjddz", NULL, NULL, NULL, "2004",
	"Chaoji Dou Dizhu (V219CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddz), INPUT_FN(M027Ddz), DIP_FN(Cjddz),
	cjddzInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvcjddz217cn = {
	"cjddz217cn", "cjddz", NULL, NULL, "2004",
	"Chaoji Dou Dizhu (V217CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddz217cn), INPUT_FN(M027Ddz), DIP_FN(Cjddz),
	cjddzInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvcjddz215cn = {
	"cjddz215cn", "cjddz", NULL, NULL, "2004",
	"Chaoji Dou Dizhu (V215CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddz215cn), INPUT_FN(M027Ddz), DIP_FN(Cjddz),
	cjddzInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvcjddz213cn = {
	"cjddz213cn", "cjddz", NULL, NULL, "2004",
	"Chaoji Dou Dizhu (V213CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddz213cn), INPUT_FN(M027Ddz), DIP_FN(Cjddz),
	cjddzInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvCjddzp = {
	"cjddzp", NULL, NULL, NULL, "2004",
	"Chaoji Dou Dizhu Jiaqiang Ban (S300CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddzp), INPUT_FN(M027Ddz), DIP_FN(Cjddzp),
	cjddzpInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvcjddzp206cn = {
	"cjddzp206cn", "cjddzp", NULL, NULL, "2004",
	"Chaoji Dou Dizhu Jiaqiang Ban (S206CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddzp206cn), INPUT_FN(M027Ddz), DIP_FN(Cjddzp),
	cjddzpInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvcjddzs = {
	"cjddzs", NULL, NULL, NULL, "2007",
	"Chaoji Dou Dizhu (V219CN) / Chaoji Dou Dizhu Jianan Ban (V302CN)\0", NULL, "bootleg (WDF)", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddzs), INPUT_FN(M027Ddz), DIP_FN(Cjddz),
	cjddzsInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvcjddzsa = {
	"cjddzsa", "cjddzs", NULL, NULL, "2009",
	"Chaoji Dou Dizhu (V219CN) / Chaoji Dou Dizhu Jianan Ban (V405CN)\0", NULL, "bootleg (WDF)", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddzsa), INPUT_FN(M027Ddz), DIP_FN(Cjddz),
	cjddzsaInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvcjddzps = {
	"cjddzps", NULL, NULL, NULL, "2000",
	"Chaoji Dou Dizhu Jiaqiang Ban (S300CN) / unknown second set\0", NULL, "bootleg (WDF)", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddzps), INPUT_FN(M027Ddz), DIP_FN(Cjddzp),
	cjddzpsInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvCjddzlf = {
	"cjddzlf", NULL, NULL, NULL, "2005",
	"Chaoji Dou Dizhu Liang Fu Pai (V109CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjddzlf), INPUT_FN(M027Ddz), DIP_FN(Cjddz),
	cjddzlfInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvCjtljp = {
	"cjtljp", NULL, NULL, NULL, "2005",
	"Chaoji Tuolaji Jiaqiang Ban (V206CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjtljp), INPUT_FN(M027Ddz), DIP_FN(Lhzb4),
	cjtljpInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvXypdk = {
	"xypdk", NULL, NULL, NULL, "2005",
	"Xingyun Pao De Kuai (V106CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(xypdk), INPUT_FN(M027Ddz), DIP_FN(Lhzb4),
	xypdkInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvRoyal5p = {
	"royal5p", NULL, NULL, NULL, "2005",
	"Royal 5+ / X'mas 5 (V101US)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(royal5p), INPUT_FN(M027Slot), DIP_FN(Royal5p),
	royal5pInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvTripslot = {
	"tripslot", NULL, NULL, NULL, "2007",
	"Triple Slot (V200VE)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tripslot), INPUT_FN(M027Slot), DIP_FN(Tripslot),
	tripslotInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvCcly = {
	"ccly", NULL, NULL, NULL, "2005",
	"Chong Chong Leyuan (V100CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(ccly), INPUT_FN(M027Ccly), DIP_FN(Ccly),
	cclyInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvCjsxp = {
	"cjsxp", NULL, NULL, NULL, "2001",
	"Huangpai Zuqiu Plus / Chaoji Shuangxing Plus (V103CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CARD, 0,
	ROM_NAME(cjsxp), INPUT_FN(M027Poker), DIP_FN(Cjsxp),
	klxyjInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvTshs = {
	"tshs", NULL, NULL, NULL, "2000",
	"Tiansheng Haoshou (V201CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(tshs), INPUT_FN(M027Mahjong), DIP_FN(Tshs),
	tshsInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvTshs101 = {
	"tshs101", "tshs", NULL, NULL, "2000",
	"Tiansheng Haoshou (V101CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_NOT_WORKING | BDF_CLONE, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(tshs101), INPUT_FN(M027Mahjong), DIP_FN(Tshs),
	slqz3Init, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvTswxp = {
	"tswxp", NULL, NULL, NULL, "2006",
	"Taishan Wuxian Jiaqiang Ban (V101CN)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_CASINO, 0,
	ROM_NAME(tswxp), INPUT_FN(M027Tct2p), DIP_FN(Tswxp),
	tswxpInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

struct BurnDriver BurnDrvMgfx = {
	"mgfx", NULL, NULL, NULL, "2000",
	"Manguan Fuxing (V104T)\0", NULL, "IGS", "IGS M027",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_IGS, GBF_MAHJONG, 0,
	ROM_NAME(mgfx), INPUT_FN(M027Mahjong), DIP_FN(Mgfx),
	mgfxInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x200,
	512, 240, 4, 3
};

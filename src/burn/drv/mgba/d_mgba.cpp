#include "burnint.h"
#include "mgba_core.h"

static UINT8 MgbaInputPort0[10];
static UINT8 MgbaReset;
static UINT16 MgbaInput;
static INT32 MgbaButtonCount = 10;
static ClearOpposite<1, UINT16> MgbaClearOpposites;

static struct BurnInputInfo MgbaGbaInputList[] = {
	{"P1 Up",        BIT_DIGITAL, MgbaInputPort0 + 6, "p1 up"     },
	{"P1 Down",      BIT_DIGITAL, MgbaInputPort0 + 7, "p1 down"   },
	{"P1 Left",      BIT_DIGITAL, MgbaInputPort0 + 5, "p1 left"   },
	{"P1 Right",     BIT_DIGITAL, MgbaInputPort0 + 4, "p1 right"  },
	{"P1 Button A",  BIT_DIGITAL, MgbaInputPort0 + 0, "p1 fire 1" },
	{"P1 Button B",  BIT_DIGITAL, MgbaInputPort0 + 1, "p1 fire 2" },
	{"P1 Button L",  BIT_DIGITAL, MgbaInputPort0 + 9, "p1 fire 3" },
	{"P1 Button R",  BIT_DIGITAL, MgbaInputPort0 + 8, "p1 fire 4" },
	{"P1 Select",    BIT_DIGITAL, MgbaInputPort0 + 2, "p1 select" },
	{"P1 Start",     BIT_DIGITAL, MgbaInputPort0 + 3, "p1 start"  },
	{"Reset",        BIT_DIGITAL, &MgbaReset,         "reset"     },
};

static struct BurnInputInfo MgbaGbInputList[] = {
	{"P1 Up",        BIT_DIGITAL, MgbaInputPort0 + 6, "p1 up"     },
	{"P1 Down",      BIT_DIGITAL, MgbaInputPort0 + 7, "p1 down"   },
	{"P1 Left",      BIT_DIGITAL, MgbaInputPort0 + 5, "p1 left"   },
	{"P1 Right",     BIT_DIGITAL, MgbaInputPort0 + 4, "p1 right"  },
	{"P1 Button A",  BIT_DIGITAL, MgbaInputPort0 + 0, "p1 fire 1" },
	{"P1 Button B",  BIT_DIGITAL, MgbaInputPort0 + 1, "p1 fire 2" },
	{"P1 Select",    BIT_DIGITAL, MgbaInputPort0 + 2, "p1 select" },
	{"P1 Start",     BIT_DIGITAL, MgbaInputPort0 + 3, "p1 start"  },
	{"Reset",        BIT_DIGITAL, &MgbaReset,         "reset"     },
};

STDINPUTINFO(MgbaGba)
STDINPUTINFO(MgbaGb)

static INT32 DrvDoReset()
{
	MgbaClearOpposites.reset();
	return MgbaCoreReset();
}

static INT32 DrvInitCommon(const MgbaSystemConfig* config, INT32 buttonCount)
{
	memset(MgbaInputPort0, 0, sizeof(MgbaInputPort0));
	MgbaReset = 0;
	MgbaInput = 0;
	MgbaButtonCount = buttonCount;

	if (MgbaCoreInit(config)) {
		MgbaCoreExit();
		return 1;
	}

	return 0;
}

static INT32 DrvInitGba()
{
	return DrvInitCommon(&MgbaSystemGba, 10);
}

static INT32 DrvInitGb()
{
	return DrvInitCommon(&MgbaSystemGb, 8);
}

static INT32 DrvInitGbc()
{
	return DrvInitCommon(&MgbaSystemGbc, 8);
}

static INT32 DrvExit()
{
	return MgbaCoreExit();
}

static UINT16 MgbaBuildKeys()
{
	UINT16 keys = 0;

	for (INT32 i = 0; i < MgbaButtonCount; i++) {
		if (MgbaInputPort0[i]) {
			keys |= (1 << i);
		}
	}

	MgbaClearOpposites.neu(0, keys, 1 << 6, 1 << 7, 1 << 5, 1 << 4);
	return keys;
}

static INT32 DrvFrame()
{
	if (MgbaReset) {
		DrvDoReset();
	}

	MgbaInput = MgbaBuildKeys();
	return MgbaCoreFrame(MgbaInput);
}

static INT32 DrvDraw()
{
	return MgbaCoreDraw();
}

static INT32 DrvScan(INT32 nAction, INT32* pnMin)
{
	if (pnMin) {
		*pnMin = 0x029900;
	}

	if (nAction & ACB_VOLATILE) {
		SCAN_VAR(MgbaReset);
		SCAN_VAR(MgbaInput);
		SCAN_VAR(MgbaButtonCount);
		MgbaClearOpposites.scan();
	}

	return MgbaCoreScan(nAction, pnMin);
}

static bool MgbaZipNameHasGenericSuffix(const char* name)
{
	return name && (!strcmp(name, "gba") || !strcmp(name, "gb") || !strcmp(name, "gbc"));
}

static const char* MgbaZipNameStripPrefix(const char* name)
{
	if (name == NULL) {
		return NULL;
	}

	if (!strncmp(name, "mgba_", 5) && !MgbaZipNameHasGenericSuffix(name + 5)) {
		return name + 5;
	}
	if (!strncmp(name, "gba_", 4) && !MgbaZipNameHasGenericSuffix(name + 4)) {
		return name + 4;
	}
	if (!strncmp(name, "gbc_", 4) && !MgbaZipNameHasGenericSuffix(name + 4)) {
		return name + 4;
	}
	if (!strncmp(name, "gb_", 3) && !MgbaZipNameHasGenericSuffix(name + 3)) {
		return name + 3;
	}

	return name;
}

static INT32 MgbaGetZipName(char** pszName, UINT32 i)
{
	static char szFilename[MAX_PATH];
	char* pszGameName = BurnDrvGetTextA(DRV_NAME);
	const char* stripped = MgbaZipNameStripPrefix(pszGameName);
	bool hasAlternate = (pszGameName != NULL && stripped != NULL && strcmp(stripped, pszGameName) != 0);

	if (pszName == NULL || pszGameName == NULL) {
		return 1;
	}

	if (i == 0) {
		snprintf(szFilename, sizeof(szFilename), "%s", hasAlternate ? stripped : pszGameName);
		*pszName = szFilename;
		return 0;
	}

	if (i == 1 && hasAlternate) {
		snprintf(szFilename, sizeof(szFilename), "%s", pszGameName);
		*pszName = szFilename;
		return 0;
	}

	*pszName = NULL;
	return 1;
}

static struct BurnRomInfo mgba_gbaRomDesc[] = {
	{ "mgba_cart.gba", 0x02000000, 0x00000000, BRF_PRG | BRF_ESS },
	{ "gba_bios.bin",  0x00004000, 0x00000000, BRF_BIOS | BRF_OPT },
};

static struct BurnRomInfo mgba_gbRomDesc[] = {
	{ "mgba_cart.gb",  0x02000000, 0x00000000, BRF_PRG | BRF_ESS },
	{ "gb_bios.bin",   0x00000100, 0x00000000, BRF_BIOS | BRF_OPT },
};

static struct BurnRomInfo mgba_gbcRomDesc[] = {
	{ "mgba_cart.gbc", 0x02000000, 0x00000000, BRF_PRG | BRF_ESS },
	{ "gbc_bios.bin",  0x00000900, 0x00000000, BRF_BIOS | BRF_OPT },
};

STD_ROM_PICK(mgba_gba)
STD_ROM_FN(mgba_gba)

STD_ROM_PICK(mgba_gb)
STD_ROM_FN(mgba_gb)

STD_ROM_PICK(mgba_gbc)
STD_ROM_FN(mgba_gbc)

struct BurnDriver BurnDrvmgba_gba = {
	"gba_gba", NULL, NULL, NULL, "2026",
	"Game Boy Advance (mGBA, Generic Cartridge)\0", NULL, "Nintendo / mGBA Team", "Nintendo Game Boy Advance",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_PREFIX_CARTRIDGE | HARDWARE_GBA, GBF_MISC, 0,
	MgbaGetZipName, mgba_gbaRomInfo, mgba_gbaRomName, NULL, NULL, NULL, NULL, MgbaGbaInputInfo, NULL,
	DrvInitGba, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	240, 160, 3, 2
};

struct BurnDriver BurnDrvmgba_gb = {
	"gba_gb", NULL, NULL, NULL, "2026",
	"Game Boy (mGBA, Generic Cartridge)\0", NULL, "Nintendo / mGBA Team", "Nintendo Game Boy",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_PREFIX_CARTRIDGE | HARDWARE_GB, GBF_MISC, 0,
	MgbaGetZipName, mgba_gbRomInfo, mgba_gbRomName, NULL, NULL, NULL, NULL, MgbaGbInputInfo, NULL,
	DrvInitGb, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	256, 224, 10, 9
};

struct BurnDriver BurnDrvmgba_gbc = {
	"gba_gbc", NULL, NULL, NULL, "2026",
	"Game Boy Color (mGBA, Generic Cartridge)\0", NULL, "Nintendo / mGBA Team", "Nintendo Game Boy Color",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_PREFIX_CARTRIDGE | HARDWARE_GBC, GBF_MISC, 0,
	MgbaGetZipName, mgba_gbcRomInfo, mgba_gbcRomName, NULL, NULL, NULL, NULL, MgbaGbInputInfo, NULL,
	DrvInitGbc, DrvExit, DrvFrame, DrvDraw, DrvScan, NULL, 0x10000,
	160, 144, 10, 9
};

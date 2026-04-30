#include "burner.h"

// d_cps1.cpp
#define CPS1_68K_PROGRAM_BYTESWAP			1
#define CPS1_68K_PROGRAM_NO_BYTESWAP		2
#define CPS1_Z80_PROGRAM					3
#define CPS1_TILES							4
#define CPS1_OKIM6295_SAMPLES				5
#define CPS1_QSOUND_SAMPLES					6
#define CPS1_PIC							7
#define CPS1_EXTRA_TILES_SF2EBBL_400000		8
#define CPS1_EXTRA_TILES_400000				9
#define CPS1_EXTRA_TILES_SF2KORYU_400000	10
#define CPS1_EXTRA_TILES_SF2B_400000		11
#define CPS1_EXTRA_TILES_SF2MKOT_400000		12

// d_cps2.cpp
#define CPS2_PRG_68K						1
#define CPS2_PRG_68K_SIMM					2
#define CPS2_PRG_68K_XOR_TABLE				3
#define CPS2_GFX							5
#define CPS2_GFX_SIMM						6
#define CPS2_GFX_SPLIT4						7
#define CPS2_GFX_SPLIT8						8
#define CPS2_GFX_19XXJ						9
#define CPS2_PRG_Z80						10
#define CPS2_QSND							12
#define CPS2_QSND_SIMM						13
#define CPS2_QSND_SIMM_BYTESWAP				14
#define CPS2_ENCRYPTION_KEY					15

// gal.h
#define GAL_ROM_Z80_PROG1				1
#define GAL_ROM_Z80_PROG2				2
#define GAL_ROM_Z80_PROG3				3
#define GAL_ROM_TILES_SHARED			4
#define GAL_ROM_TILES_CHARS				5
#define GAL_ROM_TILES_SPRITES			6
#define GAL_ROM_PROM					7
#define GAL_ROM_S2650_PROG1				8

// megadrive.h
#define SEGA_MD_ROM_LOAD_NORMAL								0x10
#define SEGA_MD_ROM_LOAD16_WORD_SWAP						0x20
#define SEGA_MD_ROM_LOAD16_BYTE								0x30
#define SEGA_MD_ROM_LOAD16_WORD_SWAP_CONTINUE_040000_100000	0x40
#define SEGA_MD_ROM_LOAD_NORMAL_CONTINUE_020000_080000		0x50
#define SEGA_MD_ROM_OFFS_000000								0x01
#define SEGA_MD_ROM_OFFS_000001								0x02
#define SEGA_MD_ROM_OFFS_020000								0x03
#define SEGA_MD_ROM_OFFS_080000								0x04
#define SEGA_MD_ROM_OFFS_100000								0x05
#define SEGA_MD_ROM_OFFS_100001								0x06
#define SEGA_MD_ROM_OFFS_200000								0x07
#define SEGA_MD_ROM_OFFS_300000								0x08
#define SEGA_MD_ROM_RELOAD_200000_200000					0x09
#define SEGA_MD_ROM_RELOAD_100000_300000					0x0a
#define SEGA_MD_ARCADE_SUNMIXING							(0x4000)

// sys16.h
#define SYS16_ROM_PROG_FLAT									25
#define SYS16_ROM_PROG										1
#define SYS16_ROM_TILES										2
#define SYS16_ROM_SPRITES									3
#define SYS16_ROM_Z80PROG									4
#define SYS16_ROM_KEY										5
#define SYS16_ROM_7751PROG									6
#define SYS16_ROM_7751DATA									7
#define SYS16_ROM_UPD7759DATA								8
#define SYS16_ROM_PROG2										9
#define SYS16_ROM_ROAD										10
#define SYS16_ROM_PCMDATA									11
#define SYS16_ROM_Z80PROG2									12
#define SYS16_ROM_Z80PROG3									13
#define SYS16_ROM_Z80PROG4									14
#define SYS16_ROM_PCM2DATA									15
#define SYS16_ROM_PROM 										16
#define SYS16_ROM_PROG3										17
#define SYS16_ROM_SPRITES2									18
#define SYS16_ROM_RF5C68DATA								19
#define SYS16_ROM_I8751										20
#define SYS16_ROM_MSM6295									21
#define SYS16_ROM_TILES_20000								22

// taito.h
#define TAITO_68KROM1										1
#define TAITO_68KROM1_BYTESWAP								2
#define TAITO_68KROM1_BYTESWAP_JUMPING						3
#define TAITO_68KROM1_BYTESWAP32							4
#define TAITO_68KROM2										5
#define TAITO_68KROM2_BYTESWAP								6
#define TAITO_68KROM3										7
#define TAITO_68KROM3_BYTESWAP								8
#define TAITO_Z80ROM1										9
#define TAITO_Z80ROM2										10
#define TAITO_CHARS											11
#define TAITO_CHARS_BYTESWAP								12
#define TAITO_CHARSB										13
#define TAITO_CHARSB_BYTESWAP								14
#define TAITO_SPRITESA										15
#define TAITO_SPRITESA_BYTESWAP								16
#define TAITO_SPRITESA_BYTESWAP32							17
#define TAITO_SPRITESA_TOPSPEED								18
#define TAITO_SPRITESB										19
#define TAITO_SPRITESB_BYTESWAP								20
#define TAITO_SPRITESB_BYTESWAP32							21
#define TAITO_ROAD											22
#define TAITO_SPRITEMAP										23
#define TAITO_YM2610A										24
#define TAITO_YM2610B										25
#define TAITO_MSM5205										26
#define TAITO_MSM5205_BYTESWAP								27
#define TAITO_CHARS_PIVOT									28
#define TAITO_MSM6295										29
#define TAITO_ES5505										30
#define TAITO_ES5505_BYTESWAP								31
#define TAITO_DEFAULT_EEPROM								32
#define TAITO_CHARS_BYTESWAP32								33
#define TAITO_CCHIP_BIOS									34
#define TAITO_CCHIP_EEPROM									35

#ifndef ERANGE
  #define ERANGE 34
#endif

struct DatListInfo {
	TCHAR szRomSet[100];
	TCHAR szFullName[1024];
	TCHAR szDrvName[100];
	TCHAR szHardware[256];
	UINT32 nMarker;
};

struct HardwareIcon {
	UINT32 nHardwareCode;
	INT32  nIconIndex;
};

struct PNGRESOLUTION {
	INT32 nWidth;
	INT32 nHeight;
};

struct RomDataListEntry {
	TCHAR szDatPath[MAX_PATH];
	DatListInfo info;
	INT32 nImageIndex;
	INT32 nGroupId;
	HTREEITEM hTreeItem;
};

struct RomDataGroupEntry {
	TCHAR szDrvName[100];
	INT32 nGroupId;
	INT32 nEntryCount;
	bool bExpanded;
	bool bStateRestored;
	HTREEITEM hTreeItem;
};

struct RomDataVisibleRow {
	bool bGroupHeader;
	INT32 nGroupId;
	INT32 nEntryIndex;
};

struct RDMacroMap {
	const TCHAR* pszDRMacro;
	const UINT32 nRDMacro;
};

enum {
	SORT_ASCENDING = 0,
	SORT_DESCENDING = 1
};

struct BurnRomInfo* pDataRomDesc = NULL;

TCHAR szRomdataName[MAX_PATH] = _T("");
bool  bRDListScanSub          = false;

static struct BurnRomInfo* pDRD = NULL;
static struct RomDataInfo RDI = { 0 };

static HIMAGELIST hHardwareIconList = NULL;

RomDataInfo*  pRDI = &RDI;

static HBRUSH hWhiteBGBrush;
static INT32 sort_direction = 0;
static INT32 nSelItem       = -1;
static INT32 nRomDataSortColumn = 0;
static bool bRomdataSearchInit = false;
static TCHAR szRomdataSearchRaw[256] = { 0 };
static TCHAR szRomdataSearchNormalized[256] = { 0 };
static std::vector<RomDataListEntry> g_RomDataEntries;
static std::vector<RomDataListEntry> g_RomDataSourceEntries;
static std::vector<RomDataGroupEntry> g_RomDataGroups;
static std::vector<RomDataVisibleRow> g_RomDataVisibleRows;
static bool bRomdataHostedMode = false;
static INT32 nRomdataHostedPreviewCtrlId = IDC_ROMDATA_TITLE;
static INT32 nRomdataHostedPreviewFrameCtrlId = IDC_ROMDATA_TITLE_FRAME;
static INT32 nRomdataHostedDatTextCtrlId = IDC_ROMDATA_TEXTDATPATH;
static INT32 nRomdataHostedDriverTextCtrlId = IDC_ROMDATA_TEXTDRIVER;
static INT32 nRomdataHostedHardwareTextCtrlId = IDC_ROMDATA_TEXTHARDWARE;

static HWND hRDMgrWnd   = NULL;
static HWND hRDListView = NULL;
static HWND hRdCoverDlg = NULL;

static TCHAR szCover[MAX_PATH]      = { 0 };
static TCHAR szBackupDat[MAX_PATH]  = { 0 };

#define IDC_ROMDATA_SEARCH_TIMER 0x5301
#define IDC_ROMDATA_CTX_EDIT     0x5302
#define IDC_ROMDATA_CTX_OPEN     0x5303
#define IDC_ROMDATA_CTX_DELETE   0x5304
#define IDC_ROMDATA_SELECTION_TIMER 0x5305

static void RomDataRefreshList(bool bFocusList, bool bRescan);
static bool RomDataGetSelectedDatInternal(TCHAR* pszSelDat, INT32 nCount);
static void RomDataUpdateSelectionUi();
static void RomDataClearListUi();
static const RomDataListEntry* RomDataGetEntryByIndex(INT32 nIndex);
static const RomDataListEntry* RomDataGetSelectedEntry();
static const RomDataVisibleRow* RomDataGetVisibleRow(INT32 nItem);
static RomDataGroupEntry* RomDataGetGroupById(INT32 nGroupId);


static struct HardwareIcon IconTable[] =
{
	{	HARDWARE_SEGA_MEGADRIVE,		-1	},
	{	HARDWARE_PCENGINE_PCENGINE,		-1	},
	{	HARDWARE_PCENGINE_SGX,			-1	},
	{	HARDWARE_PCENGINE_TG16,			-1	},
	{	HARDWARE_SEGA_SG1000,			-1	},
	{	HARDWARE_COLECO,				-1	},
	{	HARDWARE_SEGA_MASTER_SYSTEM,	-1	},
	{	HARDWARE_SEGA_GAME_GEAR,		-1	},
	{	HARDWARE_MSX,					-1	},
	{	HARDWARE_SPECTRUM,				-1	},
	{	HARDWARE_NES,					-1	},
	{	HARDWARE_FDS,					-1	},
	{	HARDWARE_SNES,					-1	},
	{	HARDWARE_GB,					-1	},
	{	HARDWARE_GBC,					-1	},
	{	HARDWARE_SNK_NGPC,				-1	},
	{	HARDWARE_SNK_NGP,				-1	},
	{	HARDWARE_CHANNELF,				-1	},
	{	0,								-1	},

	{	~0U,							-1	}	// End Marker
};

#define X(a) { _T(#a), a }
static const RDMacroMap RDMacroTable[] = {
	X(BRF_PRG),
	X(BRF_GRA),
	X(BRF_SND),
	X(BRF_ESS),
	X(BRF_BIOS),
	X(BRF_SELECT),
	X(BRF_OPT),
	X(BRF_NODUMP),
	X(CPS1_68K_PROGRAM_BYTESWAP),
	X(CPS1_68K_PROGRAM_NO_BYTESWAP),
	X(CPS1_Z80_PROGRAM),
	X(CPS1_TILES),
	X(CPS1_OKIM6295_SAMPLES),
	X(CPS1_QSOUND_SAMPLES),
	X(CPS1_PIC),
	X(CPS1_EXTRA_TILES_SF2EBBL_400000),
	X(CPS1_EXTRA_TILES_400000),
	X(CPS1_EXTRA_TILES_SF2KORYU_400000),
	X(CPS1_EXTRA_TILES_SF2B_400000),
	X(CPS1_EXTRA_TILES_SF2MKOT_400000),
	X(CPS2_PRG_68K),
	X(CPS2_PRG_68K_SIMM),
	X(CPS2_PRG_68K_XOR_TABLE),
	X(CPS2_GFX),
	X(CPS2_GFX_SIMM),
	X(CPS2_GFX_SPLIT4),
	X(CPS2_GFX_SPLIT8),
	X(CPS2_GFX_19XXJ),
	X(CPS2_PRG_Z80),
	X(CPS2_QSND),
	X(CPS2_QSND_SIMM),
	X(CPS2_QSND_SIMM_BYTESWAP),
	X(CPS2_ENCRYPTION_KEY),
	X(GAL_ROM_Z80_PROG1),
	X(GAL_ROM_Z80_PROG2),
	X(GAL_ROM_Z80_PROG3),
	X(GAL_ROM_TILES_SHARED),
	X(GAL_ROM_TILES_CHARS),
	X(GAL_ROM_TILES_SPRITES),
	X(GAL_ROM_PROM),
	X(GAL_ROM_S2650_PROG1),
	X(SEGA_MD_ROM_LOAD_NORMAL),
	X(SEGA_MD_ROM_LOAD16_WORD_SWAP),
	X(SEGA_MD_ROM_LOAD16_BYTE),
	X(SEGA_MD_ROM_LOAD16_WORD_SWAP_CONTINUE_040000_100000),
	X(SEGA_MD_ROM_LOAD_NORMAL_CONTINUE_020000_080000),
	X(SEGA_MD_ROM_OFFS_000000),
	X(SEGA_MD_ROM_OFFS_000001),
	X(SEGA_MD_ROM_OFFS_020000),
	X(SEGA_MD_ROM_OFFS_080000),
	X(SEGA_MD_ROM_OFFS_100000),
	X(SEGA_MD_ROM_OFFS_100001),
	X(SEGA_MD_ROM_OFFS_200000),
	X(SEGA_MD_ROM_OFFS_300000),
	X(SEGA_MD_ROM_RELOAD_200000_200000),
	X(SEGA_MD_ROM_RELOAD_100000_300000),
	X(SEGA_MD_ARCADE_SUNMIXING),
	X(SYS16_ROM_PROG_FLAT),
	X(SYS16_ROM_PROG),
	X(SYS16_ROM_TILES),
	X(SYS16_ROM_SPRITES),
	X(SYS16_ROM_Z80PROG),
	X(SYS16_ROM_KEY),
	X(SYS16_ROM_7751PROG),
	X(SYS16_ROM_7751DATA),
	X(SYS16_ROM_UPD7759DATA),
	X(SYS16_ROM_PROG2),
	X(SYS16_ROM_ROAD),
	X(SYS16_ROM_PCMDATA),
	X(SYS16_ROM_Z80PROG2),
	X(SYS16_ROM_Z80PROG3),
	X(SYS16_ROM_Z80PROG4),
	X(SYS16_ROM_PCM2DATA),
	X(SYS16_ROM_PROM),
	X(SYS16_ROM_PROG3),
	X(SYS16_ROM_SPRITES2),
	X(SYS16_ROM_RF5C68DATA),
	X(SYS16_ROM_I8751),
	X(SYS16_ROM_MSM6295),
	X(SYS16_ROM_TILES_20000),
	X(TAITO_68KROM1),
	X(TAITO_68KROM1_BYTESWAP),
	X(TAITO_68KROM1_BYTESWAP_JUMPING),
	X(TAITO_68KROM1_BYTESWAP32),
	X(TAITO_68KROM2),
	X(TAITO_68KROM2_BYTESWAP),
	X(TAITO_68KROM3),
	X(TAITO_68KROM3_BYTESWAP),
	X(TAITO_Z80ROM1),
	X(TAITO_Z80ROM2),
	X(TAITO_CHARS),
	X(TAITO_CHARS_BYTESWAP),
	X(TAITO_CHARSB),
	X(TAITO_CHARSB_BYTESWAP),
	X(TAITO_SPRITESA),
	X(TAITO_SPRITESA_BYTESWAP),
	X(TAITO_SPRITESA_BYTESWAP32),
	X(TAITO_SPRITESA_TOPSPEED),
	X(TAITO_SPRITESB),
	X(TAITO_SPRITESB_BYTESWAP),
	X(TAITO_SPRITESB_BYTESWAP32),
	X(TAITO_ROAD),
	X(TAITO_SPRITEMAP),
	X(TAITO_YM2610A),
	X(TAITO_YM2610B),
	X(TAITO_MSM5205),
	X(TAITO_MSM5205_BYTESWAP),
	X(TAITO_CHARS_PIVOT),
	X(TAITO_MSM6295),
	X(TAITO_ES5505),
	X(TAITO_ES5505_BYTESWAP),
	X(TAITO_DEFAULT_EEPROM),
	X(TAITO_CHARS_BYTESWAP32),
	X(TAITO_CCHIP_BIOS),
	X(TAITO_CCHIP_EEPROM)
};
#undef X

// gal.h
#undef GAL_ROM_Z80_PROG1
#undef GAL_ROM_Z80_PROG2
#undef GAL_ROM_Z80_PROG3
#undef GAL_ROM_TILES_SHARED
#undef GAL_ROM_TILES_CHARS
#undef GAL_ROM_TILES_SPRITES
#undef GAL_ROM_PROM
#undef GAL_ROM_S2650_PROG1

// megadrive.h
#undef SEGA_MD_ROM_LOAD_NORMAL
#undef SEGA_MD_ROM_LOAD16_WORD_SWAP
#undef SEGA_MD_ROM_LOAD16_BYTE
#undef SEGA_MD_ROM_LOAD16_WORD_SWAP_CONTINUE_040000_100000
#undef SEGA_MD_ROM_LOAD_NORMAL_CONTINUE_020000_080000
#undef SEGA_MD_ROM_OFFS_000000
#undef SEGA_MD_ROM_OFFS_000001
#undef SEGA_MD_ROM_OFFS_020000
#undef SEGA_MD_ROM_OFFS_080000
#undef SEGA_MD_ROM_OFFS_100000
#undef SEGA_MD_ROM_OFFS_100001
#undef SEGA_MD_ROM_OFFS_200000
#undef SEGA_MD_ROM_OFFS_300000
#undef SEGA_MD_ROM_RELOAD_200000_200000
#undef SEGA_MD_ROM_RELOAD_100000_300000
#undef SEGA_MD_ARCADE_SUNMIXING

// sys16.h
#undef SYS16_ROM_PROG_FLAT
#undef SYS16_ROM_PROG
#undef SYS16_ROM_TILES
#undef SYS16_ROM_SPRITES
#undef SYS16_ROM_Z80PROG
#undef SYS16_ROM_KEY
#undef SYS16_ROM_7751PROG
#undef SYS16_ROM_7751DATA
#undef SYS16_ROM_UPD7759DATA
#undef SYS16_ROM_PROG2
#undef SYS16_ROM_ROAD
#undef SYS16_ROM_PCMDATA
#undef SYS16_ROM_Z80PROG2
#undef SYS16_ROM_Z80PROG3
#undef SYS16_ROM_Z80PROG4
#undef SYS16_ROM_PCM2DATA
#undef SYS16_ROM_PROM
#undef SYS16_ROM_PROG3
#undef SYS16_ROM_SPRITES2
#undef SYS16_ROM_RF5C68DATA
#undef SYS16_ROM_I8751
#undef SYS16_ROM_MSM6295
#undef SYS16_ROM_TILES_20000

// taito.h
#undef TAITO_68KROM1
#undef TAITO_68KROM1_BYTESWAP
#undef TAITO_68KROM1_BYTESWAP_JUMPING
#undef TAITO_68KROM1_BYTESWAP32
#undef TAITO_68KROM2
#undef TAITO_68KROM2_BYTESWAP
#undef TAITO_68KROM3
#undef TAITO_68KROM3_BYTESWAP
#undef TAITO_Z80ROM1
#undef TAITO_Z80ROM2
#undef TAITO_CHARS
#undef TAITO_CHARS_BYTESWAP
#undef TAITO_CHARSB
#undef TAITO_CHARSB_BYTESWAP
#undef TAITO_SPRITESA
#undef TAITO_SPRITESA_BYTESWAP
#undef TAITO_SPRITESA_BYTESWAP32
#undef TAITO_SPRITESA_TOPSPEED
#undef TAITO_SPRITESB
#undef TAITO_SPRITESB_BYTESWAP
#undef TAITO_SPRITESB_BYTESWAP32
#undef TAITO_ROAD
#undef TAITO_SPRITEMAP
#undef TAITO_YM2610A
#undef TAITO_YM2610B
#undef TAITO_MSM5205
#undef TAITO_MSM5205_BYTESWAP
#undef TAITO_CHARS_PIVOT
#undef TAITO_MSM6295
#undef TAITO_ES5505
#undef TAITO_ES5505_BYTESWAP
#undef TAITO_DEFAULT_EEPROM
#undef TAITO_CHARS_BYTESWAP32
#undef TAITO_CCHIP_BIOS
#undef TAITO_CCHIP_EEPROM

static UINT32 RDGetRomsType(const TCHAR* pszDrvName, const UINT32 nBaseType, const INT32 nMinIdx, const INT32 nMaxIdx)
{
	char* pRomName;
	struct BurnRomInfo ri = { 0 };
	const UINT32 nOldDrvSel = nBurnDrvActive;	// Backup

	nBurnDrvActive = BurnDrvGetIndex(TCHARToANSI(pszDrvName, NULL, 0));

	for (INT32 i = 0; !BurnDrvGetRomName(&pRomName, i, 0); i++) {
		BurnDrvGetRomInfo(&ri, i);

		if ((ri.nType & nBaseType) && ((ri.nType & 0x0f) >= nMinIdx) && ((ri.nType & 0x0f) <= nMaxIdx)) {
			nBurnDrvActive = nOldDrvSel;		// Restore
			return ri.nType;
		}
	}

	nBurnDrvActive = nOldDrvSel;				// Restore
	return nBaseType;
}

// Consoles will automatically get the Type of the ROMs
static UINT32 RDGetConsoleRomsType(const TCHAR* pszDrvName)
{
	char* pRomName = NULL;
	struct BurnRomInfo ri = { 0 };
	const UINT32 nOldDrvSel = nBurnDrvActive;	// Backup

	nBurnDrvActive = BurnDrvGetIndex(TCHARToANSI(pszDrvName, NULL, 0));

	for (INT32 i = 0; !BurnDrvGetRomName(&pRomName, i, 0); i++) {
		BurnDrvGetRomInfo(&ri, i);

		if (ri.nType & BRF_PRG) {
			nBurnDrvActive = nOldDrvSel;		// Restore
			return ri.nType;
		}
	}

	nBurnDrvActive = nOldDrvSel;				// Restore
	return BRF_PRG;
}

// FBNeo style is preferred, universal solutions are not recommended
static UINT32 RDGetUniversalRomsType(const TCHAR* pszDrvName, const TCHAR* pszMask)
{
	UINT32 nBaseType = 0;

	if (0 == _tcsicmp(pszMask, _T("Program")) || 0 == _tcsicmp(pszMask, _T("PRG1"))) {
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Graphics")) || 0 == _tcsicmp(pszMask, _T("GRA1"))) {
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Z80")) || 0 == _tcsicmp(pszMask, _T("SNDA"))) {
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Samples")) || 0 == _tcsicmp(pszMask, _T("SND1"))) {
		nBaseType = BRF_SND;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("BoardPLD")) || 0 == _tcsicmp(pszMask, _T("OPT1"))) {
		nBaseType = BRF_OPT;
	}

	char* pRomName = NULL;
	struct BurnRomInfo ri = { 0 };
	const UINT32 nOldDrvSel = nBurnDrvActive;	// Backup

	nBurnDrvActive = BurnDrvGetIndex(TCHARToANSI(pszDrvName, NULL, 0));

	for (INT32 i = 0; !BurnDrvGetRomName(&pRomName, i, 0); i++) {
		BurnDrvGetRomInfo(&ri, i);

		if (ri.nType & nBaseType) {
			nBurnDrvActive = nOldDrvSel;		// Restore
			return ri.nType;
		}
	}

	nBurnDrvActive = nOldDrvSel;				// Restore
	return nBaseType;
}

static UINT32 RDGetCps1RomsType(const TCHAR* pszDrvName, const TCHAR* pszMask, const UINT32 nPrgLen)
{
	INT32 nMinIdx = 0, nMaxIdx = 0;
	UINT32 nBaseType = 0;

	if (0 == _tcsicmp(pszMask, _T("Program")) || 0 == _tcsicmp(pszMask, _T("PRG1"))) {
		nMinIdx = CPS1_68K_PROGRAM_BYTESWAP, nMaxIdx = CPS1_68K_PROGRAM_NO_BYTESWAP;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("ProgramS")) || 0 == _tcsicmp(pszMask, _T("PRGS"))) {
		return (BRF_ESS | BRF_PRG | CPS1_68K_PROGRAM_BYTESWAP);
	}
	else
	if (0 == _tcsicmp(pszMask, _T("ProgramN")) || 0 == _tcsicmp(pszMask, _T("PRGN"))) {
		return (BRF_ESS | BRF_PRG | CPS1_68K_PROGRAM_NO_BYTESWAP);
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Z80")) || 0 == _tcsicmp(pszMask, _T("SNDA"))) {
		nMinIdx = nMaxIdx = CPS1_Z80_PROGRAM;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Graphics")) || 0 == _tcsicmp(pszMask, _T("GRA1"))) {
		nMinIdx = nMaxIdx = CPS1_TILES;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Samples")) || 0 == _tcsicmp(pszMask, _T("SND1"))) {
		nMinIdx = CPS1_OKIM6295_SAMPLES, nMaxIdx = CPS1_QSOUND_SAMPLES;
		nBaseType = BRF_SND;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Pic")) || 0 == _tcsicmp(pszMask, _T("SNDB"))) {
		nMinIdx = nMaxIdx = CPS1_PIC;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Tiles")) || 0 == _tcsicmp(pszMask, _T("Extras")) || 0 == _tcsicmp(pszMask, _T("GRA2"))) {
		nMinIdx = CPS1_EXTRA_TILES_SF2EBBL_400000, nMaxIdx = CPS1_EXTRA_TILES_SF2MKOT_400000;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("BoardPLD")) || 0 == _tcsicmp(pszMask, _T("OPT1"))) {
		return BRF_OPT;
	}

	char* pRomName = NULL;
	struct BurnRomInfo ri = { 0 };
	const UINT32 nOldDrvSel = nBurnDrvActive;	// Backup

	nBurnDrvActive = BurnDrvGetIndex(TCHARToANSI(pszDrvName, NULL, 0));

	for (INT32 i = 0; !BurnDrvGetRomName(&pRomName, i, 0); i++) {
		BurnDrvGetRomInfo(&ri, i);

		if ((ri.nType & nBaseType) && ((ri.nType & 0x0f) >= nMinIdx) && ((ri.nType & 0x0f) <= nMaxIdx)) {
			if (nPrgLen > 0 && (CPS1_68K_PROGRAM_BYTESWAP == nMinIdx || CPS1_68K_PROGRAM_NO_BYTESWAP == nMaxIdx)) {
				if (nPrgLen >= 0x40000 && (CPS1_68K_PROGRAM_BYTESWAP == (ri.nType & 0x0f))) {
					ri.nType = (ri.nType & ~CPS1_68K_PROGRAM_BYTESWAP) | CPS1_68K_PROGRAM_NO_BYTESWAP;
				}
				else
				if (nPrgLen < 0x40000 && (CPS1_68K_PROGRAM_NO_BYTESWAP == (ri.nType & 0x0f))) {
					ri.nType = (ri.nType & ~CPS1_68K_PROGRAM_NO_BYTESWAP) | CPS1_68K_PROGRAM_BYTESWAP;
				}
			}
			nBurnDrvActive = nOldDrvSel;		// Restore
			return ri.nType;
		}
	}

	nBurnDrvActive = nOldDrvSel;				// Restore
	return 0;
}

// d_cps1.cpp
#undef CPS1_68K_PROGRAM_BYTESWAP
#undef CPS1_68K_PROGRAM_NO_BYTESWAP
#undef CPS1_Z80_PROGRAM
#undef CPS1_TILES
#undef CPS1_OKIM6295_SAMPLES
#undef CPS1_QSOUND_SAMPLES
#undef CPS1_PIC
#undef CPS1_EXTRA_TILES_SF2EBBL_400000
#undef CPS1_EXTRA_TILES_400000
#undef CPS1_EXTRA_TILES_SF2KORYU_400000
#undef CPS1_EXTRA_TILES_SF2B_400000
#undef CPS1_EXTRA_TILES_SF2MKOT_400000

static UINT32 RDGetCps2RomsType(const TCHAR* pszDrvName, const TCHAR* pszMask)
{
	INT32 nMinIdx = 0, nMaxIdx = 0;
	UINT32 nBaseType = 0;

	if (0 == _tcsicmp(pszMask, _T("Program")) || 0 == _tcsicmp(pszMask, _T("PRG1"))) {
		nMinIdx = CPS2_PRG_68K, nMaxIdx = CPS2_PRG_68K_XOR_TABLE;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Graphics")) || 0 == _tcsicmp(pszMask, _T("GRA1"))) {
		nMinIdx = CPS2_GFX, nMaxIdx = CPS2_GFX_19XXJ;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Z80")) || 0 == _tcsicmp(pszMask, _T("SNDA"))) {
		nMinIdx = nMaxIdx = CPS2_PRG_Z80;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Samples")) || 0 == _tcsicmp(pszMask, _T("SND1"))) {
		nMinIdx = CPS2_QSND, nMaxIdx = CPS2_QSND_SIMM_BYTESWAP;
		nBaseType = BRF_SND;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Decryption")) || 0 == _tcsicmp(pszMask, _T("KEY1"))) {
		return CPS2_ENCRYPTION_KEY;
	}

	return RDGetRomsType(pszDrvName, nBaseType, nMinIdx, nMaxIdx);
}

// d_cps2.cpp
#undef CPS2_PRG_68K
#undef CPS2_PRG_68K_SIMM
#undef CPS2_PRG_68K_XOR_TABLE
#undef CPS2_GFX
#undef CPS2_GFX_SIMM
#undef CPS2_GFX_SPLIT4
#undef CPS2_GFX_SPLIT8
#undef CPS2_GFX_19XXJ
#undef CPS2_PRG_Z80
#undef CPS2_QSND
#undef CPS2_QSND_SIMM
#undef CPS2_QSND_SIMM_BYTESWAP
#undef CPS2_ENCRYPTION_KEY

static UINT32 RDGetCps3RomsType(const TCHAR* pszMask)
{
	if (0 == _tcsicmp(pszMask, _T("Bios"))) {
		return (BRF_ESS | BRF_BIOS);
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Program")) || 0 == _tcsicmp(pszMask, _T("PRG1"))) {
		return (BRF_ESS | BRF_PRG);
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Graphics")) || 0 == _tcsicmp(pszMask, _T("GRA1"))) {
		return BRF_GRA;
	}
	return 0;
}

static INT32 RDGetPgmRomsType(const TCHAR* pszDrvName, const TCHAR* pszMask)
{
	INT32 nMinIdx = 0, nMaxIdx = 0;
	UINT32 nBaseType = 0;

	if (0 == _tcsicmp(pszMask, _T("Program")) || 0 == _tcsicmp(pszMask, _T("PRG1"))) {
		nMinIdx = nMaxIdx = 1;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Tile")) || 0 == _tcsicmp(pszMask, _T("GRA1"))) {
		nMinIdx = nMaxIdx = 2;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("SpriteData")) || 0 == _tcsicmp(pszMask, _T("GRA2"))) {
		nMinIdx = nMaxIdx = 3;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("SpriteMasks")) || 0 == _tcsicmp(pszMask, _T("GRA3"))) {
		nMinIdx = nMaxIdx = 4;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Samples")) || 0 == _tcsicmp(pszMask, _T("SND1"))) {
		nMinIdx = nMaxIdx = 5;
		nBaseType = BRF_SND;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("InternalARM7")) || 0 == _tcsicmp(pszMask, _T("PRG2"))) {
		nMinIdx = nMaxIdx = 7;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Ramdump"))) {
		//What's the point of existing when the rom length is 0 and the Nodump marker won't load?
		return (7 | BRF_PRG | BRF_ESS | BRF_NODUMP);
	}
	else
	if (0 == _tcsicmp(pszMask, _T("ExternalARM7")) || 0 == _tcsicmp(pszMask, _T("PRG3"))) {
		nMinIdx = nMaxIdx = 8;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("ProtectionRom")) || 0 == _tcsicmp(pszMask, _T("PRG4"))) {
		nMinIdx = nMaxIdx = 9;
		nBaseType = BRF_PRG;
	}

	return RDGetRomsType(pszDrvName, nBaseType, nMinIdx, nMaxIdx);
}

static UINT32 RDGetNeoGeoRomsType(const TCHAR* pszDrvName, const TCHAR* pszMask)
{
	INT32 nMinIdx = 0, nMaxIdx = 0;
	UINT32 nBaseType = 0;

	if (0 == _tcsicmp(pszMask, _T("Program")) || 0 == _tcsicmp(pszMask, _T("PRG1"))) {
		nMinIdx = nMaxIdx = 1;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Text")) || 0 == _tcsicmp(pszMask, _T("GRA1"))) {
		nMinIdx = nMaxIdx = 2;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Graphics")) || 0 == _tcsicmp(pszMask, _T("GRA2"))) {
		nMinIdx = nMaxIdx = 3;
		nBaseType = BRF_GRA;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Z80")) || 0 == _tcsicmp(pszMask, _T("SNDA"))) {
		nMinIdx = nMaxIdx = 4;
		nBaseType = BRF_PRG;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("Samples")) || 0 == _tcsicmp(pszMask, _T("SND1"))) {
		nMinIdx = nMaxIdx = 5;
		nBaseType = BRF_SND;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("SamplesAES")) || 0 == _tcsicmp(pszMask, _T("SND2"))) {
		// See nam1975RomDesc
		nMinIdx = nMaxIdx = 6;
		nBaseType = BRF_SND;
	}
	else
	if (0 == _tcsicmp(pszMask, _T("BoardPLD")) || 0 == _tcsicmp(pszMask, _T("OPT1"))) {
		return 0 | BRF_OPT;
	}

	return RDGetRomsType(pszDrvName, nBaseType, nMinIdx, nMaxIdx);
}

static INT32 GetDrvHardwareMask(const TCHAR* pszDrvName)
{
	const UINT32 nOldDrvSel = nBurnDrvActive;	// Backup
	nBurnDrvActive = BurnDrvGetIndex(TCHARToANSI(pszDrvName, NULL, 0));

	const INT32 nHardwareCode = BurnDrvGetHardwareCode();
	nBurnDrvActive = nOldDrvSel;				// Restore

	return (nHardwareCode & HARDWARE_PUBLIC_MASK);
}

static UINT32 RDSetRomsType(const TCHAR* pszDrvName, const TCHAR* pszMask, const UINT32 nPrgLen)
{
	INT32 nHardwareMask = GetDrvHardwareMask(pszDrvName);
	switch (nHardwareMask){
		case HARDWARE_IGS_PGM:
		case HARDWARE_IGS_PGM2:
			return RDGetPgmRomsType(pszDrvName, pszMask);

		case HARDWARE_SNK_NEOGEO:
			return RDGetNeoGeoRomsType(pszDrvName, pszMask);

		case HARDWARE_CAPCOM_CPS1:
		case HARDWARE_CAPCOM_CPS1_QSOUND:
		case HARDWARE_CAPCOM_CPS1_GENERIC:
		case HARDWARE_CAPCOM_CPSCHANGER:
			return RDGetCps1RomsType(pszDrvName, pszMask, nPrgLen);

		case HARDWARE_CAPCOM_CPS2:
		case HARDWARE_CAPCOM_CPS2 | HARDWARE_CAPCOM_CPS2_SIMM:
			return RDGetCps2RomsType(pszDrvName, pszMask);

		case HARDWARE_CAPCOM_CPS3:
		case HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD:
			return RDGetCps3RomsType(pszDrvName);

		case HARDWARE_SEGA_MEGADRIVE:
		case HARDWARE_PCENGINE_PCENGINE:
		case HARDWARE_PCENGINE_SGX:
		case HARDWARE_PCENGINE_TG16:
		case HARDWARE_SEGA_SG1000:
		case HARDWARE_COLECO:
		case HARDWARE_SEGA_MASTER_SYSTEM:
		case HARDWARE_SEGA_GAME_GEAR:
		case HARDWARE_MSX:
		case HARDWARE_SPECTRUM:
		case HARDWARE_NES:
		case HARDWARE_FDS:
		case HARDWARE_SNES:
		case HARDWARE_GBA:
		case HARDWARE_GB:
		case HARDWARE_GBC:
		case HARDWARE_SNK_NGP:
		case HARDWARE_CHANNELF:
			return RDGetConsoleRomsType(pszDrvName);

		default:
			return RDGetUniversalRomsType(pszDrvName, pszMask);
	}
}

TCHAR* _strqtoken(TCHAR* s, const TCHAR* delims)
{
	static TCHAR* prev_str = NULL;
	TCHAR* token = NULL;

	if (!s) {
		if (!prev_str) return NULL;

		s = prev_str;
	}

	s += _tcsspn(s, delims);

	if (s[0] == _T('\0')) {
		prev_str = NULL;
		return NULL;
	}

	if (s[0] == _T('\"')) { // time to grab quoted string!
		token = ++s;
		if ((s = _tcspbrk(token, _T("\"")))) {
			*(s++) = '\0';
		}
		if (!s) {
			prev_str = NULL;
			return NULL;
		}
	} else {
		token = s;
	}

	if ((s = _tcspbrk(s, delims))) {
		*(s++) = _T('\0');
		prev_str = s;
	} else {
		// we're at the end of the road
#if defined (_UNICODE)
		prev_str = (TCHAR*)wmemchr(token, _T('\0'), MAX_PATH);
#else
		prev_str = (char*)memchr((void*)token, '\0', MAX_PATH);
#endif
	}

	return token;
}

static INT32 FileExists(const TCHAR* pszName)
{
	DWORD dwAttrib = GetFileAttributes(pszName);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static HIMAGELIST HardwareIconListInit()
{
	if (!bEnableIcons) return NULL;

	INT32 cx = 0, cy = 0, nIdx = 0;

	switch (nIconsSize) {
		case ICON_16x16: cx = cy = 16;	break;
		case ICON_24x24: cx = cy = 24;	break;
		case ICON_32x32: cx = cy = 32;	break;
	}

	hHardwareIconList = ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, (sizeof(IconTable) / sizeof(HardwareIcon)) - 1, 0);
	if (NULL == hHardwareIconList) return NULL;
	TreeView_SetImageList(hRDListView, hHardwareIconList, TVSIL_NORMAL);

	struct HardwareIcon* _it = &IconTable[0];

	while (~0U != _it->nHardwareCode) {
		_it->nIconIndex = ImageList_AddIcon(hHardwareIconList, pIconsCache[nBurnDrvCount + nIdx]);
		_it++, nIdx++;
	}

	return hHardwareIconList;
}

static void DestroyHardwareIconList()
{
	struct HardwareIcon* _it = &IconTable[0];

	while (~0U != _it->nHardwareCode) {
		_it->nIconIndex = -1; _it++;
	}

	if (NULL != hHardwareIconList) {
		ImageList_Destroy(hHardwareIconList); hHardwareIconList = NULL;
	}
}

static INT32 FindHardwareIconIndex(const TCHAR* pszDrvName)
{
	if (!bEnableIcons) return 0;

	const UINT32 nOldDrvSel    = nBurnDrvActive;	// Backup
	nBurnDrvActive             = BurnDrvGetIndex(TCHARToANSI(pszDrvName, NULL, 0));
	const UINT32 nHardwareCode = BurnDrvGetHardwareCode();

	struct HardwareIcon* _it = &IconTable[0];
	INT32 nIdx = 0;

	while (~0U != _it->nHardwareCode) {
		if (_it->nHardwareCode > 0) {				// Consoles
			if (_it->nHardwareCode == (nHardwareCode & HARDWARE_SNK_NGPC)) {
				nBurnDrvActive = nOldDrvSel;		// Restore
				return _it->nIconIndex;				// NeoGeo Pocket Color
			}
			if (_it->nHardwareCode == (nHardwareCode & HARDWARE_PUBLIC_MASK)) {
				nBurnDrvActive = nOldDrvSel;		// Restore
				return _it->nIconIndex;
			}
		}
		_it++, nIdx++;
	}

	nBurnDrvActive = nOldDrvSel;					// Restore
	return IconTable[--nIdx].nIconIndex;			// Arcade
}

typedef enum {
	ENCODING_ANSI,
	ENCODING_UTF8,
	ENCODING_UTF8_BOM,
	ENCODING_UTF16_LE,
	ENCODING_UTF16_BE,
	ENCODING_ERROR
} EncodingType;

static EncodingType DetectEncoding(const TCHAR* pszDatFile)
{
	FILE* fp = _tfopen(pszDatFile, _T("rb"));
	if (NULL == fp)
		return ENCODING_ERROR;

	EncodingType encType = ENCODING_UTF8;

	// Read BOM
	UINT8 cBom[3] = { 0 };
	const UINT32 nBomSize = fread(cBom, 1, 3, fp);

	if (0 == nBomSize) {	// Empty file or read error
		fclose(fp);
		return ENCODING_ERROR;
	}
	if ((nBomSize >= 3) && (0xef == cBom[0]) && (0xbb == cBom[1]) && (0xbf == cBom[2])) {
		fclose(fp);
		return ENCODING_UTF8_BOM;
	}
	if (nBomSize >= 2) {
		if ((0xff == cBom[0]) && (0xfe == cBom[1])) {
			fclose(fp);
			return ENCODING_UTF16_LE;
		}
		if ((0xfe == cBom[0]) && (0xff == cBom[1])) {
			fclose(fp);
			return ENCODING_UTF16_BE;
		}
	}

	fseek(fp, 0, SEEK_END);
	long nLen = ftell(fp);
	rewind(fp);

	UINT8* pBuf = (UINT8*)malloc(nLen);
	if (!pBuf) {
		fclose(fp);
		return ENCODING_ERROR;
	}

	UINT32 nRead = fread(pBuf, 1, nLen, fp);
	fclose(fp);

	if (nRead != (UINT32)nLen) {
		free(pBuf);
		return ENCODING_ERROR;
	}

	UINT32 p = 0;
	while (p < nRead) {
		if (pBuf[p] < 0x80) {
			p++;
			continue;
		}

		if (!((pBuf[p] >= 0xc2) && (pBuf[p] <= 0xf4))) {
			free(pBuf);
			return ENCODING_ANSI;
		}

		INT32 nBytes = 0;
		if ((pBuf[p] >= 0xc2) && (pBuf[p] <= 0xdf))
			nBytes = 1;
		if ((pBuf[p] >= 0xe0) && (pBuf[p] <= 0xef))
			nBytes = 2;
		if ((pBuf[p] >= 0xf0) && (pBuf[p] <= 0xf4))
			nBytes = 3;

		if ((p + nBytes) >= nRead) {
			free(pBuf);
			return ENCODING_ANSI;
		}

		for (int i = 1; i <= nBytes; i++) {
			if (!(0x80 == (pBuf[p + i] & 0xc0))) {
				free(pBuf);
				return ENCODING_ANSI;
			}
		}

		p += (nBytes + 1);
	}
	free(pBuf);

	return ENCODING_UTF8;
}

static TCHAR* Utf16beToUtf16le(const TCHAR* pszDatFile)
{
	FILE* fp = _tfopen(pszDatFile, _T("rb"));
	if (NULL == fp) return NULL;

	fseek(fp, 0, SEEK_END);
	long nLen = ftell(fp);
	rewind(fp);

	UINT8* pBuffer = (UINT8*)malloc(nLen);
	if (NULL == pBuffer) {
		fclose(fp);  fp = NULL;
		return NULL;
	}
	memset(pBuffer, 0, nLen);

	UINT32 nRead = fread(pBuffer, 1, nLen, fp);
	fclose(fp);

	// Ensure that even bytes are handled
	if (0 != (nRead % 2)) {
		nRead--; // Discard last byte
	}

	// Swap the order of each double byte (BE -> LE)
	for (UINT32 i = 0; i < nRead; i += 2) {
		UINT8 cTemp = pBuffer[i];
		pBuffer[i + 0] = pBuffer[i + 1];
		pBuffer[i + 1] = cTemp;
	}

	if (NULL == (fp = _tfopen(pszDatFile, _T("wb")))) {
		free(pBuffer); pBuffer = NULL;
	}

	fwrite(pBuffer, 1, nRead, fp);
	fclose(fp);    fp      = NULL;
	free(pBuffer); pBuffer = NULL;

	return (TCHAR*)pszDatFile;
}

static bool StrToUint(const TCHAR* str, UINT32* result) {
	if (NULL == str || _T('\0') == *str || NULL == result) return false;

	errno = 0;

	TCHAR* endPtr;
	unsigned long value = _tcstoul(str, &endPtr, 16);

	if (str == endPtr)       return false;
	if (_T('\0') != *endPtr) return false;
	if (value > UINT_MAX) {
		errno = ERANGE;
		return false;
	}

	*result = (UINT32)value;
	return true;
}

TCHAR* AdaptiveEncodingReads(const TCHAR* pszFileName)
{
	EncodingType nType = DetectEncoding(pszFileName);
	TCHAR* pszReadMode = NULL;

	switch (nType) {
		case ENCODING_ANSI: {
			pszReadMode = _T("rt");
			break;
		}
		case ENCODING_UTF8:
		case ENCODING_UTF8_BOM: {
			pszReadMode = _T("rt, ccs=UTF-8");
			break;
		}
		case ENCODING_UTF16_LE: {
			pszReadMode = _T("rt, ccs=UTF-16LE");
			break;
		}
		case ENCODING_UTF16_BE: {
			if (NULL == Utf16beToUtf16le(pszFileName)) return NULL;
			pszReadMode = _T("rt, ccs=UTF-16LE");
			break;
		}
		default:
			return NULL;
	}

	static TCHAR szRet[MAX_PATH] = { 0 };

	return _tcscpy(szRet, pszReadMode);
}

#define DELIM_TOKENS_NAME	_T(" \t\r\n,%:|{}")

static INT32 LoadRomdata()
{
	const TCHAR* pszReadMode = AdaptiveEncodingReads(szRomdataName);
	if (NULL == pszReadMode) return -1;

	RDI.nDescCount = -1;					// Failed

	FILE* fp = _tfopen(szRomdataName, pszReadMode);
	if (NULL == fp) return -1;

	TCHAR szBuf[MAX_PATH] = { 0 }, szRomMask[30] = { 0 }, szDrvName[100] = { 0 };
	TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;
	INT32 nDatMode = 0;						// FBNeo 0, Nebula 1

	memset(RDI.szExtraRom, 0, sizeof(RDI.szExtraRom));
	memset(RDI.szFullName, 0, sizeof(RDI.szFullName));

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = szBuf;

			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
			if (NULL == pszLabel) continue;
			if ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1])) continue;

			UINT32 nLen = _tcslen(pszLabel);
			if ((nLen > 2) && (_T('[') == pszLabel[0]) && (_T(']') == pszLabel[nLen - 1])) {
				pszLabel++, nLen -= 2;
				if (0 == _tcsicmp(_T("System"), pszLabel)) break;
				memset(szRomMask, 0, sizeof(szRomMask));
				_tcsncpy(szRomMask, pszLabel, nLen);
				szRomMask[nLen] = _T('\0');
				continue;
			}
			if (0 == _tcsicmp(_T("System"), pszLabel)) {
				nDatMode = 1;				// Nebula
				continue;
			}
			if (0 == _tcsicmp(_T("ZipName"), pszLabel) || 0 == _tcsicmp(_T("RomName"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No romset specified
				if (NULL != pDRD) {
					strcpy(RDI.szZipName, TCHARToANSI(pszInfo, NULL, 0));
				}
				continue;
			}
			if (0 == _tcsicmp(_T("DrvName"), pszLabel) || 0 == _tcsicmp(_T("Parent"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No driver specified
				if (NULL != pDRD) {
					strcpy(RDI.szDrvName, TCHARToANSI(pszInfo, NULL, 0));
				}
				memset(szDrvName, 0, sizeof(szDrvName));
				_tcscpy(szDrvName, pszInfo);
				continue;
			}
			if (0 == _tcsicmp(_T("ExtraRom"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if ((NULL != pszInfo) && (NULL != pDRD)) {
					strcpy(RDI.szExtraRom, TCHARToANSI(pszInfo, NULL, 0));
				}
				continue;
			}
			if (0 == _tcsicmp(_T("FullName"), pszLabel) || 0 == _tcsicmp(_T("Game"), pszLabel)) {
				TCHAR szMerger[260] = { 0 };
				INT32 nAdd = 0;
				while (NULL != (pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME))) {
					_stprintf(szMerger + nAdd, _T("%s "), pszInfo);
					nAdd = _tcslen(szMerger);
				}
				szMerger[nAdd - 1] = _T('\0');
				if (NULL != pDRD) {
#ifdef UNICODE
					_tcscpy(RDI.szFullName, szMerger);
#else
					wcscpy(RDI.szFullName, ANSIToTCHAR(szMerger, NULL, 0));
#endif
				}
				continue;
			}
			{
				// romname, len, crc, type
				struct BurnRomInfo ri = { 0 };
				ri.nLen  = UINT_MAX;
				ri.nCrc  = UINT_MAX;
				ri.nType = 0U;

				if (1 == nDatMode) {
					// Skips content when recognized as a Nebula style
					pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				}
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL != pszInfo) {
					StrToUint(pszInfo, &(ri.nLen));
					if ((UINT_MAX == ri.nLen) || (0 == ri.nLen)) continue;

					pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
					if (NULL != pszInfo) {
						StrToUint(pszInfo, &(ri.nCrc));
						if (UINT_MAX == ri.nCrc) continue;

						while (NULL != (pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME))) {
							UINT32 nValue = UINT_MAX;

							for (UINT32 i = 0; i < (sizeof(RDMacroTable) / sizeof(RDMacroMap)); i++) {
								if (0 == _tcscmp(pszInfo, RDMacroTable[i].pszDRMacro)) {
									ri.nType |= RDMacroTable[i].nRDMacro;
									continue;
								}
							}

							StrToUint(pszInfo, &nValue);
							if ((nValue >= 0) && (nValue < UINT_MAX)) {
								ri.nType |= nValue;
							}
						}
						// FBNeo style ROMs have a higher priority for nType than Nebula style
						if (0 == ri.nType) {
							ri.nType = RDSetRomsType(szDrvName, szRomMask, ri.nLen);
						}
						if (ri.nType > 0U) {
							RDI.nDescCount++;

							if (NULL != pDRD) {
								pDRD[RDI.nDescCount].szName = (char*)malloc(512);
								memset(pDRD[RDI.nDescCount].szName, 0, sizeof(char) * 512);
								strcpy(pDRD[RDI.nDescCount].szName, TCHARToANSI(pszLabel, NULL, 0));

								pDRD[RDI.nDescCount].nLen  = ri.nLen;
								pDRD[RDI.nDescCount].nCrc  = ri.nCrc;
								pDRD[RDI.nDescCount].nType = ri.nType;
							}
						}
					}
				}
			}
		}
	}
	fclose(fp);

	if (NULL != pDRD) {
		pDataRomDesc = (struct BurnRomInfo*)malloc((RDI.nDescCount + 1) * sizeof(BurnRomInfo));
		if (NULL == pDataRomDesc) return -1;

		for (INT32 i = 0; i <= RDI.nDescCount; i++) {
			pDataRomDesc[i].szName = (char*)malloc(512);
			memset(pDataRomDesc[i].szName, 0, sizeof(char) * 512);
			strcpy(pDataRomDesc[i].szName, pDRD[i].szName);
			free(pDRD[i].szName); pDRD[i].szName = NULL;

			pDataRomDesc[i].nLen  = pDRD[i].nLen;
			pDataRomDesc[i].nCrc  = pDRD[i].nCrc;
			pDataRomDesc[i].nType = pDRD[i].nType;
		}
		free(pDRD); pDRD = NULL;
	}

	return RDI.nDescCount;
}

bool RomDataSetQuickPath(const TCHAR* pszSelDat)
{
	if ((NULL == pszSelDat) || !FileExists(pszSelDat)) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData:\n\n"));
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXIST), pszSelDat);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return false;
	}

	const TCHAR* pszExt = _tcsrchr(pszSelDat, _T('.'));
	if (NULL == pszExt || (0 != _tcsicmp(_T(".dat"), pszExt))) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData:\n\n"));
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXTENSION), pszSelDat, _T(".dat"));
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return false;
	}

	const TCHAR* p = pszSelDat + _tcslen(pszSelDat), * dir_end = NULL;
	INT32 nCount = 0;
	while (p > pszSelDat) {
		if ((_T('/') == *p) || (_T('\\') == *p)) {
			TCHAR c = *(p - 1);
			if ((_T('/') == c) ||
				(_T('\\') == c)) {		// xxxx//ssss\\...
				FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData:\n\n"));
				FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXIST), pszSelDat);
				FBAPopupDisplay(PUF_TYPE_ERROR);
				return false;
			}
			if (1 == ++nCount) {
				dir_end = p + 1;		// Intentionally add 1
			}
		}
		p--;
	}

	memset(szAppQuickPath, 0, sizeof(szAppQuickPath));
	_tcsncpy(szAppQuickPath, pszSelDat, dir_end - pszSelDat);

	return true;
}

char* RomdataGetDrvName()
{
	const TCHAR* pszReadMode = AdaptiveEncodingReads(szRomdataName);
	if (NULL == pszReadMode) return NULL;

	FILE* fp = _tfopen(szRomdataName, pszReadMode);
	if (NULL == fp) return NULL;

	TCHAR szBuf[MAX_PATH] = { 0 };
	TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = szBuf;

			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
			if (NULL == pszLabel) continue;
			if ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1])) continue;

			if (0 == _tcsicmp(_T("DrvName"), pszLabel) || 0 == _tcsicmp(_T("Parent"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No driver specified
				fclose(fp);
				return TCHARToANSI(pszInfo, NULL, 0);
			}
		}
	}
	fclose(fp);

	return NULL;
}

TCHAR* RomdataGetZipName(const TCHAR* pszFileName)
{
	const TCHAR* pszReadMode = AdaptiveEncodingReads(pszFileName);
	if (NULL == pszReadMode) return NULL;

	FILE* fp = _tfopen(pszFileName, pszReadMode);
	if (NULL == fp) return NULL;

	TCHAR szBuf[MAX_PATH] = { 0 };
	TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = szBuf;

			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
			if (NULL == pszLabel) continue;
			if ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1])) continue;

			if (0 == _tcsicmp(_T("ZipName"), pszLabel) || 0 == _tcsicmp(_T("RomName"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No romset specified
				fclose(fp);
				static TCHAR szRet[100];
				memset(szRet, 0, sizeof(szRet));
				return _tcscpy(szRet, pszInfo);
			}
		}
	}
	fclose(fp);

	return NULL;
}

TCHAR* RomdataGetDrvName(const TCHAR* pszFileName)
{
	const TCHAR* pszReadMode = AdaptiveEncodingReads(pszFileName);
	if (NULL == pszReadMode) return NULL;

	FILE* fp = _tfopen(pszFileName, pszReadMode);
	if (NULL == fp) return NULL;

	TCHAR szBuf[MAX_PATH] = { 0 };
	TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = szBuf;

			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
			if (NULL == pszLabel) continue;
			if ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1])) continue;

			if (0 == _tcsicmp(_T("DrvName"), pszLabel) || 0 == _tcsicmp(_T("Parent"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No romset specified
				fclose(fp);
				static TCHAR szRet[100];
				memset(szRet, 0, sizeof(szRet));
				return _tcscpy(szRet, pszInfo);
			}
		}
	}
	fclose(fp);

	return NULL;
}

// It is recommended to save and restore the state of nBurnDrvActive before and after the call
INT32 RomdataGetDrvIndex(const TCHAR* pszDrvName)
{
	// nBurnDrvActive must be saved and restored, as BurnAreaScan() depends on it
	const UINT32 nOldDrvActive = nBurnDrvActive;

	for (INT32 nDrvIndex = 0; nDrvIndex < nBurnDrvCount; nDrvIndex++) {
		nBurnDrvActive = nDrvIndex;
		if ((0 == _tcscmp(pszDrvName, BurnDrvGetText(DRV_NAME))) && (!(BurnDrvGetFlags() & BDF_BOARDROM))) {
			nBurnDrvActive = nOldDrvActive;
			return nDrvIndex;
		}
	}

	nBurnDrvActive = nOldDrvActive;
	return -1;
}

TCHAR* RomdataGetFullName(const TCHAR* pszFileName)
{
	const TCHAR* pszReadMode = AdaptiveEncodingReads(pszFileName);
	if (NULL == pszReadMode) return NULL;

	FILE* fp = _tfopen(pszFileName, pszReadMode);
	if (NULL == fp) return NULL;

	TCHAR szBuf[MAX_PATH] = { 0 };
	TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = szBuf;

			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
			if (NULL == pszLabel) continue;
			if ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1])) continue;

			if (0 == _tcsicmp(_T("FullName"), pszLabel) || 0 == _tcsicmp(_T("Game"), pszLabel)) {
				static TCHAR szMerger[260];
				memset(szMerger, 0, sizeof(szMerger));
				INT32 nAdd = 0;
				while (NULL != (pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME))) {
					_stprintf(szMerger + nAdd, _T("%s "), pszInfo);
					nAdd = _tcslen(szMerger);
				}
				szMerger[nAdd - 1] = _T('\0');
				return szMerger;
			}
		}
	}
	fclose(fp);

	return NULL;
}

static void RomDataCopyFileStem(const TCHAR* pszPath, TCHAR* pszOut, INT32 nCount)
{
	if (pszOut == NULL || nCount <= 0) return;

	pszOut[0] = _T('\0');
	if (pszPath == NULL || pszPath[0] == _T('\0')) return;

	const TCHAR* pszName = _tcsrchr(pszPath, _T('\\'));
	const TCHAR* pszAltName = _tcsrchr(pszPath, _T('/'));
	if (pszAltName && (pszName == NULL || pszAltName > pszName)) {
		pszName = pszAltName;
	}
	pszName = (pszName != NULL) ? (pszName + 1) : pszPath;

	_tcsncpy(pszOut, pszName, nCount - 1);
	pszOut[nCount - 1] = _T('\0');

	TCHAR* pszExt = _tcsrchr(pszOut, _T('.'));
	if (pszExt != NULL) {
		*pszExt = _T('\0');
	}
}

bool RomDataHasActiveDat()
{
	return (szRomdataName[0] != _T('\0'));
}

bool RomDataGetActiveDatPath(TCHAR* pszOut, INT32 nCount)
{
	if (pszOut == NULL || nCount <= 0) return false;

	pszOut[0] = _T('\0');
	if (!RomDataHasActiveDat()) return false;

	_tcsncpy(pszOut, szRomdataName, nCount - 1);
	pszOut[nCount - 1] = _T('\0');
	return (pszOut[0] != _T('\0'));
}

bool RomDataGetActiveZipName(TCHAR* pszOut, INT32 nCount)
{
	if (pszOut == NULL || nCount <= 0) return false;

	pszOut[0] = _T('\0');
	if (!RomDataHasActiveDat()) return false;

	if (RDI.szZipName[0] != '\0') {
		ANSIToTCHAR(RDI.szZipName, pszOut, nCount);
		pszOut[nCount - 1] = _T('\0');
		return (pszOut[0] != _T('\0'));
	}

	const TCHAR* pszZipName = RomdataGetZipName(szRomdataName);
	if (pszZipName && pszZipName[0] != _T('\0')) {
		_tcsncpy(pszOut, pszZipName, nCount - 1);
		pszOut[nCount - 1] = _T('\0');
		return true;
	}

	return false;
}

const TCHAR* RomDataGetActiveDisplayName()
{
	static TCHAR szDisplayName[MAX_PATH];
	szDisplayName[0] = _T('\0');

	if (!RomDataHasActiveDat()) {
		return BurnDrvGetText(DRV_NAME);
	}

	if (RDI.szFullName[0] != L'\0') {
#if defined(UNICODE) || defined(_UNICODE)
		return (const TCHAR*)RDI.szFullName;
#else
		WideCharToMultiByte(CP_ACP, 0, RDI.szFullName, -1, szDisplayName, _countof(szDisplayName), NULL, NULL);
		return szDisplayName;
#endif
	}

	const TCHAR* pszFullName = RomdataGetFullName(szRomdataName);
	if (pszFullName && pszFullName[0] != _T('\0')) {
		_tcsncpy(szDisplayName, pszFullName, _countof(szDisplayName) - 1);
		szDisplayName[_countof(szDisplayName) - 1] = _T('\0');
		return szDisplayName;
	}

	RomDataCopyFileStem(szRomdataName, szDisplayName, _countof(szDisplayName));
	return szDisplayName[0] ? szDisplayName : BurnDrvGetText(DRV_NAME);
}

bool RomDataGetStorageName(TCHAR* pszOut, INT32 nCount)
{
	if (pszOut == NULL || nCount <= 0) return false;

	pszOut[0] = _T('\0');

	if (RomDataHasActiveDat()) {
		RomDataCopyFileStem(szRomdataName, pszOut, nCount);
		if (pszOut[0] != _T('\0')) {
			return true;
		}
	}

	const TCHAR* pszDrvName = BurnDrvGetText(DRV_NAME);
	if (pszDrvName && pszDrvName[0] != _T('\0')) {
		_tcsncpy(pszOut, pszDrvName, nCount - 1);
		pszOut[nCount - 1] = _T('\0');
		return true;
	}

	return false;
}

static INT32 RomsetDuplicateName(const TCHAR* pszFileName)
{
	if (NULL != pDataRomDesc) return -2;

	TCHAR* pszZipName = RomdataGetZipName(pszFileName);
	if (NULL == pszZipName)   return -3;
/*
	return:	-1 is success
	0 ~ N	The name is duplicated
	-1		Not in the list of drivers
	-2		RomData mode
	-3		No results were found in the Dat file
*/
	return BurnDrvGetIndex(_TtoA(pszZipName));
}

// Checking in RomData mode is strictly prohibited
INT32 RomDataCheck(const TCHAR* pszDatFile)
{
	if (NULL == pszDatFile || !FileExists(pszDatFile)) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData:\n\n"));
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXIST), pszDatFile);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -1;
	}

	INT32 nRet = 0;
	const TCHAR* pszDrvName = RomdataGetDrvName(pszDatFile);
	if (NULL == pszDrvName) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData: %s\n\n"), pszDatFile);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_DRIVER_NOT_EXIST));
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -3;
	}

	const INT32 nDrvIdx = BurnDrvGetIndex(_TtoA(pszDrvName));
	if (-1 == nDrvIdx) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData: %s\n\n"), pszDatFile);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_DRIVER_NOT_EXIST));
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -4;
	}

	nRet = RomsetDuplicateName(pszDatFile);
	if (nRet >= 0) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData: %s\n\n"), pszDatFile);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_ROMSET_DUPLICATE));
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -5;
	}
/*
	-2 and -3 should have no chance of being detected and are reserved for now
*/
	if (-2 == nRet) {
		return -6;								// RomData mode
	}
	if (-3 == nRet) {
		return -7;								// No romset specified,
	}

/*
	Now we're going to go into RomData mode and check the integrity of the Romset
	Exit RomData mode as soon as the check is complete
*/
	TCHAR szBackup[MAX_PATH] = { 0 };
	_tcscpy(szBackup, szRomdataName);			// Backup szRomdataName
	_tcscpy(szRomdataName, pszDatFile);
	RomDataInit();								// Replace DrvName##RomDesc
	const UINT32 nOldDrvSel = nBurnDrvActive;	// Backup nBurnDrvActive
	nBurnDrvActive = nDrvIdx;					// Required nBurnDrvActive
	nRet = BzipOpen(1);
	if (1 == nRet) {							// ROMs error report
		BzipClose();
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData: %s\n\n"), pszDatFile);
		BzipOpen(0);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		nRet = -1;
	}
	BzipClose();
	nBurnDrvActive = nOldDrvSel;				// Restore nBurnDrvActive
	RomDataExit();								// Restore DrvName##RomDesc
	_tcscpy(szRomdataName, szBackup);			// Restore szRomdataName

	return (0 == nRet) ? nDrvIdx : nRet;
}

static DatListInfo* RomdataGetListInfo(const TCHAR* pszDatFile, bool bShowErrors = true)
{
	const TCHAR* pszReadMode = AdaptiveEncodingReads(pszDatFile);
	if (NULL == pszReadMode) return NULL;

	FILE* fp = _tfopen(pszDatFile, pszReadMode);
	if (NULL == fp) return NULL;

	DatListInfo* pDatListInfo = (DatListInfo*)malloc(sizeof(DatListInfo));
	if (NULL == pDatListInfo) {
		fclose(fp); return NULL;
	}
	memset(pDatListInfo, 0, sizeof(DatListInfo));


	TCHAR szBuf[MAX_PATH] = { 0 };
	TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (NULL != _fgetts(szBuf, MAX_PATH, fp)) {
			pszBuf = szBuf;

			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
			if (NULL == pszLabel) continue;
			if ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1])) continue;

			if (0 == _tcsicmp(_T("ZipName"), pszLabel) || 0 == _tcsicmp(_T("RomName"), pszLabel)) {	// Romset
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) {						// No romset specified
					FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData:\n\n"));
					FBAPopupAddText(PUF_TEXT_DEFAULT, _T("%s:?\n\n"),   FBALoadStringEx(hAppInst, IDS_ROMDATA_ROMSET,  true));
					FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_LOAD_NOTFOUND), _T("ROM"));
					FBAPopupDisplay(PUF_TYPE_ERROR);

					free(pDatListInfo); fclose(fp);
					return NULL;
				}
				_tcscpy(pDatListInfo->szRomSet, pszInfo);
				pDatListInfo->nMarker |= (1 << 0);
			}
			if (0 == _tcsicmp(_T("DrvName"), pszLabel) || 0 == _tcsicmp(_T("Parent"), pszLabel)) {	// Driver
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) {						// No driver specified
					if (bShowErrors) {
						FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData:\n\n"));
						FBAPopupAddText(PUF_TEXT_DEFAULT, _T("%s:?\n\n"),   FBALoadStringEx(hAppInst, IDS_ROMDATA_DRIVER,  true));
						FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_NO_DRIVER_SELECTED));
						FBAPopupDisplay(PUF_TYPE_ERROR);
					}

					free(pDatListInfo); fclose(fp);
					return NULL;
				}
				_tcscpy(pDatListInfo->szDrvName, pszInfo);
				pDatListInfo->nMarker |= (1 << 1);

				{											// Hardware
					UINT32 nOldDrvSel = nBurnDrvActive;		// Backup
					nBurnDrvActive = BurnDrvGetIndex(TCHARToANSI(pDatListInfo->szDrvName, NULL, 0));
					if (-1 == nBurnDrvActive) {
						if (bShowErrors) {
							FBAPopupAddText(PUF_TEXT_DEFAULT, _T("RomData:\n\n"));
							FBAPopupAddText(PUF_TEXT_DEFAULT, _T("%s: %s\n\n"), FBALoadStringEx(hAppInst, IDS_ROMDATA_DRIVER,  true), pDatListInfo->szDrvName);
							FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_DRIVER_NOT_EXIST));
							FBAPopupDisplay(PUF_TYPE_ERROR);
						}

						nBurnDrvActive = nOldDrvSel;		// Restore
						free(pDatListInfo); fclose(fp);
						return NULL;
					}

					const INT32 nGetTextFlags = nLoadMenuShowY & (1 << 31) ? DRV_ASCIIONLY : 0;
					TCHAR szCartridge[100] = { 0 };

					_stprintf(szCartridge, FBALoadStringEx(hAppInst, IDS_MVS_CARTRIDGE, true));
					_stprintf(
						pDatListInfo->szHardware,
						FBALoadStringEx(hAppInst, IDS_ROMDATA_HARDWARE_DESC, true),
						(HARDWARE_SNK_MVS == (BurnDrvGetHardwareCode() & HARDWARE_SNK_MVS) && HARDWARE_SNK_NEOGEO == (BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK)) ? szCartridge : BurnDrvGetText(nGetTextFlags | DRV_SYSTEM)
					);

					pDatListInfo->nMarker |= (1 << 2);
					nBurnDrvActive = nOldDrvSel;			// Restore
				}
			}
			if (0 == _tcsicmp(_T("FullName"), pszLabel) || 0 == _tcsicmp(_T("Game"), pszLabel)) {
				TCHAR szMerger[260] = { 0 };
				INT32 nAdd = 0;
				while (NULL != (pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME))) {
					_stprintf(szMerger + nAdd, _T("%s "), pszInfo);
					nAdd = _tcslen(szMerger);
				}
				szMerger[nAdd - 1] = _T('\0');
				_tcscpy(pDatListInfo->szFullName, szMerger);
				pDatListInfo->nMarker |= (1 << 3);
			}
		}
	}
	fclose(fp);

	if (!(pDatListInfo->nMarker & (UINT32)0x03)) {			// No romset specified or No driver specified
		free(pDatListInfo); pDatListInfo = NULL;
		return NULL;
	}
	if (!(pDatListInfo->nMarker & (UINT32)0x08)) {			// No FullName specified
		_tcscpy(pDatListInfo->szFullName, pDatListInfo->szRomSet);
	}

	return pDatListInfo;
}

#undef DELIM_TOKENS_NAME

void RomDataInit()
{
	INT32 nLen = LoadRomdata();

	if ((nLen >= 0) && (NULL == pDRD)) {
		pDRD = (struct BurnRomInfo*)malloc((nLen + 1) * sizeof(BurnRomInfo));
		if (NULL == pDRD) return;

		LoadRomdata();
		RDI.nDriverId = BurnDrvGetIndex(RDI.szDrvName);

		if (RDI.nDriverId >= 0) {
			BurnDrvSetZipName(RDI.szZipName, RDI.nDriverId);
		}
	}
}

void RomDataSetFullName()
{
	// At this point, the driver's default ZipName has been replaced with the ZipName from the rom data
	RDI.nDriverId = BurnDrvGetIndex(RDI.szZipName);

	if (RDI.nDriverId >= 0) {
		wchar_t* szOldName = BurnDrvGetFullNameW(RDI.nDriverId);
		memset(RDI.szOldName, '\0', sizeof(RDI.szOldName));

		if (0 != wcscmp(szOldName, RDI.szOldName)) {
			wcscpy(RDI.szOldName, szOldName);
		}

		BurnDrvSetFullNameW(RDI.szFullName, RDI.nDriverId);
	}
}

void RomDataExit()
{
	if (NULL != pDataRomDesc) {
		for (INT32 i = 0; i < RDI.nDescCount + 1; i++) {
			free(pDataRomDesc[i].szName);
		}
		free(pDataRomDesc); pDataRomDesc = NULL;

		if (RDI.nDriverId >= 0) {
			BurnDrvSetZipName(RDI.szDrvName, RDI.nDriverId);
			if (0 != wcscmp(BurnDrvGetFullNameW(RDI.nDriverId), RDI.szOldName)) {
				BurnDrvSetFullNameW(RDI.szOldName, RDI.nDriverId);
			}
		}

		memset(&RDI,          0, sizeof(RomDataInfo));
		memset(szRomdataName, 0, sizeof(szRomdataName));

		RDI.nDescCount = -1;
	}
}

static bool RomdataIsSearchSeparator(TCHAR c)
{
	if (_istspace(c)) return true;

	if (c < 0x80 && _istpunct(c)) return true;

	switch (c) {
		case _T('('):
		case _T(')'):
		case _T('['):
		case _T(']'):
		case _T('{'):
		case _T('}'):
		case _T('-'):
		case _T('_'):
		case _T('/'):
		case _T('\\'):
		case _T('.'):
		case _T(','):
		case _T(':'):
		case _T(';'):
		case _T('\''):
		case _T('\"'):
		case _T('+'):
		case _T('&'):
		case _T('!'):
		case _T('?'):
			return true;
	}

	return false;
}

static void RomdataNormalizeSearchText(const TCHAR* pszSrc, TCHAR* pszDst, size_t nDstCount)
{
	if (pszDst == NULL || nDstCount == 0) return;

	pszDst[0] = _T('\0');
	if (pszSrc == NULL) return;

	size_t j = 0;
	for (size_t i = 0; pszSrc[i] && j + 1 < nDstCount; i++) {
		TCHAR c = _totlower(pszSrc[i]);
		if (RomdataIsSearchSeparator(c)) continue;

		pszDst[j++] = c;
	}

	pszDst[j] = _T('\0');
}

static TCHAR RomdataGetPinyinInitial(TCHAR c)
{
	if (c < 0x80) {
		return _istalnum(c) ? _totlower(c) : 0;
	}

#if defined(UNICODE) || defined(_UNICODE)
	WCHAR wide[2] = { c, 0 };
	char mb[4] = { 0 };
	INT32 len = WideCharToMultiByte(936, 0, wide, 1, mb, sizeof(mb), NULL, NULL);
	if (len < 2) return 0;

	UINT32 code = (((UINT8)mb[0]) << 8) | (UINT8)mb[1];
#else
	return 0;
#endif

	if (code < 0xB0A1 || code > 0xD7F9) return 0;

	if (code < 0xB0C5) return _T('a');
	if (code < 0xB2C1) return _T('b');
	if (code < 0xB4EE) return _T('c');
	if (code < 0xB6EA) return _T('d');
	if (code < 0xB7A2) return _T('e');
	if (code < 0xB8C1) return _T('f');
	if (code < 0xB9FE) return _T('g');
	if (code < 0xBBF7) return _T('h');
	if (code < 0xBFA6) return _T('j');
	if (code < 0xC0AC) return _T('k');
	if (code < 0xC2E8) return _T('l');
	if (code < 0xC4C3) return _T('m');
	if (code < 0xC5B6) return _T('n');
	if (code < 0xC5BE) return _T('o');
	if (code < 0xC6DA) return _T('p');
	if (code < 0xC8BB) return _T('q');
	if (code < 0xC8F6) return _T('r');
	if (code < 0xCBFA) return _T('s');
	if (code < 0xCDDA) return _T('t');
	if (code < 0xCEF4) return _T('w');
	if (code < 0xD1B9) return _T('x');
	if (code < 0xD4D1) return _T('y');
	return _T('z');
}

static void RomdataBuildInitialsSearchText(const TCHAR* pszSrc, TCHAR* pszDst, size_t nDstCount)
{
	if (pszDst == NULL || nDstCount == 0) return;

	pszDst[0] = _T('\0');
	if (pszSrc == NULL) return;

	size_t j = 0;
	for (size_t i = 0; pszSrc[i] && j + 1 < nDstCount; i++) {
		TCHAR c = _totlower(pszSrc[i]);
		if (RomdataIsSearchSeparator(c)) continue;

		if (c < 0x80) {
			if (_istalnum(c)) {
				pszDst[j++] = c;
			}
			continue;
		}

		TCHAR initial = RomdataGetPinyinInitial(c);
		if (initial) {
			pszDst[j++] = initial;
			continue;
		}

		pszDst[j++] = c;
	}

	pszDst[j] = _T('\0');
}

static bool RomdataStringMatchesSearch(const TCHAR* pszValue)
{
	if (szRomdataSearchNormalized[0] == _T('\0')) return true;
	if (pszValue == NULL || pszValue[0] == _T('\0')) return false;

	TCHAR szNormalized[2048] = { 0 };
	TCHAR szInitials[2048] = { 0 };

	RomdataNormalizeSearchText(pszValue, szNormalized, _countof(szNormalized));
	if (_tcsstr(szNormalized, szRomdataSearchNormalized)) return true;

	RomdataBuildInitialsSearchText(pszValue, szInitials, _countof(szInitials));
	return _tcsstr(szInitials, szRomdataSearchNormalized) != NULL;
}

static bool RomdataMatchesSearch(const DatListInfo* pDatListInfo)
{
	if (szRomdataSearchNormalized[0] == _T('\0')) return true;
	if (pDatListInfo == NULL) return false;

	return RomdataStringMatchesSearch(pDatListInfo->szFullName)
		|| RomdataStringMatchesSearch(pDatListInfo->szRomSet)
		|| RomdataStringMatchesSearch(pDatListInfo->szDrvName)
		|| RomdataStringMatchesSearch(pDatListInfo->szHardware);
}

// Add DatListInfo to List
static INT32 RomdataAddListItem(TCHAR* pszDatFile)
{
	DatListInfo* pDatListInfo = RomdataGetListInfo(pszDatFile, false);
	if (NULL == pDatListInfo) return -1;

	RomDataListEntry entry = { _T(""), { 0 }, -1, -1 };
	_tcsncpy(entry.szDatPath, pszDatFile, _countof(entry.szDatPath) - 1);
	entry.szDatPath[_countof(entry.szDatPath) - 1] = _T('\0');
	memcpy(&entry.info, pDatListInfo, sizeof(DatListInfo));
	entry.nImageIndex = FindHardwareIconIndex(pDatListInfo->szDrvName);
	g_RomDataSourceEntries.push_back(entry);

	free(pDatListInfo); pDatListInfo = NULL;

	return (INT32)g_RomDataSourceEntries.size() - 1;
}

static INT32 ends_with_slash(const TCHAR* dirPath)
{
	UINT32 len = _tcslen(dirPath);
	if (0 == len) return 0;

	TCHAR last_char = dirPath[len - 1];
	return (last_char == _T('/') || last_char == _T('\\'));
}

#define IS_STRING_EMPTY(s) (NULL == (s) || (s)[0] == _T('\0'))

static void RomdataListFindDats(const TCHAR* dirPath)
{
	if (IS_STRING_EMPTY(dirPath)) return;

	TCHAR searchPath[MAX_PATH] = { 0 };

	const TCHAR* szFormatA = ends_with_slash(dirPath) ? _T("%s*") : _T("%s\\*.*");
	const TCHAR* szFormatB = ends_with_slash(dirPath) ? _T("%s%s") : _T("%s\\%s");

	_stprintf(searchPath, szFormatA, dirPath);

	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(searchPath, &findFileData);
	if (INVALID_HANDLE_VALUE == hFind) return;

	do {
		if (0 == _tcscmp(findFileData.cFileName, _T(".")) || 0 == _tcscmp(findFileData.cFileName, _T("..")))
			continue;
		// like: c:\1st_dir + '\' + 2nd_dir + '\' + "1.dat" + '\0' = 8 chars
		if ((_tcslen(dirPath) + _tcslen(findFileData.cFileName)) > (MAX_PATH - 8))
			continue;

		TCHAR szFullPath[MAX_PATH] = { 0 };
		_stprintf(szFullPath, szFormatB, dirPath, findFileData.cFileName);

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (bRDListScanSub)
				RomdataListFindDats(szFullPath);
		} else {
			if (IsFileExt(findFileData.cFileName, _T(".dat")))
				RomdataAddListItem(szFullPath);
		}
	} while (FindNextFile(hFind, &findFileData));

	FindClose(hFind);
}

bool FindZipNameFromDats(const TCHAR* dirPath, const char* pszZipName, TCHAR* pszFindDat)
{
	if (IS_STRING_EMPTY(dirPath)) return false;
	if (NULL == pszZipName)       return false;

	char szBuf[100] = { 0 };
	strcpy(szBuf, pszZipName);

	TCHAR searchPath[MAX_PATH] = { 0 };

	const TCHAR* szFormatA = ends_with_slash(dirPath) ? _T("%s*") : _T("%s\\*.*");
	const TCHAR* szFormatB = ends_with_slash(dirPath) ? _T("%s%s") : _T("%s\\%s");

	_stprintf(searchPath, szFormatA, dirPath);

	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(searchPath, &findFileData);
	if (INVALID_HANDLE_VALUE == hFind) return false;

	do {
		if (0 == _tcscmp(findFileData.cFileName, _T(".")) || 0 == _tcscmp(findFileData.cFileName, _T("..")))
			continue;
		// like: c:\1st_dir + '\' + 2nd_dir + '\' + "1.dat" + '\0' = 8 chars
		if ((_tcslen(dirPath) + _tcslen(findFileData.cFileName)) > (MAX_PATH - 8))
			continue;

		TCHAR szFullPath[MAX_PATH] = { 0 };
		_stprintf(szFullPath, szFormatB, dirPath, findFileData.cFileName);

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (bRDListScanSub)
				FindZipNameFromDats(szFullPath, szBuf, pszFindDat);
		} else {
			if (NULL == pszFindDat) return false;
			if (IsFileExt(findFileData.cFileName, _T(".dat"))){
				const TCHAR* pszBuf = RomdataGetZipName(szFullPath);
				if (NULL == pszBuf) continue;
				if (0 == strcmp(szBuf, TCHARToANSI(pszBuf, NULL, 0))) {
					_tcscpy(pszFindDat, szFullPath);
					FindClose(hFind);
					return true;
				}
			}
		}
	} while (FindNextFile(hFind, &findFileData));

	FindClose(hFind);
	return false;
}

#undef IS_STRING_EMPTY

bool RomDataExportTemplate(HWND hWnd, const INT32 nDrvSelect)
{
	if (-1 == nDrvSelect) return false;

	const UINT32 nOldDrvSel = nBurnDrvActive;

	TCHAR szFilter[150] = { 0 };
	_stprintf(szFilter, FBALoadStringEx(hAppInst, IDS_DISK_FILE_ROMDATA, true), _T(APP_TITLE));
	memcpy(szFilter + _tcslen(szFilter), _T(" (*.dat)\0*.dat\0\0"), 16 * sizeof(TCHAR));
	_stprintf(szChoice, _T("template_%s.dat"), BurnDrvGetText(DRV_NAME));

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szChoice;
	ofn.nMaxFile = sizeof(szChoice) / sizeof(TCHAR);
	ofn.lpstrInitialDir = _T(".");
	ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = _T("dat");
	ofn.Flags |= OFN_OVERWRITEPROMPT;

	if (0 == GetOpenFileName(&ofn)) {
		nBurnDrvActive = nOldDrvSel;
		return false;
	}

	nBurnDrvActive = nDrvSelect;

	FILE* fp = _tfopen(szChoice, _T("w"));
	if (NULL == fp) {
		nBurnDrvActive = nOldDrvSel;
		return false;
	}

	_ftprintf(fp, _T("// RomData template for FinalBurn Neo\n\n"));
	_ftprintf(fp, _T("// The *** in the ZipName field cannot be empty and cannot be duplicated with DrvName\n"));
	_ftprintf(fp, _T("ZipName: ***\n"));
	_ftprintf(fp, _T("DrvName: %s\n"),        BurnDrvGetText(DRV_NAME));
	_ftprintf(fp, _T("FullName: \"%s\"\n\n"), BurnDrvGetText(DRV_FULLNAME));
	_ftprintf(fp, _T("// Name\t\tLen\t\tCrc32\t\tType\n"));

	char* pszRomName = NULL;
	for (INT32 i = 0; !BurnDrvGetRomName(&pszRomName, i, 0); i++) {
		struct BurnRomInfo ri = { 0 };

		BurnDrvGetRomInfo(&ri, i);	// Get info about the rom
		if ((NULL == pszRomName) || ('\0' == *pszRomName))
			continue;
		if ((ri.nType & BRF_BIOS) && (i >= 0x80))
			continue;

		_ftprintf(fp, _T("\"%hs\",\t0x%08x,\t0x%08x,\t0x%08x\n"), pszRomName, ri.nLen, ri.nCrc, ri.nType);
	}
	pszRomName     = NULL;
	fclose(fp); fp = NULL;
	nBurnDrvActive = nOldDrvSel;

	return true;
}

static void ListViewSort(INT32 nDirection, INT32 nColumn)
{
	sort_direction = nDirection;
	nRomDataSortColumn = nColumn;
	RomDataRefreshList(true, false);
}

static void RomDataApplyListViewStyle()
{
	if (hRDListView == NULL) {
		return;
	}

	LONG_PTR nStyle = GetWindowLongPtr(hRDListView, GWL_STYLE);
	nStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_NOHSCROLL;
	SetWindowLongPtr(hRDListView, GWL_STYLE, nStyle);

	TreeView_SetBkColor(hRDListView, RGB(0xFF, 0xFF, 0xFF));
	TreeView_SetTextColor(hRDListView, RGB(0x00, 0x00, 0x00));
	TreeView_SetLineColor(hRDListView, RGB(0x80, 0x80, 0x80));
#ifdef TVS_EX_DOUBLEBUFFER
	TreeView_SetExtendedStyle(hRDListView, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
#endif
}

static void RomDataClearCache()
{
	g_RomDataEntries.clear();
	g_RomDataSourceEntries.clear();
	g_RomDataGroups.clear();
	g_RomDataVisibleRows.clear();
}

static void RomDataClearVisibleCache()
{
	g_RomDataEntries.clear();
	g_RomDataGroups.clear();
	g_RomDataVisibleRows.clear();
}

static bool RomDataRestoreGroupState(const std::vector<RomDataGroupEntry>& previousGroups, const TCHAR* pszDrvName, bool* pbExpanded)
{
	if (pbExpanded == NULL) {
		return false;
	}

	const TCHAR* pszSafeDrvName = (pszDrvName && pszDrvName[0]) ? pszDrvName : _T("(Unknown)");

	for (size_t i = 0; i < previousGroups.size(); i++) {
		if (_tcsicmp(previousGroups[i].szDrvName, pszSafeDrvName) == 0) {
			*pbExpanded = previousGroups[i].bExpanded;
			return true;
		}
	}

	return false;
}

static INT32 RomDataCompareText(const TCHAR* pszLhs, const TCHAR* pszRhs)
{
	return _tcsicmp(pszLhs ? pszLhs : _T(""), pszRhs ? pszRhs : _T(""));
}

static const TCHAR* RomDataGetSortText(const RomDataListEntry& entry, INT32 nColumn)
{
	switch (nColumn) {
		case 1: return entry.info.szRomSet;
		case 2: return entry.info.szDrvName;
		case 3: return entry.szDatPath;
		default: return entry.info.szFullName;
	}
}

static bool RomDataEntryLess(const RomDataListEntry& lhs, const RomDataListEntry& rhs)
{
	INT32 nCmp = RomDataCompareText(lhs.info.szDrvName, rhs.info.szDrvName);
	if (nCmp != 0) return nCmp < 0;

	nCmp = RomDataCompareText(RomDataGetSortText(lhs, nRomDataSortColumn), RomDataGetSortText(rhs, nRomDataSortColumn));
	if (sort_direction == SORT_DESCENDING) {
		nCmp = -nCmp;
	}
	if (nCmp != 0) return nCmp < 0;

	nCmp = RomDataCompareText(lhs.info.szFullName, rhs.info.szFullName);
	if (nCmp != 0) return nCmp < 0;

	nCmp = RomDataCompareText(lhs.info.szRomSet, rhs.info.szRomSet);
	if (nCmp != 0) return nCmp < 0;

	return RomDataCompareText(lhs.szDatPath, rhs.szDatPath) < 0;
}

static int __cdecl RomDataEntryCompareForQSort(const void* pLhs, const void* pRhs)
{
	const RomDataListEntry* lhs = (const RomDataListEntry*)pLhs;
	const RomDataListEntry* rhs = (const RomDataListEntry*)pRhs;

	if (lhs == NULL || rhs == NULL) {
		return 0;
	}

	if (RomDataEntryLess(*lhs, *rhs)) {
		return -1;
	}

	if (RomDataEntryLess(*rhs, *lhs)) {
		return 1;
	}

	return 0;
}

static void RomDataSortCache()
{
	if (g_RomDataEntries.size() <= 1) {
		return;
	}

	qsort(g_RomDataEntries.data(), g_RomDataEntries.size(), sizeof(RomDataListEntry), RomDataEntryCompareForQSort);
}

static void RomDataBuildGroupsFromCache()
{
	std::vector<RomDataGroupEntry> previousGroups = g_RomDataGroups;
	g_RomDataGroups.clear();
	g_RomDataGroups.reserve(g_RomDataEntries.size());

	INT32 nCurrentGroupId = -1;
	TCHAR szCurrentDrvName[100] = { 0 };

	for (size_t i = 0; i < g_RomDataEntries.size(); i++) {
		const TCHAR* pszSafeDrvName = (g_RomDataEntries[i].info.szDrvName[0] != _T('\0')) ? g_RomDataEntries[i].info.szDrvName : _T("(Unknown)");

		if (nCurrentGroupId < 0 || _tcsicmp(szCurrentDrvName, pszSafeDrvName) != 0) {
			RomDataGroupEntry entry = { _T(""), 0, 0, false, false };
			_tcsncpy(entry.szDrvName, pszSafeDrvName, _countof(entry.szDrvName) - 1);
			entry.szDrvName[_countof(entry.szDrvName) - 1] = _T('\0');
			entry.nGroupId = (INT32)g_RomDataGroups.size();
			entry.bStateRestored = RomDataRestoreGroupState(previousGroups, pszSafeDrvName, &entry.bExpanded) ? true : false;
			g_RomDataGroups.push_back(entry);
			nCurrentGroupId = entry.nGroupId;
			_tcsncpy(szCurrentDrvName, pszSafeDrvName, _countof(szCurrentDrvName) - 1);
			szCurrentDrvName[_countof(szCurrentDrvName) - 1] = _T('\0');
		}

		g_RomDataEntries[i].nGroupId = nCurrentGroupId;
		if (nCurrentGroupId >= 0 && nCurrentGroupId < (INT32)g_RomDataGroups.size()) {
			g_RomDataGroups[nCurrentGroupId].nEntryCount++;
		}
	}

	for (size_t i = 0; i < g_RomDataGroups.size(); i++) {
		if (!g_RomDataGroups[i].bStateRestored && g_RomDataGroups[i].nEntryCount <= 1) {
			g_RomDataGroups[i].bExpanded = true;
		}
	}
}

static void RomDataRebuildVisibleEntriesFromSource()
{
	RomDataClearVisibleCache();
	g_RomDataEntries.reserve(g_RomDataSourceEntries.size());

	for (size_t i = 0; i < g_RomDataSourceEntries.size(); i++) {
		if (!RomdataMatchesSearch(&g_RomDataSourceEntries[i].info)) {
			continue;
		}

		g_RomDataEntries.push_back(g_RomDataSourceEntries[i]);
	}

	RomDataSortCache();
	RomDataBuildGroupsFromCache();
}

static void RomDataBuildVisibleRows()
{
	g_RomDataVisibleRows.clear();
	g_RomDataVisibleRows.reserve(g_RomDataEntries.size() + g_RomDataGroups.size());

	const bool bForceExpand = (szRomdataSearchNormalized[0] != _T('\0'));
	size_t nEntryIndex = 0;

	for (size_t i = 0; i < g_RomDataGroups.size(); i++) {
		RomDataVisibleRow headerRow = { true, g_RomDataGroups[i].nGroupId, -1 };
		g_RomDataVisibleRows.push_back(headerRow);

		const bool bExpanded = bForceExpand || g_RomDataGroups[i].bExpanded;
		while (nEntryIndex < g_RomDataEntries.size() && g_RomDataEntries[nEntryIndex].nGroupId == g_RomDataGroups[i].nGroupId) {
			if (bExpanded) {
				RomDataVisibleRow entryRow = { false, g_RomDataGroups[i].nGroupId, (INT32)nEntryIndex };
				g_RomDataVisibleRows.push_back(entryRow);
			}
			nEntryIndex++;
		}
	}
}

static void RomDataFormatGroupLabel(const RomDataGroupEntry& group, TCHAR* pszLabel, INT32 nCount)
{
	if (pszLabel == NULL || nCount <= 0) {
		return;
	}

	pszLabel[0] = _T('\0');
	_sntprintf(pszLabel, nCount - 1, _T("%s (%d)"), group.szDrvName, group.nEntryCount);
	pszLabel[nCount - 1] = _T('\0');
}

static void RomDataFormatEntryLabel(const RomDataListEntry& entry, TCHAR* pszText, INT32 nCount)
{
	if (pszText == NULL || nCount <= 0) {
		return;
	}

	pszText[0] = _T('\0');

	if (entry.info.szRomSet[0] != _T('\0')) {
		_sntprintf(pszText, nCount - 1, _T("%s [%s]"), entry.info.szFullName, entry.info.szRomSet);
	} else {
		_tcsncpy(pszText, entry.info.szFullName, nCount - 1);
	}
	pszText[nCount - 1] = _T('\0');
}

static LPARAM RomDataMakeTreeItemParam(INT32 nIndex, bool bGroupHeader)
{
	return (((LPARAM)nIndex + 1) << 1) | (bGroupHeader ? 1 : 0);
}

static bool RomDataTreeParamIsGroup(LPARAM nParam)
{
	return (nParam & 1) != 0;
}

static INT32 RomDataTreeParamToIndex(LPARAM nParam)
{
	if (nParam <= 0) {
		return -1;
	}

	return (INT32)((nParam >> 1) - 1);
}

static HTREEITEM RomDataInsertTreeItem(HTREEITEM hParent, const TCHAR* pszText, INT32 nImageIndex, LPARAM nParam)
{
	if (hRDListView == NULL || pszText == NULL) {
		return NULL;
	}

	TVINSERTSTRUCT tvis;
	memset(&tvis, 0, sizeof(tvis));
	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText = (LPTSTR)pszText;
	tvis.item.lParam = nParam;

	if (nImageIndex >= 0) {
		tvis.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvis.item.iImage = nImageIndex;
		tvis.item.iSelectedImage = nImageIndex;
	}

	return TreeView_InsertItem(hRDListView, &tvis);
}

static LPARAM RomDataGetTreeItemParam(HTREEITEM hItem)
{
	if (hRDListView == NULL || hItem == NULL) {
		return 0;
	}

	TVITEM item;
	memset(&item, 0, sizeof(item));
	item.mask = TVIF_PARAM;
	item.hItem = hItem;
	if (!TreeView_GetItem(hRDListView, &item)) {
		return 0;
	}

	return item.lParam;
}

static const RomDataListEntry* RomDataGetEntryForTreeItem(HTREEITEM hItem)
{
	const LPARAM nParam = RomDataGetTreeItemParam(hItem);
	if (RomDataTreeParamIsGroup(nParam)) {
		return NULL;
	}

	return RomDataGetEntryByIndex(RomDataTreeParamToIndex(nParam));
}

static RomDataGroupEntry* RomDataGetGroupForTreeItem(HTREEITEM hItem)
{
	const LPARAM nParam = RomDataGetTreeItemParam(hItem);
	if (!RomDataTreeParamIsGroup(nParam)) {
		return NULL;
	}

	return RomDataGetGroupById(RomDataTreeParamToIndex(nParam));
}

static HTREEITEM RomDataGetSelectedTreeItem()
{
	return (hRDListView != NULL) ? TreeView_GetSelection(hRDListView) : NULL;
}

static void RomDataPopulateListViewFromCache()
{
	if (hRDListView == NULL) {
		return;
	}

	g_RomDataVisibleRows.clear();
	TreeView_DeleteAllItems(hRDListView);

	const bool bForceExpand = (szRomdataSearchNormalized[0] != _T('\0'));
	TCHAR szLabel[1200] = { 0 };
	size_t nEntryIndex = 0;

	for (size_t i = 0; i < g_RomDataGroups.size(); i++) {
		RomDataFormatGroupLabel(g_RomDataGroups[i], szLabel, _countof(szLabel));
		g_RomDataGroups[i].hTreeItem = RomDataInsertTreeItem(TVI_ROOT, szLabel, -1, RomDataMakeTreeItemParam(g_RomDataGroups[i].nGroupId, true));

		while (nEntryIndex < g_RomDataEntries.size() && g_RomDataEntries[nEntryIndex].nGroupId == g_RomDataGroups[i].nGroupId) {
			RomDataFormatEntryLabel(g_RomDataEntries[nEntryIndex], szLabel, _countof(szLabel));
			g_RomDataEntries[nEntryIndex].hTreeItem = RomDataInsertTreeItem(
				g_RomDataGroups[i].hTreeItem,
				szLabel,
				bEnableIcons ? g_RomDataEntries[nEntryIndex].nImageIndex : -1,
				RomDataMakeTreeItemParam((INT32)nEntryIndex, false));
			nEntryIndex++;
		}

		if (g_RomDataGroups[i].hTreeItem != NULL && (bForceExpand || g_RomDataGroups[i].bExpanded)) {
			TreeView_Expand(hRDListView, g_RomDataGroups[i].hTreeItem, TVE_EXPAND);
		}
	}
}

static const RomDataListEntry* RomDataGetEntryByIndex(INT32 nIndex)
{
	if (nIndex < 0 || nIndex >= (INT32)g_RomDataEntries.size()) {
		return NULL;
	}

	return &g_RomDataEntries[nIndex];
}

static const RomDataListEntry* RomDataGetSelectedEntry()
{
	HTREEITEM hSelected = RomDataGetSelectedTreeItem();
	const RomDataListEntry* pEntry = RomDataGetEntryForTreeItem(hSelected);
	if (pEntry != NULL) {
		nSelItem = RomDataTreeParamToIndex(RomDataGetTreeItemParam(hSelected));
	} else {
		nSelItem = -1;
	}
	return pEntry;
}

static const RomDataListEntry* RomDataGetEntryForItem(INT32 nItem)
{
	return RomDataGetEntryByIndex(nItem);
}

static const RomDataVisibleRow* RomDataGetVisibleRow(INT32 nItem)
{
	if (nItem < 0 || nItem >= (INT32)g_RomDataVisibleRows.size()) {
		return NULL;
	}

	return &g_RomDataVisibleRows[nItem];
}

static RomDataGroupEntry* RomDataGetGroupById(INT32 nGroupId)
{
	if (nGroupId < 0 || nGroupId >= (INT32)g_RomDataGroups.size()) {
		return NULL;
	}

	return &g_RomDataGroups[nGroupId];
}

static void RomDataSyncDriverForItem(INT32 nItem)
{
	nSelItem = nItem;

	const RomDataListEntry* pEntry = RomDataGetEntryForItem(nItem);
	if (pEntry == NULL) {
		return;
	}

	INT32 nDrvIdx = BurnDrvGetIndex(_TtoA(pEntry->info.szDrvName));
	if (nDrvIdx >= 0) {
		nBurnDrvActive = nDrvIdx;
	}
}

static void RomDataInitListView()
{
	RomDataApplyListViewStyle();
	TreeView_SetIndent(hRDListView, 18);
	sort_direction = SORT_ASCENDING; // dink
	nRomDataSortColumn = 0;
}

static PNGRESOLUTION GetPNGResolution(TCHAR* szFile)
{
	PNGRESOLUTION nResolution = { 0, 0 };
	IMAGE img = { 0, 0, 0, 0, NULL, NULL, 0 };

	FILE* fp = _tfopen(szFile, _T("rb"));

	if (!fp) {
		return nResolution;
	}

	PNGGetInfo(&img, fp);

	nResolution.nWidth  = img.width;
	nResolution.nHeight = img.height;

	fclose(fp);

	return nResolution;
}

static void RomDataShowPreview(HWND hDlg, TCHAR* szFile, INT32 nCtrID, INT32 nFrameCtrID, float maxw, float maxh)
{
	PNGRESOLUTION PNGRes = { 0, 0 };
	if (!_tfopen(szFile, _T("rb"))) {
		HRSRC hrsrc     = FindResource(NULL, MAKEINTRESOURCE(BMP_SPLASH), RT_BITMAP);
		HGLOBAL hglobal = LoadResource(NULL, (HRSRC)hrsrc);

		BITMAPINFOHEADER* pbmih = (BITMAPINFOHEADER*)LockResource(hglobal);

		PNGRes.nWidth  = pbmih->biWidth;
		PNGRes.nHeight = pbmih->biHeight;

		FreeResource(hglobal);
	} else {
		PNGRes = GetPNGResolution(szFile);
	}

	// ------------------------------------------------------
	// PROPER ASPECT RATIO CALCULATIONS

	float w = (float)PNGRes.nWidth;
	float h = (float)PNGRes.nHeight;
	bool bHostedPreview = bRomdataHostedMode && nCtrID == nRomdataHostedPreviewCtrlId;
	const bool bStandaloneManagerPreview =
		!bHostedPreview &&
		hDlg == hRDMgrWnd &&
		(nCtrID == IDC_ROMDATA_TITLE || nCtrID == IDC_ROMDATA_PREVIEW);
	const INT32 hostedInsetX = 10;
	const INT32 hostedInsetY = 16;
	const INT32 hostedInsetBottom = 6;
	const INT32 managerInsetX = 10;
	const INT32 managerInsetY = 16;
	const INT32 managerInsetBottom = 8;
	HWND hPreviewCtrl = GetDlgItem(hDlg, nCtrID);
	HWND hFrameCtrl = GetDlgItem(hDlg, nFrameCtrID);
	RECT frameRect = { 0, 0, 0, 0 };

	if (bStandaloneManagerPreview) {
		INT32 nGroupBoxId = (nCtrID == IDC_ROMDATA_TITLE) ? IDC_STATIC2 : IDC_STATIC3;
		HWND hGroupBox = GetDlgItem(hDlg, nGroupBoxId);
		if (hGroupBox && IsWindow(hGroupBox)) {
			hFrameCtrl = hGroupBox;
		}
	}

	if (hFrameCtrl && IsWindow(hFrameCtrl)) {
		GetWindowRect(hFrameCtrl, &frameRect);
	} else if (hPreviewCtrl && IsWindow(hPreviewCtrl)) {
		GetWindowRect(hPreviewCtrl, &frameRect);
	}

	if (maxw == 0 && maxh == 0) {
		RECT rect = frameRect;
		INT32 dw = rect.right  - rect.left;
		INT32 dh = rect.bottom - rect.top;

		if (bHostedPreview) {
			dw -= hostedInsetX * 2;
			dh -= (hostedInsetY * 2) + hostedInsetBottom;
		} else if (bStandaloneManagerPreview) {
			dw -= managerInsetX * 2;
			dh -= (managerInsetY * 2) + managerInsetBottom;
		} else {
			dw = dw * 90 / 100; // make W 90% of the "Preview / Title" windowpane
			dh = dw * 75 / 100; // make H 75% of w (4:3)
		}

		maxw = dw;
		maxh = dh;
	}

	// max WIDTH
	if (w > maxw) {
		float nh = maxw * (float)(h / w);
		float nw = maxw;

		// max HEIGHT
		if (nh > maxh) {
			nw = maxh * (float)(nw / nh);
			nh = maxh;
		}

		w = nw;
		h = nh;
	}

	// max HEIGHT
	if (h > maxh) {
		float nw = maxh * (float)(w / h);
		float nh = maxh;

		// max WIDTH
		if (nw > maxw) {
			nh = maxw * (float)(nh / nw);
			nw = maxw;
		}

		w = nw;
		h = nh;
	}

	// Proper centering of preview
	float x = ((maxw - w) / 2);
	float y = ((maxh - h) / 2);

	RECT rc = frameRect;

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.top;

	ScreenToClient(hDlg, &pt);

	if (bHostedPreview) {
		pt.x += hostedInsetX;
		pt.y += hostedInsetY;
	} else if (bStandaloneManagerPreview) {
		pt.x += managerInsetX;
		pt.y += managerInsetY;
	}

	// ------------------------------------------------------

	FILE* fp = _tfopen(szFile, _T("rb"));

	HBITMAP hCoverBmp = PNGLoadBitmap(hDlg, fp, (int)w, (int)h, 0);

	if (bHostedPreview || bStandaloneManagerPreview) {
		SetWindowPos(hPreviewCtrl, NULL, (int)(pt.x + x), (int)(pt.y + y), (int)w, (int)h, SWP_SHOWWINDOW);
	} else {
		SetWindowPos(hPreviewCtrl, NULL, (int)(pt.x + x), (int)(pt.y + y), 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	}
	SendDlgItemMessage(hDlg, nCtrID, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hCoverBmp);

	if (fp) fclose(fp);

}

static void RomDataClearHostedSelectionUi()
{
	RomDataShowPreview(hRDMgrWnd, _T(""), nRomdataHostedPreviewCtrlId, nRomdataHostedPreviewFrameCtrlId, 0, 0);

	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedDatTextCtrlId),      _T(""));
	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedDriverTextCtrlId),   _T(""));
	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedHardwareTextCtrlId), _T(""));
}

static bool RomDataSelectionStateChanged(const NMLISTVIEW* pNMLV)
{
	if (pNMLV == NULL) {
		return false;
	}

	if ((pNMLV->uChanged & LVIF_STATE) == 0) {
		return false;
	}

	return (pNMLV->uOldState & LVIS_SELECTED) != (pNMLV->uNewState & LVIS_SELECTED);
}

static bool RomDataSelectionBecameSelected(const NMLISTVIEW* pNMLV)
{
	return RomDataSelectionStateChanged(pNMLV) && ((pNMLV->uNewState & LVIS_SELECTED) != 0);
}

static void RomDataScheduleSelectionRefresh(HWND hOwner)
{
	if (hOwner == NULL) {
		return;
	}

	KillTimer(hOwner, IDC_ROMDATA_SELECTION_TIMER);
	SetTimer(hOwner, IDC_ROMDATA_SELECTION_TIMER, 90, (TIMERPROC)NULL);
}

static INT32 RomDataFindVisibleItemByDatPath(const TCHAR* pszDatPath)
{
	if (pszDatPath == NULL || pszDatPath[0] == _T('\0')) {
		return -1;
	}

	for (size_t i = 0; i < g_RomDataEntries.size(); i++) {
		if (_tcsicmp(g_RomDataEntries[i].szDatPath, pszDatPath) == 0) {
			return (INT32)i;
		}
	}

	return -1;
}

static INT32 RomDataFindFirstSelectableItem()
{
	return g_RomDataEntries.empty() ? -1 : 0;
}

static void RomDataSelectVisibleItem(INT32 nItem, bool bEnsureVisible)
{
	if (hRDListView == NULL) {
		nSelItem = -1;
		return;
	}

	if (nItem < 0 || nItem >= (INT32)g_RomDataEntries.size()) {
		TreeView_SelectItem(hRDListView, NULL);
		nSelItem = -1;
		return;
	}

	HTREEITEM hItem = g_RomDataEntries[nItem].hTreeItem;
	if (hItem == NULL) {
		nSelItem = -1;
		return;
	}

	HTREEITEM hParent = TreeView_GetParent(hRDListView, hItem);
	if (hParent != NULL) {
		TreeView_Expand(hRDListView, hParent, TVE_EXPAND);
		RomDataGroupEntry* pGroup = RomDataGetGroupForTreeItem(hParent);
		if (pGroup != NULL) {
			pGroup->bExpanded = true;
		}
	}

	TreeView_SelectItem(hRDListView, hItem);
	if (bEnsureVisible) {
		TreeView_EnsureVisible(hRDListView, hItem);
	}
	nSelItem = nItem;
}

static bool RomDataToggleGroupForItem(HTREEITEM hItem, bool bFocusList)
{
	RomDataGroupEntry* pGroup = RomDataGetGroupForTreeItem(hItem);
	if (pGroup == NULL) {
		return false;
	}

	pGroup->bExpanded = !pGroup->bExpanded;
	TreeView_Expand(hRDListView, hItem, pGroup->bExpanded ? TVE_EXPAND : TVE_COLLAPSE);

	if (!pGroup->bExpanded) {
		HTREEITEM hSelected = RomDataGetSelectedTreeItem();
		if (hSelected != NULL && TreeView_GetParent(hRDListView, hSelected) == hItem) {
			TreeView_SelectItem(hRDListView, hItem);
			nSelItem = -1;
			RomDataUpdateSelectionUi();
		}
	}
	if (bFocusList) {
		SetFocus(hRDListView);
	}

	return true;
}

static bool RomDataGetSelectedDatInternal(TCHAR* pszSelDat, INT32 nCount)
{
	if (pszSelDat == NULL || nCount <= 0 || hRDListView == NULL) {
		return false;
	}

	pszSelDat[0] = _T('\0');
	const RomDataListEntry* pEntry = RomDataGetSelectedEntry();
	if (pEntry == NULL) {
		return false;
	}

	_tcsncpy(pszSelDat, pEntry->szDatPath, nCount - 1);
	pszSelDat[nCount - 1] = _T('\0');

	return pszSelDat[0] != _T('\0');
}

static void RomDataUpdateHostedSelection()
{
	if (!bRomdataHostedMode) {
		return;
	}

	TCHAR szSelDat[MAX_PATH] = { 0 };
	if (!RomDataGetSelectedDatInternal(szSelDat, _countof(szSelDat))) {
		RomDataClearHostedSelectionUi();
		return;
	}

	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedDatTextCtrlId),      _T(""));
	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedDriverTextCtrlId),   _T(""));
	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedHardwareTextCtrlId), _T(""));

	const RomDataListEntry* pEntry = RomDataGetSelectedEntry();
	if (pEntry == NULL) {
		RomDataClearHostedSelectionUi();
		return;
	}

	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedDatTextCtrlId),      szSelDat);
	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedDriverTextCtrlId),   pEntry->info.szDrvName);
	SetWindowText(GetDlgItem(hRDMgrWnd, nRomdataHostedHardwareTextCtrlId), pEntry->info.szHardware);
	RomDataSyncDriverForItem(nSelItem);

	szCover[0] = _T('\0');
	_stprintf(szCover, _T("%s%s.png"), szAppTitlesPath, pEntry->info.szRomSet);
	if (!FileExists(szCover))
		_stprintf(szCover, _T("%s%s.png"), szAppTitlesPath, pEntry->info.szDrvName);
	if (!FileExists(szCover))
		_stprintf(szCover, _T("%s%s.png"), szAppPreviewsPath, pEntry->info.szRomSet);
	if (!FileExists(szCover))
		_stprintf(szCover, _T("%s%s.png"), szAppPreviewsPath, pEntry->info.szDrvName);
	if (!FileExists(szCover))
		szCover[0] = _T('\0');

	RomDataShowPreview(hRDMgrWnd, szCover, nRomdataHostedPreviewCtrlId, nRomdataHostedPreviewFrameCtrlId, 0, 0);
}

static void RomDataUpdateManagerSelection()
{
	if (bRomdataHostedMode || hRDMgrWnd == NULL || hRDListView == NULL) {
		return;
	}

	const RomDataListEntry* pEntry = RomDataGetSelectedEntry();
	if (TreeView_GetCount(hRDListView) <= 0 || pEntry == NULL) {
		RomDataShowPreview(hRDMgrWnd, _T(""), IDC_ROMDATA_TITLE,   IDC_ROMDATA_TITLE_FRAME,   0, 0);
		RomDataShowPreview(hRDMgrWnd, _T(""), IDC_ROMDATA_PREVIEW, IDC_ROMDATA_PREVIEW_FRAME, 0, 0);
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDATPATH),  _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDRIVER),   _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTHARDWARE), _T(""));
		nSelItem = -1;
		return;
	}

	TCHAR szSelDat[MAX_PATH] = { 0 };
	if (!RomDataGetSelectedDatInternal(szSelDat, _countof(szSelDat))) {
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDATPATH),  _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDRIVER),   _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTHARDWARE), _T(""));
		return;
	}

	SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDATPATH),  _T(""));
	SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDRIVER),   _T(""));
	SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTHARDWARE), _T(""));

	SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDATPATH),  szSelDat);
	SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDRIVER),   pEntry->info.szDrvName);
	SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTHARDWARE), pEntry->info.szHardware);
	RomDataSyncDriverForItem(nSelItem);

	_stprintf(szCover, _T("%s%s.png"), szAppTitlesPath, pEntry->info.szRomSet);
	if (!FileExists(szCover)) {
		_stprintf(szCover, _T("%s%s.png"), szAppTitlesPath, pEntry->info.szDrvName);
	}
	if (!FileExists(szCover)) {
		szCover[0] = 0;
	}
	RomDataShowPreview(hRDMgrWnd, szCover, IDC_ROMDATA_TITLE, IDC_ROMDATA_TITLE_FRAME, 0, 0);

	_stprintf(szCover, _T("%s%s.png"), szAppPreviewsPath, pEntry->info.szRomSet);
	if (!FileExists(szCover)) {
		_stprintf(szCover, _T("%s%s.png"), szAppPreviewsPath, pEntry->info.szDrvName);
	}
	if (!FileExists(szCover)) {
		szCover[0] = 0;
	}
	RomDataShowPreview(hRDMgrWnd, szCover, IDC_ROMDATA_PREVIEW, IDC_ROMDATA_PREVIEW_FRAME, 0, 0);
}

static void RomDataUpdateSelectionUi()
{
	if (bRomdataHostedMode) {
		RomDataUpdateHostedSelection();
		return;
	}

	RomDataUpdateManagerSelection();
}

static void RomDataClearListUi()
{
	if (hRDListView) {
		TreeView_DeleteAllItems(hRDListView);
	}

	if (bRomdataHostedMode) {
		RomDataClearHostedSelectionUi();
	} else {
		RomDataShowPreview(hRDMgrWnd, _T(""), IDC_ROMDATA_TITLE,   IDC_ROMDATA_TITLE_FRAME,    0, 0);
		RomDataShowPreview(hRDMgrWnd, _T(""), IDC_ROMDATA_PREVIEW, IDC_ROMDATA_PREVIEW_FRAME,  0, 0);

		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDATPATH),   _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDRIVER),    _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTHARDWARE),  _T(""));
	}

	nSelItem = -1;
}

static void RomDataClearList()
{
	RomDataClearListUi();
	RomDataClearCache();
}

static void RomdataUpdateSearchFilter(HWND hDlg)
{
	TCHAR szBuffer[_countof(szRomdataSearchRaw)] = { 0 };

	GetDlgItemText(hDlg, IDC_ROMDATA_SEARCH_EDIT, szBuffer, _countof(szBuffer));
	_tcscpy(szRomdataSearchRaw, szBuffer);
	RomdataNormalizeSearchText(szBuffer, szRomdataSearchNormalized, _countof(szRomdataSearchNormalized));
}

static void RomDataRefreshList(bool bFocusList, bool bRescan)
{
	TCHAR szRestoreDat[MAX_PATH] = { 0 };
	RomDataGetSelectedDatInternal(szRestoreDat, _countof(szRestoreDat));

	SendMessage(hRDListView, WM_SETREDRAW, FALSE, 0);
	RomDataClearListUi();
	if (bRescan || g_RomDataSourceEntries.empty()) {
		g_RomDataSourceEntries.clear();
		RomdataListFindDats(szAppRomdataPath);
	}
	RomDataRebuildVisibleEntriesFromSource();
	RomDataPopulateListViewFromCache();
	SendMessage(hRDListView, WM_SETREDRAW, TRUE, 0);

	INT32 nRestoreItem = RomDataFindVisibleItemByDatPath(szRestoreDat);
	if (nRestoreItem < 0) {
		nRestoreItem = RomDataFindFirstSelectableItem();
	}
	RomDataSelectVisibleItem(nRestoreItem, true);
	RomDataUpdateSelectionUi();

	RedrawWindow(hRDListView, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

	if (bFocusList) {
		SetFocus(hRDListView);
	}
}

static void RomDataNormalizePath(const TCHAR* pszSrc, TCHAR* pszDst, INT32 nDstCount)
{
	if (pszDst == NULL || nDstCount <= 0) {
		return;
	}

	pszDst[0] = _T('\0');
	if (pszSrc == NULL || pszSrc[0] == _T('\0')) {
		return;
	}

	GetFullPathName(pszSrc, nDstCount, pszDst, NULL);
	if (pszDst[0] == _T('\0')) {
		_tcsncpy(pszDst, pszSrc, nDstCount - 1);
		pszDst[nDstCount - 1] = _T('\0');
	}

	StrReplace(pszDst, _T('/'), _T('\\'));
}

static bool RomDataSelectItemFromScreenPoint(const POINT* pScreenPoint, bool bToggleGroups)
{
	if (hRDListView == NULL || pScreenPoint == NULL) {
		return false;
	}

	POINT ptClient = *pScreenPoint;
	ScreenToClient(hRDListView, &ptClient);

	TVHITTESTINFO hit = { 0 };
	hit.pt = ptClient;

	TreeView_HitTest(hRDListView, &hit);
	if (hit.hItem == NULL || (hit.flags & TVHT_ONITEM) == 0) {
		return false;
	}

	TreeView_SelectItem(hRDListView, hit.hItem);

	RomDataGroupEntry* pGroup = RomDataGetGroupForTreeItem(hit.hItem);
	if (pGroup != NULL) {
		if (bToggleGroups) {
			return RomDataToggleGroupForItem(hit.hItem, false);
		}
		nSelItem = -1;
		RomDataUpdateSelectionUi();
		return false;
	}

	const RomDataListEntry* pEntry = RomDataGetEntryForTreeItem(hit.hItem);
	if (pEntry == NULL) {
		return false;
	}

	nSelItem = RomDataTreeParamToIndex(RomDataGetTreeItemParam(hit.hItem));
	if (nSelItem < 0) {
		nSelItem = (INT32)(pEntry - &g_RomDataEntries[0]);
	}

	return true;
}

static void RomDataEditFile(const TCHAR* pszDatFile)
{
	if (pszDatFile == NULL || pszDatFile[0] == _T('\0')) {
		return;
	}

	INT_PTR nRet = (INT_PTR)ShellExecute(NULL, _T("edit"), pszDatFile, NULL, NULL, SW_SHOWNORMAL);
	if (nRet > 32) {
		return;
	}

	nRet = (INT_PTR)ShellExecute(NULL, _T("open"), pszDatFile, NULL, NULL, SW_SHOWNORMAL);
	if (nRet > 32) {
		return;
	}

	TCHAR szParams[MAX_PATH * 2] = { 0 };
	_stprintf(szParams, _T("\"%s\""), pszDatFile);
	ShellExecute(NULL, NULL, _T("notepad.exe"), szParams, NULL, SW_SHOWNORMAL);
}

static void RomDataOpenContainingFolder(const TCHAR* pszDatFile)
{
	if (pszDatFile == NULL || pszDatFile[0] == _T('\0')) {
		return;
	}

	TCHAR szFullPath[MAX_PATH] = { 0 };
	RomDataNormalizePath(pszDatFile, szFullPath, _countof(szFullPath));

	TCHAR szParams[MAX_PATH * 2] = { 0 };
	_stprintf(szParams, _T("/select,\"%s\""), szFullPath[0] ? szFullPath : pszDatFile);

	INT_PTR nRet = (INT_PTR)ShellExecute(NULL, _T("open"), _T("explorer.exe"), szParams, NULL, SW_SHOWNORMAL);
	if (nRet > 32) {
		return;
	}

	TCHAR szDir[MAX_PATH] = { 0 };
	_tcscpy(szDir, szFullPath[0] ? szFullPath : pszDatFile);
	TCHAR* pSlash = _tcsrchr(szDir, _T('\\'));
	if (pSlash) {
		*pSlash = _T('\0');
	}

	ShellExecute(NULL, _T("open"), szDir, NULL, NULL, SW_SHOWNORMAL);
}

static void RomDataDeleteFile(HWND hOwner, const TCHAR* pszDatFile)
{
	if (pszDatFile == NULL || pszDatFile[0] == _T('\0')) {
		return;
	}

	TCHAR szFullPath[MAX_PATH] = { 0 };
	RomDataNormalizePath(pszDatFile, szFullPath, _countof(szFullPath));

	TCHAR szActiveDat[MAX_PATH] = { 0 };
	RomDataNormalizePath(szRomdataName, szActiveDat, _countof(szActiveDat));
	const bool bDeleteCurrentRomData = (szActiveDat[0] != _T('\0') && 0 == _tcsicmp(szActiveDat, szFullPath));

	TCHAR szBackupFullPath[MAX_PATH] = { 0 };
	RomDataNormalizePath(szBackupDat, szBackupFullPath, _countof(szBackupFullPath));
	const bool bDeleteBackupRomData = (szBackupFullPath[0] != _T('\0') && 0 == _tcsicmp(szBackupFullPath, szFullPath));

	TCHAR szMessage[MAX_PATH * 2] = { 0 };
	_stprintf(szMessage, _T("\x786E\x5B9A\x5220\x9664\x8FD9\x4E2A RomData \x6587\x4EF6\x5417\xFF1F\n\n%s"), szFullPath[0] ? szFullPath : pszDatFile);
	if (MessageBox(hOwner, szMessage, _T("RomData"), MB_ICONQUESTION | MB_YESNO) != IDYES) {
		return;
	}

	const TCHAR* pszTargetPath = szFullPath[0] ? szFullPath : pszDatFile;
	bool bTemporarilyExitedRomData = false;
	DWORD nDeleteError = 0;

	if (bDeleteBackupRomData) {
		memset(szBackupDat, 0, sizeof(szBackupDat));
	}

	SetFileAttributes(pszTargetPath, FILE_ATTRIBUTE_NORMAL);
	bool bDeleteSucceeded = (DeleteFile(pszTargetPath) != 0);
	if (!bDeleteSucceeded) {
		nDeleteError = GetLastError();
	}

	if (!bDeleteSucceeded && NULL != pDataRomDesc) {
		RomDataExit();
		bTemporarilyExitedRomData = true;
		SetFileAttributes(pszTargetPath, FILE_ATTRIBUTE_NORMAL);
		bDeleteSucceeded = (DeleteFile(pszTargetPath) != 0);
		if (!bDeleteSucceeded) {
			nDeleteError = GetLastError();
		}
	}

	const bool bFileStillExists = (GetFileAttributes(pszTargetPath) != INVALID_FILE_ATTRIBUTES);
	if (!bDeleteSucceeded && !bFileStillExists) {
		bDeleteSucceeded = true;
	}

	if (bTemporarilyExitedRomData && szActiveDat[0] != _T('\0')) {
		const bool bNeedRestoreRomData = (!bDeleteSucceeded || !bDeleteCurrentRomData);
		if (bNeedRestoreRomData) {
			_tcscpy(szRomdataName, szActiveDat);
			RomDataInit();
		}
	}

	if (!bDeleteSucceeded && bFileStillExists) {
		TCHAR szError[MAX_PATH * 2] = { 0 };
		_stprintf(szError, _T("\x5220\x9664\x5931\x8D25\xFF1A\n\n%s\n\nWin32 Error: %lu"), pszTargetPath, (unsigned long)nDeleteError);
		MessageBox(hOwner, szError, _T("RomData"), MB_ICONERROR | MB_OK);
		return;
	}

	RomDataRefreshList(true, true);
}

static void RomDataShowContextMenu(HWND hOwner)
{
	if (hRDListView == NULL) {
		return;
	}

	DWORD dwPos = GetMessagePos();
	POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
	RomDataSelectItemFromScreenPoint(&pt, false);

	TCHAR szSelDat[MAX_PATH] = { 0 };
	if (!RomDataGetSelectedDatInternal(szSelDat, _countof(szSelDat))) {
		return;
	}

	HMENU hPopupMenu = CreatePopupMenu();
	if (hPopupMenu == NULL) {
		return;
	}

	AppendMenu(hPopupMenu, MF_STRING, IDC_ROMDATA_CTX_EDIT,   _T("\x7F16\x8F91"));
	AppendMenu(hPopupMenu, MF_STRING, IDC_ROMDATA_CTX_OPEN,   _T("\x6253\x5F00\x6587\x4EF6\x6240\x5728\x76EE\x5F55"));
	AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hPopupMenu, MF_STRING, IDC_ROMDATA_CTX_DELETE, _T("\x5220\x9664"));

	const UINT nCmd = TrackPopupMenu(hPopupMenu,
		TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hOwner ? hOwner : hRDMgrWnd, NULL);

	DestroyMenu(hPopupMenu);

	switch (nCmd) {
		case IDC_ROMDATA_CTX_EDIT:
			RomDataEditFile(szSelDat);
			break;

		case IDC_ROMDATA_CTX_OPEN:
			RomDataOpenContainingFolder(szSelDat);
			break;

		case IDC_ROMDATA_CTX_DELETE:
			RomDataDeleteFile(hOwner ? hOwner : hRDMgrWnd, szSelDat);
			break;

		default:
			break;
	}
}

static void RomDataCreateSearchControls(HWND hDlg)
{
	HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);

	HWND hSearchEdit = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		_T("Edit"),
		szRomdataSearchRaw,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		13, 10, 337, 18,
		hDlg,
		(HMENU)(INT_PTR)IDC_ROMDATA_SEARCH_EDIT,
		hAppInst,
		NULL
	);

	if (hFont) {
		SendMessage(hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
	}
}

static void RomDataLayoutSearchControls(HWND hDlg)
{
	RECT rcList = { 0 }, rcSearchEdit = { 0 };
	RECT rcScan = { 0 }, rcDir = { 0 }, rcSubDir = { 0 };

	HWND hSearchEdit  = GetDlgItem(hDlg, IDC_ROMDATA_SEARCH_EDIT);
	HWND hScanButton  = GetDlgItem(hDlg, IDC_ROMDATA_SCAN_BUTTON);
	HWND hDirButton   = GetDlgItem(hDlg, IDC_ROMDATA_SELDIR_BUTTON);
	HWND hSubDirCheck = GetDlgItem(hDlg, IDC_ROMDATA_SUBDIR_CHECK);

	GetWindowRect(hRDListView, &rcList);
	GetWindowRect(hSearchEdit, &rcSearchEdit);
	GetWindowRect(hScanButton, &rcScan);
	GetWindowRect(hDirButton, &rcDir);
	GetWindowRect(hSubDirCheck, &rcSubDir);

	MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rcList, 2);
	MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rcSearchEdit, 2);
	MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rcScan, 2);
	MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rcDir, 2);
	MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rcSubDir, 2);

	const INT32 searchEditHeight  = rcSearchEdit.bottom - rcSearchEdit.top;
	const INT32 searchTop         = rcList.top + 9;
	const INT32 listBottom        = rcList.bottom;
	const INT32 searchGap         = 7;
	const INT32 searchLeft        = rcList.left + 6;
	const INT32 searchEditWidth   = rcList.right - searchLeft - 5;
	const INT32 listTop           = searchTop + searchEditHeight + searchGap;

	MoveWindow(hSearchEdit, searchLeft, searchTop, searchEditWidth, searchEditHeight, TRUE);
	MoveWindow(hRDListView, rcList.left, listTop, rcList.right - rcList.left, listBottom - listTop, TRUE);

	const INT32 subDirWidth = rcSubDir.right - rcSubDir.left;
	const INT32 optionsSpan = rcDir.right - rcScan.left;
	const INT32 subDirLeft  = rcScan.left + ((optionsSpan - subDirWidth) / 2);

	MoveWindow(hSubDirCheck, subDirLeft, rcSubDir.top, subDirWidth, rcSubDir.bottom - rcSubDir.top, TRUE);
}

static INT_PTR CALLBACK RomDataCoveProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM /*lParam*/)
{
	if (Msg == WM_INITDIALOG) {
		hRdCoverDlg = hDlg;
		RomDataShowPreview(hDlg, szCover, IDC_ROMDATA_COVER_PREVIEW_PIC, IDC_ROMDATA_COVER_PREVIEW_PIC, 580, 415);

		return TRUE;
	}

	if (Msg == WM_CLOSE) {
		EndDialog(hDlg, 0); hRdCoverDlg = NULL;
	}

	if (Msg == WM_COMMAND) {
		if (LOWORD(wParam) == WM_DESTROY) SendMessage(hDlg, WM_CLOSE, 0, 0);
	}

	return 0;
}

static void RomdataCoverInit()
{
	FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_ROMDATA_COVER_DLG), hRDMgrWnd, (DLGPROC)RomDataCoveProc);
}

void RomDataStateBackup()
{
	if (NULL != pDataRomDesc) {
		memset(szBackupDat, 0, sizeof(szBackupDat));
		_tcscpy(szBackupDat, szRomdataName);
		RomDataExit();
	}
}

void RomDataStateRestore()
{
	if (FileExists(szBackupDat)) {
		_tcscpy(szRomdataName, szBackupDat);
		memset(szBackupDat, 0, sizeof(szBackupDat));
		RomDataInit();
	}
}

static void RomDataManagerExit()
{
	KillTimer(hRDMgrWnd, IDC_ROMDATA_SEARCH_TIMER);
	KillTimer(hRDMgrWnd, IDC_ROMDATA_SELECTION_TIMER);
	RomDataStateRestore();
	RomDataClearList();
	DestroyHardwareIconList();
	DeleteObject(hWhiteBGBrush);

	hRDMgrWnd = hRDListView = NULL;
}

INT32 RomDataHostedInit(HWND hHostWnd, HWND hListView, INT32 nPreviewCtrlId, INT32 nPreviewFrameCtrlId, INT32 nDatTextCtrlId, INT32 nDriverTextCtrlId, INT32 nHardwareTextCtrlId)
{
	if (hHostWnd == NULL || hListView == NULL) {
		return 0;
	}

	bRomdataHostedMode = true;
	hRDMgrWnd = hHostWnd;
	hRDListView = hListView;
	nRomdataHostedPreviewCtrlId = nPreviewCtrlId;
	nRomdataHostedPreviewFrameCtrlId = nPreviewFrameCtrlId;
	nRomdataHostedDatTextCtrlId = nDatTextCtrlId;
	nRomdataHostedDriverTextCtrlId = nDriverTextCtrlId;
	nRomdataHostedHardwareTextCtrlId = nHardwareTextCtrlId;
	nSelItem = -1;

	RomDataApplyListViewStyle();
	RomDataInitListView();

	DestroyHardwareIconList();
	HardwareIconListInit();

	if (hWhiteBGBrush) {
		DeleteObject(hWhiteBGBrush);
	}
	hWhiteBGBrush = CreateSolidBrush(RGB(0xff, 0xff, 0xff));

	return TreeView_GetCount(hRDListView);
}

void RomDataHostedExit()
{
	if (!bRomdataHostedMode) {
		return;
	}

	KillTimer(hRDMgrWnd, IDC_ROMDATA_SELECTION_TIMER);
	RomDataClearList();
	DestroyHardwareIconList();
	if (hWhiteBGBrush) {
		DeleteObject(hWhiteBGBrush);
		hWhiteBGBrush = NULL;
	}

	hRDMgrWnd = NULL;
	hRDListView = NULL;
	bRomdataHostedMode = false;
	nSelItem = -1;
}

void RomDataHostedSetSearchText(const TCHAR* pszSearch)
{
	_tcscpy(szRomdataSearchRaw, pszSearch ? pszSearch : _T(""));
	RomdataNormalizeSearchText(szRomdataSearchRaw, szRomdataSearchNormalized, _countof(szRomdataSearchNormalized));

	if (bRomdataHostedMode && hRDListView) {
		RomDataRefreshList(false, false);
	}
}

void RomDataHostedSetScanSubdirs(INT32 bEnable)
{
	bRDListScanSub = bEnable ? true : false;
}

void RomDataHostedRefresh(INT32 bFocusList)
{
	if (bRomdataHostedMode && hRDListView) {
		RomDataRefreshList(bFocusList ? true : false, true);
	}
}

INT32 RomDataHostedGetSelectedDat(TCHAR* pszSelDat, INT32 nCount)
{
	return RomDataGetSelectedDatInternal(pszSelDat, nCount) ? 1 : 0;
}

INT32 RomDataHostedGetSelectedRomSet(TCHAR* pszRomSet, INT32 nCount)
{
	if (pszRomSet == NULL || nCount <= 0) {
		return 0;
	}

	pszRomSet[0] = _T('\0');

	const RomDataListEntry* pEntry = RomDataGetSelectedEntry();
	if (pEntry == NULL || pEntry->info.szRomSet[0] == _T('\0')) {
		return 0;
	}

	_tcsncpy(pszRomSet, pEntry->info.szRomSet, nCount - 1);
	pszRomSet[nCount - 1] = _T('\0');
	return 1;
}

INT32 RomDataHostedHandleNotify(NMHDR* pNmHdr)
{
	if (!bRomdataHostedMode || pNmHdr == NULL || hRDListView == NULL) {
		return 0;
	}

	if (pNmHdr->idFrom != IDC_ROMDATA_LIST) {
		return 0;
	}

	if (pNmHdr->code == TVN_SELCHANGED) {
		NMTREEVIEW* pNMTV = (NMTREEVIEW*)pNmHdr;
		const RomDataListEntry* pEntry = RomDataGetEntryForTreeItem(pNMTV->itemNew.hItem);
		if (pEntry == NULL) {
			nSelItem = -1;
			RomDataClearHostedSelectionUi();
			return 1;
		}
		nSelItem = RomDataTreeParamToIndex(RomDataGetTreeItemParam(pNMTV->itemNew.hItem));
		RomDataSyncDriverForItem(nSelItem);
		RomDataScheduleSelectionRefresh(hRDMgrWnd);
		return 1;
	}

	if (pNmHdr->code == TVN_ITEMEXPANDED) {
		NMTREEVIEW* pNMTV = (NMTREEVIEW*)pNmHdr;
		RomDataGroupEntry* pGroup = RomDataGetGroupForTreeItem(pNMTV->itemNew.hItem);
		if (pGroup != NULL) {
			pGroup->bExpanded = (pNMTV->action == TVE_EXPAND) ? true : false;
			return 1;
		}
	}

	if (pNmHdr->code == NM_RCLICK) {
		RomDataShowContextMenu(hRDMgrWnd);
		return 1;
	}

	if (pNmHdr->code == NM_DBLCLK) {
		DWORD dwPos = GetMessagePos();
		POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
		if (!RomDataSelectItemFromScreenPoint(&pt, false)) {
			return 1;
		}
		return 2;
	}

	return 0;
}

INT32 RomDataHostedHandleTimer(HWND hHostWnd, WPARAM wParam)
{
	if (!bRomdataHostedMode || hHostWnd == NULL || hHostWnd != hRDMgrWnd) {
		return 0;
	}

	if (wParam != IDC_ROMDATA_SELECTION_TIMER) {
		return 0;
	}

	KillTimer(hHostWnd, IDC_ROMDATA_SELECTION_TIMER);
	RomDataUpdateHostedSelection();
	return 1;
}

static INT_PTR CALLBACK RomDataManagerProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_INITDIALOG) {
		hRDMgrWnd   = hDlg;
		hRDListView = GetDlgItem(hDlg, IDC_ROMDATA_LIST);
		RomDataApplyListViewStyle();

		RomDataInitListView();
		HardwareIconListInit();

		HICON hIcon   = LoadIcon(hAppInst, MAKEINTRESOURCE(IDI_APP));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);			// Set the Game Selection dialog icon.

		hWhiteBGBrush = CreateSolidBrush(RGB(0xff, 0xff, 0xff));

		RomDataShowPreview(hRDMgrWnd, _T(""), IDC_ROMDATA_TITLE,   IDC_ROMDATA_TITLE_FRAME,    0, 0);
		RomDataShowPreview(hRDMgrWnd, _T(""), IDC_ROMDATA_PREVIEW, IDC_ROMDATA_PREVIEW_FRAME,  0, 0);

		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDATPATH),  _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDRIVER),   _T(""));
		SetWindowText(GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTHARDWARE), _T(""));
		RomDataCreateSearchControls(hDlg);
		SendDlgItemMessage(hDlg, IDC_ROMDATA_SEARCH_EDIT, EM_LIMITTEXT, _countof(szRomdataSearchRaw) - 1, 0);
		bRomdataSearchInit = true;
		SetDlgItemText(hDlg, IDC_ROMDATA_SEARCH_EDIT, szRomdataSearchRaw);
		bRomdataSearchInit = false;

		CheckDlgButton(hRDMgrWnd, IDC_ROMDATA_SUBDIR_CHECK, bRDListScanSub ? BST_CHECKED : BST_UNCHECKED);

		TCHAR szDialogTitle[200] = { 0 };
		_stprintf(szDialogTitle, FBALoadStringEx(hAppInst, IDS_ROMDATAMANAGER_TITLE, true), _T(APP_TITLE), _T(SEPERATOR_1), _T(SEPERATOR_1));
		SetWindowText(hDlg, szDialogTitle);

		RomDataLayoutSearchControls(hDlg);
		RomdataUpdateSearchFilter(hDlg);
		RomDataRefreshList(false, true);
		WndInMid(hDlg, hScrnWnd);
		SetFocus(hRDListView);

		return TRUE;
	}

	if (Msg == WM_CLOSE) {
		RomDataManagerExit();
		EndDialog(hDlg, 0);
	}

	if (Msg == WM_CTLCOLORSTATIC) {
		if ((HWND)lParam == GetDlgItem(hRDMgrWnd, IDC_ROMDATA_LABELDAT))      return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hRDMgrWnd, IDC_ROMDATA_LABELDRIVER))   return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hRDMgrWnd, IDC_ROMDATA_LABELHARDWARE)) return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDATPATH))   return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTDRIVER))    return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hRDMgrWnd, IDC_ROMDATA_TEXTHARDWARE))  return (INT_PTR)hWhiteBGBrush;
	}

	if (Msg == WM_NOTIFY) {
		NMHDR* pNmHdr = (NMHDR*)lParam;
		if (pNmHdr && pNmHdr->idFrom == IDC_ROMDATA_LIST && pNmHdr->code == TVN_SELCHANGED) {
			NMTREEVIEW* pNMTV = (NMTREEVIEW*)lParam;
			const RomDataListEntry* pEntry = RomDataGetEntryForTreeItem(pNMTV->itemNew.hItem);
			if (pEntry == NULL) {
				nSelItem = -1;
				RomDataUpdateManagerSelection();
				return 1;
			}
			nSelItem = RomDataTreeParamToIndex(RomDataGetTreeItemParam(pNMTV->itemNew.hItem));
			RomDataScheduleSelectionRefresh(hDlg);
			return 1;
		}

		if (pNmHdr && pNmHdr->idFrom == IDC_ROMDATA_LIST && pNmHdr->code == TVN_ITEMEXPANDED) {
			NMTREEVIEW* pNMTV = (NMTREEVIEW*)lParam;
			RomDataGroupEntry* pGroup = RomDataGetGroupForTreeItem(pNMTV->itemNew.hItem);
			if (pGroup != NULL) {
				pGroup->bExpanded = (pNMTV->action == TVE_EXPAND) ? true : false;
				return 1;
			}
		}

		if (pNmHdr && pNmHdr->idFrom == IDC_ROMDATA_LIST && pNmHdr->code == NM_RCLICK) {
			RomDataShowContextMenu(hDlg);
			return 1;
		}

		if (pNmHdr && pNmHdr->idFrom == IDC_ROMDATA_LIST && pNmHdr->code == NM_DBLCLK) {
			DWORD dwPos = GetMessagePos();
			POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
			if (!RomDataSelectItemFromScreenPoint(&pt, false)) {
				return 1;
			}
			TCHAR szSelDat[MAX_PATH] = { 0 };
			if (RomDataGetSelectedDatInternal(szSelDat, _countof(szSelDat))) {
				RomDataManagerExit();
				EndDialog(hDlg, 0);
				RomDataLoadDriver(szSelDat);
			}
		}
	}

	if (Msg == WM_COMMAND) {
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_ROMDATA_SEARCH_EDIT) {
			if (bRomdataSearchInit) {
				bRomdataSearchInit = false;
				return 0;
			}

			KillTimer(hDlg, IDC_ROMDATA_SEARCH_TIMER);
			SetTimer(hDlg, IDC_ROMDATA_SEARCH_TIMER, 200, (TIMERPROC)NULL);
			return 0;
		}

		if (HIWORD(wParam) == STN_CLICKED) {
			INT32 nCtrlID = LOWORD(wParam);

			if (nCtrlID == IDC_ROMDATA_TITLE) {
				const RomDataListEntry* pEntry = RomDataGetEntryForItem(nSelItem);
				if (pEntry != NULL) {
					_stprintf(szCover, _T("%s%s.png"), szAppTitlesPath, pEntry->info.szRomSet);
					if (!FileExists(szCover))
						_stprintf(szCover, _T("%s%s.png"), szAppTitlesPath, pEntry->info.szDrvName);
					if (!FileExists(szCover)) {
						szCover[0] = 0;
						return 0;
					}
					RomdataCoverInit();
				}
				return 0;
			}

			if (nCtrlID == IDC_ROMDATA_PREVIEW) {
				const RomDataListEntry* pEntry = RomDataGetEntryForItem(nSelItem);
				if (pEntry != NULL) {
					_stprintf(szCover, _T("%s%s.png"), szAppPreviewsPath, pEntry->info.szRomSet);
					if (!FileExists(szCover))
						_stprintf(szCover, _T("%s%s.png"), szAppPreviewsPath, pEntry->info.szDrvName);
					if (!FileExists(szCover)) {
						szCover[0] = 0;
						return 0;
					}
					RomdataCoverInit();
				}
				return 0;
			}
		}

		if (LOWORD(wParam) == WM_DESTROY) {
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		}

		if (HIWORD(wParam) == BN_CLICKED) {
			INT32 nCtrlID = LOWORD(wParam);

			switch (nCtrlID) {
				case IDC_ROMDATA_PLAY_BUTTON: {
					TCHAR szSelDat[MAX_PATH] = { 0 };
					if (RomDataGetSelectedDatInternal(szSelDat, _countof(szSelDat))) {
						RomDataManagerExit();
						EndDialog(hDlg, 0);
						RomDataLoadDriver(szSelDat);
					}
					break;
				}

				case IDC_ROMDATA_SCAN_BUTTON: {
					RomdataUpdateSearchFilter(hDlg);
					RomDataRefreshList(true, true);
					break;
				}

				case IDC_ROMDATA_SELDIR_BUTTON: {
					SupportDirCreate(hRDMgrWnd);
					RomdataUpdateSearchFilter(hDlg);
					RomDataRefreshList(true, true);
					break;
				}

				case IDC_ROMDATA_SUBDIR_CHECK: {
					bRDListScanSub = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ROMDATA_SUBDIR_CHECK));
					RomdataUpdateSearchFilter(hDlg);
					RomDataRefreshList(true, true);
					break;
				}

				case IDC_ROMDATA_CANCEL_BUTTON: {
					RomDataManagerExit();
					EndDialog(hDlg, 0);
					break;
				}
			}
		}
	}

	if (Msg == WM_TIMER) {
		switch (wParam) {
			case IDC_ROMDATA_SEARCH_TIMER:
				KillTimer(hDlg, IDC_ROMDATA_SEARCH_TIMER);
				RomdataUpdateSearchFilter(hDlg);
				RomDataRefreshList(false, false);
				return 0;

			case IDC_ROMDATA_SELECTION_TIMER:
				KillTimer(hDlg, IDC_ROMDATA_SELECTION_TIMER);
				RomDataUpdateManagerSelection();
				return 0;
		}
	}
	return 0;
}

INT32 RomDataManagerInit()
{
	RomDataStateBackup();

	return FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_ROMDATA_MANAGER), hScrnWnd, (DLGPROC)RomDataManagerProc);
}

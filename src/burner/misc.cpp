// Misc functions module
#include <math.h>
#include "burner.h"

static void CopyTString(TCHAR* pszDest, size_t nDestCount, const TCHAR* pszSrc)
{
	if (pszDest == NULL || nDestCount == 0) {
		return;
	}

	if (pszSrc == NULL) {
		pszDest[0] = _T('\0');
		return;
	}

	_tcsncpy(pszDest, pszSrc, nDestCount - 1);
	pszDest[nDestCount - 1] = _T('\0');
}

static void AppendTString(TCHAR* pszDest, size_t nDestCount, const TCHAR* pszSrc)
{
	if (pszDest == NULL || nDestCount == 0 || pszSrc == NULL) {
		return;
	}

	size_t nDestLen = _tcslen(pszDest);
	if (nDestLen >= nDestCount - 1) {
		pszDest[nDestCount - 1] = _T('\0');
		return;
	}

	_tcsncpy(pszDest + nDestLen, pszSrc, nDestCount - nDestLen - 1);
	pszDest[nDestCount - 1] = _T('\0');
}

// ---------------------------------------------------------------------------
// Software gamma

INT32 bDoGamma = 0;
INT32 bHardwareGammaOnly = 1;
double nGamma = 1.25;
const INT32 nMaxRGB = 255;

static UINT8 GammaLUT[256];

void ComputeGammaLUT()
{
	for (INT32 i = 0; i < 256; i++) {
		GammaLUT[i] = (UINT8)((double)nMaxRGB * pow((i / 255.0), nGamma));
	}
}

// Standard callbacks for 16/24/32 bit color:
static UINT32 __cdecl HighCol15(INT32 r, INT32 g, INT32 b, INT32  /* i */)
{
	UINT32 t;
	t =(r<<7)&0x7c00; // 0rrr rr00 0000 0000
	t|=(g<<2)&0x03e0; // 0000 00gg ggg0 0000
	t|=(b>>3)&0x001f; // 0000 0000 000b bbbb
	return t;
}

static UINT32 __cdecl HighCol16(INT32 r, INT32 g, INT32 b, INT32 /* i */)
{
	UINT32 t;
	t =(r<<8)&0xf800; // rrrr r000 0000 0000
	t|=(g<<3)&0x07e0; // 0000 0ggg ggg0 0000
	t|=(b>>3)&0x001f; // 0000 0000 000b bbbb
	return t;
}

// 24-bit/32-bit
static UINT32 __cdecl HighCol24(INT32 r, INT32 g, INT32 b, INT32  /* i */)
{
	UINT32 t;
	t =(r<<16)&0xff0000;
	t|=(g<<8 )&0x00ff00;
	t|=(b    )&0x0000ff;

	return t;
}

static UINT32 __cdecl HighCol15Gamma(INT32 r, INT32 g, INT32 b, INT32  /* i */)
{
	UINT32 t;
	t = (GammaLUT[r] << 7) & 0x7C00; // 0rrr rr00 0000 0000
	t |= (GammaLUT[g] << 2) & 0x03E0; // 0000 00gg ggg0 0000
	t |= (GammaLUT[b] >> 3) & 0x001F; // 0000 0000 000b bbbb
	return t;
}

static UINT32 __cdecl HighCol16Gamma(INT32 r, INT32 g ,INT32 b, INT32  /* i */)
{
	UINT32 t;
	t = (GammaLUT[r] << 8) & 0xF800; // rrrr r000 0000 0000
	t |= (GammaLUT[g] << 3) & 0x07E0; // 0000 0ggg ggg0 0000
	t |= (GammaLUT[b] >> 3) & 0x001F; // 0000 0000 000b bbbb
	return t;
}

// 24-bit/32-bit
static UINT32 __cdecl HighCol24Gamma(INT32 r, INT32 g, INT32 b, INT32  /* i */)
{
	UINT32 t;
	t = (GammaLUT[r] << 16);
	t |= (GammaLUT[g] << 8);
	t |= GammaLUT[b];

	return t;
}

INT32 SetBurnHighCol(INT32 nDepth)
{
	VidRecalcPal();

	if (bDoGamma && ((nVidFullscreen && !bVidUseHardwareGamma) || (!nVidFullscreen && !bHardwareGammaOnly))) {
		if (nDepth == 15) {
			VidHighCol = HighCol15Gamma;
		}
		if (nDepth == 16) {
			VidHighCol = HighCol16Gamma;
		}
		if (nDepth > 16) {
			VidHighCol = HighCol24Gamma;
		}
	} else {
		if (nDepth == 15) {
			VidHighCol = HighCol15;
		}
		if (nDepth == 16) {
			VidHighCol = HighCol16;
		}
		if (nDepth > 16) {
			VidHighCol = HighCol24;
		}
	}
	if ((bDrvOkay && !(BurnDrvGetFlags() & BDF_16BIT_ONLY)) || nDepth <= 16) {
		BurnHighCol = VidHighCol;
	}

	return 0;
}

// ---------------------------------------------------------------------------

char* GameDecoration(UINT32 nBurnDrv)
{
	static char szGameDecoration[256];
	UINT32 nOldBurnDrv = nBurnDrvActive;

	nBurnDrvActive = nBurnDrv;

	const char* s1 = "";
	const char* s2 = "";
	const char* s3 = "";
	const char* s4 = "";
	const char* s5 = "";
	const char* s6 = "";
	const char* s7 = "";
	const char* s8 = "";
	const char* s9 = "";
	const char* s10 = "";
	const char* s11 = "";

	if ((BurnDrvGetFlags() & BDF_DEMO) || (BurnDrvGetFlags() & BDF_HACK) || (BurnDrvGetFlags() & BDF_HOMEBREW) || (BurnDrvGetFlags() & BDF_PROTOTYPE) || (BurnDrvGetFlags() & BDF_BOOTLEG) || (BurnDrvGetTextA(DRV_COMMENT) && strlen(BurnDrvGetTextA(DRV_COMMENT)) > 0)) {
		if (BurnDrvGetFlags() & BDF_DEMO) {
			s1 = "Demo";
			if ((BurnDrvGetFlags() & BDF_HACK) || (BurnDrvGetFlags() & BDF_HOMEBREW) || (BurnDrvGetFlags() & BDF_PROTOTYPE) || (BurnDrvGetFlags() & BDF_BOOTLEG) || (BurnDrvGetTextA(DRV_COMMENT) && strlen(BurnDrvGetTextA(DRV_COMMENT)) > 0)) {
				s2 = ", ";
			}
		}
		if (BurnDrvGetFlags() & BDF_HACK) {
			s3 = "Hack";
			if ((BurnDrvGetFlags() & BDF_HOMEBREW) || (BurnDrvGetFlags() & BDF_PROTOTYPE) || (BurnDrvGetFlags() & BDF_BOOTLEG) || (BurnDrvGetTextA(DRV_COMMENT) && strlen(BurnDrvGetTextA(DRV_COMMENT)) > 0)) {
				s4 = ", ";
			}
		}
		if (BurnDrvGetFlags() & BDF_HOMEBREW) {
			s5 = "Homebrew";
			if ((BurnDrvGetFlags() & BDF_PROTOTYPE) || (BurnDrvGetFlags() & BDF_BOOTLEG) || (BurnDrvGetTextA(DRV_COMMENT) && strlen(BurnDrvGetTextA(DRV_COMMENT)) > 0)) {
				s6 = ", ";
			}
		}
		if (BurnDrvGetFlags() & BDF_PROTOTYPE) {
			s7 = "Prototype";
			if ((BurnDrvGetFlags() & BDF_BOOTLEG) || (BurnDrvGetTextA(DRV_COMMENT) && strlen(BurnDrvGetTextA(DRV_COMMENT)) > 0)) {
				s8 = ", ";
			}
		}		
		if (BurnDrvGetFlags() & BDF_BOOTLEG) {
			s9 = "Bootleg";
			if (BurnDrvGetTextA(DRV_COMMENT) && strlen(BurnDrvGetTextA(DRV_COMMENT)) > 0) {
				s10 = ", ";
			}
		}
		if (BurnDrvGetTextA(DRV_COMMENT) && strlen(BurnDrvGetTextA(DRV_COMMENT)) > 0) {
			s11 = BurnDrvGetTextA(DRV_COMMENT);
		}
	}

	snprintf(szGameDecoration, sizeof(szGameDecoration), "%s%s%s%s%s%s%s%s%s%s%s", s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11);

	nBurnDrvActive = nOldBurnDrv;
	return szGameDecoration;
}

char* DecorateGameName(UINT32 nBurnDrv)
{
	static char szDecoratedName[256];
	UINT32 nOldBurnDrv = nBurnDrvActive;

	nBurnDrvActive = nBurnDrv;

	const char* s1 = "";
	const char* s2 = "";
	const char* s3 = "";
	const char* s4 = "";

	s1 = BurnDrvGetTextA(DRV_FULLNAME);

	s3 = GameDecoration(nBurnDrv);
	if (strlen(s3) > 0) {
		s2 = " [";
		s4 = "]";
	}

	snprintf(szDecoratedName, sizeof(szDecoratedName), "%s%s%s%s", s1, s2, s3, s4);

	nBurnDrvActive = nOldBurnDrv;
	return szDecoratedName;
}

TCHAR* DecorateGenreInfo()
{
	INT32 nGenre = BurnDrvGetGenreFlags();
	INT32 nFamily = BurnDrvGetFamilyFlags();

	static TCHAR szDecoratedGenre[256];
	TCHAR szFamily[256];

	CopyTString(szDecoratedGenre, _countof(szDecoratedGenre), _T(""));
	CopyTString(szFamily, _countof(szFamily), _T(""));

#ifdef BUILD_WIN32
//TODO: Translations are not working in non-win32 builds. This needs to be fixed

#define APPEND_GENRE_STRING(id) \
	do { \
		AppendTString(szDecoratedGenre, _countof(szDecoratedGenre), FBALoadStringEx(hAppInst, id, true)); \
		AppendTString(szDecoratedGenre, _countof(szDecoratedGenre), _T(", ")); \
	} while (0)

#define APPEND_FAMILY_STRING(id) \
	do { \
		AppendTString(szFamily, _countof(szFamily), FBALoadStringEx(hAppInst, id, true)); \
		AppendTString(szFamily, _countof(szFamily), _T(", ")); \
	} while (0)

	if (nGenre) {
		if (nGenre & GBF_HORSHOOT) {
			APPEND_GENRE_STRING(IDS_GENRE_HORSHOOT);
		}

		if (nGenre & GBF_VERSHOOT) {
			APPEND_GENRE_STRING(IDS_GENRE_VERSHOOT);
		}

		if (nGenre & GBF_SCRFIGHT) {
			APPEND_GENRE_STRING(IDS_GENRE_SCRFIGHT);
		}

		if (nGenre & GBF_VSFIGHT) {
			APPEND_GENRE_STRING(IDS_GENRE_VSFIGHT);
		}

		if (nGenre & GBF_BIOS) {
			APPEND_GENRE_STRING(IDS_GENRE_BIOS);
		}

		if (nGenre & GBF_BREAKOUT) {
			APPEND_GENRE_STRING(IDS_GENRE_BREAKOUT);
		}

		if (nGenre & GBF_CASINO) {
			APPEND_GENRE_STRING(IDS_GENRE_CASINO);
		}

		if (nGenre & GBF_BALLPADDLE) {
			APPEND_GENRE_STRING(IDS_GENRE_BALLPADDLE);
		}

		if (nGenre & GBF_MAZE) {
			APPEND_GENRE_STRING(IDS_GENRE_MAZE);
		}

		if (nGenre & GBF_MINIGAMES) {
			APPEND_GENRE_STRING(IDS_GENRE_MINIGAMES);
		}

		if (nGenre & GBF_PINBALL) {
			APPEND_GENRE_STRING(IDS_GENRE_PINBALL);
		}

		if (nGenre & GBF_PLATFORM) {
			APPEND_GENRE_STRING(IDS_GENRE_PLATFORM);
		}

		if (nGenre & GBF_PUZZLE) {
			APPEND_GENRE_STRING(IDS_GENRE_PUZZLE);
		}

		if (nGenre & GBF_QUIZ) {
			APPEND_GENRE_STRING(IDS_GENRE_QUIZ);
		}

		if (nGenre & GBF_SPORTSMISC) {
			APPEND_GENRE_STRING(IDS_GENRE_SPORTSMISC);
		}

		if (nGenre & GBF_SPORTSFOOTBALL) {
			APPEND_GENRE_STRING(IDS_GENRE_SPORTSFOOTBALL);
		}

		if (nGenre & GBF_MISC) {
			APPEND_GENRE_STRING(IDS_GENRE_MISC);
		}

		if (nGenre & GBF_MAHJONG) {
			APPEND_GENRE_STRING(IDS_GENRE_MAHJONG);
		}

		if (nGenre & GBF_RACING) {
			APPEND_GENRE_STRING(IDS_GENRE_RACING);
		}

		if (nGenre & GBF_SHOOT) {
			APPEND_GENRE_STRING(IDS_GENRE_SHOOT);
		}

		if (nGenre & GBF_MULTISHOOT) {
			APPEND_GENRE_STRING(IDS_GENRE_MULTISHOOT);
		}

		if (nGenre & GBF_ACTION) {
			APPEND_GENRE_STRING(IDS_GENRE_ACTION);
		}

		if (nGenre & GBF_RUNGUN) {
			APPEND_GENRE_STRING(IDS_GENRE_RUNGUN);
		}

		if (nGenre & GBF_STRATEGY) {
			APPEND_GENRE_STRING(IDS_GENRE_STRATEGY);
		}

		if (nGenre & GBF_RPG) {
			APPEND_GENRE_STRING(IDS_GENRE_RPG);
		}

		if (nGenre & GBF_SIM) {
			APPEND_GENRE_STRING(IDS_GENRE_SIM);
		}

		if (nGenre & GBF_ADV) {
			APPEND_GENRE_STRING(IDS_GENRE_ADV);
		}

		if (nGenre & GBF_CARD) {
			APPEND_GENRE_STRING(IDS_GENRE_CARD);
		}

		if (nGenre & GBF_BOARD) {
			APPEND_GENRE_STRING(IDS_GENRE_BOARD);
		}

		size_t nGenreLen = _tcslen(szDecoratedGenre);
		if (nGenreLen >= 2) {
			szDecoratedGenre[nGenreLen - 2] = _T('\0');
		}
	}

	if (nFamily) {
		CopyTString(szFamily, _countof(szFamily), _T(" ("));

		if (nFamily & FBF_MSLUG) {
			APPEND_FAMILY_STRING(IDS_FAMILY_MSLUG);
		}

		if (nFamily & FBF_SF) {
			APPEND_FAMILY_STRING(IDS_FAMILY_SF);
		}

		if (nFamily & FBF_KOF) {
			APPEND_FAMILY_STRING(IDS_FAMILY_KOF);
		}

		if (nFamily & FBF_DSTLK) {
			APPEND_FAMILY_STRING(IDS_FAMILY_DSTLK);
		}

		if (nFamily & FBF_FATFURY) {
			APPEND_FAMILY_STRING(IDS_FAMILY_FATFURY);
		}

		if (nFamily & FBF_SAMSHO) {
			APPEND_FAMILY_STRING(IDS_FAMILY_SAMSHO);
		}

		if (nFamily & FBF_19XX) {
			APPEND_FAMILY_STRING(IDS_FAMILY_19XX);
		}

		if (nFamily & FBF_SONICWI) {
			APPEND_FAMILY_STRING(IDS_FAMILY_SONICWI);
		}

		if (nFamily & FBF_PWRINST) {
			APPEND_FAMILY_STRING(IDS_FAMILY_PWRINST);
		}

		if (nFamily & FBF_SONIC) {
			APPEND_FAMILY_STRING(IDS_FAMILY_SONIC);
		}

		if (nFamily & FBF_DONPACHI) {
			APPEND_FAMILY_STRING(IDS_FAMILY_DONPACHI);
		}

		if (nFamily & FBF_MAHOU) {
			APPEND_FAMILY_STRING(IDS_FAMILY_MAHOU);
		}

		size_t nFamilyLen = _tcslen(szFamily);
		if (nFamilyLen >= 2) {
			szFamily[nFamilyLen - 2] = _T(')');
			szFamily[nFamilyLen - 1] = _T('\0');
		}

		AppendTString(szDecoratedGenre, _countof(szDecoratedGenre), szFamily);
	}

#undef APPEND_FAMILY_STRING
#undef APPEND_GENRE_STRING
#endif

	return szDecoratedGenre;
}

// ---------------------------------------------------------------------------
// config file parsing

TCHAR* LabelCheck(TCHAR* s, TCHAR* pszLabel)
{
	INT32 nLen;
	if (s == NULL) {
		return NULL;
	}
	if (pszLabel == NULL) {
		return NULL;
	}
	nLen = _tcslen(pszLabel);

	SKIP_WS(s);													// Skip whitespace

	if (_tcsncmp(s, pszLabel, nLen)){							// Doesn't match
		return NULL;
	}
	return s + nLen;
}

INT32 QuoteRead(TCHAR** ppszQuote, TCHAR** ppszEnd, TCHAR* pszSrc)	// Read a (quoted) string from szSrc and poINT32 to the end
{
	static TCHAR szQuote[QUOTE_MAX];
	TCHAR* s = pszSrc;
	TCHAR* e;

	// Skip whitespace
	SKIP_WS(s);

	e = s;

	if (*s == _T('\"')) {										// Quoted string
		s++;
		e++;
		// Find end quote
		FIND_QT(e);
		_tcsncpy(szQuote, s, e - s);
		// Zero-terminate
		szQuote[e - s] = _T('\0');
		e++;
	} else {													// Non-quoted string
		// Find whitespace
		FIND_WS(e);
		_tcsncpy(szQuote, s, e - s);
		// Zero-terminate
		szQuote[e - s] = _T('\0');
	}

	if (ppszQuote) {
		*ppszQuote = szQuote;
	}
	if (ppszEnd)	{
		*ppszEnd = e;
	}

	return 0;
}

TCHAR* ExtractFilename(TCHAR* fullname)
{
	TCHAR* filename = fullname + _tcslen(fullname);

	do {
		filename--;
	} while (filename >= fullname && *filename != _T('\\') && *filename != _T('/') && *filename != _T(':'));

	return filename;
}

TCHAR* DriverToName(UINT32 nDrv)
{
	TCHAR* szName = NULL;
	UINT32 nOldDrv;

	if (nDrv >= nBurnDrvCount) {
		return NULL;
	}

	nOldDrv = nBurnDrvActive;
	nBurnDrvActive = nDrv;
	szName = BurnDrvGetText(DRV_NAME);
	nBurnDrvActive = nOldDrv;

	return szName;
}

UINT32 NameToDriver(TCHAR* szName)
{
	UINT32 nOldDrv = nBurnDrvActive;
	UINT32 nDrv = 0;

	for (nBurnDrvActive = 0; nBurnDrvActive < nBurnDrvCount; nBurnDrvActive++) {
		if (_tcscmp(szName, BurnDrvGetText(DRV_NAME)) == 0 && !(BurnDrvGetFlags() & BDF_BOARDROM)) {
			break;
		}
	}
	nDrv = nBurnDrvActive;
	if (nDrv >= nBurnDrvCount) {
		nDrv = ~0U;
	}

	nBurnDrvActive = nOldDrv;

	return nDrv;
}

TCHAR *FileExt(TCHAR *str)
{
	TCHAR *dot = _tcsrchr(str, _T('.'));

	return (dot) ? StrLower(dot) : str;
}

bool IsFileExt(TCHAR *str, TCHAR *ext)
{
	return (_tcsicmp(ext, FileExt(str)) == 0);
}

TCHAR *StrReplace(TCHAR *str, TCHAR find, TCHAR replace)
{
	INT32 length = _tcslen(str);

	for (INT32 i = 0; i < length; i++) {
		if (str[i] == find) str[i] = replace;
	}

	return str;
}

// StrLower() - leaves str untouched, returns modified string
TCHAR *StrLower(TCHAR *str)
{
	static TCHAR szBuffer[256] = _T("");
	INT32 length = _tcslen(str);

	if (length > 255) length = 255;

	for (INT32 i = 0; i < length; i++) {
		if (str[i] >= _T('A') && str[i] <= _T('Z'))
			szBuffer[i] = (str[i] + _T(' '));
		else
			szBuffer[i] = str[i];
	}
	szBuffer[length] = 0;

	return &szBuffer[0];
}

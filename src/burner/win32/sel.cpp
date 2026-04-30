// Driver Selector module
// TreeView Version by HyperYagami
#include "burner.h"
#include <process.h>

static void SelCopyString(TCHAR* pszDest, size_t nDestCount, const TCHAR* pszSrc)
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

static void SelAppendString(TCHAR* pszDest, size_t nDestCount, const TCHAR* pszSrc)
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

// reduce the total number of sets by this number - (isgsm, neogeo, nmk004, pgm, skns, ym2608, coleco, msx_msx, spectrum, spec128, spec1282a, decocass, midssio, cchip, fdsbios, ngp, bubsys, channelf, namcoc69, namcoc70, namcoc75, snes stuff)
// don't reduce for these as we display them in the list (neogeo, neocdz)
#define REDUCE_TOTAL_SETS_BIOS      28

UINT_PTR nTimer					= 0;
UINT_PTR nInitPreviewTimer		= 0;
int nDialogSelect				= -1;										// The driver which this dialog selected
int nOldDlgSelected				= -1;
bool bDialogCancel				= false;

bool bDrvSelected				= false;
static bool bSelOkay			= false;									// true: About to run the non-RomData game from the list
static bool bSelRomDataLaunch	= false;									// true: Hosted RomData already launched a driver before SelDialog() returns

static int nShowMVSCartsOnly	= 0;

HBITMAP hPrevBmp				= NULL;
HBITMAP hTitleBmp				= NULL;

HWND hSelDlg					= NULL;
static HWND hSelList			= NULL;
static HWND hParent				= NULL;
static HWND hInfoLabel[6]		= { NULL, NULL, NULL, NULL, NULL };			// 4 things in our Info-box
static HWND hInfoText[6]		= { NULL, NULL, NULL, NULL, NULL };			// 4 things in our Info-box

TCHAR szSearchString[256]       = { _T("") };                               // Stores the search text between game loads
TCHAR szSearchStringNormalized[256] = { _T("") };
bool bSearchStringInit          = false;                                    // Used (internally) while dialog is initting
static TCHAR szIpsSearchString[256] = { _T("") };
static TCHAR szRomDataSearchString[256] = { _T("") };
static bool bSelModeSwitching = false;

static HBRUSH hWhiteBGBrush;
static HICON hExpand, hCollapse;
static HICON hNotWorking, hNotFoundEss, hNotFoundNonEss;

static HICON hDrvIconMiss;

static char TreeBuilding		= 0;										// if 1, ignore TVN_SELCHANGED messages

static int bImageOrientation;
static int UpdatePreview(bool bReset, TCHAR *szPath, int HorCtrl, int VerCrtl);
static void GetTitlePreviewScale();
static void RebuildEverything();
static void ReselectCurrentDriverInList();
static void ForceHostedListFullRedraw(HWND hWnd);
static void UpdateRomSelectionDetails(HWND hDlg);
static bool RestoreCurrentDriverInRomList(HWND hDlg);
static void ApplyRomInfoFieldCaptions(HWND hDlg);
static void ShowRomInfoFields(HWND hDlg, bool bShowRom, bool bShowRomData);

int	nIconsSize					= ICON_16x16;
int	nIconsSizeXY				= 16;
bool bEnableIcons				= 0;
bool bIconsLoaded				= 0;
bool bIconsOnlyParents			= 1;
bool bIconsByHardwares			= 0;
int nIconsXDiff;
int nIconsYDiff;
static HICON *hDrvIcon;
bool bGameInfoOpen				= false;

HICON* pIconsCache              = NULL;

// Dialog Sizing
int nSelDlgWidth = 992;
int nSelDlgHeight = 638;
static int nSelMinTrackWidth = 992;
static int nSelMinTrackHeight = 638;
static int nDlgInitialWidth;
static int nDlgInitialHeight;
static int nDlgOptionsGrpInitialPos[4];
static int nDlgAvailableChbInitialPos[4];
static int nDlgUnavailableChbInitialPos[4];
static int nDlgAlwaysClonesChbInitialPos[4];
static int nDlgZipnamesChbInitialPos[4];
static int nDlgLatinTextChbInitialPos[4];
static int nDlgSearchSubDirsChbInitialPos[4];
static int nDlgDisableCrcChbInitialPos[4];
static int nDlgRomDirsBtnInitialPos[4];
static int nDlgScanRomsBtnInitialPos[4];
static int nDlgFilterGrpInitialPos[4];
static int nDlgFilterTreeInitialPos[4];
static int nDlgIpsGrpInitialPos[4];
static int nDlgApplyIpsChbInitialPos[4];
static int nDlgIpsManBtnInitialPos[4];
static int nDlgSearchGrpInitialPos[4];
static int nDlgSearchTxtInitialPos[4];
static int nDlgCancelBtnInitialPos[4];
static int nDlgPlayBtnInitialPos[4];
static int nDlgTitleGrpInitialPos[4];
static int nDlgTitleImgHInitialPos[4];
static int nDlgTitleImgVInitialPos[4];
static int nDlgPreviewGrpInitialPos[4];
static int nDlgPreviewImgHInitialPos[4];
static int nDlgPreviewImgVInitialPos[4];
static int nDlgWhiteBoxInitialPos[4];
static int nDlgGameInfoLblInitialPos[4];
static int nDlgRomNameLblInitialPos[4];
static int nDlgRomInfoLblInitialPos[4];
static int nDlgReleasedByLblInitialPos[4];
static int nDlgGenreLblInitialPos[4];
static int nDlgNotesLblInitialPos[4];
static int nDlgGameInfoTxtInitialPos[4];
static int nDlgRomNameTxtInitialPos[4];
static int nDlgRomInfoTxtInitialPos[4];
static int nDlgReleasedByTxtInitialPos[4];
static int nDlgGenreTxtInitialPos[4];
static int nDlgNotesTxtInitialPos[4];
static int nDlgDrvCountTxtInitialPos[4];
static int nDlgDrvRomInfoBtnInitialPos[4];
static int nDlgSelectGameGrpInitialPos[4];
static int nDlgSelectGameLstInitialPos[4];
static int nDlgModeRomBtnInitialPos[4];
static int nDlgModeIpsBtnInitialPos[4];
static int nDlgModeRomDataBtnInitialPos[4];
static int nDlgIpsTreeInitialPos[4];
static int nDlgIpsDescInitialPos[4];
static int nDlgIpsLangInitialPos[4];
static int nDlgIpsClearInitialPos[4];
static int nDlgRomDataListInitialPos[4];
static int nDlgRomDataDirBtnInitialPos[4];
static int nDlgRomDataScanBtnInitialPos[4];
static int nDlgRomDataSubdirInitialPos[4];

static int _213 = 213;
static int _160 = 160;
static int dpi_x = 96;

static HWND hSelModeTitle[3] = { NULL, NULL, NULL };
static INT32 nSelActiveMode = 0; // visual shell: 0=ROM, 1=IPS, 2=RomData
static INT32 nSelRememberedMode = 0; // keep the last open selector panel during this session
static HFONT hSelModeFontActive = NULL;
static HWND hSelIpsTree = NULL;
static HWND hSelIpsDesc = NULL;
static HWND hSelIpsLang = NULL;
static HWND hSelIpsClear = NULL;
static HWND hSelRomDataList = NULL;
static HWND hSelRomDataDir = NULL;
static HWND hSelRomDataScan = NULL;
static HWND hSelRomDataSubdir = NULL;

static bool UsingBuiltinChineseLocalisation()
{
	return bLocalisationActive && szLocalisationTemplate[0] == _T('\0');
}

static bool SelectDialogLabelsAreChinese(HWND hDlg)
{
	if (UsingBuiltinChineseLocalisation()) {
		return true;
	}

#if defined(_UNICODE)
	if (hDlg && IsWindow(hDlg)) {
		wchar_t szCaption[16] = L"";
		GetDlgItemTextW(hDlg, IDC_STATIC_OPT, szCaption, _countof(szCaption));
		return wcscmp(szCaption, L"\x9009\x9879") == 0;
	}
#endif

	return false;
}

static void ApplyHostedSelectDialogCaptions(HWND hDlg)
{
	const bool bChinese = SelectDialogLabelsAreChinese(hDlg);

#if defined(_UNICODE)
	if (hSelIpsClear && IsWindow(hSelIpsClear)) {
		SetWindowTextW(hSelIpsClear, bChinese ? L"\x53D6\x6D88\x6240\x6709" : L"Deselect All");
	}
	if (hSelRomDataDir && IsWindow(hSelRomDataDir)) {
		SetWindowTextW(hSelRomDataDir, bChinese ? L"Dat \x76EE\x5F55" : L"Dirs...");
	}
	if (hSelRomDataScan && IsWindow(hSelRomDataScan)) {
		SetWindowTextW(hSelRomDataScan, bChinese ? L"\x626B\x63CF Dat" : L"Scan Dat");
	}
#else
	if (hSelIpsClear && IsWindow(hSelIpsClear)) {
		SetWindowText(hSelIpsClear, _T("Deselect All"));
	}
	if (hSelRomDataDir && IsWindow(hSelRomDataDir)) {
		SetWindowText(hSelRomDataDir, _T("Dirs..."));
	}
	if (hSelRomDataScan && IsWindow(hSelRomDataScan)) {
		SetWindowText(hSelRomDataScan, _T("Scan Dat"));
	}
#endif
}

static void FixBuiltinChineseSelectDialogLabels(HWND hDlg)
{
	if (!UsingBuiltinChineseLocalisation()) {
		return;
	}

	SetDlgItemTextW(hDlg, IDC_SEL_IPSGROUP,    L"IPS");
	SetDlgItemTextW(hDlg, IDC_SEL_APPLYIPS,    L"\x5E94\x7528\x8865\x4E01");
	SetDlgItemTextW(hDlg, IDC_SEL_IPSMANAGER,  L"IPS \x7BA1\x7406\x5668");
	SetDlgItemTextW(hDlg, IDC_SEL_SEARCHGROUP, L"");
	SetDlgItemTextW(hDlg, IDCANCEL,            L"\x53D6\x6D88");
	SetDlgItemTextW(hDlg, IDOK,                L"\x8FD0\x884C");
}

static void SetDialogItemTextMerged(HWND hDlg, INT32 nID, const wchar_t* pszZh, const TCHAR* pszEn)
{
#if defined(_UNICODE)
	if (UsingBuiltinChineseLocalisation() && pszZh) {
		SetDlgItemTextW(hDlg, nID, pszZh);
		return;
	}
#endif
	SetDlgItemText(hDlg, nID, pszEn);
}

static void UpdateSelectModeTitleStyles()
{
	for (INT32 i = 0; i < 3; i++) {
		if (hSelModeTitle[i] && IsWindow(hSelModeTitle[i])) {
			const bool bActive = (i == nSelActiveMode);
			HFONT hFont = (bActive && hSelModeFontActive) ? hSelModeFontActive : (HFONT)SendMessage(hSelDlg, WM_GETFONT, 0, 0);
			if (hFont) {
				SendMessage(hSelModeTitle[i], WM_SETFONT, (WPARAM)hFont, TRUE);
			}
			InvalidateRect(hSelModeTitle[i], NULL, TRUE);
		}
	}
}

static bool IsSelectDialogButtonId(INT32 nID)
{
	switch (nID) {
		case IDRESCAN:
		case IDROM:
		case IDC_SEL_MODE_ROM:
		case IDC_SEL_MODE_IPS:
		case IDC_SEL_MODE_ROMDATA:
		case IDGAMEINFO:
		case IDCANCEL:
		case IDOK:
		case IDC_SEL_IPS_CLEAR:
		case IDC_ROMDATA_SELDIR_BUTTON:
		case IDC_ROMDATA_SCAN_BUTTON:
			return true;
	}

	return false;
}

static void ApplySelectButtonOwnerDraw(HWND hDlg)
{
	const INT32 nIDs[] = {
		IDRESCAN, IDROM,
		IDC_SEL_MODE_ROM, IDC_SEL_MODE_IPS, IDC_SEL_MODE_ROMDATA,
		IDGAMEINFO, IDCANCEL, IDOK,
		IDC_SEL_IPS_CLEAR,
		IDC_ROMDATA_SELDIR_BUTTON, IDC_ROMDATA_SCAN_BUTTON
	};

	for (INT32 i = 0; i < (INT32)_countof(nIDs); i++) {
		HWND hCtrl = GetDlgItem(hDlg, nIDs[i]);
		if (hCtrl == NULL || !IsWindow(hCtrl)) continue;

		LONG_PTR nStyle = GetWindowLongPtr(hCtrl, GWL_STYLE);
		nStyle &= ~0x0F;
		nStyle |= BS_OWNERDRAW;
		SetWindowLongPtr(hCtrl, GWL_STYLE, nStyle);

		HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
		if (hFont) {
			SendMessage(hCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);
		}
	}
}

static void DrawSelectDialogButton(const DRAWITEMSTRUCT* pDraw)
{
	if (pDraw == NULL) return;

	const UINT nID = pDraw->CtlID;
	const bool bModeButton = (nID == IDC_SEL_MODE_ROM || nID == IDC_SEL_MODE_IPS || nID == IDC_SEL_MODE_ROMDATA);
	const bool bActiveMode =
		(nID == IDC_SEL_MODE_ROM && nSelActiveMode == 0) ||
		(nID == IDC_SEL_MODE_IPS && nSelActiveMode == 1) ||
		(nID == IDC_SEL_MODE_ROMDATA && nSelActiveMode == 2);
	const bool bPressed = (pDraw->itemState & ODS_SELECTED) != 0;
	const bool bDisabled = (pDraw->itemState & ODS_DISABLED) != 0;

	COLORREF nFill = RGB(242, 242, 242);
	COLORREF nBorder = RGB(150, 150, 150);
	COLORREF nText = RGB(48, 48, 48);

	if (bDisabled) {
		nFill = RGB(236, 236, 236);
		nBorder = RGB(188, 188, 188);
		nText = RGB(150, 150, 150);
	} else if (bPressed || bActiveMode) {
		nFill = bPressed ? RGB(204, 48, 48) : RGB(220, 62, 62);
		nBorder = RGB(150, 28, 28);
		nText = RGB(255, 255, 255);
	}

	HBRUSH hFillBrush = CreateSolidBrush(nFill);
	HBRUSH hBorderBrush = CreateSolidBrush(nBorder);
	FillRect(pDraw->hDC, &pDraw->rcItem, hFillBrush);
	FrameRect(pDraw->hDC, &pDraw->rcItem, hBorderBrush);
	DeleteObject(hFillBrush);
	DeleteObject(hBorderBrush);

	RECT rcText = pDraw->rcItem;
	if (bPressed) {
		OffsetRect(&rcText, 1, 1);
	}

	TCHAR szText[128] = _T("");
	GetWindowText((HWND)pDraw->hwndItem, szText, _countof(szText));

	HFONT hFont = (bModeButton && bActiveMode && hSelModeFontActive) ? hSelModeFontActive : (HFONT)SendMessage(hSelDlg, WM_GETFONT, 0, 0);
	HFONT hOldFont = NULL;
	if (hFont) {
		hOldFont = (HFONT)SelectObject(pDraw->hDC, hFont);
	}

	SetBkMode(pDraw->hDC, TRANSPARENT);
	SetTextColor(pDraw->hDC, nText);
	DrawText(pDraw->hDC, szText, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

	if ((pDraw->itemState & ODS_FOCUS) && !bDisabled) {
		RECT rcFocus = pDraw->rcItem;
		InflateRect(&rcFocus, -3, -3);
		DrawFocusRect(pDraw->hDC, &rcFocus);
	}

	if (hOldFont) {
		SelectObject(pDraw->hDC, hOldFont);
	}
}

static void ApplyMergedSelectDialogCaptions(HWND hDlg)
{
	SetDialogItemTextMerged(hDlg, IDC_STATIC_SYS,      L"\x7B5B\x9009", _T("Filters"));
	SetDialogItemTextMerged(hDlg, IDC_STATIC_OPT,      L"\x9009\x9879", _T("Options"));
	SetDialogItemTextMerged(hDlg, IDC_STATIC1,         L"\x6E38\x620F\x5217\x8868", _T("Game List"));
	SetDialogItemTextMerged(hDlg, IDC_STATIC2,         L"\x9884\x89C8\x56FE", _T("Preview"));
	SetDialogItemTextMerged(hDlg, IDC_STATIC3,         L"\x6E38\x620F\x4FE1\x606F", _T("Game Info"));
	SetDialogItemTextMerged(hDlg, IDC_CHECKAVAILABLE,   L"\x62E5\x6709\x7684\x6E38\x620F", _T("Owned games"));
	SetDialogItemTextMerged(hDlg, IDC_CHECKUNAVAILABLE, L"\x672A\x62E5\x6709\x6E38\x620F", _T("Missing games"));
	SetDialogItemTextMerged(hDlg, IDC_CHECKAUTOEXPAND,  L"\x514B\x9686\x6E38\x620F", _T("Show clone games"));
	SetDialogItemTextMerged(hDlg, IDC_SEL_SHORTNAME,    L"\x538B\x7F29\x5305\x540D", _T("Show zip names"));
	SetDialogItemTextMerged(hDlg, IDC_SEL_ASCIIONLY,    L"\x53EA\x663E\x793A\x62C9\x4E01\x6587", _T("Latin text only"));
	SetDialogItemTextMerged(hDlg, IDC_SEL_SUBDIRS,      L"\x641C\x7D22\x5B50\x76EE\x5F55", _T("Search subdirectories"));
	SetDialogItemTextMerged(hDlg, IDC_SEL_DISABLECRC,   L"\x7981\x7528" L"CRC", _T("Disable CRC"));
	SetDialogItemTextMerged(hDlg, IDC_SEL_IPSGROUP,    L"", _T(""));
	SetDialogItemTextMerged(hDlg, IDC_SEL_SEARCHGROUP, L"", _T(""));
	SetDialogItemTextMerged(hDlg, IDC_SEL_APPLYIPS,    L"\x5E94\x7528\x8865\x4E01", _T("Apply IPS"));
	SetDialogItemTextMerged(hDlg, IDC_SEL_IPSMANAGER,  L"IPS \x7BA1\x7406\x5668", _T("IPS Manager"));
	SetDialogItemTextMerged(hDlg, IDROM,               L"\x76EE\x5F55...", _T("Dirs..."));
	SetDialogItemTextMerged(hDlg, IDRESCAN,            L"\x626B\x63CF\x6E38\x620F", _T("Scan Games"));
	SetDialogItemTextMerged(hDlg, IDGAMEINFO,          L"\x6E38\x620F\x8BE6\x60C5", _T("Game Info"));
	SetDialogItemTextMerged(hDlg, IDCANCEL,            L"\x53D6\x6D88", _T("Cancel"));
	SetDialogItemTextMerged(hDlg, IDOK,                L"\x8FD0\x884C", _T("Run"));
	ApplyHostedSelectDialogCaptions(hDlg);
}

static void ApplyRomInfoFieldCaptions(HWND hDlg)
{
	SetDialogItemTextMerged(hDlg, IDC_LABELCOMMENT, L"\x6E38\x620F:", _T("Game:"));
	SetDialogItemTextMerged(hDlg, IDC_LABELROMNAME, L"\x6587\x4EF6:", _T("ROM:"));
	SetDialogItemTextMerged(hDlg, IDC_LABELROMINFO, L"\x4FE1\x606F:", _T("Info:"));
	SetDialogItemTextMerged(hDlg, IDC_LABELSYSTEM,  L"\x53D1\x884C:", _T("Released:"));
	SetDialogItemTextMerged(hDlg, IDC_LABELGENRE,   L"\x7C7B\x578B:", _T("Genre:"));
	SetDialogItemTextMerged(hDlg, IDC_LABELNOTES,   L"\x6CE8\x91CA:", _T("Notes:"));
}

static void ShowRomInfoFields(HWND hDlg, bool bShowRom, bool bShowRomData)
{
	if (hDlg == NULL) return;

	const INT32 nAllLabelIds[6] = {
		IDC_LABELCOMMENT, IDC_LABELROMNAME, IDC_LABELROMINFO,
		IDC_LABELSYSTEM, IDC_LABELGENRE, IDC_LABELNOTES
	};
	const INT32 nAllTextIds[6] = {
		IDC_TEXTCOMMENT, IDC_TEXTROMNAME, IDC_TEXTROMINFO,
		IDC_TEXTSYSTEM, IDC_TEXTGENRE, IDC_TEXTNOTES
	};

	for (INT32 i = 0; i < 6; i++) {
		ShowWindow(GetDlgItem(hDlg, nAllLabelIds[i]), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, nAllTextIds[i]), SW_HIDE);
	}

	if (bShowRom) {
		for (INT32 i = 0; i < 6; i++) {
			HWND hLabel = GetDlgItem(hDlg, nAllLabelIds[i]);
			HWND hText = GetDlgItem(hDlg, nAllTextIds[i]);
			ShowWindow(hLabel, SW_SHOW);
			ShowWindow(hText, SW_SHOW);
			EnableWindow(hLabel, TRUE);
		}
		return;
	}

	if (bShowRomData) {
		const INT32 nRomDataLabelIds[3] = { IDC_LABELCOMMENT, IDC_LABELROMNAME, IDC_LABELROMINFO };
		const INT32 nRomDataTextIds[3]  = { IDC_TEXTCOMMENT, IDC_TEXTROMNAME, IDC_TEXTROMINFO };
		for (INT32 i = 0; i < 3; i++) {
			HWND hLabel = GetDlgItem(hDlg, nRomDataLabelIds[i]);
			HWND hText = GetDlgItem(hDlg, nRomDataTextIds[i]);
			ShowWindow(hLabel, SW_SHOW);
			ShowWindow(hText, SW_SHOW);
			EnableWindow(hLabel, TRUE);
		}
	}
}

static void EnsureSelectModeTitles(HWND hDlg)
{
	const INT32 nID[3] = { IDC_SEL_MODE_ROM, IDC_SEL_MODE_IPS, IDC_SEL_MODE_ROMDATA };
	const TCHAR* pszTitle[3] = { _T("ROM"), _T("IPS"), _T("RomData") };
	HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);

	for (INT32 i = 0; i < 3; i++) {
		if (hSelModeTitle[i] == NULL || !IsWindow(hSelModeTitle[i])) {
			hSelModeTitle[i] = GetDlgItem(hDlg, nID[i]);
		}
	}

	if (hSelModeFontActive == NULL && hFont) {
		LOGFONT lf = { 0 };
		if (GetObject(hFont, sizeof(LOGFONT), &lf)) {
			lf.lfWeight = FW_BOLD;
			lf.lfUnderline = TRUE;
			if (lf.lfHeight < 0) {
				lf.lfHeight = (lf.lfHeight * 12) / 10;
			} else if (lf.lfHeight > 0) {
				lf.lfHeight = (lf.lfHeight * 12) / 10;
			}
			hSelModeFontActive = CreateFontIndirect(&lf);
		}
	}

	for (INT32 i = 0; i < 3; i++) {
		if (hSelModeTitle[i] == NULL || !IsWindow(hSelModeTitle[i])) {
			hSelModeTitle[i] = CreateWindowEx(0, _T("BUTTON"), pszTitle[i],
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
				0, 0, 84, 24,
				hDlg, (HMENU)(INT_PTR)nID[i], hAppInst, NULL);
		} else {
			SetWindowText(hSelModeTitle[i], pszTitle[i]);
		}

		if (hSelModeTitle[i] && IsWindow(hSelModeTitle[i]) && hFont) {
			SendMessage(hSelModeTitle[i], WM_SETFONT, (WPARAM)hFont, TRUE);
		}
	}

	UpdateSelectModeTitleStyles();
}

static bool SyncIpsHostedStorageNameFromSelection()
{
	if (nSelActiveMode == 2) {
		TCHAR szRomSet[100] = { 0 };
		if (RomDataHostedGetSelectedRomSet(szRomSet, _countof(szRomSet))) {
			IpsSetHostedStorageName(szRomSet);
			return true;
		}

		IpsSetHostedStorageName(NULL);
		return false;
	}

	if (nSelActiveMode == 0) {
		IpsSetHostedStorageName(NULL);
	}

	return true;
}

static bool SelectIpsAvailable()
{
	if (nShowMVSCartsOnly) {
		return false;
	}

	if (nSelActiveMode != 1 && !SyncIpsHostedStorageNameFromSelection()) {
		return false;
	}

	INT32 nTargetDrv = -1;
	if (nSelActiveMode == 2) {
		nTargetDrv = nBurnDrvActive;
	} else {
		if (nDialogSelect < 0) {
			return false;
		}
		if (nSelActiveMode == 0 && !bDrvSelected) {
			return false;
		}
		nTargetDrv = nDialogSelect;
	}

	if (nTargetDrv < 0) {
		return false;
	}

	INT32 nOldDrv = nBurnDrvActive;
	nBurnDrvActive = nTargetDrv;
	bool bAvailable = GetIpsNumPatches() > 0;
	nBurnDrvActive = nOldDrv;
	return bAvailable;
}

static void EnsureSelectIpsHostedControls(HWND hDlg)
{
	HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);

	if (hSelIpsTree == NULL || !IsWindow(hSelIpsTree)) {
		hSelIpsTree = GetDlgItem(hDlg, IDC_SEL_IPS_TREE);
	}

	if (hSelIpsTree == NULL || !IsWindow(hSelIpsTree)) {
		hSelIpsTree = CreateWindowEx(WS_EX_STATICEDGE, WC_TREEVIEW, _T(""),
			WS_CHILD | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_NOHSCROLL | TVS_CHECKBOXES,
			0, 0, 100, 100, hDlg, (HMENU)(INT_PTR)IDC_SEL_IPS_TREE, hAppInst, NULL);
		ShowWindow(hSelIpsTree, SW_HIDE);
	}

	if (hSelIpsTree && IsWindow(hSelIpsTree)) {
		if (hFont) SendMessage(hSelIpsTree, WM_SETFONT, (WPARAM)hFont, TRUE);
		TreeView_SetBkColor(hSelIpsTree, RGB(0xFF, 0xFF, 0xFF));
		TreeView_SetTextColor(hSelIpsTree, RGB(0x00, 0x00, 0x00));
		TreeView_SetLineColor(hSelIpsTree, RGB(0x80, 0x80, 0x80));
	}

	if (hSelIpsDesc == NULL || !IsWindow(hSelIpsDesc)) {
		hSelIpsDesc = GetDlgItem(hDlg, IDC_SEL_IPS_DESC);
	}

	if (hSelIpsDesc == NULL || !IsWindow(hSelIpsDesc)) {
		hSelIpsDesc = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
			WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
			0, 0, 100, 100, hDlg, (HMENU)(INT_PTR)IDC_SEL_IPS_DESC, hAppInst, NULL);
		if (hFont) SendMessage(hSelIpsDesc, WM_SETFONT, (WPARAM)hFont, TRUE);
		ShowWindow(hSelIpsDesc, SW_HIDE);
	}

	if (hSelIpsLang == NULL || !IsWindow(hSelIpsLang)) {
		hSelIpsLang = GetDlgItem(hDlg, IDC_SEL_IPS_LANG);
	}

	if (hSelIpsLang == NULL || !IsWindow(hSelIpsLang)) {
		hSelIpsLang = CreateWindowEx(0, _T("COMBOBOX"), _T(""),
			WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
			0, 0, 100, 120, hDlg, (HMENU)(INT_PTR)IDC_SEL_IPS_LANG, hAppInst, NULL);
		if (hFont) SendMessage(hSelIpsLang, WM_SETFONT, (WPARAM)hFont, TRUE);
		ShowWindow(hSelIpsLang, SW_HIDE);
	}

	if (hSelIpsClear == NULL || !IsWindow(hSelIpsClear)) {
		hSelIpsClear = GetDlgItem(hDlg, IDC_SEL_IPS_CLEAR);
	}

	if (hSelIpsClear == NULL || !IsWindow(hSelIpsClear)) {
		const TCHAR* pszClearText = _T("Deselect All");
#if defined(_UNICODE)
		if (UsingBuiltinChineseLocalisation()) {
			pszClearText = L"\x53D6\x6D88\x6240\x6709";
		}
#endif
		hSelIpsClear = CreateWindowEx(0, _T("BUTTON"), pszClearText,
			WS_CHILD | WS_TABSTOP,
			0, 0, 88, 22, hDlg, (HMENU)(INT_PTR)IDC_SEL_IPS_CLEAR, hAppInst, NULL);
		if (hFont) SendMessage(hSelIpsClear, WM_SETFONT, (WPARAM)hFont, TRUE);
		ShowWindow(hSelIpsClear, SW_HIDE);
	}

	if (hSelIpsDesc && IsWindow(hSelIpsDesc) && hFont) {
		SendMessage(hSelIpsDesc, WM_SETFONT, (WPARAM)hFont, TRUE);
	}
	if (hSelIpsLang && IsWindow(hSelIpsLang) && hFont) {
		SendMessage(hSelIpsLang, WM_SETFONT, (WPARAM)hFont, TRUE);
	}
	if (hSelIpsClear && IsWindow(hSelIpsClear) && hFont) {
		SendMessage(hSelIpsClear, WM_SETFONT, (WPARAM)hFont, TRUE);
	}

	ApplyHostedSelectDialogCaptions(hDlg);
}

static void EnsureSelectRomDataHostedControls(HWND hDlg)
{
	HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);

	if (hSelRomDataList == NULL || !IsWindow(hSelRomDataList)) {
		hSelRomDataList = GetDlgItem(hDlg, IDC_ROMDATA_LIST);
	}

	if (hSelRomDataList == NULL || !IsWindow(hSelRomDataList)) {
		hSelRomDataList = CreateWindowEx(WS_EX_STATICEDGE, WC_TREEVIEW, _T(""),
			WS_CHILD | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_NOHSCROLL | WS_BORDER,
			0, 0, 100, 100, hDlg, (HMENU)(INT_PTR)IDC_ROMDATA_LIST, hAppInst, NULL);
		ShowWindow(hSelRomDataList, SW_HIDE);
	}

	if (hSelRomDataList && IsWindow(hSelRomDataList)) {
		if (hFont) SendMessage(hSelRomDataList, WM_SETFONT, (WPARAM)hFont, TRUE);
		TreeView_SetBkColor(hSelRomDataList, RGB(0xFF, 0xFF, 0xFF));
		TreeView_SetTextColor(hSelRomDataList, RGB(0x00, 0x00, 0x00));
		TreeView_SetLineColor(hSelRomDataList, RGB(0x80, 0x80, 0x80));
	}

	if (hSelRomDataDir == NULL || !IsWindow(hSelRomDataDir)) {
		hSelRomDataDir = GetDlgItem(hDlg, IDC_ROMDATA_SELDIR_BUTTON);
	}

	if (hSelRomDataDir == NULL || !IsWindow(hSelRomDataDir)) {
		hSelRomDataDir = CreateWindowEx(0, _T("BUTTON"),
			UsingBuiltinChineseLocalisation() ? L"Dat \x76EE\x5F55" : _T("Dirs..."),
			WS_CHILD | WS_TABSTOP,
			0, 0, 76, 22, hDlg, (HMENU)(INT_PTR)IDC_ROMDATA_SELDIR_BUTTON, hAppInst, NULL);
		if (hFont) SendMessage(hSelRomDataDir, WM_SETFONT, (WPARAM)hFont, TRUE);
		ShowWindow(hSelRomDataDir, SW_HIDE);
	}

	if (hSelRomDataScan == NULL || !IsWindow(hSelRomDataScan)) {
		hSelRomDataScan = GetDlgItem(hDlg, IDC_ROMDATA_SCAN_BUTTON);
	}

	if (hSelRomDataScan == NULL || !IsWindow(hSelRomDataScan)) {
		hSelRomDataScan = CreateWindowEx(0, _T("BUTTON"),
			UsingBuiltinChineseLocalisation() ? L"\x626B\x63CF Dat" : _T("Scan Dat"),
			WS_CHILD | WS_TABSTOP,
			0, 0, 88, 22, hDlg, (HMENU)(INT_PTR)IDC_ROMDATA_SCAN_BUTTON, hAppInst, NULL);
		if (hFont) SendMessage(hSelRomDataScan, WM_SETFONT, (WPARAM)hFont, TRUE);
		ShowWindow(hSelRomDataScan, SW_HIDE);
	}

	if (hSelRomDataSubdir == NULL || !IsWindow(hSelRomDataSubdir)) {
		hSelRomDataSubdir = GetDlgItem(hDlg, IDC_ROMDATA_SUBDIR_CHECK);
	}

	if (hSelRomDataSubdir == NULL || !IsWindow(hSelRomDataSubdir)) {
		hSelRomDataSubdir = CreateWindowEx(0, _T("BUTTON"),
			UsingBuiltinChineseLocalisation() ? L"\x5B50\x76EE\x5F55" : _T("Subdirs"),
			WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
			0, 0, 72, 18, hDlg, (HMENU)(INT_PTR)IDC_ROMDATA_SUBDIR_CHECK, hAppInst, NULL);
		if (hFont) SendMessage(hSelRomDataSubdir, WM_SETFONT, (WPARAM)hFont, TRUE);
		ShowWindow(hSelRomDataSubdir, SW_HIDE);
	}

	if (hSelRomDataDir && IsWindow(hSelRomDataDir) && hFont) {
		SendMessage(hSelRomDataDir, WM_SETFONT, (WPARAM)hFont, TRUE);
	}
	if (hSelRomDataScan && IsWindow(hSelRomDataScan) && hFont) {
		SendMessage(hSelRomDataScan, WM_SETFONT, (WPARAM)hFont, TRUE);
	}
	if (hSelRomDataSubdir && IsWindow(hSelRomDataSubdir) && hFont) {
		SendMessage(hSelRomDataSubdir, WM_SETFONT, (WPARAM)hFont, TRUE);
	}

	ApplyHostedSelectDialogCaptions(hDlg);
}

static void UpdateSelectModeAvailability(HWND hDlg)
{
	EnsureSelectModeTitles(hDlg);
	SyncIpsHostedStorageNameFromSelection();

	bool bIpsAvailable = SelectIpsAvailable();
	if (hSelModeTitle[1]) {
		EnableWindow(hSelModeTitle[1], bIpsAvailable ? TRUE : FALSE);
	}

	INT32 nActivePatches = 0;
	if (bIpsAvailable) {
		if (nSelActiveMode == 1) {
			nActivePatches = IpsHostedGetCheckedCount();
		} else {
			INT32 nTargetDrv = (nSelActiveMode == 2) ? nBurnDrvActive : nDialogSelect;
			INT32 nOldDrv = nBurnDrvActive;
			nBurnDrvActive = nTargetDrv;
			nActivePatches = LoadIpsActivePatches();
			nBurnDrvActive = nOldDrv;
		}
	}

	EnableWindow(GetDlgItem(hDlg, IDC_SEL_APPLYIPS), bIpsAvailable && nActivePatches > 0);
	if (bIpsAvailable && nActivePatches > 0) {
		CheckDlgButton(hDlg, IDC_SEL_APPLYIPS, BST_CHECKED);
		bDoIpsPatch = true;
	} else {
		CheckDlgButton(hDlg, IDC_SEL_APPLYIPS, BST_UNCHECKED);
		bDoIpsPatch = false;
	}

	UpdateSelectModeTitleStyles();
	InvalidateRect(hDlg, NULL, FALSE);
}

static INT32 SelectClamp(INT32 nValue, INT32 nMin, INT32 nMax)
{
	if (nValue < nMin) return nMin;
	if (nValue > nMax) return nMax;
	return nValue;
}

static bool HasInitialRect(const int* pRect)
{
	return pRect != NULL && pRect[2] > 0 && pRect[3] > 0;
}

static bool SelectHasMergedRcShell()
{
	return
		HasInitialRect(nDlgFilterGrpInitialPos) &&
		HasInitialRect(nDlgFilterTreeInitialPos) &&
		HasInitialRect(nDlgOptionsGrpInitialPos) &&
		HasInitialRect(nDlgSelectGameGrpInitialPos) &&
		HasInitialRect(nDlgSelectGameLstInitialPos) &&
		HasInitialRect(nDlgIpsGrpInitialPos) &&
		HasInitialRect(nDlgSearchTxtInitialPos) &&
		HasInitialRect(nDlgPreviewGrpInitialPos) &&
		HasInitialRect(nDlgTitleGrpInitialPos) &&
		HasInitialRect(nDlgModeRomBtnInitialPos) &&
		HasInitialRect(nDlgModeIpsBtnInitialPos) &&
		HasInitialRect(nDlgModeRomDataBtnInitialPos);
}

static void LayoutSelectInfoFields(HWND hDlg, INT32 infoX, INT32 infoY, INT32 rightW, INT32 infoH)
{
	if (hDlg == NULL) return;

	const INT32 nAllLabelIds[6] = {
		IDC_LABELCOMMENT, IDC_LABELROMNAME, IDC_LABELROMINFO,
		IDC_LABELSYSTEM, IDC_LABELGENRE, IDC_LABELNOTES
	};
	const INT32 nAllTextIds[6] = {
		IDC_TEXTCOMMENT, IDC_TEXTROMNAME, IDC_TEXTROMINFO,
		IDC_TEXTSYSTEM, IDC_TEXTGENRE, IDC_TEXTNOTES
	};

	const bool bRomDataMode = (nSelActiveMode == 2);
	const INT32 nFieldCount = bRomDataMode ? 3 : 6;
	const INT32 fieldGap = bRomDataMode ? 4 : 3;
	const INT32 labelX = infoX + 14;
	const INT32 labelW = UsingBuiltinChineseLocalisation() ? 30 : 40;
	const INT32 labelH = 14;
	const INT32 textX = labelX + labelW + 4;
	const INT32 textW = rightW - (textX - infoX) - 12;
	const INT32 topPad = 26;
	const INT32 bottomPad = 12;
	INT32 fieldY = infoY + topPad;
	INT32 fieldContentH = infoH - topPad - bottomPad - fieldGap * (nFieldCount - 1);

	if (fieldContentH < (nFieldCount * 16)) {
		fieldContentH = nFieldCount * 16;
	}

	INT32 fieldHeights[6] = { 0 };
	if (bRomDataMode) {
		fieldHeights[0] = (fieldContentH * 50) / 100;
		fieldHeights[1] = (fieldContentH * 24) / 100;
		fieldHeights[2] = fieldContentH - fieldHeights[0] - fieldHeights[1];
	} else {
		fieldHeights[0] = (fieldContentH * 18) / 100;
		fieldHeights[1] = (fieldContentH * 13) / 100;
		fieldHeights[2] = (fieldContentH * 20) / 100;
		fieldHeights[3] = (fieldContentH * 13) / 100;
		fieldHeights[4] = (fieldContentH * 12) / 100;
		fieldHeights[5] = fieldContentH - fieldHeights[0] - fieldHeights[1] - fieldHeights[2] - fieldHeights[3] - fieldHeights[4];
	}

	for (INT32 i = 0; i < nFieldCount; i++) {
		MoveWindow(GetDlgItem(hDlg, nAllLabelIds[i]), labelX, fieldY, labelW, labelH, TRUE);
		MoveWindow(GetDlgItem(hDlg, nAllTextIds[i]), textX, fieldY - 1, textW, fieldHeights[i], TRUE);
		fieldY += fieldHeights[i] + fieldGap;
	}
}

static void GetPreviewBitmapFit(INT32* pnWidth, INT32* pnHeight, bool* pbVertical)
{
	if (pnWidth == NULL || pnHeight == NULL) return;

	INT32 ax = 4, ay = 3;
	const TCHAR* pszDrvName = NULL;
	if (bDrvSelected && nBurnDrvActive >= 0 && nBurnDrvActive < nBurnDrvCount) {
		BurnDrvGetAspect(&ax, &ay);
		pszDrvName = BurnDrvGetText(DRV_NAME);
	}

	if (ax <= 0 || ay <= 0 || ax > 100 || (pszDrvName && !_tcsncmp(pszDrvName, _T("wrally2"), 7))) {
		ax = 4;
		ay = 3;
	}

	const bool bVertical = (ay > ax);
	INT32 boxW = _213;
	INT32 boxH = _160;

	if (boxW <= 0) boxW = 213;
	if (boxH <= 0) boxH = 160;

	INT32 w = boxW;
	INT32 h = boxH;

	if (w <= 0) w = boxW;
	if (h <= 0) h = boxH;

	*pnWidth = w;
	*pnHeight = h;
	if (pbVertical) {
		*pbVertical = bVertical;
	}
}

static void GetDefaultPreviewBitmapFit(INT32* pnWidth, INT32* pnHeight, bool* pbVertical)
{
	if (pnWidth) *pnWidth = _213;
	if (pnHeight) *pnHeight = _160;
	if (pbVertical) *pbVertical = false;
}

static void LayoutMergedSelectDialog(HWND hDlg)
{
	if (hDlg == NULL) return;
	EnsureSelectIpsHostedControls(hDlg);
	EnsureSelectRomDataHostedControls(hDlg);
	ApplySelectButtonOwnerDraw(hDlg);

	// Use the unified runtime layout for all selector modes.
	if (false && SelectHasMergedRcShell()) {
		HWND hMainTree = GetDlgItem(hDlg, IDC_TREE1);
		HWND hDrvCount = GetDlgItem(hDlg, IDC_DRVCOUNT);

		EnsureSelectModeTitles(hDlg);
		ApplyMergedSelectDialogCaptions(hDlg);
		ApplyRomInfoFieldCaptions(hDlg);

		if (hDrvCount) {
			LONG_PTR nStyle = GetWindowLongPtr(hDrvCount, GWL_STYLE);
			nStyle &= ~(SS_LEFT | SS_CENTER);
			nStyle |= SS_RIGHT;
			SetWindowLongPtr(hDrvCount, GWL_STYLE, nStyle);
			SetWindowPos(hDrvCount, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}

		ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCHGROUP), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_SCREENSHOT2_H), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_SCREENSHOT2_V), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_SEL_IPSMANAGER), SW_HIDE);
		EnableWindow(GetDlgItem(hDlg, IDC_SEL_IPSMANAGER), FALSE);

		if (nSelActiveMode == 1) {
			SetDialogItemTextMerged(hDlg, IDC_STATIC1, L"IPS\x8865\x4E01", _T("IPS Patches"));
			SetDialogItemTextMerged(hDlg, IDC_STATIC3, L"\x8865\x4E01\x4FE1\x606F", _T("Patch Info"));
			SetDialogItemTextMerged(hDlg, IDOK, L"\x786E\x5B9A", _T("OK"));
			SetDialogItemTextMerged(hDlg, IDCANCEL, L"\x53D6\x6D88", _T("Cancel"));

			ShowWindow(hMainTree, SW_HIDE);
			ShowWindow(hSelRomDataList, SW_HIDE);
			if (hSelIpsTree) {
				ShowWindow(hSelIpsTree, SW_SHOW);
				SetWindowPos(hSelIpsTree, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				ForceHostedListFullRedraw(hSelIpsTree);
			}
			ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), SW_SHOW);
			ShowWindow(hSelIpsLang, SW_SHOW);
			ShowWindow(hSelIpsClear, SW_SHOW);
			ShowWindow(hSelRomDataSubdir, SW_HIDE);
			ShowWindow(hSelRomDataDir, SW_HIDE);
			ShowWindow(hSelRomDataScan, SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDGAMEINFO), SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), SW_HIDE);
			ShowWindow(hSelIpsDesc, SW_SHOW);
			ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_SHOW);
			ShowWindow(GetDlgItem(hDlg, IDOK), SW_SHOW);

			ShowRomInfoFields(hDlg, false, false);
		} else if (nSelActiveMode == 2) {
			SetDialogItemTextMerged(hDlg, IDC_STATIC1, L"RomData", _T("RomData"));
			SetDialogItemTextMerged(hDlg, IDC_STATIC3, L"RomData\x4FE1\x606F", _T("RomData Info"));
			SetDialogItemTextMerged(hDlg, IDC_LABELCOMMENT, L"\x8DEF\x5F84:", _T("Path:"));
			SetDialogItemTextMerged(hDlg, IDC_LABELROMNAME, L"\x9A71\x52A8:", _T("Driver:"));
			SetDialogItemTextMerged(hDlg, IDC_LABELROMINFO, L"\x786C\x4EF6:", _T("Hardware:"));
			SetDialogItemTextMerged(hDlg, IDC_ROMDATA_SUBDIR_CHECK, L"\x5B50\x76EE\x5F55", _T("Subdirs"));
			SetDialogItemTextMerged(hDlg, IDCANCEL, L"\x53D6\x6D88", _T("Cancel"));

			ShowWindow(hMainTree, SW_HIDE);
			if (hSelIpsTree) {
				ShowWindow(hSelIpsTree, SW_HIDE);
			}
			ShowWindow(hSelRomDataList, SW_SHOW);
			SetWindowPos(hSelRomDataList, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			ForceHostedListFullRedraw(hSelRomDataList);
			ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), SW_SHOW);
			ShowWindow(hSelIpsLang, SW_HIDE);
			ShowWindow(hSelIpsClear, SW_HIDE);
			ShowWindow(hSelRomDataSubdir, SW_SHOW);
			ShowWindow(hSelRomDataDir, SW_SHOW);
			ShowWindow(hSelRomDataScan, SW_SHOW);
			ShowWindow(GetDlgItem(hDlg, IDGAMEINFO), SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_SHOW);
			ShowWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), SW_SHOW);
			ShowWindow(hSelIpsDesc, SW_HIDE);

			ShowRomInfoFields(hDlg, false, true);
		} else {
			ShowWindow(hMainTree, SW_SHOW);
			SetWindowPos(hMainTree, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			InvalidateRect(hMainTree, NULL, FALSE);
			UpdateWindow(hMainTree);
			if (hSelIpsTree) {
				ShowWindow(hSelIpsTree, SW_HIDE);
			}
			ShowWindow(hSelRomDataList, SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), SW_SHOW);
			ShowWindow(hSelIpsLang, SW_HIDE);
			ShowWindow(hSelIpsClear, SW_HIDE);
			ShowWindow(hSelRomDataSubdir, SW_HIDE);
			ShowWindow(hSelRomDataDir, SW_HIDE);
			ShowWindow(hSelRomDataScan, SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDGAMEINFO), SW_SHOW);
			ShowWindow(GetDlgItem(hDlg, IDOK), SW_SHOW);
			ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), SW_SHOW);
			ShowWindow(hSelIpsDesc, SW_HIDE);

			ShowRomInfoFields(hDlg, true, false);
		}

		UpdateSelectModeTitleStyles();
		GetTitlePreviewScale();
		return;
	}

	RECT rc = { 0 };
	GetClientRect(hDlg, &rc);

	const INT32 margin = 8;
	const INT32 topMargin = 12;
	const INT32 gap = 8;
	const INT32 statusH = 0;
	const INT32 listHeaderH = 30;
	const INT32 actionRowH = 28;
	const INT32 buttonGap = 8;

	INT32 stripH = 28;
	INT32 leftW = 186;
	INT32 rightW = 242;
	INT32 optionsH = 110;
	INT32 buttonW = 84;
	INT32 buttonH = 24;

	const INT32 contentBottom = rc.bottom - margin - statusH;

	INT32 filterX = margin;
	INT32 filterY = topMargin;
	INT32 optionsX = margin;
	INT32 optionsW = leftW;
	INT32 optionsY = SelectClamp(contentBottom - optionsH, filterY + 230 + gap, contentBottom - 76);
	INT32 filterH = SelectClamp(optionsY - gap - filterY, 230, 4000);

	INT32 rightX = rc.right - margin - rightW;
	INT32 mainX = filterX + leftW + gap;
	INT32 mainW = SelectClamp(rightX - gap - mainX, 220, 4000);
	INT32 listY = topMargin;
	INT32 listH = SelectClamp(contentBottom - listY - stripH - gap, 248, 4000);
	INT32 stripY = listY + listH + gap;

	INT32 previewX = rightX;
	INT32 previewY = topMargin;
	INT32 previewInsetX = 10;
	INT32 previewInsetY = 16;
	INT32 previewInnerW = SelectClamp(rightW - previewInsetX * 2, 214, 252);
	INT32 previewInnerH = (previewInnerW * 3) / 4;
	INT32 previewH = previewInnerH + previewInsetY * 2 + 6;

	INT32 infoX = rightX;
	INT32 infoY = previewY + previewH + 4;
	INT32 infoH = SelectClamp(contentBottom - infoY - actionRowH - gap, 188, 4000);

	INT32 actionRowY = contentBottom - actionRowH;
	INT32 runBtnX = infoX + rightW - 14 - buttonW;
	INT32 cancelBtnX = runBtnX - buttonGap - buttonW;
	INT32 gameInfoBtnX = cancelBtnX - buttonGap - buttonW;
	INT32 listGroupBottom = stripY - 4;
	INT32 listInnerX = mainX + 8;
	INT32 listInnerY = listY + listHeaderH;
	INT32 listInnerW = mainW - 16;
	INT32 listInnerBottom = stripY - 8;
	INT32 listInnerH = SelectClamp(listInnerBottom - listInnerY, 140, 4000);
	INT32 ipsListX = listInnerX;
	INT32 ipsListY = listInnerY;
	INT32 ipsListW = listInnerW;
	INT32 ipsListH = listInnerH;
	INT32 romDataListX = listInnerX;
	INT32 romDataListY = listInnerY;
	INT32 romDataListW = listInnerW;
	INT32 romDataListH = listInnerH;

	const bool bMergedRcShell = false;

	if (bMergedRcShell) {
		filterX = nDlgFilterGrpInitialPos[0];
		filterY = nDlgFilterGrpInitialPos[1];
		leftW = nDlgFilterGrpInitialPos[2];
		filterH = nDlgFilterGrpInitialPos[3];

		optionsX = nDlgOptionsGrpInitialPos[0];
		optionsY = nDlgOptionsGrpInitialPos[1];
		optionsW = nDlgOptionsGrpInitialPos[2];
		optionsH = nDlgOptionsGrpInitialPos[3];

		mainX = nDlgSelectGameGrpInitialPos[0];
		listY = nDlgSelectGameGrpInitialPos[1];
		mainW = nDlgSelectGameGrpInitialPos[2];

		stripY = nDlgIpsGrpInitialPos[1];
		stripH = nDlgIpsGrpInitialPos[3];
		listH = SelectClamp(stripY - gap - listY, 180, 4000);

		previewX = nDlgPreviewGrpInitialPos[0];
		previewY = nDlgPreviewGrpInitialPos[1];
		rightW = nDlgPreviewGrpInitialPos[2];
		previewH = nDlgPreviewGrpInitialPos[3];

		if (HasInitialRect(nDlgPreviewImgHInitialPos)) {
			previewInsetX = nDlgPreviewImgHInitialPos[0] - previewX;
			previewInsetY = nDlgPreviewImgHInitialPos[1] - previewY;
			previewInnerW = nDlgPreviewImgHInitialPos[2];
			previewInnerH = nDlgPreviewImgHInitialPos[3];
		}

		infoX = nDlgTitleGrpInitialPos[0];
		infoY = nDlgTitleGrpInitialPos[1];
		infoH = nDlgTitleGrpInitialPos[3];

		if (HasInitialRect(nDlgPlayBtnInitialPos)) {
			buttonW = nDlgPlayBtnInitialPos[2];
			buttonH = nDlgPlayBtnInitialPos[3];
			actionRowY = nDlgPlayBtnInitialPos[1];
			runBtnX = nDlgPlayBtnInitialPos[0];
		}
		if (HasInitialRect(nDlgCancelBtnInitialPos)) {
			cancelBtnX = nDlgCancelBtnInitialPos[0];
		}
		if (HasInitialRect(nDlgDrvRomInfoBtnInitialPos)) {
			gameInfoBtnX = nDlgDrvRomInfoBtnInitialPos[0];
		}

		listGroupBottom = nDlgSelectGameGrpInitialPos[1] + nDlgSelectGameGrpInitialPos[3];
		listInnerX = nDlgSelectGameLstInitialPos[0];
		listInnerY = nDlgSelectGameLstInitialPos[1];
		listInnerW = nDlgSelectGameLstInitialPos[2];
		listInnerBottom = listInnerY + nDlgSelectGameLstInitialPos[3];
		listInnerH = nDlgSelectGameLstInitialPos[3];

		ipsListX = nDlgIpsTreeInitialPos[0];
		ipsListY = nDlgIpsTreeInitialPos[1];
		ipsListW = nDlgIpsTreeInitialPos[2];
		ipsListH = nDlgIpsTreeInitialPos[3];

		romDataListX = nDlgRomDataListInitialPos[0];
		romDataListY = nDlgRomDataListInitialPos[1];
		romDataListW = nDlgRomDataListInitialPos[2];
		romDataListH = nDlgRomDataListInitialPos[3];
	}

	MoveWindow(GetDlgItem(hDlg, IDC_STATIC_SYS), filterX, filterY, leftW, filterH, TRUE);
	if (bMergedRcShell) {
		MoveWindow(GetDlgItem(hDlg, IDC_TREE2), nDlgFilterTreeInitialPos[0], nDlgFilterTreeInitialPos[1], nDlgFilterTreeInitialPos[2], nDlgFilterTreeInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDRESCAN), nDlgScanRomsBtnInitialPos[0], nDlgScanRomsBtnInitialPos[1], nDlgScanRomsBtnInitialPos[2], nDlgScanRomsBtnInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDROM), nDlgRomDirsBtnInitialPos[0], nDlgRomDirsBtnInitialPos[1], nDlgRomDirsBtnInitialPos[2], nDlgRomDirsBtnInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_TREE2), filterX + 8, filterY + 18, leftW - 16, filterH - 56, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDRESCAN), filterX + 8, filterY + filterH - 30, (leftW - 24) / 2, 22, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDROM), filterX + 16 + (leftW - 24) / 2, filterY + filterH - 30, (leftW - 24) / 2, 22, TRUE);
	}

	MoveWindow(GetDlgItem(hDlg, IDC_STATIC_OPT), optionsX, optionsY, optionsW, optionsH, TRUE);

	const bool bChineseLabels = SelectDialogLabelsAreChinese(hDlg);
	const INT32 optInnerX = optionsX + 10;
	const INT32 optInnerRight = optionsX + optionsW - 10;
	const INT32 optGap = bChineseLabels ? 16 : 12;
	const INT32 optCol1 = optInnerX;
	const INT32 optCol1W = bChineseLabels ? 76 : ((optInnerRight - optInnerX - optGap) / 2);
	const INT32 optCol2 = optCol1 + optCol1W + optGap;
	const INT32 optCol2W = optInnerRight - optCol2;
	const INT32 optLine1 = optionsY + 20;
	const INT32 optStep = 22;
	if (bMergedRcShell) {
		MoveWindow(GetDlgItem(hDlg, IDC_CHECKAVAILABLE),   nDlgAvailableChbInitialPos[0], nDlgAvailableChbInitialPos[1], nDlgAvailableChbInitialPos[2], nDlgAvailableChbInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_CHECKUNAVAILABLE), nDlgUnavailableChbInitialPos[0], nDlgUnavailableChbInitialPos[1], nDlgUnavailableChbInitialPos[2], nDlgUnavailableChbInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_CHECKAUTOEXPAND),  nDlgAlwaysClonesChbInitialPos[0], nDlgAlwaysClonesChbInitialPos[1], nDlgAlwaysClonesChbInitialPos[2], nDlgAlwaysClonesChbInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_SHORTNAME),    nDlgZipnamesChbInitialPos[0], nDlgZipnamesChbInitialPos[1], nDlgZipnamesChbInitialPos[2], nDlgZipnamesChbInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_ASCIIONLY),    nDlgLatinTextChbInitialPos[0], nDlgLatinTextChbInitialPos[1], nDlgLatinTextChbInitialPos[2], nDlgLatinTextChbInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_SUBDIRS),      nDlgSearchSubDirsChbInitialPos[0], nDlgSearchSubDirsChbInitialPos[1], nDlgSearchSubDirsChbInitialPos[2], nDlgSearchSubDirsChbInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_DISABLECRC),   nDlgDisableCrcChbInitialPos[0], nDlgDisableCrcChbInitialPos[1], nDlgDisableCrcChbInitialPos[2], nDlgDisableCrcChbInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_APPLYIPS),     nDlgApplyIpsChbInitialPos[0], nDlgApplyIpsChbInitialPos[1], nDlgApplyIpsChbInitialPos[2], nDlgApplyIpsChbInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_CHECKAVAILABLE),   optCol1, optLine1,               optCol1W, 18, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_CHECKUNAVAILABLE), optCol1, optLine1 + optStep,     optCol1W, 18, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_CHECKAUTOEXPAND),  optCol1, optLine1 + optStep * 2, optCol1W, 18, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_SHORTNAME),    optCol1, optLine1 + optStep * 3, optCol1W, 18, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_ASCIIONLY),    optCol2, optLine1,               optCol2W, 18, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_SUBDIRS),      optCol2, optLine1 + optStep,     optCol2W, 18, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_DISABLECRC),   optCol2, optLine1 + optStep * 2, optCol2W, 18, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_APPLYIPS),     optCol2, optLine1 + optStep * 3, optCol2W, 18, TRUE);
	}
	ShowWindow(GetDlgItem(hDlg, IDC_SEL_APPLYIPS), SW_SHOW);

	if (bMergedRcShell && HasInitialRect(nDlgSelectGameGrpInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC1), nDlgSelectGameGrpInitialPos[0], nDlgSelectGameGrpInitialPos[1], nDlgSelectGameGrpInitialPos[2], nDlgSelectGameGrpInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC1), mainX, listY, mainW, listGroupBottom - listY, TRUE);
	}
	HWND hMainTree = GetDlgItem(hDlg, IDC_TREE1);
	if (bMergedRcShell && HasInitialRect(nDlgSelectGameLstInitialPos)) {
		MoveWindow(hMainTree, nDlgSelectGameLstInitialPos[0], nDlgSelectGameLstInitialPos[1], nDlgSelectGameLstInitialPos[2], nDlgSelectGameLstInitialPos[3], TRUE);
	} else {
		MoveWindow(hMainTree, listInnerX, listInnerY, listInnerW, listInnerH, TRUE);
	}
	if (hSelIpsTree) {
		if (bMergedRcShell && HasInitialRect(nDlgIpsTreeInitialPos)) {
			MoveWindow(hSelIpsTree, nDlgIpsTreeInitialPos[0], nDlgIpsTreeInitialPos[1], nDlgIpsTreeInitialPos[2], nDlgIpsTreeInitialPos[3], TRUE);
		} else {
			MoveWindow(hSelIpsTree, ipsListX, ipsListY, ipsListW, ipsListH, TRUE);
		}
	}
	if (bMergedRcShell && HasInitialRect(nDlgRomDataListInitialPos)) {
		MoveWindow(hSelRomDataList, nDlgRomDataListInitialPos[0], nDlgRomDataListInitialPos[1], nDlgRomDataListInitialPos[2], nDlgRomDataListInitialPos[3], TRUE);
	} else {
		MoveWindow(hSelRomDataList, romDataListX, romDataListY, romDataListW, romDataListH, TRUE);
	}

	if (bMergedRcShell && HasInitialRect(nDlgIpsGrpInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_IPSGROUP), nDlgIpsGrpInitialPos[0], nDlgIpsGrpInitialPos[1], nDlgIpsGrpInitialPos[2], nDlgIpsGrpInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_IPSGROUP), mainX, stripY, mainW, stripH, TRUE);
	}
	ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCHGROUP), SW_HIDE);

	EnsureSelectModeTitles(hDlg);
	HWND hDrvCount = GetDlgItem(hDlg, IDC_DRVCOUNT);
	if (hDrvCount) {
		const INT32 drvCountRight = mainX + mainW - 12;
		const INT32 drvCountLeft = mainX + (UsingBuiltinChineseLocalisation() ? 114 : 92);
		const INT32 drvCountW = SelectClamp(drvCountRight - drvCountLeft, 220, mainW - 24);

		LONG_PTR nStyle = GetWindowLongPtr(hDrvCount, GWL_STYLE);
		nStyle &= ~(SS_LEFT | SS_CENTER);
		nStyle |= SS_RIGHT;
		SetWindowLongPtr(hDrvCount, GWL_STYLE, nStyle);
		SetWindowPos(hDrvCount, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		if (bMergedRcShell && HasInitialRect(nDlgDrvCountTxtInitialPos)) {
			MoveWindow(hDrvCount, nDlgDrvCountTxtInitialPos[0], nDlgDrvCountTxtInitialPos[1], nDlgDrvCountTxtInitialPos[2], nDlgDrvCountTxtInitialPos[3], TRUE);
		} else {
			MoveWindow(hDrvCount, drvCountRight - drvCountW, listY + 10, drvCountW, 14, TRUE);
		}
	}

	INT32 titleX = mainX + 10;
	const INT32 titleY = stripY + 1;
	const INT32 titleGap = 8;
	const INT32 titleWidth[3] = { 84, 84, 84 };
	for (INT32 i = 0; i < 3; i++) {
		if (hSelModeTitle[i] == NULL || !IsWindow(hSelModeTitle[i])) continue;

		if (bMergedRcShell && i == 0 && HasInitialRect(nDlgModeRomBtnInitialPos)) {
			MoveWindow(hSelModeTitle[i], nDlgModeRomBtnInitialPos[0], nDlgModeRomBtnInitialPos[1], nDlgModeRomBtnInitialPos[2], nDlgModeRomBtnInitialPos[3], TRUE);
		} else if (bMergedRcShell && i == 1 && HasInitialRect(nDlgModeIpsBtnInitialPos)) {
			MoveWindow(hSelModeTitle[i], nDlgModeIpsBtnInitialPos[0], nDlgModeIpsBtnInitialPos[1], nDlgModeIpsBtnInitialPos[2], nDlgModeIpsBtnInitialPos[3], TRUE);
		} else if (bMergedRcShell && i == 2 && HasInitialRect(nDlgModeRomDataBtnInitialPos)) {
			MoveWindow(hSelModeTitle[i], nDlgModeRomDataBtnInitialPos[0], nDlgModeRomDataBtnInitialPos[1], nDlgModeRomDataBtnInitialPos[2], nDlgModeRomDataBtnInitialPos[3], TRUE);
		} else {
			MoveWindow(hSelModeTitle[i], titleX, titleY, titleWidth[i], 24, TRUE);
			titleX += titleWidth[i] + titleGap;
		}

		ShowWindow(hSelModeTitle[i], SW_SHOW);
	}
	if (bMergedRcShell && HasInitialRect(nDlgSearchTxtInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), nDlgSearchTxtInitialPos[0], nDlgSearchTxtInitialPos[1], nDlgSearchTxtInitialPos[2], nDlgSearchTxtInitialPos[3], TRUE);
	} else {
		const INT32 searchX = titleX;
		const INT32 searchRight = mainX + mainW - 10;
		const INT32 searchW = SelectClamp(searchRight - searchX, 140, mainW - 20);
		MoveWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), searchX, titleY, searchW, buttonH, TRUE);
	}
	ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), SW_SHOW);

	if (bMergedRcShell && HasInitialRect(nDlgPreviewGrpInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC2), nDlgPreviewGrpInitialPos[0], nDlgPreviewGrpInitialPos[1], nDlgPreviewGrpInitialPos[2], nDlgPreviewGrpInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC2), previewX, previewY, rightW, previewH, TRUE);
	}
	if (bMergedRcShell && HasInitialRect(nDlgPreviewImgHInitialPos) && HasInitialRect(nDlgPreviewImgVInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_SCREENSHOT_H), nDlgPreviewImgHInitialPos[0], nDlgPreviewImgHInitialPos[1], nDlgPreviewImgHInitialPos[2], nDlgPreviewImgHInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SCREENSHOT_V), nDlgPreviewImgVInitialPos[0], nDlgPreviewImgVInitialPos[1], nDlgPreviewImgVInitialPos[2], nDlgPreviewImgVInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_SCREENSHOT_H), previewX + previewInsetX, previewY + previewInsetY, previewInnerW, previewInnerH, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_SCREENSHOT_V), previewX + previewInsetX, previewY + previewInsetY, previewInnerW, previewInnerH, TRUE);
	}
	ShowWindow(GetDlgItem(hDlg, IDC_SCREENSHOT2_H), SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_SCREENSHOT2_V), SW_HIDE);

	if (bMergedRcShell && HasInitialRect(nDlgTitleGrpInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC3), nDlgTitleGrpInitialPos[0], nDlgTitleGrpInitialPos[1], nDlgTitleGrpInitialPos[2], nDlgTitleGrpInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC3), infoX, infoY, rightW, infoH, TRUE);
	}
	if (bMergedRcShell && HasInitialRect(nDlgWhiteBoxInitialPos) && HasInitialRect(nDlgIpsDescInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), nDlgWhiteBoxInitialPos[0], nDlgWhiteBoxInitialPos[1], nDlgWhiteBoxInitialPos[2], nDlgWhiteBoxInitialPos[3], TRUE);
		MoveWindow(hSelIpsDesc, nDlgIpsDescInitialPos[0], nDlgIpsDescInitialPos[1], nDlgIpsDescInitialPos[2], nDlgIpsDescInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), infoX + 8, infoY + 18, rightW - 16, infoH - 26, TRUE);
		MoveWindow(hSelIpsDesc, infoX + 12, infoY + 24, rightW - 24, infoH - 38, TRUE);
	}

	if (bMergedRcShell &&
		HasInitialRect(nDlgGameInfoLblInitialPos) &&
		HasInitialRect(nDlgGameInfoLblInitialPos) &&
		HasInitialRect(nDlgGameInfoTxtInitialPos) &&
		HasInitialRect(nDlgRomNameLblInitialPos) &&
		HasInitialRect(nDlgRomNameTxtInitialPos) &&
		HasInitialRect(nDlgRomInfoLblInitialPos) &&
		HasInitialRect(nDlgRomInfoTxtInitialPos) &&
		HasInitialRect(nDlgReleasedByLblInitialPos) &&
		HasInitialRect(nDlgReleasedByTxtInitialPos) &&
		HasInitialRect(nDlgGenreLblInitialPos) &&
		HasInitialRect(nDlgGenreTxtInitialPos) &&
		HasInitialRect(nDlgNotesLblInitialPos) &&
		HasInitialRect(nDlgNotesTxtInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDC_LABELCOMMENT), nDlgGameInfoLblInitialPos[0], nDlgGameInfoLblInitialPos[1], nDlgGameInfoLblInitialPos[2], nDlgGameInfoLblInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_LABELROMNAME), nDlgRomNameLblInitialPos[0], nDlgRomNameLblInitialPos[1], nDlgRomNameLblInitialPos[2], nDlgRomNameLblInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_LABELROMINFO), nDlgRomInfoLblInitialPos[0], nDlgRomInfoLblInitialPos[1], nDlgRomInfoLblInitialPos[2], nDlgRomInfoLblInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_LABELSYSTEM),  nDlgReleasedByLblInitialPos[0], nDlgReleasedByLblInitialPos[1], nDlgReleasedByLblInitialPos[2], nDlgReleasedByLblInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_LABELGENRE),   nDlgGenreLblInitialPos[0], nDlgGenreLblInitialPos[1], nDlgGenreLblInitialPos[2], nDlgGenreLblInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_LABELNOTES),   nDlgNotesLblInitialPos[0], nDlgNotesLblInitialPos[1], nDlgNotesLblInitialPos[2], nDlgNotesLblInitialPos[3], TRUE);

		MoveWindow(GetDlgItem(hDlg, IDC_TEXTCOMMENT), nDlgGameInfoTxtInitialPos[0], nDlgGameInfoTxtInitialPos[1], nDlgGameInfoTxtInitialPos[2], nDlgGameInfoTxtInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_TEXTROMNAME), nDlgRomNameTxtInitialPos[0], nDlgRomNameTxtInitialPos[1], nDlgRomNameTxtInitialPos[2], nDlgRomNameTxtInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_TEXTROMINFO), nDlgRomInfoTxtInitialPos[0], nDlgRomInfoTxtInitialPos[1], nDlgRomInfoTxtInitialPos[2], nDlgRomInfoTxtInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_TEXTSYSTEM),  nDlgReleasedByTxtInitialPos[0], nDlgReleasedByTxtInitialPos[1], nDlgReleasedByTxtInitialPos[2], nDlgReleasedByTxtInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_TEXTGENRE),   nDlgGenreTxtInitialPos[0], nDlgGenreTxtInitialPos[1], nDlgGenreTxtInitialPos[2], nDlgGenreTxtInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDC_TEXTNOTES),   nDlgNotesTxtInitialPos[0], nDlgNotesTxtInitialPos[1], nDlgNotesTxtInitialPos[2], nDlgNotesTxtInitialPos[3], TRUE);
	} else {
		LayoutSelectInfoFields(hDlg, infoX, infoY, rightW, infoH);
	}

	if (bMergedRcShell &&
		HasInitialRect(nDlgIpsLangInitialPos) &&
		HasInitialRect(nDlgIpsLangInitialPos) &&
		HasInitialRect(nDlgIpsClearInitialPos) &&
		HasInitialRect(nDlgRomDataSubdirInitialPos) &&
		HasInitialRect(nDlgRomDataDirBtnInitialPos) &&
		HasInitialRect(nDlgRomDataScanBtnInitialPos)) {
		MoveWindow(hSelIpsLang, nDlgIpsLangInitialPos[0], nDlgIpsLangInitialPos[1], nDlgIpsLangInitialPos[2], nDlgIpsLangInitialPos[3], TRUE);
		MoveWindow(hSelIpsClear, nDlgIpsClearInitialPos[0], nDlgIpsClearInitialPos[1], nDlgIpsClearInitialPos[2], nDlgIpsClearInitialPos[3], TRUE);
		MoveWindow(hSelRomDataSubdir, nDlgRomDataSubdirInitialPos[0], nDlgRomDataSubdirInitialPos[1], nDlgRomDataSubdirInitialPos[2], nDlgRomDataSubdirInitialPos[3], TRUE);
		MoveWindow(hSelRomDataDir, nDlgRomDataDirBtnInitialPos[0], nDlgRomDataDirBtnInitialPos[1], nDlgRomDataDirBtnInitialPos[2], nDlgRomDataDirBtnInitialPos[3], TRUE);
		MoveWindow(hSelRomDataScan, nDlgRomDataScanBtnInitialPos[0], nDlgRomDataScanBtnInitialPos[1], nDlgRomDataScanBtnInitialPos[2], nDlgRomDataScanBtnInitialPos[3], TRUE);
	} else {
		const INT32 ipsLangX = infoX + 12;
		const INT32 ipsLangW = SelectClamp(cancelBtnX - buttonGap - ipsLangX, 88, rightW - buttonW - 32);
		const INT32 romDataSubdirW = UsingBuiltinChineseLocalisation() ? 60 : 68;
		const INT32 romDataSubdirX = cancelBtnX - romDataSubdirW - 2;
		const INT32 romDataSubdirY = actionRowY + ((buttonH - 18) / 2);
		MoveWindow(hSelIpsLang, ipsLangX, actionRowY, ipsLangW, buttonH, TRUE);
		MoveWindow(hSelIpsClear, infoX + rightW - buttonW - 14, contentBottom - actionRowH, buttonW, buttonH, TRUE);
		MoveWindow(hSelRomDataSubdir, romDataSubdirX, romDataSubdirY, romDataSubdirW, 18, TRUE);
		MoveWindow(hSelRomDataDir, gameInfoBtnX, actionRowY, buttonW, buttonH, TRUE);
		MoveWindow(hSelRomDataScan, cancelBtnX, actionRowY, buttonW, buttonH, TRUE);
	}
	ShowWindow(GetDlgItem(hDlg, IDC_SEL_IPSMANAGER), SW_HIDE);
	EnableWindow(GetDlgItem(hDlg, IDC_SEL_IPSMANAGER), FALSE);
	if (bMergedRcShell &&
		HasInitialRect(nDlgDrvRomInfoBtnInitialPos) &&
		HasInitialRect(nDlgDrvRomInfoBtnInitialPos) &&
		HasInitialRect(nDlgCancelBtnInitialPos) &&
		HasInitialRect(nDlgPlayBtnInitialPos)) {
		MoveWindow(GetDlgItem(hDlg, IDGAMEINFO), nDlgDrvRomInfoBtnInitialPos[0], nDlgDrvRomInfoBtnInitialPos[1], nDlgDrvRomInfoBtnInitialPos[2], nDlgDrvRomInfoBtnInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDCANCEL),   nDlgCancelBtnInitialPos[0], nDlgCancelBtnInitialPos[1], nDlgCancelBtnInitialPos[2], nDlgCancelBtnInitialPos[3], TRUE);
		MoveWindow(GetDlgItem(hDlg, IDOK),       nDlgPlayBtnInitialPos[0], nDlgPlayBtnInitialPos[1], nDlgPlayBtnInitialPos[2], nDlgPlayBtnInitialPos[3], TRUE);
	} else {
		MoveWindow(GetDlgItem(hDlg, IDGAMEINFO), gameInfoBtnX, actionRowY, buttonW, buttonH, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDCANCEL),   cancelBtnX, actionRowY, buttonW, buttonH, TRUE);
		MoveWindow(GetDlgItem(hDlg, IDOK),       runBtnX, actionRowY, buttonW, buttonH, TRUE);
	}

	if (nSelActiveMode == 1) {
		const INT32 ipsLangDropH = buttonH + 160;
		const INT32 ipsLangItemH = SelectClamp(buttonH - 8, 14, 24);

		SetDialogItemTextMerged(hDlg, IDC_STATIC1, L"IPS\x8865\x4E01", _T("IPS Patches"));
		SetDialogItemTextMerged(hDlg, IDC_STATIC3, L"\x8865\x4E01\x4FE1\x606F", _T("Patch Info"));

		ShowWindow(hMainTree, SW_HIDE);
		ShowWindow(hSelRomDataList, SW_HIDE);
		if (hSelIpsTree) {
			ShowWindow(hSelIpsTree, SW_SHOW);
			SetWindowPos(hSelIpsTree, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			ForceHostedListFullRedraw(hSelIpsTree);
		}
		ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), SW_SHOW);
		ShowWindow(hSelIpsLang, SW_SHOW);
		ShowWindow(hSelIpsClear, SW_SHOW);
		ShowWindow(hSelRomDataSubdir, SW_HIDE);
		ShowWindow(hSelRomDataDir, SW_HIDE);
		ShowWindow(hSelRomDataScan, SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDGAMEINFO), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), SW_HIDE);
		ShowWindow(hSelIpsDesc, SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		if (!bMergedRcShell) {
			MoveWindow(hSelIpsLang, runBtnX, actionRowY, buttonW, ipsLangDropH, TRUE);
			SendMessage(hSelIpsLang, CB_SETITEMHEIGHT, (WPARAM)-1, ipsLangItemH);
			SendMessage(hSelIpsLang, CB_SETITEMHEIGHT, 0, ipsLangItemH);
			MoveWindow(hSelIpsClear, cancelBtnX, contentBottom - actionRowH, buttonW, buttonH, TRUE);
		}

		ShowRomInfoFields(hDlg, false, false);
	} else if (nSelActiveMode == 2) {
		const INT32 romDataSubdirW = UsingBuiltinChineseLocalisation() ? 60 : 68;
		const INT32 romDataSubdirX = cancelBtnX - romDataSubdirW - 2;
		const INT32 romDataSubdirY = actionRowY + ((buttonH - 18) / 2);
		SetDialogItemTextMerged(hDlg, IDC_STATIC1, L"RomData", _T("RomData"));
		SetDialogItemTextMerged(hDlg, IDC_STATIC3, L"RomData\x4FE1\x606F", _T("RomData Info"));
		SetDialogItemTextMerged(hDlg, IDC_LABELCOMMENT, L"\x8DEF\x5F84:", _T("Path:"));
		SetDialogItemTextMerged(hDlg, IDC_LABELROMNAME, L"\x9A71\x52A8:", _T("Driver:"));
		SetDialogItemTextMerged(hDlg, IDC_LABELROMINFO, L"\x786C\x4EF6:", _T("Hardware:"));
		SetDialogItemTextMerged(hDlg, IDC_ROMDATA_SUBDIR_CHECK, L"\x5B50\x76EE\x5F55", _T("Subdirs"));
		SetDialogItemTextMerged(hDlg, IDCANCEL, L"\x53D6\x6D88", _T("Cancel"));

		ShowWindow(hMainTree, SW_HIDE);
		if (hSelIpsTree) {
			ShowWindow(hSelIpsTree, SW_HIDE);
		}
		ShowWindow(hSelRomDataList, SW_SHOW);
		SetWindowPos(hSelRomDataList, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		if (!bSelModeSwitching) {
			ForceHostedListFullRedraw(hSelRomDataList);
		}
		ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), SW_SHOW);
		ShowWindow(hSelIpsLang, SW_HIDE);
		ShowWindow(hSelIpsClear, SW_HIDE);
		ShowWindow(hSelRomDataSubdir, SW_SHOW);
		ShowWindow(hSelRomDataDir, SW_SHOW);
		ShowWindow(hSelRomDataScan, SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDGAMEINFO), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), SW_SHOW);
		ShowWindow(hSelIpsDesc, SW_HIDE);
		if (!bMergedRcShell) {
			MoveWindow(hSelRomDataSubdir, romDataSubdirX, romDataSubdirY, romDataSubdirW, 18, TRUE);
			MoveWindow(hSelRomDataDir, cancelBtnX, contentBottom - actionRowH, buttonW, buttonH, TRUE);
			MoveWindow(hSelRomDataScan, runBtnX, contentBottom - actionRowH, buttonW, buttonH, TRUE);
		}
		ShowRomInfoFields(hDlg, false, true);
	} else {
		ApplyMergedSelectDialogCaptions(hDlg);
		ApplyRomInfoFieldCaptions(hDlg);
		if (hSelIpsTree) {
			ShowWindow(hSelIpsTree, SW_HIDE);
		}
		ShowWindow(hSelRomDataList, SW_HIDE);
		ShowWindow(hMainTree, SW_SHOW);
		SetWindowPos(hMainTree, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			InvalidateRect(hMainTree, NULL, FALSE);
			UpdateWindow(hMainTree);
		ShowWindow(GetDlgItem(hDlg, IDC_SEL_SEARCH), SW_SHOW);
		ShowWindow(hSelIpsLang, SW_HIDE);
		ShowWindow(hSelIpsClear, SW_HIDE);
		ShowWindow(hSelRomDataSubdir, SW_HIDE);
		ShowWindow(hSelRomDataDir, SW_HIDE);
		ShowWindow(hSelRomDataScan, SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDGAMEINFO), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDOK), SW_SHOW);
		EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
		ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_INFOBOX), SW_SHOW);
		ShowWindow(hSelIpsDesc, SW_HIDE);
		MoveWindow(GetDlgItem(hDlg, IDGAMEINFO), cancelBtnX, contentBottom - actionRowH, buttonW, buttonH, TRUE);
		ShowRomInfoFields(hDlg, true, false);
	}

	GetTitlePreviewScale();
}

static void EnterSelectRomMode(HWND hDlg, bool bSaveIps)
{
	if (bSelModeSwitching) {
		return;
	}
	bSelModeSwitching = true;
	KillTimer(hDlg, IDC_SEL_SEARCHTIMER);

	if (nSelActiveMode == 1) {
		if (hSelIpsTree && IsWindow(hSelIpsTree)) {
			TreeView_SelectItem(hSelIpsTree, NULL);
		}
		IpsHostedExit(bSaveIps ? 1 : 0);
	} else if (nSelActiveMode == 2) {
		ShowWindow(hSelRomDataList, SW_HIDE);
		RomDataHostedExit();
	}

	IpsSetHostedStorageName(NULL);
	nSelActiveMode = 0;
	nSelRememberedMode = 0;
	bSearchStringInit = true;
	SetDlgItemText(hDlg, IDC_SEL_SEARCH, szSearchString);
	LayoutMergedSelectDialog(hDlg);
	RestoreCurrentDriverInRomList(hDlg);
	UpdateSelectModeAvailability(hDlg);

	if (bDrvSelected) {
		nBurnDrvActive = nDialogSelect;
		UpdatePreview(true, szAppPreviewsPath, IDC_SCREENSHOT_H, IDC_SCREENSHOT_V);
	}

	ShowWindow(hSelList, SW_SHOW);
	SetWindowPos(hSelList, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	InvalidateRect(hSelList, NULL, FALSE);
	UpdateWindow(hSelList);
	SetFocus(hSelList);
	bSelModeSwitching = false;
}

static void EnterSelectIpsMode(HWND hDlg)
{
	if (bSelModeSwitching) {
		return;
	}
	if (!SelectIpsAvailable()) {
		return;
	}
	bSelModeSwitching = true;
	KillTimer(hDlg, IDC_SEL_SEARCHTIMER);

	EnsureSelectIpsHostedControls(hDlg);
	if (nSelActiveMode == 2) {
		SyncIpsHostedStorageNameFromSelection();
	} else {
		IpsSetHostedStorageName(NULL);
	}
	if (nSelActiveMode == 2 && nBurnDrvActive >= 0) {
		nDialogSelect = nBurnDrvActive;
	} else {
		nBurnDrvActive = nDialogSelect;
	}
	nSelActiveMode = 1;
	nSelRememberedMode = 1;
	LayoutMergedSelectDialog(hDlg);
	IpsHostedInit(hDlg, hSelIpsTree, GetDlgItem(hDlg, IDC_SCREENSHOT_H), hSelIpsDesc, hSelIpsLang);
	bSearchStringInit = true;
	SetDlgItemText(hDlg, IDC_SEL_SEARCH, szIpsSearchString);
	IpsHostedSetSearchText(szIpsSearchString);
	SetWindowPos(hSelIpsTree, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	UpdateSelectModeAvailability(hDlg);
	ForceHostedListFullRedraw(hSelIpsTree);
	SetFocus(hSelIpsTree);
	bSelModeSwitching = false;
}

static void EnterSelectRomDataMode(HWND hDlg)
{
	if (bSelModeSwitching) {
		return;
	}
	bSelModeSwitching = true;
	KillTimer(hDlg, IDC_SEL_SEARCHTIMER);

	EnsureSelectRomDataHostedControls(hDlg);
	nSelActiveMode = 2;
	nSelRememberedMode = 2;
	bSearchStringInit = true;
	SetDlgItemText(hDlg, IDC_SEL_SEARCH, szRomDataSearchString);
	LayoutMergedSelectDialog(hDlg);
	CheckDlgButton(hDlg, IDC_ROMDATA_SUBDIR_CHECK, bRDListScanSub ? BST_CHECKED : BST_UNCHECKED);
	RomDataHostedSetScanSubdirs(bRDListScanSub ? 1 : 0);
	RomDataHostedInit(hDlg, hSelRomDataList, IDC_SCREENSHOT_H, IDC_STATIC2, IDC_TEXTCOMMENT, IDC_TEXTROMNAME, IDC_TEXTROMINFO);
	RomDataHostedSetSearchText(szRomDataSearchString);
	UpdateSelectModeAvailability(hDlg);
	SetFocus(hSelRomDataList);
	bSelModeSwitching = false;
}

static bool SelIsSearchSeparator(TCHAR c)
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

static void SelNormalizeSearchText(const TCHAR* pszSrc, TCHAR* pszDst, size_t nDstCount)
{
	if (pszDst == NULL || nDstCount == 0) return;

	pszDst[0] = _T('\0');
	if (pszSrc == NULL) return;

	size_t j = 0;
	for (size_t i = 0; pszSrc[i] && j + 1 < nDstCount; i++) {
		TCHAR c = _totlower(pszSrc[i]);
		if (SelIsSearchSeparator(c)) continue;

		pszDst[j++] = c;
	}

	pszDst[j] = _T('\0');
}

static TCHAR SelGetPinyinInitial(TCHAR c)
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

static void SelBuildInitialsSearchText(const TCHAR* pszSrc, TCHAR* pszDst, size_t nDstCount)
{
	if (pszDst == NULL || nDstCount == 0) return;

	pszDst[0] = _T('\0');
	if (pszSrc == NULL) return;

	size_t j = 0;
	for (size_t i = 0; pszSrc[i] && j + 1 < nDstCount; i++) {
		TCHAR c = _totlower(pszSrc[i]);
		if (SelIsSearchSeparator(c)) continue;

		if (c < 0x80) {
			if (_istalnum(c)) {
				pszDst[j++] = c;
			}
			continue;
		}

		TCHAR initial = SelGetPinyinInitial(c);
		if (initial) {
			pszDst[j++] = initial;
			continue;
		}

		pszDst[j++] = c;
	}

	pszDst[j] = _T('\0');
}

static bool SelStringMatchesSearch(const TCHAR* pszValue)
{
	if (szSearchStringNormalized[0] == _T('\0')) return true;
	if (pszValue == NULL || pszValue[0] == _T('\0')) return false;

	TCHAR szNormalized[2048] = { 0 };
	TCHAR szInitials[2048] = { 0 };

	SelNormalizeSearchText(pszValue, szNormalized, _countof(szNormalized));
	if (_tcsstr(szNormalized, szSearchStringNormalized)) return true;

	SelBuildInitialsSearchText(pszValue, szInitials, _countof(szInitials));
	return _tcsstr(szInitials, szSearchStringNormalized) != NULL;
}

static bool SelCurrentDriverMatchesSearch()
{
	if (szSearchStringNormalized[0] == _T('\0')) return true;

	TCHAR szDriverNameA[256] = { _T("") };
	TCHAR szManufacturerName[256] = { _T("") };
	const TCHAR* pszManufacturer = BurnDrvGetText(DRV_MANUFACTURER);
	const TCHAR* pszSystem = BurnDrvGetText(DRV_SYSTEM);
	const char* pszFullNameA = BurnDrvGetTextA(DRV_FULLNAME);

	if (pszFullNameA) {
		_sntprintf(szDriverNameA, _countof(szDriverNameA), _T("%S"), pszFullNameA);
	}

	_sntprintf(szManufacturerName, _countof(szManufacturerName), _T("%s %s"),
		pszManufacturer ? pszManufacturer : _T(""),
		pszSystem ? pszSystem : _T(""));

	return SelStringMatchesSearch(BurnDrvGetText(DRV_FULLNAME))
		|| SelStringMatchesSearch(szDriverNameA)
		|| SelStringMatchesSearch(BurnDrvGetText(DRV_NAME))
		|| SelStringMatchesSearch(szManufacturerName);
}

// Filter TreeView
HWND hFilterList					= NULL;
HTREEITEM hFilterCapcomMisc			= NULL;
HTREEITEM hFilterCave				= NULL;
HTREEITEM hFilterCps1				= NULL;
HTREEITEM hFilterCps2				= NULL;
HTREEITEM hFilterCps3				= NULL;
HTREEITEM hFilterDataeast			= NULL;
HTREEITEM hFilterGalaxian			= NULL;
HTREEITEM hFilterIrem				= NULL;
HTREEITEM hFilterKaneko				= NULL;
HTREEITEM hFilterKonami				= NULL;
HTREEITEM hFilterNeogeo				= NULL;
HTREEITEM hFilterPacman				= NULL;
HTREEITEM hFilterPgm				= NULL;
HTREEITEM hFilterPgm2				= NULL;
HTREEITEM hFilterPsikyo				= NULL;
HTREEITEM hFilterSega				= NULL;
HTREEITEM hFilterSeta				= NULL;
HTREEITEM hFilterTaito				= NULL;
HTREEITEM hFilterTechnos			= NULL;
HTREEITEM hFilterToaplan			= NULL;
HTREEITEM hFilterMiscPre90s			= NULL;
HTREEITEM hFilterMiscPost90s		= NULL;
HTREEITEM hFilterAtomiswave			= NULL;
HTREEITEM hFilterNaomi				= NULL;
HTREEITEM hFilterMegadrive			= NULL;
HTREEITEM hFilterPce				= NULL;
HTREEITEM hFilterMsx				= NULL;
HTREEITEM hFilterNes				= NULL;
HTREEITEM hFilterFds				= NULL;
HTREEITEM hFilterSnes				= NULL;
HTREEITEM hFilterNgp				= NULL;
HTREEITEM hFilterChannelF			= NULL;
HTREEITEM hFilterSms				= NULL;
HTREEITEM hFilterGg					= NULL;
HTREEITEM hFilterSg1000				= NULL;
HTREEITEM hFilterColeco				= NULL;
HTREEITEM hFilterSpectrum			= NULL;
HTREEITEM hFilterMidway				= NULL;
HTREEITEM hFilterBootleg			= NULL;
HTREEITEM hFilterDemo				= NULL;
HTREEITEM hFilterHack				= NULL;
HTREEITEM hFilterHomebrew			= NULL;
HTREEITEM hFilterPrototype			= NULL;
HTREEITEM hFilterGenuine			= NULL;
HTREEITEM hFilterHorshoot			= NULL;
HTREEITEM hFilterVershoot			= NULL;
HTREEITEM hFilterScrfight			= NULL;
HTREEITEM hFilterVsfight			= NULL;
HTREEITEM hFilterBios				= NULL;
HTREEITEM hFilterBreakout			= NULL;
HTREEITEM hFilterCasino				= NULL;
HTREEITEM hFilterBallpaddle			= NULL;
HTREEITEM hFilterMaze				= NULL;
HTREEITEM hFilterMinigames			= NULL;
HTREEITEM hFilterPinball			= NULL;
HTREEITEM hFilterPlatform			= NULL;
HTREEITEM hFilterPuzzle				= NULL;
HTREEITEM hFilterQuiz				= NULL;
HTREEITEM hFilterSportsmisc			= NULL;
HTREEITEM hFilterSportsfootball 	= NULL;
HTREEITEM hFilterMisc				= NULL;
HTREEITEM hFilterMahjong			= NULL;
HTREEITEM hFilterRacing				= NULL;
HTREEITEM hFilterShoot				= NULL;
HTREEITEM hFilterMultiShoot			= NULL;
HTREEITEM hFilterRungun 			= NULL;
HTREEITEM hFilterAction				= NULL;
HTREEITEM hFilterStrategy   		= NULL;
HTREEITEM hFilterRpg        		= NULL;
HTREEITEM hFilterSim        		= NULL;
HTREEITEM hFilterAdv        		= NULL;
HTREEITEM hFilterCard        		= NULL;
HTREEITEM hFilterBoard        		= NULL;
HTREEITEM hFilterOtherFamily		= NULL;
HTREEITEM hFilterMslug				= NULL;
HTREEITEM hFilterSf					= NULL;
HTREEITEM hFilterKof				= NULL;
HTREEITEM hFilterDstlk				= NULL;
HTREEITEM hFilterFatfury			= NULL;
HTREEITEM hFilterSamsho				= NULL;
HTREEITEM hFilter19xx				= NULL;
HTREEITEM hFilterSonicwi			= NULL;
HTREEITEM hFilterPwrinst			= NULL;
HTREEITEM hFilterSonic				= NULL;
HTREEITEM hFilterDonpachi			= NULL;
HTREEITEM hFilterMahou				= NULL;
HTREEITEM hFilterIgs				= NULL;
HTREEITEM hFilterSkeleton			= NULL;
HTREEITEM hFilterGba				= NULL;
HTREEITEM hFilterCapcomGrp			= NULL;
HTREEITEM hFilterSegaGrp			= NULL;

HTREEITEM hRoot						= NULL;
HTREEITEM hBoardType				= NULL;
HTREEITEM hFamily					= NULL;
HTREEITEM hGenre					= NULL;
HTREEITEM hFavorites				= NULL;
HTREEITEM hHardware					= NULL;

// GCC doesn't seem to define these correctly.....
#define _TreeView_SetItemState(hwndTV, hti, data, _mask) \
{ TVITEM _ms_TVi;\
  _ms_TVi.mask = TVIF_STATE; \
  _ms_TVi.hItem = hti; \
  _ms_TVi.stateMask = _mask;\
  _ms_TVi.state = data;\
  SNDMSG((hwndTV), TVM_SETITEM, 0, (LPARAM)(TV_ITEM *)&_ms_TVi);\
}

#define _TreeView_SetCheckState(hwndTV, hti, fCheck) \
  _TreeView_SetItemState(hwndTV, hti, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), TVIS_STATEIMAGEMASK)

// -----------------------------------------------------------------------------------------------------------------

#define DISABLE_NON_AVAILABLE_SELECT	0						// Disable selecting non-available sets
#define NON_WORKING_PROMPT_ON_LOAD		1						// Prompt user on loading non-working sets

static UINT64 CapcomMiscValue		= HARDWARE_PREFIX_CAPCOM_MISC >> 24;
static UINT64 MASKCAPMISC			= 1 << CapcomMiscValue;
static UINT64 CaveValue				= HARDWARE_PREFIX_CAVE >> 24;
static UINT64 MASKCAVE				= 1 << CaveValue;
static UINT64 CpsValue				= HARDWARE_PREFIX_CAPCOM >> 24;
static UINT64 MASKCPS				= 1 << CpsValue;
static UINT64 Cps2Value				= HARDWARE_PREFIX_CPS2 >> 24;
static UINT64 MASKCPS2				= 1 << Cps2Value;
static UINT64 Cps3Value				= HARDWARE_PREFIX_CPS3 >> 24;
static UINT64 MASKCPS3				= 1 << Cps3Value;
static UINT64 DataeastValue			= HARDWARE_PREFIX_DATAEAST >> 24;
static UINT64 MASKDATAEAST			= 1 << DataeastValue;
static UINT64 GalaxianValue			= HARDWARE_PREFIX_GALAXIAN >> 24;
static UINT64 MASKGALAXIAN			= 1 << GalaxianValue;
static UINT64 IremValue				= HARDWARE_PREFIX_IREM >> 24;
static UINT64 MASKIREM				= 1 << IremValue;
static UINT64 KanekoValue			= HARDWARE_PREFIX_KANEKO >> 24;
static UINT64 MASKKANEKO			= 1 << KanekoValue;
static UINT64 KonamiValue			= HARDWARE_PREFIX_KONAMI >> 24;
static UINT64 MASKKONAMI			= 1 << KonamiValue;
static UINT64 NeogeoValue			= HARDWARE_PREFIX_SNK >> 24;
static UINT64 MASKNEOGEO			= 1 << NeogeoValue;
static UINT64 PacmanValue			= HARDWARE_PREFIX_PACMAN >> 24;
static UINT64 MASKPACMAN			= 1 << PacmanValue;
static UINT64 PgmValue				= HARDWARE_PREFIX_IGS_PGM >> 24;
static UINT64 MASKPGM				= 1 << PgmValue;
static UINT64 Pgm2Value				= (UINT64)HARDWARE_PREFIX_IGS_PGM2 >> 24;
static UINT64 MASKPGM2				= (UINT64)1 << Pgm2Value;
static UINT64 PsikyoValue			= HARDWARE_PREFIX_PSIKYO >> 24;
static UINT64 MASKPSIKYO			= 1 << PsikyoValue;
static UINT64 SegaValue				= HARDWARE_PREFIX_SEGA >> 24;
static UINT64 MASKSEGA				= 1 << SegaValue;
static UINT64 SetaValue				= HARDWARE_PREFIX_SETA >> 24;
static UINT64 MASKSETA				= 1 << SetaValue;
static UINT64 TaitoValue			= HARDWARE_PREFIX_TAITO >> 24;
static UINT64 MASKTAITO				= 1 << TaitoValue;
static UINT64 TechnosValue			= HARDWARE_PREFIX_TECHNOS >> 24;
static UINT64 MASKTECHNOS			= 1 << TechnosValue;
static UINT64 ToaplanValue			= HARDWARE_PREFIX_TOAPLAN >> 24;
static UINT64 MASKTOAPLAN			= 1 << ToaplanValue;
static UINT64 MiscPre90sValue		= HARDWARE_PREFIX_MISC_PRE90S >> 24;
static UINT64 MASKMISCPRE90S		= 1 << MiscPre90sValue;
static UINT64 MiscPost90sValue		= HARDWARE_PREFIX_MISC_POST90S >> 24;
static UINT64 MASKMISCPOST90S		= 1 << MiscPost90sValue;
static UINT64 MASKATOMISWAVE		= (UINT64)1 << 60;
static UINT64 MASKNAOMI				= (UINT64)1 << 61;
static UINT64 MegadriveValue		= HARDWARE_PREFIX_SEGA_MEGADRIVE >> 24;
static UINT64 MASKMEGADRIVE			= 1 << MegadriveValue;
static UINT64 PCEngineValue			= HARDWARE_PREFIX_PCENGINE >> 24;
static UINT64 MASKPCENGINE			= 1 << PCEngineValue;
static UINT64 SmsValue				= HARDWARE_PREFIX_SEGA_MASTER_SYSTEM >> 24;
static UINT64 MASKSMS				= 1 << SmsValue;
static UINT64 GgValue				= HARDWARE_PREFIX_SEGA_GAME_GEAR >> 24;
static UINT64 MASKGG				= 1 << GgValue;
static UINT64 Sg1000Value			= HARDWARE_PREFIX_SEGA_SG1000 >> 24;
static UINT64 MASKSG1000			= 1 << Sg1000Value;
static UINT64 ColecoValue			= HARDWARE_PREFIX_COLECO >> 24;
static UINT64 MASKCOLECO			= 1 << ColecoValue;
static UINT64 MsxValue				= HARDWARE_PREFIX_MSX >> 24;
static UINT64 MASKMSX				= 1 << MsxValue;
static UINT64 SpecValue				= HARDWARE_PREFIX_SPECTRUM >> 24;
static UINT64 MASKSPECTRUM			= 1 << SpecValue;
static UINT64 MidwayValue			= HARDWARE_PREFIX_MIDWAY >> 24;
static UINT64 MASKMIDWAY			= 1 << MidwayValue;
static UINT64 NesValue				= HARDWARE_PREFIX_NES >> 24;
static UINT64 MASKNES				= 1 << NesValue;

// this is where things start going above the 32bit-zone. *solved w/64bit UINT?*
static UINT64 FdsValue				= (UINT64)HARDWARE_PREFIX_FDS >> 24;
static UINT64 MASKFDS				= (UINT64)1 << FdsValue;            // 1 << 0x1f - needs casting or.. bonkers!
static UINT64 SnesValue				= (UINT64)HARDWARE_PREFIX_SNES >> 24;
static UINT64 MASKSNES				= (UINT64)1 << SnesValue;
static UINT64 NgpValue				= (UINT64)HARDWARE_PREFIX_NGP >> 24;
static UINT64 MASKNGP				= (UINT64)1 << NgpValue;
static UINT64 ChannelFValue			= (UINT64)HARDWARE_PREFIX_CHANNELF >> 24;
static UINT64 MASKCHANNELF			= (UINT64)1 << ChannelFValue;
static UINT64 IgsValue				= (UINT64)HARDWARE_PREFIX_IGS >> 24;
static UINT64 MASKIGS				= (UINT64)1 << IgsValue;
static UINT64 SkeletonValue			= (UINT64)HARDWARE_PREFIX_SKELETON >> 24;
static UINT64 MASKSKELETON			= (UINT64)1 << SkeletonValue;
static UINT64 GbaValue				= (UINT64)HARDWARE_PREFIX_GBA >> 24;
static UINT64 MASKGBA				= (UINT64)1 << GbaValue;
static UINT64 GbValue				= (UINT64)HARDWARE_PREFIX_GB >> 24;
static UINT64 MASKGB				= (UINT64)1 << GbValue;
static UINT64 GbcValue				= (UINT64)HARDWARE_PREFIX_GBC >> 24;
static UINT64 MASKGBC				= (UINT64)1 << GbcValue;

static UINT64 MASKALL				= ((UINT64)MASKCAPMISC | MASKCAVE | MASKCPS | MASKCPS2 | MASKCPS3 | MASKDATAEAST | MASKGALAXIAN | MASKIREM | MASKKANEKO | MASKKONAMI | MASKNEOGEO | MASKPACMAN | MASKPGM | MASKPGM2 | MASKIGS | MASKSKELETON | MASKGBA | MASKGB | MASKGBC | MASKPSIKYO | MASKSEGA | MASKSETA | MASKTAITO | MASKTECHNOS | MASKTOAPLAN | MASKMISCPRE90S | MASKMISCPOST90S | MASKATOMISWAVE | MASKNAOMI | MASKMEGADRIVE | MASKPCENGINE | MASKSMS | MASKGG | MASKSG1000 | MASKCOLECO | MASKMSX | MASKSPECTRUM | MASKMIDWAY | MASKNES | MASKFDS | MASKSNES | MASKNGP | MASKCHANNELF );

#define SEARCHSUBDIRS			(1 << 26)
#define UNAVAILABLE				(1 << 27)
#define AVAILABLE				(1 << 28)
#define AUTOEXPAND				(1 << 29)
#define SHOWSHORT				(1 << 30)
#define ASCIIONLY				(1 << 31)

#define MASKBOARDTYPEGENUINE	(1)
#define MASKFAMILYOTHER			0x10000000

#define MASKALLGENRE			(GBF_HORSHOOT | GBF_VERSHOOT | GBF_SCRFIGHT | GBF_VSFIGHT | GBF_BIOS | GBF_BREAKOUT | GBF_CASINO | GBF_BALLPADDLE | GBF_MAZE | GBF_MINIGAMES | GBF_PINBALL | GBF_PLATFORM | GBF_PUZZLE | GBF_QUIZ | GBF_SPORTSMISC | GBF_SPORTSFOOTBALL | GBF_MISC | GBF_MAHJONG | GBF_RACING | GBF_SHOOT | GBF_MULTISHOOT | GBF_ACTION | GBF_RUNGUN | GBF_STRATEGY | GBF_RPG | GBF_SIM | GBF_ADV | GBF_CARD | GBF_BOARD)
#define MASKALLFAMILY			(MASKFAMILYOTHER | FBF_MSLUG | FBF_SF | FBF_KOF | FBF_DSTLK | FBF_FATFURY | FBF_SAMSHO | FBF_19XX | FBF_SONICWI | FBF_PWRINST | FBF_SONIC | FBF_DONPACHI | FBF_MAHOU)
#define MASKALLBOARD			(MASKBOARDTYPEGENUINE | BDF_BOOTLEG | BDF_DEMO | BDF_HACK | BDF_HOMEBREW | BDF_PROTOTYPE)

#define MASKCAPGRP				(MASKCAPMISC | MASKCPS | MASKCPS2 | MASKCPS3)
#define MASKSEGAGRP				(MASKSEGA | MASKSG1000 | MASKSMS | MASKMEGADRIVE | MASKGG)

static bool SelDriverIsSystem(const TCHAR* pszExpectedSystem)
{
	const TCHAR* pszSystem = BurnDrvGetText(DRV_SYSTEM);
	return pszSystem && !_tcscmp(pszSystem, pszExpectedSystem);
}

static UINT64 SelGetCurrentDriverHardwareMask()
{
	if (SelDriverIsSystem(_T("Sega Atomiswave"))) {
		return MASKATOMISWAVE;
	}

	if (SelDriverIsSystem(_T("Sega Naomi"))) {
		return MASKNAOMI;
	}

	return (UINT64)1 << (((UINT32)BurnDrvGetHardwareCode() >> 24) & 0x3f);
}

UINT64 nLoadMenuShowX			= 0; // hardware etc
int nLoadMenuShowY				= AVAILABLE; // selector options, default to show available
int nLoadMenuBoardTypeFilter	= 0;
int nLoadMenuGenreFilter		= 0;
int nLoadMenuFavoritesFilter	= 0;
int nLoadMenuFamilyFilter		= 0;

// expanded/collapsed state of filter nodes. default: expand main (0x01) & hardware (0x10)
int nLoadMenuExpand             = 0x10 | 0x01;

struct NODEINFO {
	int nBurnDrvNo;
	bool bIsParent;
	char* pszROMName;
	HTREEITEM hTreeHandle;
};

static NODEINFO* nBurnDrv = NULL;
static NODEINFO* nBurnZipListDrv = NULL;
static unsigned int nTmpDrvCount;

// prototype  -----------------------
static void RebuildEverything();
static void ReselectCurrentDriverInList();
static void UpdateRomSelectionDetails(HWND hDlg);
static bool RestoreCurrentDriverInRomList(HWND hDlg);
static void ForceHostedListFullRedraw(HWND hWnd);
// ----------------------------------

static void ReselectCurrentDriverInList()
{
	if (hSelList == NULL || nDialogSelect < 0) {
		return;
	}

	for (unsigned int i = 0; i < nTmpDrvCount; i++) {
		if (nBurnDrv[i].nBurnDrvNo == nDialogSelect) {
			nBurnDrvActive = nBurnDrv[i].nBurnDrvNo;
			TreeView_EnsureVisible(hSelList, nBurnDrv[i].hTreeHandle);
			TreeView_Select(hSelList, nBurnDrv[i].hTreeHandle, TVGN_CARET);
			TreeView_SelectSetFirstVisible(hSelList, nBurnDrv[i].hTreeHandle);
			break;
		}
	}
}

static void UpdateRomSelectionDetails(HWND hDlg)
{
	if (hDlg == NULL || nBurnDrvActive < 0 || nBurnDrvActive >= nBurnDrvCount) {
		return;
	}

	bDrvSelected = true;
	UpdatePreview(true, szAppPreviewsPath, IDC_SCREENSHOT_H, IDC_SCREENSHOT_V);

	CheckDlgButton(hDlg, IDC_SEL_APPLYIPS, BST_UNCHECKED);
	UpdateSelectModeAvailability(hDlg);

	// Get the text from the drivers via BurnDrvGetText()
	for (int i = 0; i < 6; i++) {
		int nGetTextFlags = nLoadMenuShowY & ASCIIONLY ? DRV_ASCIIONLY : 0;
		TCHAR szItemText[256];
		szItemText[0] = _T('\0');

		switch (i) {
			case 0: {
				bool bBracket = false;

				SelCopyString(szItemText, _countof(szItemText), BurnDrvGetText(DRV_NAME));
				if ((BurnDrvGetFlags() & BDF_CLONE) && BurnDrvGetTextA(DRV_PARENT)) {
					int nOldDrvSelect = nBurnDrvActive;
					TCHAR* pszName = BurnDrvGetText(DRV_PARENT);

					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_CLONE_OF, true), BurnDrvGetText(DRV_PARENT));

					for (nBurnDrvActive = 0; nBurnDrvActive < nBurnDrvCount; nBurnDrvActive++) {
						if (!_tcsicmp(pszName, BurnDrvGetText(DRV_NAME))) {
							break;
						}
					}
					if (nBurnDrvActive < nBurnDrvCount) {
						if (BurnDrvGetText(DRV_PARENT)) {
							_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_ROMS_FROM_1, true), BurnDrvGetText(DRV_PARENT));
						}
					}
					nBurnDrvActive = nOldDrvSelect;
					bBracket = true;
				} else {
					if (BurnDrvGetTextA(DRV_PARENT)) {
						_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_ROMS_FROM_2, true), bBracket ? _T(", ") : _T(" ("), BurnDrvGetText(DRV_PARENT));
						bBracket = true;
					}
				}
				if (BurnDrvGetTextA(DRV_SAMPLENAME)) {
					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_SAMPLES_FROM, true), bBracket ? _T(", ") : _T(" ("), BurnDrvGetText(DRV_SAMPLENAME));
					bBracket = true;
				}
				if (bBracket) {
					SelAppendString(szItemText, _countof(szItemText), _T(")"));
				}
				SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
				EnableWindow(hInfoLabel[i], TRUE);
				break;
			}
			case 1: {
				bool bUseInfo = false;

				if (BurnDrvGetFlags() & BDF_PROTOTYPE) {
					SelAppendString(szItemText, _countof(szItemText), FBALoadStringEx(hAppInst, IDS_SEL_PROTOTYPE, true));
					bUseInfo = true;
				}
				if (BurnDrvGetFlags() & BDF_BOOTLEG) {
					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_SEL_BOOTLEG, true), bUseInfo ? _T(", ") : _T(""));
					bUseInfo = true;
				}
				if (BurnDrvGetFlags() & BDF_HACK) {
					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_SEL_HACK, true), bUseInfo ? _T(", ") : _T(""));
					bUseInfo = true;
				}
				if (BurnDrvGetFlags() & BDF_HOMEBREW) {
					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_SEL_HOMEBREW, true), bUseInfo ? _T(", ") : _T(""));
					bUseInfo = true;
				}
				if (BurnDrvGetFlags() & BDF_DEMO) {
					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_SEL_DEMO, true), bUseInfo ? _T(", ") : _T(""));
					bUseInfo = true;
				}
				TCHAR szPlayersMax[100];
				SelCopyString(szPlayersMax, _countof(szPlayersMax), FBALoadStringEx(hAppInst, IDS_NUM_PLAYERS_MAX, true));
				_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_NUM_PLAYERS, true), bUseInfo ? _T(", ") : _T(""), BurnDrvGetMaxPlayers(), (BurnDrvGetMaxPlayers() != 1) ? szPlayersMax : _T(""));
				bUseInfo = true;
				if (BurnDrvGetText(DRV_BOARDROM)) {
					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_BOARD_ROMS_FROM, true), bUseInfo ? _T(", ") : _T(""), BurnDrvGetText(DRV_BOARDROM));
					SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
					EnableWindow(hInfoLabel[i], TRUE);
					bUseInfo = true;
				}
				SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
				EnableWindow(hInfoLabel[i], bUseInfo);
				break;
			}
			case 2: {
				TCHAR szUnknown[100];
				TCHAR szCartridge[100];
				SelCopyString(szUnknown, _countof(szUnknown), FBALoadStringEx(hAppInst, IDS_ERR_UNKNOWN, true));
				SelCopyString(szCartridge, _countof(szCartridge), FBALoadStringEx(hAppInst, IDS_MVS_CARTRIDGE, true));
				_stprintf(szItemText, FBALoadStringEx(hAppInst, IDS_HARDWARE_DESC, true), BurnDrvGetTextA(DRV_MANUFACTURER) ? BurnDrvGetText(nGetTextFlags | DRV_MANUFACTURER) : szUnknown, BurnDrvGetText(DRV_DATE), (((BurnDrvGetHardwareCode() & HARDWARE_SNK_MVS) == HARDWARE_SNK_MVS) && ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK)) == HARDWARE_SNK_NEOGEO)? szCartridge : BurnDrvGetText(nGetTextFlags | DRV_SYSTEM));
				SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
				EnableWindow(hInfoLabel[i], TRUE);
				break;
			}
			case 3: {
				TCHAR szText[1024] = _T("");
				TCHAR* pszPosition = szText;
				TCHAR* pszName = BurnDrvGetText(nGetTextFlags | DRV_FULLNAME);

				SelCopyString(szText, _countof(szText), pszName);
				pszPosition = szText + _tcslen(szText);

				pszName = BurnDrvGetText(nGetTextFlags | DRV_FULLNAME);
				while ((pszName = BurnDrvGetText(nGetTextFlags | DRV_NEXTNAME | DRV_FULLNAME)) != NULL) {
					if (pszPosition + _tcslen(pszName) - 1024 > szText) {
						break;
					}
					pszPosition += _stprintf(pszPosition, _T(SEPERATOR_2) _T("%s"), pszName);
				}
				SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)szText);
				if (szText[0]) {
					EnableWindow(hInfoLabel[i], TRUE);
				} else {
					EnableWindow(hInfoLabel[i], FALSE);
				}
				break;
			}
			case 4: {
				SelCopyString(szItemText, _countof(szItemText), BurnDrvGetTextA(DRV_COMMENT) ? BurnDrvGetText(nGetTextFlags | DRV_COMMENT) : _T(""));
				if (BurnDrvGetFlags() & BDF_HISCORE_SUPPORTED) {
					_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_HISCORES_SUPPORTED, true), _tcslen(szItemText) ? _T(", ") : _T(""));
				}
				SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
				EnableWindow(hInfoLabel[i], TRUE);
				break;
			}

			case 5: {
				SelCopyString(szItemText, _countof(szItemText), DecorateGenreInfo());
				SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
				EnableWindow(hInfoLabel[i], TRUE);
				break;
			}
		}
	}
}

static bool RestoreCurrentDriverInRomList(HWND hDlg)
{
	if (hDlg == NULL || hSelList == NULL || nDialogSelect < 0) {
		return false;
	}

	for (unsigned int i = 0; i < nTmpDrvCount; i++) {
		if (nBurnDrv[i].nBurnDrvNo == nDialogSelect && nBurnDrv[i].hTreeHandle != NULL) {
			nBurnDrvActive = nBurnDrv[i].nBurnDrvNo;

			char nOldTreeBuilding = TreeBuilding;
			TreeBuilding = 1;
			TreeView_EnsureVisible(hSelList, nBurnDrv[i].hTreeHandle);
			TreeView_SelectItem(hSelList, nBurnDrv[i].hTreeHandle);
			TreeView_SelectSetFirstVisible(hSelList, nBurnDrv[i].hTreeHandle);
			TreeBuilding = nOldTreeBuilding;

			UpdateRomSelectionDetails(hDlg);
			RedrawWindow(hSelList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			return true;
		}
	}

	return false;
}

static void ForceHostedListFullRedraw(HWND hWnd)
{
	if (hWnd == NULL || !IsWindow(hWnd)) {
		return;
	}

	InvalidateRect(hWnd, NULL, FALSE);
	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

// Dialog sizing support functions and macros (everything working in client co-ords)
#define GetInititalControlPos(a, b)								\
	GetWindowRect(GetDlgItem(hSelDlg, a), &rect);				\
	memset(&point, 0, sizeof(POINT));							\
	point.x = rect.left;										\
	point.y = rect.top;											\
	ScreenToClient(hSelDlg, &point);							\
	b[0] = point.x;												\
	b[1] = point.y;												\
	GetClientRect(GetDlgItem(hSelDlg, a), &rect);				\
	b[2] = rect.right;											\
	b[3] = rect.bottom;

#define SetControlPosAlignTopRight(a, b)						\
	SetWindowPos(GetDlgItem(hSelDlg, a), hSelDlg, b[0] - xDelta, b[1], 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOSENDCHANGING);

#define SetControlPosAlignTopLeft(a, b)							\
	SetWindowPos(GetDlgItem(hSelDlg, a), hSelDlg, b[0], b[1], 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOSENDCHANGING);

#define SetControlPosAlignBottomRight(a, b)						\
	SetWindowPos(GetDlgItem(hSelDlg, a), hSelDlg, b[0] - xDelta, b[1] - yDelta, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOSENDCHANGING);

#define SetControlPosAlignBottomLeftResizeHor(a, b)				\
	SetWindowPos(GetDlgItem(hSelDlg, a), hSelDlg, b[0], b[1] - yDelta, b[2] - xDelta, b[3], SWP_NOZORDER | SWP_NOSENDCHANGING);

#define SetControlPosAlignTopRightResizeVert(a, b)				\
	xScrollBarDelta = (a == IDC_TREE2) ? -18 : 0;				\
	SetWindowPos(GetDlgItem(hSelDlg, a), hSelDlg, b[0] - xDelta, b[1], b[2] - xScrollBarDelta, b[3] - yDelta, SWP_NOZORDER | SWP_NOSENDCHANGING);

#define SetControlPosAlignTopLeftResizeHorVert(a, b)			\
	xScrollBarDelta = (a == IDC_TREE1) ? -18 : 0;				\
	SetWindowPos(GetDlgItem(hSelDlg, a), hSelDlg, b[0], b[1], b[2] - xDelta - xScrollBarDelta, b[3] - yDelta, SWP_NOZORDER | SWP_NOSENDCHANGING);

#define SetControlPosAlignTopLeftResizeHorVertALT(a, b)			\
	SetWindowPos(GetDlgItem(hSelDlg, a), hSelDlg, b[0], b[1], b[2] - xDelta, b[3] - yDelta, SWP_NOZORDER | SWP_NOSENDCHANGING);

static void GetTitlePreviewScale()
{
	RECT rect = { 0 };
	HWND hPreviewCtrl = GetDlgItem(hSelDlg, IDC_STATIC2);
	if (hPreviewCtrl && IsWindow(hPreviewCtrl) && GetClientRect(hPreviewCtrl, &rect)) {
		rect.right -= 20;
		rect.bottom -= 38;
	} else {
		hPreviewCtrl = GetDlgItem(hSelDlg, IDC_SCREENSHOT_H);
		if (hPreviewCtrl == NULL || GetClientRect(hPreviewCtrl, &rect) == 0) {
			rect.right = 213;
			rect.bottom = 160;
		}
	}
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;

	if (w <= 0) w = 213;
	if (h <= 0) h = 160;

	_213 = w;
	_160 = h;

	HDC hDc = GetDC(0);

	dpi_x = GetDeviceCaps(hDc, LOGPIXELSX);
	//bprintf(0, _T("dpi_x is %d\n"), dpi_x);
	ReleaseDC(0, hDc);
}

static void GetInitialPositions()
{
	RECT rect;
	POINT point;

	GetClientRect(hSelDlg, &rect);
	nDlgInitialWidth = rect.right;
	nDlgInitialHeight = rect.bottom;

	GetInititalControlPos(IDC_STATIC_OPT, nDlgOptionsGrpInitialPos);
	GetInititalControlPos(IDC_CHECKAVAILABLE, nDlgAvailableChbInitialPos);
	GetInititalControlPos(IDC_CHECKUNAVAILABLE, nDlgUnavailableChbInitialPos);
	GetInititalControlPos(IDC_CHECKAUTOEXPAND, nDlgAlwaysClonesChbInitialPos);
	GetInititalControlPos(IDC_SEL_SHORTNAME, nDlgZipnamesChbInitialPos);
	GetInititalControlPos(IDC_SEL_ASCIIONLY, nDlgLatinTextChbInitialPos);
	GetInititalControlPos(IDC_SEL_SUBDIRS, nDlgSearchSubDirsChbInitialPos);
	GetInititalControlPos(IDC_SEL_DISABLECRC, nDlgDisableCrcChbInitialPos);
	GetInititalControlPos(IDROM, nDlgRomDirsBtnInitialPos);
	GetInititalControlPos(IDRESCAN, nDlgScanRomsBtnInitialPos);
	GetInititalControlPos(IDC_STATIC_SYS, nDlgFilterGrpInitialPos);
	GetInititalControlPos(IDC_TREE2, nDlgFilterTreeInitialPos);
	GetInititalControlPos(IDC_SEL_IPSGROUP, nDlgIpsGrpInitialPos);
	GetInititalControlPos(IDC_SEL_APPLYIPS, nDlgApplyIpsChbInitialPos);
	GetInititalControlPos(IDC_SEL_IPSMANAGER, nDlgIpsManBtnInitialPos);
	GetInititalControlPos(IDC_SEL_SEARCHGROUP, nDlgSearchGrpInitialPos);
	GetInititalControlPos(IDC_SEL_SEARCH, nDlgSearchTxtInitialPos);
	GetInititalControlPos(IDCANCEL, nDlgCancelBtnInitialPos);
	GetInititalControlPos(IDOK, nDlgPlayBtnInitialPos);
	GetInititalControlPos(IDC_STATIC3, nDlgTitleGrpInitialPos);
	GetInititalControlPos(IDC_SCREENSHOT2_H, nDlgTitleImgHInitialPos);
	GetInititalControlPos(IDC_SCREENSHOT2_V, nDlgTitleImgVInitialPos);
	GetInititalControlPos(IDC_STATIC2, nDlgPreviewGrpInitialPos);
	GetInititalControlPos(IDC_SCREENSHOT_H, nDlgPreviewImgHInitialPos);
	GetInititalControlPos(IDC_SCREENSHOT_V, nDlgPreviewImgVInitialPos);
	GetInititalControlPos(IDC_STATIC_INFOBOX, nDlgWhiteBoxInitialPos);
	GetInititalControlPos(IDC_LABELCOMMENT, nDlgGameInfoLblInitialPos);
	GetInititalControlPos(IDC_LABELROMNAME, nDlgRomNameLblInitialPos);
	GetInititalControlPos(IDC_LABELROMINFO, nDlgRomInfoLblInitialPos);
	GetInititalControlPos(IDC_LABELSYSTEM, nDlgReleasedByLblInitialPos);
	GetInititalControlPos(IDC_LABELGENRE, nDlgGenreLblInitialPos);
	GetInititalControlPos(IDC_LABELNOTES, nDlgNotesLblInitialPos);
	GetInititalControlPos(IDC_TEXTCOMMENT, nDlgGameInfoTxtInitialPos);
	GetInititalControlPos(IDC_TEXTROMNAME, nDlgRomNameTxtInitialPos);
	GetInititalControlPos(IDC_TEXTROMINFO, nDlgRomInfoTxtInitialPos);
	GetInititalControlPos(IDC_TEXTSYSTEM, nDlgReleasedByTxtInitialPos);
	GetInititalControlPos(IDC_TEXTGENRE, nDlgGenreTxtInitialPos);
	GetInititalControlPos(IDC_TEXTNOTES, nDlgNotesTxtInitialPos);
	GetInititalControlPos(IDC_DRVCOUNT, nDlgDrvCountTxtInitialPos);
	GetInititalControlPos(IDGAMEINFO, nDlgDrvRomInfoBtnInitialPos);
	GetInititalControlPos(IDC_STATIC1, nDlgSelectGameGrpInitialPos);
	GetInititalControlPos(IDC_TREE1, nDlgSelectGameLstInitialPos);
	GetInititalControlPos(IDC_SEL_MODE_ROM, nDlgModeRomBtnInitialPos);
	GetInititalControlPos(IDC_SEL_MODE_IPS, nDlgModeIpsBtnInitialPos);
	GetInititalControlPos(IDC_SEL_MODE_ROMDATA, nDlgModeRomDataBtnInitialPos);
	GetInititalControlPos(IDC_SEL_IPS_TREE, nDlgIpsTreeInitialPos);
	GetInititalControlPos(IDC_SEL_IPS_DESC, nDlgIpsDescInitialPos);
	GetInititalControlPos(IDC_SEL_IPS_LANG, nDlgIpsLangInitialPos);
	GetInititalControlPos(IDC_SEL_IPS_CLEAR, nDlgIpsClearInitialPos);
	GetInititalControlPos(IDC_ROMDATA_LIST, nDlgRomDataListInitialPos);
	GetInititalControlPos(IDC_ROMDATA_SELDIR_BUTTON, nDlgRomDataDirBtnInitialPos);
	GetInititalControlPos(IDC_ROMDATA_SCAN_BUTTON, nDlgRomDataScanBtnInitialPos);
	GetInititalControlPos(IDC_ROMDATA_SUBDIR_CHECK, nDlgRomDataSubdirInitialPos);

	GetTitlePreviewScale();

	// When the window is created with too few entries for TreeView to warrant the
	// use of a vertical scrollbar, the right side will be slightly askew. -dink

	if (nDlgSelectGameGrpInitialPos[2] - nDlgSelectGameLstInitialPos[2] < 30 ) {
		nDlgSelectGameLstInitialPos[2] = nDlgSelectGameGrpInitialPos[2] - 34;
	}

	if (nDlgFilterGrpInitialPos[2] - nDlgFilterTreeInitialPos[2] < 30 ) {
		nDlgFilterTreeInitialPos[2] = nDlgFilterGrpInitialPos[2] - 34;
	}
}

// Check if a specified driver is working
static bool CheckWorkingStatus(int nDriver)
{
	int nOldnBurnDrvActive = nBurnDrvActive;
	nBurnDrvActive = nDriver;
	bool bStatus = BurnDrvIsWorking();
	nBurnDrvActive = nOldnBurnDrvActive;

	return bStatus;
}

static TCHAR* MangleGamename(const TCHAR* szOldName, bool /*bRemoveArticle*/)
{
	static TCHAR szNewName[256] = _T("");

#if 0
	TCHAR* pszName = szNewName;

	if (_tcsnicmp(szOldName, _T("the "), 4) == 0) {
		int x = 0, y = 0;
		while (szOldName[x] && szOldName[x] != _T('(') && szOldName[x] != _T('-')) {
			x++;
		}
		y = x;
		while (y && szOldName[y - 1] == _T(' ')) {
			y--;
		}
		_tcsncpy(pszName, szOldName + 4, y - 4);
		pszName[y - 4] = _T('\0');
		pszName += y - 4;

		if (!bRemoveArticle) {
			pszName += _stprintf(pszName, _T(", the"));
		}
		if (szOldName[x]) {
			_stprintf(pszName, _T(" %s"), szOldName + x);
		}
	} else {
		_tcscpy(pszName, szOldName);
	}
#endif

#if 0
	static TCHAR szPrefix[256] = _T("");
	// dink's prefix-izer - it's useless...
	_tcscpy(szPrefix, BurnDrvGetText(DRV_NAME));
	int use_prefix = 0;
	int nn_len = 0;
	for (int i = 0; i < 6; i++) {
		if (szPrefix[i] == _T('_')) {
			szNewName[nn_len++] = _T(' ');
			use_prefix = 1;
			break;
		} else {
			szNewName[nn_len++] = towupper(szPrefix[i]);
		}
	}
	if (use_prefix == 0) nn_len = 0;
#endif

#if 1
	_tcscpy(szNewName, szOldName);
#endif

	return szNewName;
}

static TCHAR* RemoveSpace(const TCHAR* szOldName)
{
	if (NULL == szOldName) return NULL;

	static TCHAR szNewName[256] = _T("");
	int j = 0;
	int i = 0;

	for (i = 0; szOldName[i]; i++) {
		if (!iswspace(szOldName[i])) {
			szNewName[j++] = szOldName[i];
		}
	}

	szNewName[j] = _T('\0');

	return szNewName;
}

static int DoExtraFilters()
{
	if (nLoadMenuFavoritesFilter) {
		return (CheckFavorites(BurnDrvGetTextA(DRV_NAME)) != -1) ? 0 : 1;
	}

	if (nShowMVSCartsOnly && ((BurnDrvGetHardwareCode() & HARDWARE_PREFIX_CARTRIDGE) != HARDWARE_PREFIX_CARTRIDGE)) return 1;

	if ((nLoadMenuBoardTypeFilter & BDF_BOOTLEG)			&& (BurnDrvGetFlags() & BDF_BOOTLEG))				return 1;
	if ((nLoadMenuBoardTypeFilter & BDF_DEMO)				&& (BurnDrvGetFlags() & BDF_DEMO))					return 1;
	if ((nLoadMenuBoardTypeFilter & BDF_HACK)				&& (BurnDrvGetFlags() & BDF_HACK))					return 1;
	if ((nLoadMenuBoardTypeFilter & BDF_HOMEBREW)			&& (BurnDrvGetFlags() & BDF_HOMEBREW))				return 1;
	if ((nLoadMenuBoardTypeFilter & BDF_PROTOTYPE)			&& (BurnDrvGetFlags() & BDF_PROTOTYPE))				return 1;

	if ((nLoadMenuBoardTypeFilter & MASKBOARDTYPEGENUINE)	&& (!(BurnDrvGetFlags() & BDF_BOOTLEG))
															&& (!(BurnDrvGetFlags() & BDF_DEMO))
															&& (!(BurnDrvGetFlags() & BDF_HACK))
															&& (!(BurnDrvGetFlags() & BDF_HOMEBREW))
															&& (!(BurnDrvGetFlags() & BDF_PROTOTYPE)))	return 1;

	if ((nLoadMenuFamilyFilter & FBF_MSLUG)					&& (BurnDrvGetFamilyFlags() & FBF_MSLUG))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_SF)					&& (BurnDrvGetFamilyFlags() & FBF_SF))				return 1;
	if ((nLoadMenuFamilyFilter & FBF_KOF)					&& (BurnDrvGetFamilyFlags() & FBF_KOF))				return 1;
	if ((nLoadMenuFamilyFilter & FBF_DSTLK)					&& (BurnDrvGetFamilyFlags() & FBF_DSTLK))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_FATFURY)				&& (BurnDrvGetFamilyFlags() & FBF_FATFURY))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_SAMSHO)				&& (BurnDrvGetFamilyFlags() & FBF_SAMSHO))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_19XX)					&& (BurnDrvGetFamilyFlags() & FBF_19XX))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_SONICWI)				&& (BurnDrvGetFamilyFlags() & FBF_SONICWI))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_PWRINST)				&& (BurnDrvGetFamilyFlags() & FBF_PWRINST))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_SONIC)					&& (BurnDrvGetFamilyFlags() & FBF_SONIC))			return 1;
	if ((nLoadMenuFamilyFilter & FBF_DONPACHI)				&& (BurnDrvGetFamilyFlags() & FBF_DONPACHI))		return 1;
	if ((nLoadMenuFamilyFilter & FBF_MAHOU)					&& (BurnDrvGetFamilyFlags() & FBF_MAHOU))			return 1;

	if ((nLoadMenuFamilyFilter & MASKFAMILYOTHER)			&& (!(BurnDrvGetFamilyFlags() & FBF_MSLUG))
															&& (!(BurnDrvGetFamilyFlags() & FBF_SF))
															&& (!(BurnDrvGetFamilyFlags() & FBF_KOF))
															&& (!(BurnDrvGetFamilyFlags() & FBF_DSTLK))
															&& (!(BurnDrvGetFamilyFlags() & FBF_FATFURY))
															&& (!(BurnDrvGetFamilyFlags() & FBF_SAMSHO))
															&& (!(BurnDrvGetFamilyFlags() & FBF_19XX))
															&& (!(BurnDrvGetFamilyFlags() & FBF_SONICWI))
															&& (!(BurnDrvGetFamilyFlags() & FBF_PWRINST))
															&& (!(BurnDrvGetFamilyFlags() & FBF_SONIC))
															&& (!(BurnDrvGetFamilyFlags() & FBF_DONPACHI))
															&& (!(BurnDrvGetFamilyFlags() & FBF_MAHOU)) )	return 1;


	// This filter supports multiple genre's per game
	int bGenreOk = 0;
	if ((~nLoadMenuGenreFilter & GBF_HORSHOOT)				&& (BurnDrvGetGenreFlags() & GBF_HORSHOOT))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_VERSHOOT)				&& (BurnDrvGetGenreFlags() & GBF_VERSHOOT))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_SCRFIGHT)				&& (BurnDrvGetGenreFlags() & GBF_SCRFIGHT))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_VSFIGHT)				&& (BurnDrvGetGenreFlags() & GBF_VSFIGHT))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_BIOS)					&& (BurnDrvGetGenreFlags() & GBF_BIOS))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_BREAKOUT)				&& (BurnDrvGetGenreFlags() & GBF_BREAKOUT))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_CASINO)				&& (BurnDrvGetGenreFlags() & GBF_CASINO))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_BALLPADDLE)			&& (BurnDrvGetGenreFlags() & GBF_BALLPADDLE))		bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_MAZE)					&& (BurnDrvGetGenreFlags() & GBF_MAZE))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_MINIGAMES)				&& (BurnDrvGetGenreFlags() & GBF_MINIGAMES))		bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_PINBALL)				&& (BurnDrvGetGenreFlags() & GBF_PINBALL))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_PLATFORM)				&& (BurnDrvGetGenreFlags() & GBF_PLATFORM))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_PUZZLE)				&& (BurnDrvGetGenreFlags() & GBF_PUZZLE))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_QUIZ)					&& (BurnDrvGetGenreFlags() & GBF_QUIZ))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_SPORTSMISC)			&& (BurnDrvGetGenreFlags() & GBF_SPORTSMISC))		bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_SPORTSFOOTBALL) 		&& (BurnDrvGetGenreFlags() & GBF_SPORTSFOOTBALL))	bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_MISC)					&& (BurnDrvGetGenreFlags() & GBF_MISC))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_MAHJONG)				&& (BurnDrvGetGenreFlags() & GBF_MAHJONG))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_RACING)				&& (BurnDrvGetGenreFlags() & GBF_RACING))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_SHOOT)					&& (BurnDrvGetGenreFlags() & GBF_SHOOT))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_MULTISHOOT)			&& (BurnDrvGetGenreFlags() & GBF_MULTISHOOT))		bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_ACTION)				&& (BurnDrvGetGenreFlags() & GBF_ACTION))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_RUNGUN)				&& (BurnDrvGetGenreFlags() & GBF_RUNGUN))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_STRATEGY)				&& (BurnDrvGetGenreFlags() & GBF_STRATEGY))			bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_RPG)					&& (BurnDrvGetGenreFlags() & GBF_RPG))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_SIM)					&& (BurnDrvGetGenreFlags() & GBF_SIM))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_ADV)					&& (BurnDrvGetGenreFlags() & GBF_ADV))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_CARD)					&& (BurnDrvGetGenreFlags() & GBF_CARD))				bGenreOk = 1;
	if ((~nLoadMenuGenreFilter & GBF_BOARD)					&& (BurnDrvGetGenreFlags() & GBF_BOARD))			bGenreOk = 1;
	if (bGenreOk == 0) return 1;

	return 0;
}

static void BuildCompactDisplayName(INT32 nDriver, TCHAR* pszOut, size_t nOutChars)
{
	if (pszOut == NULL || nOutChars == 0) return;

	pszOut[0] = _T('\0');
	if (nDriver < 0 || nDriver >= nBurnDrvCount) return;

	const INT32 nOldDrv = nBurnDrvActive;
	nBurnDrvActive = nDriver;

	const TCHAR* pszName = BurnDrvGetText((nLoadMenuShowY & ASCIIONLY) ? (DRV_ASCIIONLY | DRV_FULLNAME) : DRV_FULLNAME);
	if (pszName) {
		size_t j = 0;
		for (size_t i = 0; pszName[i] && (j + 1) < nOutChars; i++) {
			if (!_istspace(pszName[i])) {
				pszOut[j++] = pszName[i];
			}
		}
		pszOut[j] = _T('\0');
	}

	nBurnDrvActive = nOldDrv;
}

static int DisplayNames_qs_cmp_desc(const void *p0, const void *p1)
{
	struct NODEINFO *ni0 = (struct NODEINFO*) p0;
	struct NODEINFO *ni1 = (struct NODEINFO*) p1;

	TCHAR szName0[256];
	TCHAR szName1[256];
	BuildCompactDisplayName(ni0->nBurnDrvNo, szName0, _countof(szName0));
	BuildCompactDisplayName(ni1->nBurnDrvNo, szName1, _countof(szName1));

	int nRet = _tcsicmp(szName1, szName0);
	if (nRet == 0) {
		nRet = stricmp(ni1->pszROMName ? ni1->pszROMName : "", ni0->pszROMName ? ni0->pszROMName : "");
	}

	return nRet;
}

// Make a tree-view control with all drivers
static int SelListMake()
{
	int i, j;
	unsigned int nMissingDrvCount = 0;

	if (nBurnDrv) {
		free(nBurnDrv);
		nBurnDrv = NULL;
	}
	nBurnDrv = (NODEINFO*)malloc(nBurnDrvCount * sizeof(NODEINFO));
	memset(nBurnDrv, 0, nBurnDrvCount * sizeof(NODEINFO));

	nTmpDrvCount = 0;

	if (hSelList == NULL) {
		return 1;
	}

	LoadFavorites();

	// Get dialog search string
	j = GetDlgItemText(hSelDlg, IDC_SEL_SEARCH, szSearchString, _countof(szSearchString));
	SelNormalizeSearchText(szSearchString, szSearchStringNormalized, _countof(szSearchStringNormalized));

	// pre-sort the list by the name actually shown in the selector
	if (nBurnZipListDrv) {
		free(nBurnZipListDrv);
		nBurnZipListDrv = NULL;
	}
	nBurnZipListDrv = (NODEINFO*)malloc(nBurnDrvCount * sizeof(NODEINFO));
	memset(nBurnZipListDrv, 0, nBurnDrvCount * sizeof(NODEINFO));

	for (i = nBurnDrvCount-1; i >= 0; i--) {
		nBurnDrvActive = i;
		nBurnZipListDrv[i].pszROMName = BurnDrvGetTextA(DRV_NAME);
		nBurnZipListDrv[i].nBurnDrvNo = i;
	}

	// sort it in descending order (we add in descending order)
	qsort(nBurnZipListDrv, nBurnDrvCount, sizeof(NODEINFO), DisplayNames_qs_cmp_desc);

	// Add all the driver names to the list
	// 1st: parents
	for (i = nBurnDrvCount-1; i >= 0; i--) {
		TV_INSERTSTRUCT TvItem;

		nBurnDrvActive = nBurnZipListDrv[nBurnDrvCount - 1 - i].nBurnDrvNo;

		if (BurnDrvGetFlags() & BDF_BOARDROM) {
			continue;
		}

		if (BurnDrvGetText(DRV_PARENT) != NULL && (BurnDrvGetFlags() & BDF_CLONE)) {	// Skip clones
			continue;
		}

		if(!gameAv[nBurnDrvActive]) nMissingDrvCount++;

		UINT64 nHardware = SelGetCurrentDriverHardwareMask();
		if ((nHardware & MASKALL) && ((nHardware & nLoadMenuShowX) || (nHardware & MASKALL) == 0)) {
			continue;
		}

		if (avOk && (!(nLoadMenuShowY & UNAVAILABLE)) && !gameAv[nBurnDrvActive]) {						// Skip non-available games if needed
			continue;
		}

		if (avOk && (!(nLoadMenuShowY & AVAILABLE)) && gameAv[nBurnDrvActive]) {						// Skip available games if needed
			continue;
		}

		if (DoExtraFilters()) continue;

		if (szSearchStringNormalized[0]) {
			if (!SelCurrentDriverMatchesSearch()) continue;
		}

		memset(&TvItem, 0, sizeof(TvItem));
		TvItem.item.mask = TVIF_TEXT | TVIF_PARAM;
		TvItem.hInsertAfter = TVI_FIRST;
		TvItem.item.pszText = RemoveSpace(BurnDrvGetText((nLoadMenuShowY & ASCIIONLY) ? (DRV_ASCIIONLY | DRV_FULLNAME) : DRV_FULLNAME));
		TvItem.item.lParam = (LPARAM)&nBurnDrv[nTmpDrvCount];
		nBurnDrv[nTmpDrvCount].hTreeHandle = (HTREEITEM)SendMessage(hSelList, TVM_INSERTITEM, 0, (LPARAM)&TvItem);
		nBurnDrv[nTmpDrvCount].nBurnDrvNo = nBurnDrvActive;
		nBurnDrv[nTmpDrvCount].pszROMName = BurnDrvGetTextA(DRV_NAME);
		nBurnDrv[nTmpDrvCount].bIsParent = true;
		nTmpDrvCount++;
	}

	// 2nd: clones
	for (i = nBurnDrvCount-1; i >= 0; i--) {
		TV_INSERTSTRUCT TvItem;

		nBurnDrvActive = nBurnZipListDrv[nBurnDrvCount - 1 - i].nBurnDrvNo;

		if (BurnDrvGetFlags() & BDF_BOARDROM) {
			continue;
		}

		if (BurnDrvGetTextA(DRV_PARENT) == NULL || !(BurnDrvGetFlags() & BDF_CLONE)) {	// Skip parents
			continue;
		}

		if(!gameAv[nBurnDrvActive]) nMissingDrvCount++;

		UINT64 nHardware = SelGetCurrentDriverHardwareMask();
		if ((nHardware & MASKALL) && ((nHardware & nLoadMenuShowX) || ((nHardware & MASKALL) == 0))) {
			continue;
		}

		if (avOk && (!(nLoadMenuShowY & UNAVAILABLE)) && !gameAv[nBurnDrvActive]) {						// Skip non-available games if needed
			continue;
		}

		if (avOk && (!(nLoadMenuShowY & AVAILABLE)) && gameAv[nBurnDrvActive]) {						// Skip available games if needed
			continue;
		}

		if (DoExtraFilters()) continue;

		if (szSearchStringNormalized[0]) {
			if (!SelCurrentDriverMatchesSearch()) continue;
		}

		memset(&TvItem, 0, sizeof(TvItem));
		TvItem.item.mask = TVIF_TEXT | TVIF_PARAM;
		TvItem.hInsertAfter = TVI_FIRST;
		TvItem.item.pszText = RemoveSpace(BurnDrvGetText((nLoadMenuShowY & ASCIIONLY) ? (DRV_ASCIIONLY | DRV_FULLNAME) : DRV_FULLNAME));

		// Find the parent's handle
		for (j = 0; j < nTmpDrvCount; j++) {
			if (nBurnDrv[j].bIsParent) {
				if (!strcmp(BurnDrvGetTextA(DRV_PARENT), nBurnDrv[j].pszROMName)) {
					TvItem.hParent = nBurnDrv[j].hTreeHandle;
					break;
				}
			}
		}

		// Find the parent and add a branch to the tree
		if (!TvItem.hParent) {
			char szTempName[32];
			strcpy(szTempName, BurnDrvGetTextA(DRV_PARENT));
			int nTempBurnDrvSelect = nBurnDrvActive;
			for (j = 0; j < nBurnDrvCount; j++) {
				nBurnDrvActive = j;
				if (!strcmp(szTempName, BurnDrvGetTextA(DRV_NAME))) {
					TV_INSERTSTRUCT TempTvItem;
					memset(&TempTvItem, 0, sizeof(TempTvItem));
					TempTvItem.item.mask = TVIF_TEXT | TVIF_PARAM;
					//TempTvItem.hInsertAfter = TVI_FIRST;
					TempTvItem.hInsertAfter = TVI_SORT; // only use the horribly-slow TVI_SORT for missing parents!
					TempTvItem.item.pszText = RemoveSpace(BurnDrvGetText((nLoadMenuShowY & ASCIIONLY) ? (DRV_ASCIIONLY | DRV_FULLNAME) : DRV_FULLNAME));
					TempTvItem.item.lParam = (LPARAM)&nBurnDrv[nTmpDrvCount];
					nBurnDrv[nTmpDrvCount].hTreeHandle = (HTREEITEM)SendMessage(hSelList, TVM_INSERTITEM, 0, (LPARAM)&TempTvItem);
					nBurnDrv[nTmpDrvCount].nBurnDrvNo = j;
					nBurnDrv[nTmpDrvCount].bIsParent = true;
					nBurnDrv[nTmpDrvCount].pszROMName = BurnDrvGetTextA(DRV_NAME);
					TvItem.item.lParam = (LPARAM)&nBurnDrv[nTmpDrvCount];
					TvItem.hParent = nBurnDrv[nTmpDrvCount].hTreeHandle;
					nTmpDrvCount++;
					break;
				}
			}
			nBurnDrvActive = nTempBurnDrvSelect;
		}

		TvItem.item.lParam = (LPARAM)&nBurnDrv[nTmpDrvCount];
		nBurnDrv[nTmpDrvCount].hTreeHandle = (HTREEITEM)SendMessage(hSelList, TVM_INSERTITEM, 0, (LPARAM)&TvItem);
		nBurnDrv[nTmpDrvCount].pszROMName = BurnDrvGetTextA(DRV_NAME);
		nBurnDrv[nTmpDrvCount].nBurnDrvNo = nBurnDrvActive;
		nTmpDrvCount++;
	}

	for (i = 0; i < nTmpDrvCount; i++) {
		// See if we need to expand the branch of an unavailable or non-working parent
		if (nBurnDrv[i].bIsParent && ((nLoadMenuShowY & AUTOEXPAND) || !gameAv[nBurnDrv[i].nBurnDrvNo] || !CheckWorkingStatus(nBurnDrv[i].nBurnDrvNo))) {
			for (j = 0; j < nTmpDrvCount; j++) {

				// Expand the branch only if a working clone is available -or-
				// If ROM Scanning is disabled (bSkipStartupCheck==1) and expand clones is set. -dink March 22, 2020
				if ( gameAv[nBurnDrv[j].nBurnDrvNo] || (bSkipStartupCheck && (nLoadMenuShowY & AUTOEXPAND)) ) {
					nBurnDrvActive = nBurnDrv[j].nBurnDrvNo;
					if (BurnDrvGetTextA(DRV_PARENT)) {
						if (strcmp(nBurnDrv[i].pszROMName, BurnDrvGetTextA(DRV_PARENT)) == 0) {
							SendMessage(hSelList, TVM_EXPAND, TVE_EXPAND, (LPARAM)nBurnDrv[i].hTreeHandle);
							break;
						}
					}
				}
			}
		}
	}

	// Update the status info
	TCHAR szRomsAvailableInfo[128] = _T("");

	_stprintf(szRomsAvailableInfo, FBALoadStringEx(hAppInst, IDS_SEL_SETSTATUS, true), nTmpDrvCount, nBurnDrvCount - REDUCE_TOTAL_SETS_BIOS, nMissingDrvCount);
	SendDlgItemMessage(hSelDlg, IDC_DRVCOUNT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szRomsAvailableInfo);

	return 0;
}

static void MyEndDialog()
{
	if (nTimer) {
		KillTimer(hSelDlg, nTimer);
		nTimer = 0;
	}

	if (nInitPreviewTimer) {
		KillTimer(hSelDlg, nInitPreviewTimer);
		nInitPreviewTimer = 0;
	}

	SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT_H, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
	SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT_V, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);

	SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT2_H, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
	SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT2_V, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);

	if (hPrevBmp) {
		DeleteObject((HGDIOBJ)hPrevBmp);
		hPrevBmp = NULL;
	}
	if(hTitleBmp) {
		DeleteObject((HGDIOBJ)hTitleBmp);
		hTitleBmp = NULL;
	}
	if (hSelModeFontActive) {
		DeleteObject(hSelModeFontActive);
		hSelModeFontActive = NULL;
	}

	for (INT32 i = 0; i < 3; i++) {
		hSelModeTitle[i] = NULL;
	}

	if (hExpand) {
		DestroyIcon(hExpand);
		hExpand = NULL;
	}
	if (hCollapse) {
		DestroyIcon(hCollapse);
		hCollapse = NULL;
	}
	if (hNotWorking) {
		DestroyIcon(hNotWorking);
		hNotWorking = NULL;
	}
	if (hNotFoundEss) {
		DestroyIcon(hNotFoundEss);
		hNotFoundEss = NULL;
	}
	if (hNotFoundNonEss) {
		DestroyIcon(hNotFoundNonEss);
		hNotFoundNonEss = NULL;
	}
	if(hDrvIconMiss) {
		DestroyIcon(hDrvIconMiss);
		hDrvIconMiss = NULL;
	}

	RECT rect;

	GetWindowRect(hSelDlg, &rect);
	nSelDlgWidth  = rect.right - rect.left;
	nSelDlgHeight = rect.bottom - rect.top;

	if (!bSelOkay) {
		RomDataStateRestore();
	}
	bSelOkay = false;

	EndDialog(hSelDlg, 0);
}

static void SelOkayRomData(const TCHAR* pszSelDat)
{
	if (pszSelDat == NULL || pszSelDat[0] == _T('\0')) {
		return;
	}

	// Match the standalone RomData manager flow:
	// restore RomData state first, then tear down hosted controls,
	// then close the selector and finally load the dat-backed driver.
	RomDataStateRestore();

	IpsHostedExit(1);
	RomDataHostedExit();

	bSelRomDataLaunch = true;
	bSelOkay = true;
	nDialogSelect = -1;
	bDialogCancel = true;
	MyEndDialog();
	RomDataLoadDriver(pszSelDat);
}

static void SelOkayDriverIndex(INT32 nSelect)
{
	if (nSelect < 0) {
		return;
	}

#if DISABLE_NON_AVAILABLE_SELECT
	if (!gameAv[nSelect]) {
		return;
	}
#endif

#if NON_WORKING_PROMPT_ON_LOAD
	if (!CheckWorkingStatus(nSelect)) {
		TCHAR szWarningText[1024];
		_stprintf(szWarningText, _T("%s"), FBALoadStringEx(hAppInst, IDS_ERR_WARNING, true));
		if (MessageBox(hSelDlg, FBALoadStringEx(hAppInst, IDS_ERR_NON_WORKING, true), szWarningText, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) == IDNO) {
			return;
		}
	}
#endif

	nDialogSelect = nSelect;
	bSelOkay      = true;
	if (bDoIpsPatch) {
		IpsSetHostedStorageName(NULL);
		nBurnDrvActive = nSelect;
		LoadIpsActivePatches();
		IpsPatchInit();
	}

	bDialogCancel = false;
	MyEndDialog();
}

// User clicked ok for a driver in the list
static void SelOkay()
{
	TV_ITEM TvItem;
	unsigned int nSelect = 0;
	HTREEITEM hSelectHandle = (HTREEITEM)SendMessage(hSelList, TVM_GETNEXTITEM, TVGN_CARET, ~0U);

	if (!hSelectHandle)	{			// Nothing is selected, return without closing the window
		return;
	}

	TvItem.hItem = hSelectHandle;
	TvItem.mask = TVIF_PARAM;
	SendMessage(hSelList, TVM_GETITEM, 0, (LPARAM)&TvItem);
	nSelect = ((NODEINFO*)TvItem.lParam)->nBurnDrvNo;

#if DISABLE_NON_AVAILABLE_SELECT
	if (!gameAv[nSelect]) {			// Game not available, return without closing the window
		return;
	}
#endif
	SelOkayDriverIndex(nSelect);
}

static void RefreshPanel()
{
	// clear preview shot
	if (hPrevBmp) {
		DeleteObject((HGDIOBJ)hPrevBmp);
		hPrevBmp = NULL;
	}
	if (hTitleBmp) {
		DeleteObject((HGDIOBJ)hTitleBmp);
		hTitleBmp = NULL;
	}
	if (nTimer) {
		KillTimer(hSelDlg, nTimer);
		nTimer = 0;
	}

	GetTitlePreviewScale();
	INT32 x = _213, y = _160;
	bool bVertical = false;
	GetDefaultPreviewBitmapFit(&x, &y, &bVertical);

	hPrevBmp  = PNGLoadBitmap(hSelDlg, NULL, x, y, 2);

	if (!bVertical) {
		SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT_H, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hPrevBmp);
		SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT_V, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
		ShowWindow(GetDlgItem(hSelDlg, IDC_SCREENSHOT_H), SW_SHOW);
		ShowWindow(GetDlgItem(hSelDlg, IDC_SCREENSHOT_V), SW_HIDE);
	} else {
		SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT_H, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
		SendDlgItemMessage(hSelDlg, IDC_SCREENSHOT_V, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hPrevBmp);
		ShowWindow(GetDlgItem(hSelDlg, IDC_SCREENSHOT_H), SW_HIDE);
		ShowWindow(GetDlgItem(hSelDlg, IDC_SCREENSHOT_V), SW_SHOW);
	}
	ShowWindow(GetDlgItem(hSelDlg, IDC_SCREENSHOT2_H), SW_HIDE);
	ShowWindow(GetDlgItem(hSelDlg, IDC_SCREENSHOT2_V), SW_HIDE);

	// Clear the things in our Info-box
	for (int i = 0; i < 6; i++) {
		SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
		EnableWindow(hInfoLabel[i], FALSE);
	}

	CheckDlgButton(hSelDlg, IDC_CHECKAUTOEXPAND,  (nLoadMenuShowY & AUTOEXPAND)  ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hSelDlg, IDC_CHECKAVAILABLE,   (nLoadMenuShowY & AVAILABLE)   ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hSelDlg, IDC_CHECKUNAVAILABLE, (nLoadMenuShowY & UNAVAILABLE) ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hSelDlg, IDC_SEL_SHORTNAME, nLoadMenuShowY & SHOWSHORT     ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hSelDlg, IDC_SEL_ASCIIONLY, nLoadMenuShowY & ASCIIONLY     ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hSelDlg, IDC_SEL_SUBDIRS,   nLoadMenuShowY & SEARCHSUBDIRS ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hSelDlg, IDC_SEL_DISABLECRC, nLoadMenuShowY & DISABLECRCVERIFY ? BST_CHECKED : BST_UNCHECKED);
}

FILE* OpenPreview(int nIndex, TCHAR *szPath)
{
	static bool bTryParent;

	TCHAR szBaseName[MAX_PATH];
	TCHAR szFileName[MAX_PATH];

	FILE* fp = NULL;

	// Try to load a .PNG preview image
	_sntprintf(szBaseName, sizeof(szBaseName), _T("%s%s"), szPath, BurnDrvGetText(DRV_NAME));
	if (nIndex <= 1) {
		_stprintf(szFileName, _T("%s.png"), szBaseName);
		fp = _tfopen(szFileName, _T("rb"));
	}
	if (!fp) {
		_stprintf(szFileName, _T("%s [%02i].png"), szBaseName, nIndex);
		fp = _tfopen(szFileName, _T("rb"));
	}

	if (nIndex <= 1) {
		bTryParent = fp ? false : true;
	}

	if (!fp && BurnDrvGetText(DRV_PARENT) && bTryParent) {						// Try the parent
		_sntprintf(szBaseName, sizeof(szBaseName), _T("%s%s"), szPath, BurnDrvGetText(DRV_PARENT));
		if (nIndex <= 1) {
			_stprintf(szFileName, _T("%s.png"), szBaseName);
			fp = _tfopen(szFileName, _T("rb"));
		}
		if (!fp) {
			_stprintf(szFileName, _T("%s [%02i].png"), szBaseName, nIndex);
			fp = _tfopen(szFileName, _T("rb"));
		}
	}

	return fp;
}

static VOID CALLBACK PreviewTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	UpdatePreview(false, szAppPreviewsPath, IDC_SCREENSHOT_H, IDC_SCREENSHOT_V);
}

static VOID CALLBACK InitPreviewTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	UpdatePreview(true, szAppPreviewsPath, IDC_SCREENSHOT_H, IDC_SCREENSHOT_V);

	if (GetIpsNumPatches()) {
		if (!nShowMVSCartsOnly) {
			EnableWindow(GetDlgItem(hSelDlg, IDC_SEL_IPSMANAGER), TRUE);
			INT32 nActivePatches = LoadIpsActivePatches();

			// Whether IDC_SEL_APPLYIPS is enabled must be subordinate to IDC_SEL_IPSMANAGER
			// to verify that xxx.dat is not removed after saving config.
			// Reduce useless array lookups.
			EnableWindow(GetDlgItem(hSelDlg, IDC_SEL_APPLYIPS), nActivePatches);
		}
	} else {
		EnableWindow(GetDlgItem(hSelDlg, IDC_SEL_IPSMANAGER), FALSE);
		EnableWindow(GetDlgItem(hSelDlg, IDC_SEL_APPLYIPS),   FALSE);	// xxx.dat path not found, must be disabled.
	}

	KillTimer(hSelDlg, nInitPreviewTimer);
	nInitPreviewTimer = 0;
}

static int UpdatePreview(bool bReset, TCHAR *szPath, int HorCtrl, int VerCtrl)
{
	static int nIndex;
	int nOldIndex = 0;

	FILE* fp = NULL;
	HBITMAP hNewImage = NULL;

	if (HorCtrl == IDC_SCREENSHOT_H) {
		nOldIndex = nIndex;
		nIndex++;
		if (bReset) {
			nIndex = 1;
			nOldIndex = -1;
			if (nTimer) {
				KillTimer(hSelDlg, 1);
				nTimer = 0;
			}
		}
	}

	nBurnDrvActive = nDialogSelect;

	if ((nIndex != nOldIndex) || (HorCtrl == IDC_SCREENSHOT2_H)) {
		int x, y, ax, ay;
		bool bPreviewVertical = false;
		GetPreviewBitmapFit(&x, &y, &bPreviewVertical);
		bImageOrientation = bPreviewVertical ? TRUE : FALSE;
		BurnDrvGetAspect(&ax, &ay);

		if (HorCtrl == IDC_SCREENSHOT_H) {
			fp = OpenPreview(nIndex, szPath);
		} else {
			fp = OpenPreview(0, szPath);
		}
		if (!fp && nIndex > 1 && HorCtrl == IDC_SCREENSHOT_H) {
			if (nIndex == 2) {

				// There's only a single preview image, stop timer

				if (nTimer) {
					KillTimer(hSelDlg, 1);
					nTimer = 0;
				}

				return 0;
			}

			nIndex = 1;
			fp = OpenPreview(nIndex, szPath);
		}
		if (fp) {
			if (ax > 4) {
				// Check if title/preview image is captured from 1 monitor on a
				// multi-monitor game, then make the proper adjustments so it
				// looks correct.
				IMAGE img;
				INT32 game_x, game_y;

				PNGGetInfo(&img, fp);
				rewind(fp);

				BurnDrvGetFullSize(&game_x, &game_y);

				if (img.width <= game_x / 2) {
					ax = 4;
					ay = 3;
					bImageOrientation = FALSE;
					x = _213;
					y = _160;
				}
			}
			hNewImage = PNGLoadBitmap(hSelDlg, fp, x, y, 3);
		}
	}

	if (fp) {
		fclose(fp);

		if (HorCtrl == IDC_SCREENSHOT_H) nTimer = SetTimer(hSelDlg, 1, 2500, PreviewTimerProc);
	} else {

		// We couldn't load a new image for this game, so kill the timer (it will be restarted when a new game is selected)

		if (HorCtrl == IDC_SCREENSHOT_H) {
			if (nTimer) {
				KillTimer(hSelDlg, 1);
				nTimer = 0;
			}
		}

		INT32 x = _213, y = _160;
		bool bPreviewVertical = false;
		GetDefaultPreviewBitmapFit(&x, &y, &bPreviewVertical);
		bImageOrientation = FALSE;
		hNewImage = PNGLoadBitmap(hSelDlg, NULL, x, y, 2);
	}

	if (hPrevBmp && (HorCtrl == IDC_SCREENSHOT_H || VerCtrl == IDC_SCREENSHOT_V)) {
		DeleteObject((HGDIOBJ)hPrevBmp);
		*&hPrevBmp = NULL;
		hPrevBmp = hNewImage;
	}

	if (hTitleBmp && (HorCtrl == IDC_SCREENSHOT2_H || VerCtrl == IDC_SCREENSHOT2_V)) {
		DeleteObject((HGDIOBJ)hTitleBmp);
		*&hTitleBmp = NULL;
		hTitleBmp = hNewImage;
	}

	if (bImageOrientation == 0) {
		SendDlgItemMessage(hSelDlg, HorCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hNewImage);
		SendDlgItemMessage(hSelDlg, VerCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
		ShowWindow(GetDlgItem(hSelDlg, HorCtrl), SW_SHOW);
		ShowWindow(GetDlgItem(hSelDlg, VerCtrl), SW_HIDE);
	} else {
		SendDlgItemMessage(hSelDlg, HorCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
		SendDlgItemMessage(hSelDlg, VerCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hNewImage);
		ShowWindow(GetDlgItem(hSelDlg, HorCtrl), SW_HIDE);
		ShowWindow(GetDlgItem(hSelDlg, VerCtrl), SW_SHOW);
	}

	UpdateWindow(hSelDlg);

	return 0;
}

// Sometimes we have different setnames than other emu's, the table below
// will translate our set to their set to keep hiscore.dat/arcadedb happy
// NOTE: asciiz version repeated in burn/hiscore.cpp

struct game_replace_entry {
	TCHAR fb_name[80];
	TCHAR mame_name[80];
};

static game_replace_entry replace_table[] = {
	{ _T("vsraidbbay"),			_T("bnglngby")		},
	{ _T("vsrbibbal"),			_T("rbibb")			},
	{ _T("vsbattlecity"),		_T("btlecity")		},
	{ _T("vscastlevania"),		_T("cstlevna")		},
	{ _T("vsclucluland"),		_T("cluclu")		},
	{ _T("vsdrmario"),			_T("drmario")		},
	{ _T("vsduckhunt"),			_T("duckhunt")		},
	{ _T("vsexcitebike"),		_T("excitebk")		},
	{ _T("vsfreedomforce"),		_T("vsfdf")			},
	{ _T("vsgoonies"),			_T("goonies")		},
	{ _T("vsgradius"),			_T("vsgradus")		},
	{ _T("vsgumshoe"),			_T("vsgshoe")		},
	{ _T("vshogansalley"),		_T("hogalley")		},
	{ _T("vsiceclimber"),		_T("iceclimb")		},
	{ _T("vsmachrider"),		_T("nvs_machrider")	},
	{ _T("vsmightybomjack"),	_T("nvs_mightybj")	},
	{ _T("vsninjajkun"),		_T("jajamaru")		},
	{ _T("vspinball"),			_T("vspinbal")		},
	{ _T("vsplatoon"),			_T("nvs_platoon")	},
	{ _T("vsslalom"),			_T("vsslalom")		},
	{ _T("vssoccer"),			_T("vssoccer")		},
	{ _T("vsstarluster"),		_T("starlstr")		},
	{ _T("vssmgolf"),			_T("smgolf")		},
	{ _T("vssmgolfla"),			_T("ladygolf")		},
	{ _T("vssmb"),				_T("suprmrio")		},
	{ _T("vssuperskykid"),		_T("vsskykid")		},
	{ _T("vssuperxevious"),		_T("supxevs")		},
	{ _T("vstetris"),			_T("vstetris")		},
	{ _T("vstkoboxing"),		_T("tkoboxng")		},
	{ _T("vstopgun"),			_T("topgun")		},
	{ _T("\0"), 				_T("\0")			}
};

static TCHAR *fbn_to_mame(TCHAR *name)
{
	TCHAR *game = name; // default to passed name

	// Check the above table to see if we should use an alias
	for (INT32 i = 0; replace_table[i].fb_name[0] != '\0'; i++) {
		if (!_tcscmp(replace_table[i].fb_name, name)) {
			game = replace_table[i].mame_name;
			break;
		}
	}

	return game;
}

static unsigned __stdcall DoShellExThread(void *arg)
{
	ShellExecute(NULL, _T("open"), (TCHAR*)arg, NULL, NULL, SW_SHOWNORMAL);

	return 0;
}

static void ViewEmma()
{
	HANDLE hThread = NULL;
	unsigned ThreadID = 0;
	static TCHAR szShellExURL[MAX_PATH];

	_stprintf(szShellExURL, _T("http://adb.arcadeitalia.net/dettaglio_mame.php?game_name=%s"), fbn_to_mame(BurnDrvGetText(DRV_NAME)));

	hThread = (HANDLE)_beginthreadex(NULL, 0, DoShellExThread, (void*)szShellExURL, 0, &ThreadID);
}

static void RebuildEverything()
{
	RefreshPanel();

	bDrvSelected = false;

	TreeBuilding = 1;
	SendMessage(hSelList, WM_SETREDRAW, (WPARAM)FALSE,(LPARAM)TVI_ROOT);	// disable redraw
	SendMessage(hSelList, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);				// Destory all nodes
	SelListMake();
	SendMessage(hSelList, WM_SETREDRAW, (WPARAM)TRUE, (LPARAM)TVI_ROOT);	// enable redraw

	// Clear the things in our Info-box
	for (int i = 0; i < 6; i++) {
		SendMessage(hInfoText[i], WM_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
		EnableWindow(hInfoLabel[i], FALSE);
	}

	TreeBuilding = 0;

	if (nSelActiveMode == 0) {
		RestoreCurrentDriverInRomList(hSelDlg);
		InvalidateRect(hSelList, NULL, FALSE);
		UpdateWindow(hSelList);
	}
}

static int TvhFilterToBitmask(HTREEITEM hHandle)
{
	if (hHandle == hRoot)				return (1 << 0);
	if (hHandle == hBoardType)			return (1 << 1);
	if (hHandle == hFamily)				return (1 << 2);
	if (hHandle == hGenre)				return (1 << 3);
	if (hHandle == hHardware)			return (1 << 4);
	if (hHandle == hFilterCapcomGrp)	return (1 << 5);
	if (hHandle == hFilterSegaGrp)		return (1 << 6);

	return 0;
}

static HTREEITEM TvBitTohFilter(int nBit)
{
	switch (nBit) {
		case (1 << 0): return hRoot;
		case (1 << 1): return hBoardType;
		case (1 << 2): return hFamily;
		case (1 << 3): return hGenre;
		case (1 << 4): return hHardware;
		case (1 << 5): return hFilterCapcomGrp;
		case (1 << 6): return hFilterSegaGrp;
	}

	return 0;
}

#define _TVCreateFiltersA(a, b, c, d)								\
{																	\
	TvItem.hParent = a;												\
	TvItem.item.pszText = FBALoadStringEx(hAppInst, b, true);		\
	c = TreeView_InsertItem(hFilterList, &TvItem);					\
	_TreeView_SetCheckState(hFilterList, c, (d) ? FALSE : TRUE);	\
}

#define _TVCreateFiltersB(a, b, c)								\
{																\
	TvItem.hParent = a;												\
	TvItem.item.pszText = FBALoadStringEx(hAppInst, b, true);		\
	c = TreeView_InsertItem(hFilterList, &TvItem);					\
}

#define _TVCreateFiltersC(a, b, c, d)								\
{																	\
	TvItem.hParent = a;												\
	TvItem.item.pszText = FBALoadStringEx(hAppInst, b, true);		\
	c = TreeView_InsertItem(hFilterList, &TvItem);					\
	_TreeView_SetCheckState(hFilterList, c, (d) ? FALSE : TRUE);	\
}

#define _TVCreateFiltersD(a, b, c, d)								\
{																	\
	TvItem.hParent = a;												\
	TvItem.item.pszText = FBALoadStringEx(hAppInst, b, true);		\
	c = TreeView_InsertItem(hFilterList, &TvItem);					\
	_TreeView_SetCheckState(hFilterList, c, (d) ? FALSE : TRUE);	\
}

static HTREEITEM CreateLiteralFilterItem(HTREEITEM hTreeParent, TCHAR* pszText, bool bHidden)
{
	TV_INSERTSTRUCT TvItem;
	memset(&TvItem, 0, sizeof(TvItem));
	TvItem.item.mask = TVIF_TEXT | TVIF_PARAM;
	TvItem.hInsertAfter = TVI_LAST;
	TvItem.hParent = hTreeParent;
	TvItem.item.pszText = pszText;

	HTREEITEM hItem = TreeView_InsertItem(hFilterList, &TvItem);
	_TreeView_SetCheckState(hFilterList, hItem, bHidden ? FALSE : TRUE);

	return hItem;
}

static void CreateFilters()
{
	TV_INSERTSTRUCT TvItem;
	memset(&TvItem, 0, sizeof(TvItem));

	hFilterList			= GetDlgItem(hSelDlg, IDC_TREE2);

	TvItem.item.mask	= TVIF_TEXT | TVIF_PARAM;
	TvItem.hInsertAfter = TVI_LAST;

	_TVCreateFiltersA(TVI_ROOT		, IDS_FAVORITES			, hFavorites            , !nLoadMenuFavoritesFilter                         );

	_TVCreateFiltersB(TVI_ROOT		, IDS_SEL_FILTERS		, hRoot						);

	_TVCreateFiltersC(hRoot			, IDS_SEL_BOARDTYPE		, hBoardType			, nLoadMenuBoardTypeFilter & MASKALLBOARD	);

	_TVCreateFiltersA(hBoardType	, IDS_SEL_GENUINE		, hFilterGenuine		, nLoadMenuBoardTypeFilter & MASKBOARDTYPEGENUINE	);
	_TVCreateFiltersA(hBoardType	, IDS_SEL_BOOTLEG		, hFilterBootleg		, nLoadMenuBoardTypeFilter & BDF_BOOTLEG			);
	_TVCreateFiltersA(hBoardType	, IDS_SEL_DEMO			, hFilterDemo			, nLoadMenuBoardTypeFilter & BDF_DEMO				);
	_TVCreateFiltersA(hBoardType	, IDS_SEL_HACK			, hFilterHack			, nLoadMenuBoardTypeFilter & BDF_HACK				);
	_TVCreateFiltersA(hBoardType	, IDS_SEL_HOMEBREW		, hFilterHomebrew		, nLoadMenuBoardTypeFilter & BDF_HOMEBREW			);
	_TVCreateFiltersA(hBoardType	, IDS_SEL_PROTOTYPE		, hFilterPrototype		, nLoadMenuBoardTypeFilter & BDF_PROTOTYPE			);

	_TVCreateFiltersC(hRoot			, IDS_FAMILY			, hFamily				, nLoadMenuFamilyFilter & MASKALLFAMILY	);

	_TVCreateFiltersA(hFamily		, IDS_FAMILY_19XX		, hFilter19xx			, nLoadMenuFamilyFilter & FBF_19XX					);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_DONPACHI	, hFilterDonpachi		, nLoadMenuFamilyFilter & FBF_DONPACHI				);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_SONICWI	, hFilterSonicwi		, nLoadMenuFamilyFilter & FBF_SONICWI				);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_DSTLK		, hFilterDstlk			, nLoadMenuFamilyFilter & FBF_DSTLK					);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_FATFURY	, hFilterFatfury		, nLoadMenuFamilyFilter & FBF_FATFURY				);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_KOF		, hFilterKof			, nLoadMenuFamilyFilter & FBF_KOF					);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_MSLUG		, hFilterMslug			, nLoadMenuFamilyFilter & FBF_MSLUG					);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_MAHOU		, hFilterMahou			, nLoadMenuFamilyFilter & FBF_MAHOU					);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_PWRINST	, hFilterPwrinst		, nLoadMenuFamilyFilter & FBF_PWRINST				);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_SAMSHO		, hFilterSamsho			, nLoadMenuFamilyFilter & FBF_SAMSHO				);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_SONIC		, hFilterSonic			, nLoadMenuFamilyFilter & FBF_SONIC					);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_SF			, hFilterSf				, nLoadMenuFamilyFilter & FBF_SF					);
	_TVCreateFiltersA(hFamily		, IDS_FAMILY_OTHER		, hFilterOtherFamily	, nLoadMenuFamilyFilter & MASKFAMILYOTHER			);

	_TVCreateFiltersC(hRoot			, IDS_GENRE				, hGenre				, nLoadMenuGenreFilter & MASKALLGENRE	);

	_TVCreateFiltersA(hGenre		, IDS_GENRE_ACTION		, hFilterAction			, nLoadMenuGenreFilter & GBF_ACTION					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_BALLPADDLE	, hFilterBallpaddle		, nLoadMenuGenreFilter & GBF_BALLPADDLE				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_BREAKOUT	, hFilterBreakout		, nLoadMenuGenreFilter & GBF_BREAKOUT				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_CASINO		, hFilterCasino			, nLoadMenuGenreFilter & GBF_CASINO					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_SCRFIGHT	, hFilterScrfight		, nLoadMenuGenreFilter & GBF_SCRFIGHT				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_VSFIGHT		, hFilterVsfight		, nLoadMenuGenreFilter & GBF_VSFIGHT				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_MAHJONG		, hFilterMahjong		, nLoadMenuGenreFilter & GBF_MAHJONG				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_MAZE		, hFilterMaze			, nLoadMenuGenreFilter & GBF_MAZE					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_MINIGAMES	, hFilterMinigames		, nLoadMenuGenreFilter & GBF_MINIGAMES				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_MISC		, hFilterMisc			, nLoadMenuGenreFilter & GBF_MISC					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_PINBALL		, hFilterPinball		, nLoadMenuGenreFilter & GBF_PINBALL				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_PLATFORM	, hFilterPlatform		, nLoadMenuGenreFilter & GBF_PLATFORM				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_PUZZLE		, hFilterPuzzle			, nLoadMenuGenreFilter & GBF_PUZZLE					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_QUIZ		, hFilterQuiz			, nLoadMenuGenreFilter & GBF_QUIZ					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_RACING		, hFilterRacing			, nLoadMenuGenreFilter & GBF_RACING					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_RUNGUN		, hFilterRungun			, nLoadMenuGenreFilter & GBF_RUNGUN 				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_SHOOT		, hFilterShoot			, nLoadMenuGenreFilter & GBF_SHOOT					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_MULTISHOOT	, hFilterMultiShoot		, nLoadMenuGenreFilter & GBF_MULTISHOOT				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_HORSHOOT	, hFilterHorshoot		, nLoadMenuGenreFilter & GBF_HORSHOOT				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_VERSHOOT	, hFilterVershoot		, nLoadMenuGenreFilter & GBF_VERSHOOT				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_SPORTSMISC	, hFilterSportsmisc		, nLoadMenuGenreFilter & GBF_SPORTSMISC				);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_SPORTSFOOTBALL, hFilterSportsfootball, nLoadMenuGenreFilter & GBF_SPORTSFOOTBALL		);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_STRATEGY	, hFilterStrategy   	, nLoadMenuGenreFilter & GBF_STRATEGY   			);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_RPG			, hFilterRpg   			, nLoadMenuGenreFilter & GBF_RPG   					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_SIM			, hFilterSim   			, nLoadMenuGenreFilter & GBF_SIM   					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_ADV			, hFilterAdv   			, nLoadMenuGenreFilter & GBF_ADV   					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_CARD		, hFilterCard   		, nLoadMenuGenreFilter & GBF_CARD					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_BOARD		, hFilterBoard   		, nLoadMenuGenreFilter & GBF_BOARD					);
	_TVCreateFiltersA(hGenre		, IDS_GENRE_BIOS		, hFilterBios			, nLoadMenuGenreFilter & GBF_BIOS					);

	_TVCreateFiltersC(hRoot			, IDS_SEL_HARDWARE		, hHardware				, nLoadMenuShowX & MASKALL					);

	_TVCreateFiltersD(hHardware		, IDS_SEL_CAPCOM_GRP	, hFilterCapcomGrp			, nLoadMenuShowX & MASKCAPGRP				);

	_TVCreateFiltersA(hFilterCapcomGrp	, IDS_SEL_CPS1			, hFilterCps1			, nLoadMenuShowX & MASKCPS							);
	_TVCreateFiltersA(hFilterCapcomGrp	, IDS_SEL_CPS2			, hFilterCps2			, nLoadMenuShowX & MASKCPS2							);
	_TVCreateFiltersA(hFilterCapcomGrp	, IDS_SEL_CPS3			, hFilterCps3			, nLoadMenuShowX & MASKCPS3							);
	_TVCreateFiltersA(hFilterCapcomGrp	, IDS_SEL_CAPCOM_MISC	, hFilterCapcomMisc		, nLoadMenuShowX & MASKCAPMISC						);

	_TVCreateFiltersA(hHardware		, IDS_SEL_CAVE			, hFilterCave			, nLoadMenuShowX & MASKCAVE							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_DATAEAST		, hFilterDataeast		, nLoadMenuShowX & MASKDATAEAST						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_GALAXIAN		, hFilterGalaxian		, nLoadMenuShowX & MASKGALAXIAN						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_IREM			, hFilterIrem			, nLoadMenuShowX & MASKIREM							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_KANEKO		, hFilterKaneko			, nLoadMenuShowX & MASKKANEKO						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_KONAMI		, hFilterKonami			, nLoadMenuShowX & MASKKONAMI						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_MIDWAY		, hFilterMidway			, nLoadMenuShowX & MASKMIDWAY						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_NEOGEO		, hFilterNeogeo			, nLoadMenuShowX & MASKNEOGEO						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_PACMAN		, hFilterPacman			, nLoadMenuShowX & MASKPACMAN						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_PGM			, hFilterPgm			, nLoadMenuShowX & MASKPGM							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_PGM2			, hFilterPgm2			, nLoadMenuShowX & MASKPGM2							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_IGS			, hFilterIgs			, nLoadMenuShowX & MASKIGS							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_SKELETON		, hFilterSkeleton		, nLoadMenuShowX & MASKSKELETON						);
	hFilterGba = CreateLiteralFilterItem(hHardware, _T("GB/A/C"), (nLoadMenuShowX & (MASKGBA | MASKGB | MASKGBC)) ? true : false);
	_TVCreateFiltersA(hHardware		, IDS_SEL_PSIKYO		, hFilterPsikyo			, nLoadMenuShowX & MASKPSIKYO						);

	_TVCreateFiltersD(hHardware		, IDS_SEL_SEGA_GRP		, hFilterSegaGrp				, nLoadMenuShowX & MASKSEGAGRP				);

	_TVCreateFiltersA(hFilterSegaGrp	, IDS_SEL_SG1000		, hFilterSg1000			, nLoadMenuShowX & MASKSG1000						);
	_TVCreateFiltersA(hFilterSegaGrp	, IDS_SEL_SMS			, hFilterSms			, nLoadMenuShowX & MASKSMS							);
	_TVCreateFiltersA(hFilterSegaGrp	, IDS_SEL_MEGADRIVE		, hFilterMegadrive		, nLoadMenuShowX & MASKMEGADRIVE					);
	_TVCreateFiltersA(hFilterSegaGrp	, IDS_SEL_GG			, hFilterGg				, nLoadMenuShowX & MASKGG							);
	_TVCreateFiltersA(hFilterSegaGrp	, IDS_SEL_SEGA			, hFilterSega			, nLoadMenuShowX & MASKSEGA							);

	hFilterAtomiswave = CreateLiteralFilterItem(hHardware, _T("Atomiswave"), (nLoadMenuShowX & MASKATOMISWAVE) ? true : false);
	hFilterNaomi = CreateLiteralFilterItem(hHardware, _T("Naomi"), (nLoadMenuShowX & MASKNAOMI) ? true : false);

	_TVCreateFiltersA(hHardware		, IDS_SEL_SETA			, hFilterSeta			, nLoadMenuShowX & MASKSETA							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_TAITO			, hFilterTaito			, nLoadMenuShowX & MASKTAITO						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_TECHNOS		, hFilterTechnos		, nLoadMenuShowX & MASKTECHNOS						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_TOAPLAN		, hFilterToaplan		, nLoadMenuShowX & MASKTOAPLAN						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_PCE			, hFilterPce			, nLoadMenuShowX & MASKPCENGINE						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_COLECO		, hFilterColeco			, nLoadMenuShowX & MASKCOLECO						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_MSX			, hFilterMsx			, nLoadMenuShowX & MASKMSX							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_SPECTRUM		, hFilterSpectrum		, nLoadMenuShowX & MASKSPECTRUM						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_NES			, hFilterNes			, nLoadMenuShowX & MASKNES							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_FDS			, hFilterFds			, nLoadMenuShowX & MASKFDS							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_SNES			, hFilterSnes			, nLoadMenuShowX & MASKSNES							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_NGP			, hFilterNgp			, nLoadMenuShowX & MASKNGP							);
	_TVCreateFiltersA(hHardware		, IDS_SEL_CHANNELF		, hFilterChannelF		, nLoadMenuShowX & MASKCHANNELF						);
	_TVCreateFiltersA(hHardware		, IDS_SEL_MISCPRE90S	, hFilterMiscPre90s		, nLoadMenuShowX & MASKMISCPRE90S					);
	_TVCreateFiltersA(hHardware		, IDS_SEL_MISCPOST90S	, hFilterMiscPost90s	, nLoadMenuShowX & MASKMISCPOST90S					);

	// restore expanded filter nodes
	for (INT32 i = 0; i < 16; i++)
	{
		if (nLoadMenuExpand & (1 << i))
			SendMessage(hFilterList, TVM_EXPAND, TVE_EXPAND, (LPARAM)TvBitTohFilter(1 << i));
	}
	//SendMessage(hFilterList	, TVM_EXPAND,TVE_EXPAND, (LPARAM)hRoot);
	//SendMessage(hFilterList	, TVM_EXPAND,TVE_EXPAND, (LPARAM)hHardware);
	//SendMessage(hFilterList	, TVM_EXPAND,TVE_EXPAND, (LPARAM)hFavorites);
	TreeView_SelectSetFirstVisible(hFilterList, hFavorites);
}

enum {
	ICON_MEGADRIVE,
	ICON_PCE,
	ICON_SGX,
	ICON_TG16,
	ICON_SG1000,
	ICON_COLECO,
	ICON_SMS,
	ICON_GG,
	ICON_MSX,
	ICON_SPECTRUM,
	ICON_NES,
	ICON_FDS,
	ICON_SNES,
	ICON_NGPC,
	ICON_NGP,
	ICON_CHANNELF,
	ICON_ENUMEND	// arcade
};

static HWND hIconDlg      = NULL;
static HANDLE hICThread   = NULL;	// IconsCache
static HANDLE hICEvent    = NULL;

static CRITICAL_SECTION cs;

static INT32 xClick, yClick;

static UINT32 __stdcall CacheDrvIconsProc(void* lpParam)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	HICON* pCache = (HICON*)lpParam;
	TCHAR szIcon[MAX_PATH] = { 0 };

	switch (nIconsSize) {
		case ICON_16x16: nIconsSizeXY = 16;	nIconsYDiff =  2;	break;
		case ICON_24x24: nIconsSizeXY = 24;	nIconsYDiff =  6;	break;
		case ICON_32x32: nIconsSizeXY = 32;	nIconsYDiff = 10;	break;
	}

	const UINT32 nDrvCount = nBurnDrvCount;
	const UINT32 nAllCount = nDrvCount + ICON_ENUMEND + 1;

	for (UINT32 nDrvIndex = 0; nDrvIndex < nAllCount; nDrvIndex++) {
		// See if we need to abort
		if (WaitForSingleObject(hICEvent, 0) == WAIT_OBJECT_0) {
			ExitThread(0);
		}

		SendDlgItemMessage(hIconDlg, IDC_WAIT_PROG, PBM_STEPIT, 0, 0);

		// By games
		if (nDrvIndex < nDrvCount) {
			// Occasional anomaly in debugging, suspected resource contention
			EnterCriticalSection(&cs);
			const UINT32 nBackup  = nBurnDrvActive;

			// Prevents nBurnDrvActive from being modified externally under certain circumstances
			nBurnDrvActive        = nDrvIndex;

			const INT32 nFlag     = BurnDrvGetFlags();
			const char* pszParent = BurnDrvGetTextA(DRV_PARENT);
			const TCHAR* pszName  = BurnDrvGetText(DRV_NAME);

			// Now we can safely restore the data (if modified)
			nBurnDrvActive        = nBackup;
			LeaveCriticalSection(&cs);

			// GDI limits the number of objects and does not cache Clone.
			if ((NULL != pszParent) && (nFlag & BDF_CLONE)) {
				pCache[nDrvIndex] = NULL; continue;
			}

			_stprintf(szIcon, _T("%s%s.ico"), szAppIconsPath, pszName);
			pCache[nDrvIndex] = (HICON)LoadImage(NULL, szIcon, IMAGE_ICON, nIconsSizeXY, nIconsSizeXY, LR_LOADFROMFILE | LR_SHARED);
		}
		// By hardware
		// The start of the hardwares icon is immediately after the end of the games icon
		else {
			const TCHAR* szConsIcon[] = {
				_T("icon_md"),
				_T("icon_pce"),
				_T("icon_sgx"),
				_T("icon_tg"),
				_T("icon_sg1k"),
				_T("icon_cv"),
				_T("icon_sms"),
				_T("icon_gg"),
				_T("icon_msx"),
				_T("icon_spec"),
				_T("icon_nes"),
				_T("icon_fds"),
				_T("icon_snes"),
				_T("icon_ngpc"),
				_T("icon_ngp"),
				_T("icon_chf"),
				_T("icon_gbc"),
				_T("icon_gba"),
				_T("icon_gb"),
				_T("icon_arc")
			};

			const INT32 nConsIndex = nDrvIndex - nDrvCount;

			_stprintf(szIcon, _T("%s%s.ico"), szAppIconsPath, szConsIcon[nConsIndex]);
			pCache[nDrvIndex] = (HICON)LoadImage(NULL, szIcon, IMAGE_ICON, nIconsSizeXY, nIconsSizeXY, LR_LOADFROMFILE | LR_SHARED);
		}
	}

	PostMessage(hIconDlg, WM_CLOSE, 0, 0);
	return 0;
}

static void IconsCacheThreadExit()
{
	DWORD dwExitCode = 0;
	GetExitCodeThread(hICThread, &dwExitCode);

	if (dwExitCode == STILL_ACTIVE) {

		// Signal the scan thread to abort
		SetEvent(hICEvent);

		// Wait for the thread to finish
		if (WaitForSingleObject(hICThread, 10000) != WAIT_OBJECT_0) {
			// If the thread doesn't finish within 10 seconds, forcibly kill it
			TerminateThread(hICThread, 1);
		}
	}

	DeleteCriticalSection(&cs);
	CloseHandle(hICThread); hICThread = NULL;
	CloseHandle(hICEvent);  hICEvent  = NULL;
	dwExitCode = 0;
}

static INT_PTR CALLBACK CacheDrvIconsWaitProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)		// LPARAM lParam
{
	switch (Msg) {
		case WM_INITDIALOG: {
			hIconDlg = hDlg;
			SendDlgItemMessage(hDlg, IDC_WAIT_PROG, PBM_SETRANGE, 0, MAKELPARAM(0, nBurnDrvCount + ICON_ENUMEND + 1));
			SendDlgItemMessage(hDlg, IDC_WAIT_PROG, PBM_SETSTEP, (WPARAM)1, 0);

			ShowWindow( GetDlgItem(hDlg, IDC_WAIT_LABEL_A), TRUE);
			SendMessage(GetDlgItem(hDlg, IDC_WAIT_LABEL_A), WM_SETTEXT, (WPARAM)0, (LPARAM)FBALoadStringEx(hAppInst, IDS_CACHING_ICONS, true));
			ShowWindow( GetDlgItem(hDlg, IDCANCEL),         TRUE);

			hICThread = (HANDLE)_beginthreadex(NULL, 0, CacheDrvIconsProc, pIconsCache, 0, NULL);
			hICEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

			WndInMid(hDlg, hParent);
			SetFocus(hDlg);	// Enable Esc=close
			break;
		}

		case WM_LBUTTONDOWN: {
			SetCapture(hDlg);

			xClick = GET_X_LPARAM(lParam);
			yClick = GET_Y_LPARAM(lParam);
			break;
		}

		case WM_LBUTTONUP: {
			ReleaseCapture();
			break;
		}

		case WM_MOUSEMOVE: {
			if (GetCapture() == hDlg) {
				RECT rcWindow;
				GetWindowRect(hDlg, &rcWindow);

				INT32 xMouse = GET_X_LPARAM(lParam);
				INT32 yMouse = GET_Y_LPARAM(lParam);
				INT32 xWindow = rcWindow.left + xMouse - xClick;
				INT32 yWindow = rcWindow.top  + yMouse - yClick;

				SetWindowPos(hDlg, NULL, xWindow, yWindow, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			}
			break;
		}

		case WM_COMMAND: {
			if (LOWORD(wParam) == IDCANCEL) {
				PostMessage(hDlg, WM_CLOSE, 0, 0);
			}
			break;
		}

		case WM_CLOSE: {
			IconsCacheThreadExit();
			EndDialog(hDlg, 0);
			hIconDlg = hParent = NULL;
			LoadDrvIcons();
		}
	}

	return 0;
}

void DestroyDrvIconsCache()
{
	if (NULL == pIconsCache) return;

	for (UINT32 i = 0; i < (nBurnDrvCount + ICON_ENUMEND + 1); i++) {
		if (NULL == pIconsCache[i]) continue;	// LoadImage failed and returned NULL.
		DestroyIcon(pIconsCache[i]);
	}
	free(pIconsCache); pIconsCache = NULL;
	nIconsSizeXY = 16; nIconsYDiff = 2;
}

void CreateDrvIconsCache()
{
	if (!bEnableIcons) {
//		nIconsSize   = ICON_16x16;
		nIconsSizeXY = 16;
		nIconsYDiff  = 2;
		return;
	}

	if (NULL != pIconsCache) DestroyDrvIconsCache();
	pIconsCache = (HICON*)malloc((nBurnDrvCount + ICON_ENUMEND + 1) * sizeof(HICON));

	InitializeCriticalSection(&cs);
	FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_WAIT), hParent, (DLGPROC)CacheDrvIconsWaitProc);
}

void LoadDrvIcons()
{
	if (!bEnableIcons) return;

	bIconsLoaded = 0;

	if (NULL == hDrvIcon) {
		hDrvIcon = (HICON*)malloc((nBurnDrvCount + ICON_ENUMEND + 1) * sizeof(HICON));
	}

	const UINT32 nDrvCount = nBurnDrvCount;

	for (UINT32 nDrvIndex = 0; nDrvIndex < nDrvCount; nDrvIndex++) {
		const UINT32 nBackup = nBurnDrvActive;
		nBurnDrvActive       = nDrvIndex;
		const INT32 nFlag    = BurnDrvGetFlags();
		const INT32 nCode    = BurnDrvGetHardwareCode();
		const TCHAR* pszName = BurnDrvGetText(DRV_NAME);
		char* pszParent      = BurnDrvGetTextA(DRV_PARENT);
		nBurnDrvActive       = nBackup;

		// Skip Clone when only the parent item is selected nBurnDrvCount + ICON_ENUMEND
		if (bIconsOnlyParents && (NULL != pszParent) && (nFlag & BDF_CLONE)) {
			hDrvIcon[nDrvIndex] = NULL;											continue;
		}

		// By hardwares
		if (bIconsByHardwares) {
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_SEGA_MEGADRIVE) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_MEGADRIVE];	continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_PCENGINE_PCENGINE) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_PCE];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_PCENGINE_TG16) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_TG16];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_PCENGINE_SGX) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_SGX];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_SEGA_SG1000) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_SG1000];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_COLECO) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_COLECO];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_SEGA_MASTER_SYSTEM) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_SMS];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_SEGA_GAME_GEAR) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_GG];			continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_MSX) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_MSX];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_SPECTRUM) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_SPECTRUM];	continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_NES) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_NES];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_FDS) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_FDS];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_SNES) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_SNES];		continue;
			}
			else
			if ((nCode & HARDWARE_SNK_NGPC)    == HARDWARE_SNK_NGPC) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_NGPC];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_SNK_NGP) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_NGP];		continue;
			}
			else
			if ((nCode & HARDWARE_PUBLIC_MASK) == HARDWARE_CHANNELF) {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_CHANNELF];	continue;
			}
			else {
				hDrvIcon[nDrvIndex] = pIconsCache[nDrvCount + ICON_ENUMEND];	continue;
			}
		}
		// By games
		else {
			// When allowed and Clone is checked, loads the icon of the parent item when checking that the icon file does not exist
			if ((NULL != pszParent) && (nFlag & BDF_CLONE)) {
				TCHAR szIcon[MAX_PATH] = { 0 };
				_stprintf(szIcon, _T("%s%s.ico"), szAppIconsPath, pszName);

				// The icon file exists, and given the GDI cap, now is not the time to deal with it
				if (GetFileAttributes(szIcon) != INVALID_FILE_ATTRIBUTES) {
					// Must be NULL or it will be recognized as having an icon and ignored in message processing
					hDrvIcon[nDrvIndex] = NULL;									continue;
				}
				INT32 nParentDrv = BurnDrvGetIndex(pszParent);

				// Clone icon file does not exist, use parent item icon
				// Icons are reused and do not take up GDI resources
				hDrvIcon[nDrvIndex] = pIconsCache[nParentDrv];					continue;
			}
			// Associate all non-Clone icons
			hDrvIcon[nDrvIndex] = pIconsCache[nDrvIndex];
		}
	}

	bIconsLoaded = 1;
}

void UnloadDrvIcons()
{
	free(hDrvIcon); hDrvIcon = NULL;
}

#define UM_CHECKSTATECHANGE (WM_USER + 100)
#define UM_CLOSE			(WM_USER + 101)

#define _ToggleGameListing(nShowX, nMASK)													\
{																							\
	nShowX ^= nMASK;																		\
	_TreeView_SetCheckState(hFilterList, hItemChanged, (nShowX & nMASK) ? FALSE : TRUE);	\
}

static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_INITDIALOG) {
		hSelDlg = hDlg;
		for (INT32 i = 0; i < 3; i++) {
			hSelModeTitle[i] = NULL;
		}
		nSelActiveMode = 0;
		hSelIpsTree = NULL;
		hSelIpsDesc = NULL;
		hSelIpsLang = NULL;
		hSelIpsClear = NULL;

		// Keep the selector at a fixed size while we refine the merged layout.
		{
			LONG_PTR nStyle = GetWindowLongPtr(hSelDlg, GWL_STYLE);
			nStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
			SetWindowLongPtr(hSelDlg, GWL_STYLE, nStyle);
			SetWindowPos(hSelDlg, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}

		SendDlgItemMessage(hDlg, IDC_SCREENSHOT_H, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
		SendDlgItemMessage(hDlg, IDC_SCREENSHOT_V, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);

		SendDlgItemMessage(hDlg, IDC_SCREENSHOT2_H, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
		SendDlgItemMessage(hDlg, IDC_SCREENSHOT2_V, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);

		hWhiteBGBrush	= CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

		hExpand			= (HICON)LoadImage(hAppInst, MAKEINTRESOURCE(IDI_TV_PLUS),			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		hCollapse		= (HICON)LoadImage(hAppInst, MAKEINTRESOURCE(IDI_TV_MINUS),			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

		hNotWorking		= (HICON)LoadImage(hAppInst, MAKEINTRESOURCE(IDI_TV_NOTWORKING),	IMAGE_ICON, nIconsSizeXY, nIconsSizeXY, LR_DEFAULTCOLOR);
		hNotFoundEss	= (HICON)LoadImage(hAppInst, MAKEINTRESOURCE(IDI_TV_NOTFOUND_ESS),	IMAGE_ICON, nIconsSizeXY, nIconsSizeXY, LR_DEFAULTCOLOR);
		hNotFoundNonEss = (HICON)LoadImage(hAppInst, MAKEINTRESOURCE(IDI_TV_NOTFOUND_NON),	IMAGE_ICON, nIconsSizeXY, nIconsSizeXY, LR_DEFAULTCOLOR);

		hDrvIconMiss	= (HICON)LoadImage(hAppInst, MAKEINTRESOURCE(IDI_APP),	IMAGE_ICON, nIconsSizeXY, nIconsSizeXY, LR_DEFAULTCOLOR);

		TCHAR szOldTitle[1024] = _T(""), szNewTitle[1024] = _T("");
		GetWindowText(hSelDlg, szOldTitle, 1024);
		_sntprintf(szNewTitle, 1024, _T(APP_TITLE) _T(SEPERATOR_1) _T("%s"), szOldTitle);
		SetWindowText(hSelDlg, szNewTitle);

		hSelList		= GetDlgItem(hSelDlg, IDC_TREE1);

		hInfoLabel[0]	= GetDlgItem(hSelDlg, IDC_LABELROMNAME);
		hInfoLabel[1]	= GetDlgItem(hSelDlg, IDC_LABELROMINFO);
		hInfoLabel[2]	= GetDlgItem(hSelDlg, IDC_LABELSYSTEM);
		hInfoLabel[3]	= GetDlgItem(hSelDlg, IDC_LABELCOMMENT);
		hInfoLabel[4]	= GetDlgItem(hSelDlg, IDC_LABELNOTES);
		hInfoLabel[5]	= GetDlgItem(hSelDlg, IDC_LABELGENRE);
		hInfoText[0]	= GetDlgItem(hSelDlg, IDC_TEXTROMNAME);
		hInfoText[1]	= GetDlgItem(hSelDlg, IDC_TEXTROMINFO);
		hInfoText[2]	= GetDlgItem(hSelDlg, IDC_TEXTSYSTEM);
		hInfoText[3]	= GetDlgItem(hSelDlg, IDC_TEXTCOMMENT);
		hInfoText[4]	= GetDlgItem(hSelDlg, IDC_TEXTNOTES);
		hInfoText[5]	= GetDlgItem(hSelDlg, IDC_TEXTGENRE);

#if !defined _UNICODE
		EnableWindow(GetDlgItem(hDlg, IDC_SEL_ASCIIONLY), FALSE);
#endif

#if defined (_UNICODE)
		SetDlgItemTextW(hSelDlg, IDC_SEL_DISABLECRC, L"\x7981\x7528" L"CRC");
#else
		SetDlgItemTextA(hSelDlg, IDC_SEL_DISABLECRC, "Disable CRC");
#endif

		bSearchStringInit = true; // so we don't set off the search timer during init w/SetDlgItemText() below
		SetDlgItemText(hSelDlg, IDC_SEL_SEARCH, szSearchString); // Re-populate the search text
		ApplyMergedSelectDialogCaptions(hSelDlg);
		EnsureSelectModeTitles(hSelDlg);
		{
			RECT rcWindow = { 0 };
			GetWindowRect(hDlg, &rcWindow);
			if ((rcWindow.right - rcWindow.left) > 0 && (rcWindow.bottom - rcWindow.top) > 0) {
				nSelDlgWidth = rcWindow.right - rcWindow.left;
				nSelDlgHeight = rcWindow.bottom - rcWindow.top;
			}
		}
		SetWindowPos(hDlg, NULL, 0, 0, nSelDlgWidth, nSelDlgHeight, SWP_NOZORDER | SWP_NOMOVE);
		GetInitialPositions();
		LayoutMergedSelectDialog(hSelDlg);
		{
			RECT rcWindow = { 0 };
			GetWindowRect(hDlg, &rcWindow);
			nSelMinTrackWidth = rcWindow.right - rcWindow.left;
			nSelMinTrackHeight = rcWindow.bottom - rcWindow.top;
		}

		bool bFoundROMs = false;
		for (unsigned int i = 0; i < nBurnDrvCount; i++) {
			if (gameAv[i]) {
				bFoundROMs = true;
				break;
			}
		}
		if (!bFoundROMs && bSkipStartupCheck == false) {
			RomsDirCreate(hSelDlg);
		}

		// Icons size related -----------------------------------------
		SHORT cyItem = nIconsSizeXY + 4;								// height (in pixels) of each item on the TreeView list
		TreeView_SetItemHeight(hSelList, cyItem);

		SetFocus(hSelList);
		RebuildEverything();

		TreeView_SetItemHeight(hSelList, cyItem);

		if (nDialogSelect == -1) nDialogSelect = nOldDlgSelected;
		if (nDialogSelect > -1) {
			for (unsigned int i = 0; i < nTmpDrvCount; i++) {
				if (nBurnDrv[i].nBurnDrvNo == nDialogSelect) {
					nBurnDrvActive	= nBurnDrv[i].nBurnDrvNo;
					TreeView_EnsureVisible(hSelList, nBurnDrv[i].hTreeHandle);
					TreeView_Select(hSelList, nBurnDrv[i].hTreeHandle, TVGN_CARET);
					TreeView_SelectSetFirstVisible(hSelList, nBurnDrv[i].hTreeHandle);
					break;
				}
			}

			// hack to load the preview image after a delay
			nInitPreviewTimer = SetTimer(hSelDlg, 1, 20, InitPreviewTimerProc);
		}

		LONG_PTR Style;
		Style = GetWindowLongPtr (GetDlgItem(hSelDlg, IDC_TREE2), GWL_STYLE);
		Style |= TVS_CHECKBOXES;
		SetWindowLongPtr (GetDlgItem(hSelDlg, IDC_TREE2), GWL_STYLE, Style);

		CreateFilters();

		EnableWindow(GetDlgItem(hDlg, IDC_SEL_APPLYIPS), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_SEL_IPSMANAGER), FALSE);
		IpsPatchExit();	// bDoIpsPatch = false;

		WndInMid(hDlg, hParent);

		HICON hIcon = LoadIcon(hAppInst, MAKEINTRESOURCE(IDI_APP));
		SendMessage(hSelDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);		// Set the Game Selection dialog icon.

		LayoutMergedSelectDialog(hDlg);
		UpdateSelectModeAvailability(hDlg);
		if (!nShowMVSCartsOnly) {
			if (nSelRememberedMode == 2) {
				EnterSelectRomDataMode(hDlg);
			} else if (nSelRememberedMode == 1 && SelectIpsAvailable()) {
				EnterSelectIpsMode(hDlg);
			}
		}
		if (nSelActiveMode == 0 && !bDrvSelected) {
			// Refresh the ROM-mode placeholder after the final startup layout so
			// it uses the same preview size as images loaded after selection.
			RefreshPanel();
		}

		WndInMid(hDlg, hParent);

		return TRUE;
	}

	if(Msg == UM_CHECKSTATECHANGE) {
		HTREEITEM hItemChanged = (HTREEITEM)lParam;

		if (hItemChanged == hHardware) {
			if ((nLoadMenuShowX & MASKALL) == 0) {
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);

				_TreeView_SetCheckState(hFilterList, hFilterSegaGrp, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCapcomGrp, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCapcomMisc, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCave, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCps1, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCps2, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCps3, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterDataeast, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterGalaxian, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterIrem, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterKaneko, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterKonami, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterNeogeo, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPacman, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPgm, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPgm2, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterIgs, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSkeleton, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterGba, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPsikyo, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSega, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSeta, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterTaito, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterTechnos, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterToaplan, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMiscPre90s, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMiscPost90s, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterAtomiswave, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterNaomi, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMegadrive, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPce, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSms, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterGg, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSg1000, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterColeco, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMsx, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSpectrum, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterNes, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterFds, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSnes, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterNgp, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterChannelF, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMidway, FALSE);

				nLoadMenuShowX |= MASKALL;
			} else {
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);

				_TreeView_SetCheckState(hFilterList, hFilterSegaGrp, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCapcomGrp, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCapcomMisc, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCave, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCps1, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCps2, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCps3, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterDataeast, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterGalaxian, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterIrem, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterKaneko, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterKonami, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterNeogeo, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPacman, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPgm, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPgm2, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterIgs, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSkeleton, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterGba, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPsikyo, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSega, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSeta, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterTaito, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterTechnos, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterToaplan, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMiscPre90s, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMiscPost90s, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterAtomiswave, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterNaomi, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMegadrive, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPce, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSms, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterGg, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSg1000, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterColeco, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMsx, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSpectrum, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterNes, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterFds, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSnes, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterNgp, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterChannelF, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMidway, TRUE);

				nLoadMenuShowX &= ~MASKALL; //0xf8000000; make this dynamic for future hardware additions -dink
			}
		}

		if (hItemChanged == hBoardType) {
			if ((nLoadMenuBoardTypeFilter & MASKALLBOARD) == 0) {
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);

				_TreeView_SetCheckState(hFilterList, hFilterBootleg, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterDemo, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterHack, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterHomebrew, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPrototype, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterGenuine, FALSE);

				nLoadMenuBoardTypeFilter = MASKALLBOARD;
			} else {
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);

				_TreeView_SetCheckState(hFilterList, hFilterBootleg, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterDemo, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterHack, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterHomebrew, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPrototype, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterGenuine, TRUE);

				nLoadMenuBoardTypeFilter = 0;
			}
		}

		if (hItemChanged == hFamily) {
			if ((nLoadMenuFamilyFilter & MASKALLFAMILY) == 0) {
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);

				_TreeView_SetCheckState(hFilterList, hFilterOtherFamily, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMslug, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSf, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterKof, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterDstlk, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterFatfury, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSamsho, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilter19xx, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSonicwi, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPwrinst, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSonic, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterDonpachi, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMahou, FALSE);

				nLoadMenuFamilyFilter = MASKALLFAMILY;
			} else {
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);

				_TreeView_SetCheckState(hFilterList, hFilterOtherFamily, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMslug, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSf, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterKof, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterDstlk, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterFatfury, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSamsho, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilter19xx, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSonicwi, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPwrinst, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSonic, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterDonpachi, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMahou, TRUE);

				nLoadMenuFamilyFilter = 0;
			}
		}

		if (hItemChanged == hFavorites) {
			if (nLoadMenuFavoritesFilter) {
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);

				nLoadMenuFavoritesFilter = 0;
			} else {
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);

				nLoadMenuFavoritesFilter = 0xff;
			}
		}

		if (hItemChanged == hFilterSegaGrp) {
			if ((nLoadMenuShowX & MASKSEGAGRP) == 0) {
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);

				_TreeView_SetCheckState(hFilterList, hFilterSega, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSg1000, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSms, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMegadrive, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterGg, FALSE);

				nLoadMenuShowX &= ~MASKSEGAGRP;
			} else {
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);


				_TreeView_SetCheckState(hFilterList, hFilterSega, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSg1000, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSms, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMegadrive, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterGg, TRUE);

				nLoadMenuShowX |= MASKSEGAGRP;
			}
		}

		if (hItemChanged == hFilterCapcomGrp) {
			if ((nLoadMenuShowX & MASKCAPGRP) == 0) {
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);

				_TreeView_SetCheckState(hFilterList, hFilterCapcomMisc, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCps1, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCps2, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCps3, FALSE);

				nLoadMenuShowX &= ~MASKCAPGRP;
			} else {
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);

				_TreeView_SetCheckState(hFilterList, hFilterCapcomMisc, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCps1, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCps2, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCps3, TRUE);

				nLoadMenuShowX |= MASKCAPGRP;
			}
		}

		if (hItemChanged == hGenre) {
			if ((nLoadMenuGenreFilter & MASKALLGENRE) == 0) {
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);

				_TreeView_SetCheckState(hFilterList, hFilterHorshoot, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterVershoot, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterScrfight, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterVsfight, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterBios, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterBreakout, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCasino, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterBallpaddle, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMaze, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMinigames, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPinball, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPlatform, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterPuzzle, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterQuiz, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSportsmisc, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSportsfootball, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMisc, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMahjong, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterRacing, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterShoot, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterMultiShoot, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterAction, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterRungun, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterStrategy, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterRpg, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterSim, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterAdv, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterCard, FALSE);
				_TreeView_SetCheckState(hFilterList, hFilterBoard, FALSE);

				nLoadMenuGenreFilter = MASKALLGENRE;
			} else {
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);

				_TreeView_SetCheckState(hFilterList, hFilterHorshoot, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterVershoot, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterScrfight, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterVsfight, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterBios, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterBreakout, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCasino, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterBallpaddle, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMaze, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMinigames, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPinball, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPlatform, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterPuzzle, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterQuiz, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSportsmisc, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSportsfootball, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMisc, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMahjong, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterRacing, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterShoot, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterMultiShoot, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterAction, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterRungun, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterStrategy, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterRpg, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterSim, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterAdv, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterCard, TRUE);
				_TreeView_SetCheckState(hFilterList, hFilterBoard, TRUE);

				nLoadMenuGenreFilter = 0;
			}
		}

		if (hItemChanged == hFilterCapcomGrp)		_ToggleGameListing(nLoadMenuShowX, MASKCAPMISC);
		if (hItemChanged == hFilterCapcomGrp)		_ToggleGameListing(nLoadMenuShowX, MASKCPS);
		if (hItemChanged == hFilterCapcomGrp)		_ToggleGameListing(nLoadMenuShowX, MASKCPS2);
		if (hItemChanged == hFilterCapcomGrp)		_ToggleGameListing(nLoadMenuShowX, MASKCPS3);

		if (hItemChanged == hFilterSegaGrp)			_ToggleGameListing(nLoadMenuShowX, MASKSG1000);
		if (hItemChanged == hFilterSegaGrp)			_ToggleGameListing(nLoadMenuShowX, MASKSMS);
		if (hItemChanged == hFilterSegaGrp)			_ToggleGameListing(nLoadMenuShowX, MASKMEGADRIVE);
		if (hItemChanged == hFilterSegaGrp)			_ToggleGameListing(nLoadMenuShowX, MASKGG);
		if (hItemChanged == hFilterSegaGrp)			_ToggleGameListing(nLoadMenuShowX, MASKSEGA);

		if (hItemChanged == hFilterCapcomMisc)		_ToggleGameListing(nLoadMenuShowX, MASKCAPMISC);
		if (hItemChanged == hFilterCave)			_ToggleGameListing(nLoadMenuShowX, MASKCAVE);
		if (hItemChanged == hFilterCps1)			_ToggleGameListing(nLoadMenuShowX, MASKCPS);
		if (hItemChanged == hFilterCps2)			_ToggleGameListing(nLoadMenuShowX, MASKCPS2);
		if (hItemChanged == hFilterCps3)			_ToggleGameListing(nLoadMenuShowX, MASKCPS3);
		if (hItemChanged == hFilterDataeast)		_ToggleGameListing(nLoadMenuShowX, MASKDATAEAST);
		if (hItemChanged == hFilterGalaxian)		_ToggleGameListing(nLoadMenuShowX, MASKGALAXIAN);
		if (hItemChanged == hFilterIrem)			_ToggleGameListing(nLoadMenuShowX, MASKIREM);
		if (hItemChanged == hFilterKaneko)			_ToggleGameListing(nLoadMenuShowX, MASKKANEKO);
		if (hItemChanged == hFilterKonami)			_ToggleGameListing(nLoadMenuShowX, MASKKONAMI);
		if (hItemChanged == hFilterNeogeo)			_ToggleGameListing(nLoadMenuShowX, MASKNEOGEO);
		if (hItemChanged == hFilterPacman)			_ToggleGameListing(nLoadMenuShowX, MASKPACMAN);
		if (hItemChanged == hFilterPgm)				_ToggleGameListing(nLoadMenuShowX, MASKPGM);
		if (hItemChanged == hFilterPgm2)			_ToggleGameListing(nLoadMenuShowX, MASKPGM2);
		if (hItemChanged == hFilterIgs)				_ToggleGameListing(nLoadMenuShowX, MASKIGS);
		if (hItemChanged == hFilterSkeleton)		_ToggleGameListing(nLoadMenuShowX, MASKSKELETON);
		if (hItemChanged == hFilterGba) {
			UINT64 nGbMask = MASKGBA | MASKGB | MASKGBC;
			if (nLoadMenuShowX & nGbMask) {
				nLoadMenuShowX &= ~nGbMask;
				_TreeView_SetCheckState(hFilterList, hItemChanged, TRUE);
			} else {
				nLoadMenuShowX |= nGbMask;
				_TreeView_SetCheckState(hFilterList, hItemChanged, FALSE);
			}
		}
		if (hItemChanged == hFilterPsikyo)			_ToggleGameListing(nLoadMenuShowX, MASKPSIKYO);
		if (hItemChanged == hFilterSega)			_ToggleGameListing(nLoadMenuShowX, MASKSEGA);
		if (hItemChanged == hFilterSeta)			_ToggleGameListing(nLoadMenuShowX, MASKSETA);
		if (hItemChanged == hFilterTaito)			_ToggleGameListing(nLoadMenuShowX, MASKTAITO);
		if (hItemChanged == hFilterTechnos)			_ToggleGameListing(nLoadMenuShowX, MASKTECHNOS);
		if (hItemChanged == hFilterToaplan)			_ToggleGameListing(nLoadMenuShowX, MASKTOAPLAN);
		if (hItemChanged == hFilterMiscPre90s)		_ToggleGameListing(nLoadMenuShowX, MASKMISCPRE90S);
		if (hItemChanged == hFilterMiscPost90s)		_ToggleGameListing(nLoadMenuShowX, MASKMISCPOST90S);
		if (hItemChanged == hFilterAtomiswave)		_ToggleGameListing(nLoadMenuShowX, MASKATOMISWAVE);
		if (hItemChanged == hFilterNaomi)			_ToggleGameListing(nLoadMenuShowX, MASKNAOMI);
		if (hItemChanged == hFilterMegadrive)		_ToggleGameListing(nLoadMenuShowX, MASKMEGADRIVE);
		if (hItemChanged == hFilterPce)				_ToggleGameListing(nLoadMenuShowX, MASKPCENGINE);
		if (hItemChanged == hFilterSms)				_ToggleGameListing(nLoadMenuShowX, MASKSMS);
		if (hItemChanged == hFilterGg)				_ToggleGameListing(nLoadMenuShowX, MASKGG);
		if (hItemChanged == hFilterSg1000)			_ToggleGameListing(nLoadMenuShowX, MASKSG1000);
		if (hItemChanged == hFilterColeco)			_ToggleGameListing(nLoadMenuShowX, MASKCOLECO);
		if (hItemChanged == hFilterMsx)				_ToggleGameListing(nLoadMenuShowX, MASKMSX);
		if (hItemChanged == hFilterSpectrum)		_ToggleGameListing(nLoadMenuShowX, MASKSPECTRUM);
		if (hItemChanged == hFilterNes)				_ToggleGameListing(nLoadMenuShowX, MASKNES);
		if (hItemChanged == hFilterFds)				_ToggleGameListing(nLoadMenuShowX, MASKFDS);
		if (hItemChanged == hFilterSnes)			_ToggleGameListing(nLoadMenuShowX, MASKSNES);
		if (hItemChanged == hFilterNgp)				_ToggleGameListing(nLoadMenuShowX, MASKNGP);
		if (hItemChanged == hFilterChannelF)		_ToggleGameListing(nLoadMenuShowX, MASKCHANNELF);
		if (hItemChanged == hFilterMidway)			_ToggleGameListing(nLoadMenuShowX, MASKMIDWAY);

		if (hItemChanged == hFilterBootleg)			_ToggleGameListing(nLoadMenuBoardTypeFilter, BDF_BOOTLEG);
		if (hItemChanged == hFilterDemo)			_ToggleGameListing(nLoadMenuBoardTypeFilter, BDF_DEMO);
		if (hItemChanged == hFilterHack)			_ToggleGameListing(nLoadMenuBoardTypeFilter, BDF_HACK);
		if (hItemChanged == hFilterHomebrew)		_ToggleGameListing(nLoadMenuBoardTypeFilter, BDF_HOMEBREW);
		if (hItemChanged == hFilterPrototype)		_ToggleGameListing(nLoadMenuBoardTypeFilter, BDF_PROTOTYPE);
		if (hItemChanged == hFilterGenuine)			_ToggleGameListing(nLoadMenuBoardTypeFilter, MASKBOARDTYPEGENUINE);

		if (hItemChanged == hFilterOtherFamily)		_ToggleGameListing(nLoadMenuFamilyFilter, MASKFAMILYOTHER);
		if (hItemChanged == hFilterMahou)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_MAHOU);
		if (hItemChanged == hFilterDonpachi)		_ToggleGameListing(nLoadMenuFamilyFilter, FBF_DONPACHI);
		if (hItemChanged == hFilterMslug)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_MSLUG);
		if (hItemChanged == hFilterSf)				_ToggleGameListing(nLoadMenuFamilyFilter, FBF_SF);
		if (hItemChanged == hFilterKof)				_ToggleGameListing(nLoadMenuFamilyFilter, FBF_KOF);
		if (hItemChanged == hFilterDstlk)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_DSTLK);
		if (hItemChanged == hFilterFatfury)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_FATFURY);
		if (hItemChanged == hFilterSamsho)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_SAMSHO);
		if (hItemChanged == hFilter19xx)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_19XX);
		if (hItemChanged == hFilterSonicwi)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_SONICWI);
		if (hItemChanged == hFilterPwrinst)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_PWRINST);
		if (hItemChanged == hFilterSonic)			_ToggleGameListing(nLoadMenuFamilyFilter, FBF_SONIC);

		if (hItemChanged == hFilterHorshoot)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_HORSHOOT);
		if (hItemChanged == hFilterVershoot)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_VERSHOOT);
		if (hItemChanged == hFilterScrfight)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_SCRFIGHT);
		if (hItemChanged == hFilterVsfight)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_VSFIGHT);
		if (hItemChanged == hFilterBios)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_BIOS);
		if (hItemChanged == hFilterBreakout)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_BREAKOUT);
		if (hItemChanged == hFilterCasino)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_CASINO);
		if (hItemChanged == hFilterBallpaddle)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_BALLPADDLE);
		if (hItemChanged == hFilterMaze)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_MAZE);
		if (hItemChanged == hFilterMinigames)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_MINIGAMES);
		if (hItemChanged == hFilterPinball)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_PINBALL);
		if (hItemChanged == hFilterPlatform)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_PLATFORM);
		if (hItemChanged == hFilterPuzzle)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_PUZZLE);
		if (hItemChanged == hFilterQuiz)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_QUIZ);
		if (hItemChanged == hFilterSportsmisc)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_SPORTSMISC);
		if (hItemChanged == hFilterSportsfootball)	_ToggleGameListing(nLoadMenuGenreFilter, GBF_SPORTSFOOTBALL);
		if (hItemChanged == hFilterMisc)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_MISC);
		if (hItemChanged == hFilterMahjong)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_MAHJONG);
		if (hItemChanged == hFilterRacing)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_RACING);
		if (hItemChanged == hFilterShoot)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_SHOOT);
		if (hItemChanged == hFilterMultiShoot)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_MULTISHOOT);
		if (hItemChanged == hFilterAction)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_ACTION);
		if (hItemChanged == hFilterRungun)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_RUNGUN);
		if (hItemChanged == hFilterStrategy)		_ToggleGameListing(nLoadMenuGenreFilter, GBF_STRATEGY);
		if (hItemChanged == hFilterRpg)				_ToggleGameListing(nLoadMenuGenreFilter, GBF_RPG);
		if (hItemChanged == hFilterSim)				_ToggleGameListing(nLoadMenuGenreFilter, GBF_SIM);
		if (hItemChanged == hFilterAdv)				_ToggleGameListing(nLoadMenuGenreFilter, GBF_ADV);
		if (hItemChanged == hFilterCard)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_CARD);
		if (hItemChanged == hFilterBoard)			_ToggleGameListing(nLoadMenuGenreFilter, GBF_BOARD);

		RebuildEverything();
	}

	if (Msg == WM_COMMAND) {
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_SEL_SEARCH) {
			if (bSelModeSwitching) {
				return 0;
			}
			if (bSearchStringInit) {
				bSearchStringInit = false;
				return 0;
			}

			KillTimer(hDlg, IDC_SEL_SEARCHTIMER);
			SetTimer(hDlg, IDC_SEL_SEARCHTIMER, 500, (TIMERPROC)NULL);
		}

		if (HIWORD(wParam) == BN_CLICKED) {
			int wID = LOWORD(wParam);
			switch (wID) {
				case IDC_SEL_MODE_ROM:
					EnterSelectRomMode(hDlg, true);
					return 0;

				case IDC_SEL_MODE_IPS:
					EnterSelectIpsMode(hDlg);
					return 0;

				case IDC_SEL_MODE_ROMDATA:
					EnterSelectRomDataMode(hDlg);
					return 0;

				case IDOK:
					if (nSelActiveMode == 1) {
						EnterSelectRomMode(hDlg, true);
						return 0;
					}
					if (nSelActiveMode == 2) {
						TCHAR szSelDat[MAX_PATH] = { 0 };
						if (RomDataHostedGetSelectedDat(szSelDat, _countof(szSelDat))) {
							SelOkayRomData(szSelDat);
						}
						return 0;
					}
					SelOkay();
					break;
				case IDROM:
					RomsDirCreate(hSelDlg);
					RebuildEverything();
					break;
				case IDRESCAN:
					LookupSubDirThreads();
					bRescanRoms = true;
					CreateROMInfo(hSelDlg);
					RebuildEverything();
					break;
				case IDCANCEL:
					if (nSelActiveMode == 1) {
						EnterSelectRomMode(hDlg, false);
						return 0;
					}
					if (nSelActiveMode == 2) {
						EnterSelectRomMode(hDlg, false);
						return 0;
					}
					bDialogCancel = true;
					SendMessage(hDlg, WM_CLOSE, 0, 0);
					return 0;
				case IDC_CHECKAVAILABLE:
					nLoadMenuShowY ^= AVAILABLE;
					RebuildEverything();
					break;
				case IDC_CHECKUNAVAILABLE:
					nLoadMenuShowY ^= UNAVAILABLE;
					RebuildEverything();
					break;
				case IDC_CHECKAUTOEXPAND:
					nLoadMenuShowY ^= AUTOEXPAND;
					RebuildEverything();
					break;
				case IDC_SEL_SHORTNAME:
					nLoadMenuShowY ^= SHOWSHORT;
					RebuildEverything();
					break;
				case IDC_SEL_ASCIIONLY:
					nLoadMenuShowY ^= ASCIIONLY;
					RebuildEverything();
					break;
				case IDC_SEL_SUBDIRS:
					nLoadMenuShowY ^= SEARCHSUBDIRS;
					LookupSubDirThreads();
					break;
				case IDC_SEL_DISABLECRC:
					nLoadMenuShowY ^= DISABLECRCVERIFY;
					bRescanRoms = true;
					CreateROMInfo(hSelDlg);
					RebuildEverything();
					break;
				case IDGAMEINFO:
					if (bDrvSelected) {
						GameInfoDialogCreate(hSelDlg, nBurnDrvActive);
						SetFocus(hSelList); // Update list for Rescan Romset button
					} else {
						MessageBox(hSelDlg, FBALoadStringEx(hAppInst, IDS_ERR_NO_DRIVER_SELECTED, true), FBALoadStringEx(hAppInst, IDS_ERR_ERROR, true), MB_OK);
					}
					break;
				case IDC_SEL_APPLYIPS:
					bDoIpsPatch = !bDoIpsPatch;
					break;
				case IDC_SEL_IPS_CLEAR:
					if (nSelActiveMode == 1) {
						IpsHostedDeselectAll();
						UpdateSelectModeAvailability(hDlg);
					}
					break;
				case IDC_ROMDATA_SCAN_BUTTON:
					if (nSelActiveMode == 2) {
						bRDListScanSub = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ROMDATA_SUBDIR_CHECK));
						RomDataHostedSetScanSubdirs(bRDListScanSub ? 1 : 0);
						RomDataHostedRefresh(true);
					}
					break;
				case IDC_ROMDATA_SELDIR_BUTTON:
					if (nSelActiveMode == 2) {
						SupportDirCreate(hDlg);
						bRDListScanSub = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ROMDATA_SUBDIR_CHECK));
						RomDataHostedSetScanSubdirs(bRDListScanSub ? 1 : 0);
						RomDataHostedRefresh(true);
					}
					break;
				case IDC_ROMDATA_SUBDIR_CHECK:
					if (nSelActiveMode == 2) {
						bRDListScanSub = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ROMDATA_SUBDIR_CHECK));
						RomDataHostedSetScanSubdirs(bRDListScanSub ? 1 : 0);
						RomDataHostedRefresh(true);
					}
					break;
			}
		}

		if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_SEL_IPS_LANG) {
			if (nSelActiveMode == 1) {
				IpsHostedSetLanguage((INT32)SendMessage(hSelIpsLang, CB_GETCURSEL, 0, 0));
				UpdateSelectModeAvailability(hDlg);
			}
			return 0;
		}

		int id = LOWORD(wParam);

		switch (id) {
			case GAMESEL_MENU_PLAY: {
				SelOkay();
				break;
			}

			case GAMESEL_MENU_GAMEINFO: {
				/*UpdatePreview(true, hSelDlg, szAppPreviewsPath);
				if (nTimer) {
					KillTimer(hSelDlg, nTimer);
					nTimer = 0;
				}
				GameInfoDialogCreate(hSelDlg, nBurnDrvSelect);*/
				if (bDrvSelected) {
					GameInfoDialogCreate(hSelDlg, nBurnDrvActive);
					SetFocus(hSelList); // Update list for Rescan Romset button
				} else {
					MessageBox(hSelDlg, FBALoadStringEx(hAppInst, IDS_ERR_NO_DRIVER_SELECTED, true), FBALoadStringEx(hAppInst, IDS_ERR_ERROR, true), MB_OK);
				}
				break;
			}

			case GAMESEL_MENU_VIEWEMMA: {
				if (!nVidFullscreen) {
					ViewEmma();
				}
				break;
			}

			case GAMESEL_MENU_FAVORITE: { // toggle favorite status.
				if (bDrvSelected) {
					AddFavorite_Ext((CheckFavorites(BurnDrvGetTextA(DRV_NAME)) == -1) ? 1 : 0);
				} else {
					MessageBox(hSelDlg, FBALoadStringEx(hAppInst, IDS_ERR_NO_DRIVER_SELECTED, true), FBALoadStringEx(hAppInst, IDS_ERR_ERROR, true), MB_OK);
				}

				break;
			}

			case GAMESEL_MENU_ROMDATA: { // Export to RomData template
				if (bDrvSelected) {
					RomDataExportTemplate(hSelDlg, nDialogSelect);
				}
				else {
					MessageBox(hSelDlg, FBALoadStringEx(hAppInst, IDS_ERR_NO_DRIVER_SELECTED, true), FBALoadStringEx(hAppInst, IDS_ERR_ERROR, true), MB_OK);
				}

				break;
			}
		}
	}

	if (Msg == WM_TIMER) {
		if (nSelActiveMode == 2 && RomDataHostedHandleTimer(hDlg, wParam)) {
			return 0;
		}

		switch (wParam) {
			case IDC_SEL_SEARCHTIMER:
				KillTimer(hDlg, IDC_SEL_SEARCHTIMER);
				if (nSelActiveMode == 1) {
					TCHAR szCurrentSearch[_countof(szSearchString)] = { 0 };
					GetDlgItemText(hDlg, IDC_SEL_SEARCH, szCurrentSearch, _countof(szCurrentSearch));
					_tcscpy(szIpsSearchString, szCurrentSearch);
					IpsHostedSetSearchText(szCurrentSearch);
					UpdateSelectModeAvailability(hDlg);
					RedrawWindow(hSelList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
				} else if (nSelActiveMode == 2) {
					TCHAR szCurrentSearch[_countof(szSearchString)] = { 0 };
					GetDlgItemText(hDlg, IDC_SEL_SEARCH, szCurrentSearch, _countof(szCurrentSearch));
					_tcscpy(szRomDataSearchString, szCurrentSearch);
					RomDataHostedSetSearchText(szCurrentSearch);
					UpdateSelectModeAvailability(hDlg);
					RedrawWindow(hSelRomDataList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
				} else {
					RebuildEverything();
				}
				break;
		}
	}

	if (Msg == UM_CLOSE) {
		if (nSelActiveMode == 2) {
			RomDataHostedExit();
		}
		nDialogSelect = nOldDlgSelected;
		MyEndDialog();
		DeleteObject(hWhiteBGBrush);
		return 0;
	}

	if (Msg == WM_CLOSE) {
		if (nSelActiveMode == 2) {
			RomDataHostedExit();
		}
		bDialogCancel = true;
		nDialogSelect = nOldDlgSelected;
		MyEndDialog();
		DeleteObject(hWhiteBGBrush);
		return 0;
	}

	if (Msg == WM_GETMINMAXINFO) {
		MINMAXINFO *info = (MINMAXINFO*)lParam;

		info->ptMinTrackSize.x = nSelMinTrackWidth;
		info->ptMinTrackSize.y = nSelMinTrackHeight;
		info->ptMaxTrackSize.x = nSelMinTrackWidth;
		info->ptMaxTrackSize.y = nSelMinTrackHeight;

		return 0;
	}

	if (Msg == WM_DRAWITEM) {
		const DRAWITEMSTRUCT* pDraw = (const DRAWITEMSTRUCT*)lParam;
		if (pDraw && pDraw->CtlType == ODT_BUTTON && IsSelectDialogButtonId((INT32)wParam)) {
			DrawSelectDialogButton(pDraw);
			return TRUE;
		}
	}

#if 0
	if (Msg == WM_WINDOWPOSCHANGED) {	// All controls blink when dragging the window
#endif // 0

	if (Msg == WM_SIZE) {
		LayoutMergedSelectDialog(hDlg);
		InvalidateRect(hSelDlg, NULL, true);
		UpdateWindow(hSelDlg);

		return 0;
	}

//	if (Msg == WM_TIMER) {
//		UpdatePreview(false, szAppPreviewsPath, IDC_SCREENSHOT_H, IDC_SCREENSHOT_V);
//		return 0;
//	}

	if (Msg == WM_CTLCOLORSTATIC) {
		for (INT32 i = 0; i < 3; i++) {
			if ((HWND)lParam == hSelModeTitle[i]) {
				SetBkMode((HDC)wParam, TRANSPARENT);
				COLORREF nColor = RGB(88, 88, 88);
				if (!IsWindowEnabled(hSelModeTitle[i])) {
					nColor = RGB(155, 155, 155);
				} else if (i == nSelActiveMode) {
					nColor = RGB(230, 92, 0);
				}
				SetTextColor((HDC)wParam, nColor);
				return (INT_PTR)GetSysColorBrush(COLOR_3DFACE);
			}
		}

		for (int i = 0; i < 6; i++) {
			if ((HWND)lParam == hInfoLabel[i])	{ return (INT_PTR)hWhiteBGBrush; }
			if ((HWND)lParam == hInfoText[i])	{ return (INT_PTR)hWhiteBGBrush; }
		}
	}

	NMHDR* pNmHdr = (NMHDR*)lParam;
	if (Msg == WM_NOTIFY)
	{
		if (bSelModeSwitching) {
			return 0;
		}
		if (nSelActiveMode == 1 && pNmHdr && pNmHdr->hwndFrom == hSelIpsTree) {
			INT32 nIpsNotify = IpsHostedHandleNotify(pNmHdr);
			if (nIpsNotify) {
				UpdateSelectModeAvailability(hDlg);
			}
			if (nIpsNotify == 3) {
				if (IpsHostedGetCheckedCount() > 0) {
					bDoIpsPatch = true;

					TCHAR szSelDat[MAX_PATH] = { 0 };
					if (RomDataHostedGetSelectedDat(szSelDat, _countof(szSelDat))) {
						SelOkayRomData(szSelDat);
					} else {
						SelOkayDriverIndex(nDialogSelect);
					}
				}
				SetWindowLongPtr(hSelDlg, DWLP_MSGRESULT, 1);
				return 1;
			}
			if (nIpsNotify == 2) {
				SetWindowLongPtr(hSelDlg, DWLP_MSGRESULT, 1);
				return 1;
			}
			if (nIpsNotify == 1) {
				return 0;
			}
		}
		if (nSelActiveMode == 2 && pNmHdr && pNmHdr->idFrom == IDC_ROMDATA_LIST) {
			INT32 nHandled = RomDataHostedHandleNotify(pNmHdr);
			if (nHandled) {
				UpdateSelectModeAvailability(hDlg);
			}
			if (nHandled == 2) {
				TCHAR szSelDat[MAX_PATH] = { 0 };
				if (RomDataHostedGetSelectedDat(szSelDat, _countof(szSelDat))) {
					SelOkayRomData(szSelDat);
				}
				return 1;
			}
			if (nHandled) {
				return 1;
			}
		}

		if ((pNmHdr->code == TVN_ITEMEXPANDED) && (pNmHdr->idFrom == IDC_TREE2))
		{
			// save the expanded state of the filter nodes
			NM_TREEVIEW *pnmtv = (NM_TREEVIEW *)lParam;
			TV_ITEM curItem = pnmtv->itemNew;

			if (pnmtv->action == TVE_COLLAPSE)
			{
				nLoadMenuExpand &= ~TvhFilterToBitmask(curItem.hItem);
			}
			else if (pnmtv->action == TVE_EXPAND)
			{
				nLoadMenuExpand |= TvhFilterToBitmask(curItem.hItem);
			}
		}

		if ((pNmHdr->code == NM_CLICK) && (pNmHdr->idFrom == IDC_TREE2))
		{
			TVHITTESTINFO thi;
			DWORD dwpos = GetMessagePos();
			thi.pt.x	= GET_X_LPARAM(dwpos);
			thi.pt.y	= GET_Y_LPARAM(dwpos);
			MapWindowPoints(HWND_DESKTOP, pNmHdr->hwndFrom, &thi.pt, 1);
			TreeView_HitTest(pNmHdr->hwndFrom, &thi);

			if(TVHT_ONITEMSTATEICON & thi.flags) {
				PostMessage(hSelDlg, UM_CHECKSTATECHANGE, 0, (LPARAM)thi.hItem);
			}

			return 1;
		}

		NMTREEVIEW* pnmtv = (NMTREEVIEW*)lParam;

		if (nSelActiveMode == 0 && !TreeBuilding && pnmtv->hdr.code == NM_DBLCLK && pnmtv->hdr.idFrom == IDC_TREE1)
		{
			DWORD dwpos = GetMessagePos();

			TVHITTESTINFO thi;
			thi.pt.x	= GET_X_LPARAM(dwpos);
			thi.pt.y	= GET_Y_LPARAM(dwpos);

			MapWindowPoints(HWND_DESKTOP, pNmHdr->hwndFrom, &thi.pt, 1);

			TreeView_HitTest(pNmHdr->hwndFrom, &thi);

			HTREEITEM hSelectHandle = thi.hItem;
			if(hSelectHandle == NULL) return 1;

			TreeView_SelectItem(hSelList, hSelectHandle);

			// Search through nBurnDrv[] for the nBurnDrvNo according to the returned hSelectHandle
			for (unsigned int i = 0; i < nTmpDrvCount; i++) {
				if (hSelectHandle == nBurnDrv[i].hTreeHandle) {
					nBurnDrvActive = nBurnDrv[i].nBurnDrvNo;
					break;
				}
			}

			nDialogSelect	= nBurnDrvActive;
			bDrvSelected	= true;

			SelOkay();

			// disable double-click node-expand
			SetWindowLongPtr(hSelDlg, DWLP_MSGRESULT, 1);

			return 1;
		}

		if (nSelActiveMode == 0 && pNmHdr->code == NM_CUSTOMDRAW && LOWORD(wParam) == IDC_TREE1) {
			LPNMTVCUSTOMDRAW lptvcd = (LPNMTVCUSTOMDRAW)lParam;
			int nGetTextFlags = nLoadMenuShowY & ASCIIONLY ? DRV_ASCIIONLY : 0;
			HTREEITEM hSelectHandle;

			switch (lptvcd->nmcd.dwDrawStage) {
				case CDDS_PREPAINT: {
					SetWindowLongPtr(hSelDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
					return 1;
				}

				case CDDS_ITEMPREPAINT:	{
					hSelectHandle = (HTREEITEM)(lptvcd->nmcd.dwItemSpec);
					RECT rect;

					{
						RECT rcClip;
						TreeView_GetItemRect(lptvcd->nmcd.hdr.hwndFrom, hSelectHandle, &rect, TRUE);

						// Check if the current item is in the area to be redrawn(only the visible part is drawn)
						if (!IntersectRect(&rcClip, &lptvcd->nmcd.rc, &rect)) {
//							SetWindowLongPtr(hSelDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT);
							return 1;
						}
					}

					// TVITEM (msdn.microsoft.com) This structure is identical to the TV_ITEM structure, but it has been renamed to
					// follow current naming conventions. New applications should use this structure.

					//TV_ITEM TvItem;
					TVITEM TvItem;
					TvItem.hItem = hSelectHandle;
					TvItem.mask  = TVIF_PARAM | TVIF_STATE | TVIF_CHILDREN;
					SendMessage(hSelList, TVM_GETITEM, 0, (LPARAM)&TvItem);

//					dprintf(_T("  - Item (%i?i) - (%i?i) %hs\n"), lptvcd->nmcd.rc.left, lptvcd->nmcd.rc.top, lptvcd->nmcd.rc.right, lptvcd->nmcd.rc.bottom, ((NODEINFO*)TvItem.lParam)->pszROMName);

					// Set the foreground and background colours unless the item is highlighted
					if (!(TvItem.state & (TVIS_SELECTED | TVIS_DROPHILITED))) {

						// Set less contrasting colours for clones
						if (!((NODEINFO*)TvItem.lParam)->bIsParent) {
							lptvcd->clrTextBk = RGB(0xD7, 0xD7, 0xD7);
							lptvcd->clrText   = RGB(0x3F, 0x3F, 0x3F);
						}

						// For parents, change the colour of the background, for clones, change only the text colour
						if (!CheckWorkingStatus(((NODEINFO*)TvItem.lParam)->nBurnDrvNo)) {
							lptvcd->clrText = RGB(0x7F, 0x7F, 0x7F);
						}

						// Slightly different color for favorites (key lime pie anyone?)
						if (CheckFavorites(((NODEINFO*)TvItem.lParam)->pszROMName) != -1) {
							if (!((NODEINFO*)TvItem.lParam)->bIsParent) {
								lptvcd->clrTextBk = RGB(0xd7, 0xe7, 0xd7);
							} else {
								lptvcd->clrTextBk = RGB(0xe6, 0xff, 0xe6);

								// Both parent and clone are in favorites
								if (!(TvItem.state & TVIS_EXPANDED) && TvItem.cChildren) {
									HTREEITEM hChild = TreeView_GetChild(hSelList, TvItem.hItem);
									while (NULL != hChild) {
										TVITEM tvi = { 0 };
										tvi.mask   = TVIF_PARAM | TVIF_HANDLE;
										tvi.hItem  = hChild;
										if (TreeView_GetItem(hSelList, &tvi)) {
											if (-1 != CheckFavorites(((NODEINFO*)tvi.lParam)->pszROMName)) {
												lptvcd->clrTextBk = RGB(0xe6, 0xe6, 0xfa);		// Lavender
												break;
											}
										}
										hChild = TreeView_GetNextSibling(hSelList, hChild);
									}
								}
							}
						} else {
							// Only clones are favorites
							if (((NODEINFO*)TvItem.lParam)->bIsParent) {
								if (!(TvItem.state & TVIS_EXPANDED) && TvItem.cChildren) {
									HTREEITEM hChild = TreeView_GetChild(hSelList, TvItem.hItem);
									while (NULL != hChild) {
										TVITEM tvi = { 0 };
										tvi.mask   = TVIF_PARAM | TVIF_HANDLE;
										tvi.hItem  = hChild;
										if (TreeView_GetItem(hSelList, &tvi)) {
											if (-1 != CheckFavorites(((NODEINFO*)tvi.lParam)->pszROMName)) {
												lptvcd->clrTextBk = RGB(0xff, 0xf0, 0xf5);		// Lavender blush
												break;
											}
										}
										hChild = TreeView_GetNextSibling(hSelList, hChild);
									}
								}
							}
						}
					}

					rect.left   = lptvcd->nmcd.rc.left;
					rect.right  = lptvcd->nmcd.rc.right;
					rect.top    = lptvcd->nmcd.rc.top;
					rect.bottom = lptvcd->nmcd.rc.bottom;

					nBurnDrvActive = ((NODEINFO*)TvItem.lParam)->nBurnDrvNo;

					{
						// Fill background
						HBRUSH hBackBrush = CreateSolidBrush(lptvcd->clrTextBk);
						FillRect(lptvcd->nmcd.hdc, &lptvcd->nmcd.rc, hBackBrush);
						DeleteObject(hBackBrush);
					}

					{
						// Draw plus and minus buttons
						if (((NODEINFO*)TvItem.lParam)->bIsParent && TvItem.cChildren) {
							HICON hIcon = (TvItem.state & TVIS_EXPANDED) ? hCollapse : hExpand;
							DrawIconEx(lptvcd->nmcd.hdc, rect.left + 4, rect.top + nIconsYDiff, hIcon, 16, 16, 0, NULL, DI_NORMAL);
						}
						rect.left += 16 + 8;
					}

					{
						// Draw text

						TCHAR  szText[1024];
						TCHAR* pszPosition = szText;
						TCHAR* pszName;
						SIZE   size = { 0, 0 };

						SetTextColor(lptvcd->nmcd.hdc, lptvcd->clrText);
						SetBkMode(lptvcd->nmcd.hdc, TRANSPARENT);

						// Display the short name if needed
						if (nLoadMenuShowY & SHOWSHORT) {
							// calculate field expansion (between romname & description)
							int expansion = (rect.right / dpi_x) - 4;
							if (expansion < 0) expansion = 0;

							const int FIELD_SIZE = 16 + 40 + 20 + 20 + (expansion*8);
							const int EXPAND_ICON_SIZE = 16 + 8;
							const int temp_right = rect.right;
							rect.right = EXPAND_ICON_SIZE + FIELD_SIZE - 2;

							DrawText(lptvcd->nmcd.hdc, BurnDrvGetText(DRV_NAME), -1, &rect, DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
							rect.right = temp_right;
							rect.left += FIELD_SIZE;
						}

						rect.top += 2;

						bool bParentExp = false;	// If true, Item is clone and parent is expanded
						if (!((NODEINFO*)TvItem.lParam)->bIsParent) {
							HTREEITEM hParent = TreeView_GetParent(hSelList, TvItem.hItem);
							if (NULL != hParent) {
								bParentExp = (TreeView_GetItemState(hSelList, hParent, TVIS_EXPANDED) & TVIS_EXPANDED);
							}
						}

						{
							// Draw icons if needed
							if (((NODEINFO*)TvItem.lParam)->bIsParent || bParentExp) {
								if (!CheckWorkingStatus(nBurnDrvActive)) {
									DrawIconEx(lptvcd->nmcd.hdc, rect.left, rect.top, hNotWorking, nIconsSizeXY, nIconsSizeXY, 0, NULL, DI_NORMAL);
									rect.left += nIconsSizeXY + 4;
								} else {
									if (!(gameAv[nBurnDrvActive]) && !bSkipStartupCheck) {
										DrawIconEx(lptvcd->nmcd.hdc, rect.left, rect.top, hNotFoundEss, nIconsSizeXY, nIconsSizeXY, 0, NULL, DI_NORMAL);
										rect.left += nIconsSizeXY + 4;
									} else {
										if (!(nLoadMenuShowY & AVAILABLE) && !(gameAv[nBurnDrvActive] & 2)) {
											DrawIconEx(lptvcd->nmcd.hdc, rect.left, rect.top, hNotFoundNonEss, nIconsSizeXY, nIconsSizeXY, 0, NULL, DI_NORMAL);
											rect.left += nIconsSizeXY + 4;
										}
									}
								}
							}
						}

						// Driver Icon drawing code...
						if (bEnableIcons && bIconsLoaded && (((NODEINFO*)TvItem.lParam)->bIsParent || bParentExp)) {
							// Windows GDI limitation, can not cache all icons, can only cache the following icons
							// All hardware icon exist (By hardware)
							// All non-Clone icon exist (By game)
							// When the Clone icon option is turned on, the parent item has an icon and Clone does not (By game, They do not take up GDI resources)
							if (hDrvIcon[nBurnDrvActive]) {
								DrawIconEx(lptvcd->nmcd.hdc, rect.left, rect.top, hDrvIcon[nBurnDrvActive], nIconsSizeXY, nIconsSizeXY, 0, NULL, DI_NORMAL);
							}

							if (!hDrvIcon[nBurnDrvActive]) {
								// Non-Clone
								if ((NULL == BurnDrvGetText(DRV_PARENT)) && !(BurnDrvGetFlags() & BDF_CLONE)) {
									DrawIconEx(lptvcd->nmcd.hdc, rect.left, rect.top, hDrvIconMiss, nIconsSizeXY, nIconsSizeXY, 0, NULL, DI_NORMAL);
								}
								// Clone
								else {
									if (!bIconsOnlyParents) {
										// By hardware
										if (bIconsByHardwares) {
											DrawIconEx(lptvcd->nmcd.hdc, rect.left, rect.top, hDrvIconMiss, nIconsSizeXY, nIconsSizeXY, 0, NULL, DI_NORMAL);
										}
										// By game
										else {
											TCHAR szIcon[MAX_PATH] = { 0 };
											_stprintf(szIcon, _T("%s%s.ico"), szAppIconsPath, BurnDrvGetText(DRV_NAME));

											// Find the icons that meet the conditions, load and redraw them one by one and then recycle the resources to avoid memory leakage due to GDI resource overflow
											// Exclude all parent set
											// Exclude all hardware icons
											// All the Clones where you can find icons
											// Creates a temporary HICON object, which is destroyed immediately upon completion of the redraw.
											HICON hTempIcon = (HICON)LoadImage(NULL, szIcon, IMAGE_ICON, nIconsSizeXY, nIconsSizeXY, LR_LOADFROMFILE);
											HICON hGameIcon = (NULL != hTempIcon) ? hTempIcon : hDrvIconMiss;
											DrawIconEx(lptvcd->nmcd.hdc, rect.left, rect.top, hGameIcon, nIconsSizeXY, nIconsSizeXY, 0, NULL, DI_NORMAL);

											if (NULL != hTempIcon) {	// Clone icon exist (By game)
												DestroyIcon(hTempIcon); hTempIcon = NULL;
											}
										}
									}
								}
							}
							rect.left += nIconsSizeXY + 4;
						}

						_tcsncpy(szText, MangleGamename(BurnDrvGetText(nGetTextFlags | DRV_FULLNAME), false), 1024);
						szText[1023] = _T('\0');

						GetTextExtentPoint32(lptvcd->nmcd.hdc, szText, _tcslen(szText), &size);

						DrawText(lptvcd->nmcd.hdc, szText, -1, &rect, DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_VCENTER);

						// Display extra info if needed
						szText[0] = _T('\0');

						pszName = BurnDrvGetText(nGetTextFlags | DRV_FULLNAME);
						while ((pszName = BurnDrvGetText(nGetTextFlags | DRV_NEXTNAME | DRV_FULLNAME)) != NULL) {
							if (pszPosition + _tcslen(pszName) - 1024 > szText) {
								break;
							}
							pszPosition += _stprintf(pszPosition, _T(SEPERATOR_2) _T("%s"), pszName);
						}
						if (szText[0]) {
							szText[255] = _T('\0');

							unsigned int r = ((lptvcd->clrText >> 16 & 255) * 2 + (lptvcd->clrTextBk >> 16 & 255)) / 3;
							unsigned int g = ((lptvcd->clrText >>  8 & 255) * 2 + (lptvcd->clrTextBk >>  8 & 255)) / 3;
							unsigned int b = ((lptvcd->clrText >>  0 & 255) * 2 + (lptvcd->clrTextBk >>  0 & 255)) / 3;

							rect.left += size.cx;
							SetTextColor(lptvcd->nmcd.hdc, (r << 16) | (g <<  8) | (b <<  0));
							DrawText(lptvcd->nmcd.hdc, szText, -1, &rect, DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_VCENTER);
						}
					}

					SetWindowLongPtr(hSelDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT);
					return 1;
				}

				default: {
					SetWindowLongPtr(hSelDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
					return 1;
				}
			}
		}

		if (pNmHdr->code == TVN_ITEMEXPANDING && !TreeBuilding && LOWORD(wParam) == IDC_TREE1) {
			SendMessage(hSelList, TVM_SELECTITEM, TVGN_CARET, (LPARAM)((LPNMTREEVIEW)lParam)->itemNew.hItem);
			return FALSE;
		}

		if (nSelActiveMode == 0 && pNmHdr->code == TVN_SELCHANGED && !TreeBuilding && LOWORD(wParam) == IDC_TREE1) {
			HTREEITEM hSelectHandle = (HTREEITEM)SendMessage(hSelList, TVM_GETNEXTITEM, TVGN_CARET, ~0U);

			// Search through nBurnDrv[] for the nBurnDrvNo according to the returned hSelectHandle
			for (unsigned int i = 0; i < nTmpDrvCount; i++) {
				if (hSelectHandle == nBurnDrv[i].hTreeHandle)
				{
					nBurnDrvActive	= nBurnDrv[i].nBurnDrvNo;
					nDialogSelect	= nBurnDrvActive;
					bDrvSelected	= true;
					UpdatePreview(true, szAppPreviewsPath, IDC_SCREENSHOT_H, IDC_SCREENSHOT_V);
					break;
				}
			}
			UpdateRomSelectionDetails(hDlg);
		}

		if(!TreeBuilding && pnmtv->hdr.code == NM_RCLICK && pnmtv->hdr.idFrom == IDC_TREE1)
		{
			DWORD dwpos = GetMessagePos();

			TVHITTESTINFO thi;
			thi.pt.x	= GET_X_LPARAM(dwpos);
			thi.pt.y	= GET_Y_LPARAM(dwpos);

			MapWindowPoints(HWND_DESKTOP, pNmHdr->hwndFrom, &thi.pt, 1);

			TreeView_HitTest(pNmHdr->hwndFrom, &thi);

			HTREEITEM hSelectHandle = thi.hItem;
			if(hSelectHandle == NULL) return 1;

			TreeView_SelectItem(hSelList, hSelectHandle);

			// Search through nBurnDrv[] for the nBurnDrvNo according to the returned hSelectHandle
			for (unsigned int i = 0; i < nTmpDrvCount; i++) {
				if (hSelectHandle == nBurnDrv[i].hTreeHandle) {
					nBurnDrvSelect[0] = nBurnDrv[i].nBurnDrvNo;
					break;
				}
			}

			nDialogSelect	= nBurnDrvSelect[0];
			bDrvSelected	= true;
			UpdatePreview(true, szAppPreviewsPath, IDC_SCREENSHOT_H, IDC_SCREENSHOT_V);

			// Menu
			POINT oPoint;
			GetCursorPos(&oPoint);

			HMENU hMenuLoad = FBALoadMenu(hAppInst, MAKEINTRESOURCE(IDR_MENU_GAMESEL));
			HMENU hMenuX = GetSubMenu(hMenuLoad, 0);

			CheckMenuItem(hMenuX, GAMESEL_MENU_FAVORITE, (CheckFavorites(BurnDrvGetTextA(DRV_NAME)) != -1) ? MF_CHECKED : MF_UNCHECKED);

			TrackPopupMenu(hMenuX, TPM_LEFTALIGN | TPM_RIGHTBUTTON, oPoint.x, oPoint.y, 0, hSelDlg, NULL);
			DestroyMenu(hMenuLoad);

			return 1;
		}
	}
	return 0;
}

int SelDialog(int nMVSCartsOnly, HWND hParentWND)
{
	int nOldSelect = nBurnDrvActive;
	bSelRomDataLaunch = false;

	if(bDrvOkay) {
		nOldDlgSelected = nBurnDrvActive;
	}

	hParent = hParentWND;
	nShowMVSCartsOnly = nMVSCartsOnly;
	RomDataStateBackup();

	FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_SELNEW), hParent, (DLGPROC)DialogProc);

	hSelDlg = NULL;
	hSelList = NULL;

	if (nBurnDrv) {
		free(nBurnDrv);
		nBurnDrv = NULL;
	}

	if (!bSelRomDataLaunch) {
		nBurnDrvActive = nOldSelect;
	}
	bSelRomDataLaunch = false;

	return nDialogSelect;
}

// -----------------------------------------------------------------------------

static HBITMAP hMVSpreview[6];
static unsigned int nPrevDrvSelect[6];

static void UpdateInfoROMInfo()
{
//	int nGetTextFlags = nLoadMenuShowY & ASCIIONLY ? DRV_ASCIIONLY : 0;
	TCHAR szItemText[256] = _T("");
	bool bBracket = false;

	_stprintf(szItemText, _T("%s"), BurnDrvGetText(DRV_NAME));
	if ((BurnDrvGetFlags() & BDF_CLONE) && BurnDrvGetTextA(DRV_PARENT)) {
		int nOldDrvSelect = nBurnDrvActive;
		TCHAR* pszName = BurnDrvGetText(DRV_PARENT);

		_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_CLONE_OF, true), BurnDrvGetText(DRV_PARENT));

		for (nBurnDrvActive = 0; nBurnDrvActive < nBurnDrvCount; nBurnDrvActive++) {
			if (!_tcsicmp(pszName, BurnDrvGetText(DRV_NAME))) {
				break;
			}
		}
		if (nBurnDrvActive < nBurnDrvCount) {
			if (BurnDrvGetText(DRV_PARENT)) {
				_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_ROMS_FROM_1, true), BurnDrvGetText(DRV_PARENT));
			}
		}
		nBurnDrvActive = nOldDrvSelect;
		bBracket = true;
	} else {
		if (BurnDrvGetTextA(DRV_PARENT)) {
			_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_ROMS_FROM_2, true), bBracket ? _T(", ") : _T(" ("), BurnDrvGetText(DRV_PARENT));
			bBracket = true;
		}
	}
	if (BurnDrvGetText(DRV_BOARDROM)) {
		_stprintf(szItemText + _tcslen(szItemText), FBALoadStringEx(hAppInst, IDS_BOARD_ROMS_FROM, true), bBracket ? _T(", ") : _T(" ("), BurnDrvGetText(DRV_BOARDROM));
		bBracket = true;
	}
	if (bBracket) {
		_stprintf(szItemText + _tcslen(szItemText), _T(")"));
	}

	if (hInfoText[0]) {
		SendMessage(hInfoText[0], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
	}
	if (hInfoLabel[0]) {
		EnableWindow(hInfoLabel[0], TRUE);
	}

	return;
}

static void UpdateInfoRelease()
{
	int nGetTextFlags = nLoadMenuShowY & ASCIIONLY ? DRV_ASCIIONLY : 0;
	TCHAR szItemText[256] = _T("");

	TCHAR szUnknown[100];
	TCHAR szCartridge[100];
	TCHAR szHardware[100];
	_stprintf(szUnknown, FBALoadStringEx(hAppInst, IDS_ERR_UNKNOWN, true));
	_stprintf(szCartridge, FBALoadStringEx(hAppInst, IDS_CARTRIDGE, true));
	_stprintf(szHardware, FBALoadStringEx(hAppInst, IDS_HARDWARE, true));
	_stprintf(szItemText, _T("%s (%s, %s %s)"), BurnDrvGetTextA(DRV_MANUFACTURER) ? BurnDrvGetText(nGetTextFlags | DRV_MANUFACTURER) : szUnknown, BurnDrvGetText(DRV_DATE),
		BurnDrvGetText(nGetTextFlags | DRV_SYSTEM), (BurnDrvGetHardwareCode() & HARDWARE_PREFIX_CARTRIDGE) ? szCartridge : szHardware);

	if (hInfoText[2]) {
		SendMessage(hInfoText[2], WM_SETTEXT, (WPARAM)0, (LPARAM)szItemText);
	}
	if (hInfoLabel[2]) {
		EnableWindow(hInfoLabel[2], TRUE);
	}

	return;
}

static void UpdateInfoGameInfo()
{
	int nGetTextFlags = nLoadMenuShowY & ASCIIONLY ? DRV_ASCIIONLY : 0;
	TCHAR szText[1024] = _T("");
	TCHAR* pszPosition = szText;
	TCHAR* pszName = BurnDrvGetText(nGetTextFlags | DRV_FULLNAME);

	SelCopyString(szText, _countof(szText), pszName);
	pszPosition = szText + _tcslen(szText);

	pszName = BurnDrvGetText(nGetTextFlags | DRV_FULLNAME);
	while ((pszName = BurnDrvGetText(nGetTextFlags | DRV_NEXTNAME | DRV_FULLNAME)) != NULL) {
		if (pszPosition + _tcslen(pszName) - 1024 > szText) {
			break;
		}
		pszPosition += _stprintf(pszPosition, _T(SEPERATOR_2) _T("%s"), pszName);
	}
	if (BurnDrvGetText(nGetTextFlags | DRV_COMMENT)) {
		pszPosition += _sntprintf(pszPosition, szText + 1024 - pszPosition, _T(SEPERATOR_1) _T("%s"), BurnDrvGetText(nGetTextFlags | DRV_COMMENT));
	}

	if (hInfoText[3]) {
		SendMessage(hInfoText[3], WM_SETTEXT, (WPARAM)0, (LPARAM)szText);
	}
	if (hInfoLabel[3]) {
		if (szText[0]) {
			EnableWindow(hInfoLabel[3], TRUE);
		} else {
			EnableWindow(hInfoLabel[3], FALSE);
		}
	}

	return;
}

static int MVSpreviewUpdateSlot(int nSlot, HWND hDlg)
{
	int nOldSelect = nBurnDrvActive;

	if (nSlot >= 0 && nSlot < 6) {
		hInfoLabel[0] = 0; hInfoLabel[1] = 0; hInfoLabel[2] = 0; hInfoLabel[3] = 0;
		hInfoText[0] = GetDlgItem(hDlg, IDC_MVS_TEXTROMNAME1 + nSlot);
		hInfoText[1] = 0;
		hInfoText[2] = GetDlgItem(hDlg, IDC_MVS_TEXTSYSTEM1 + nSlot);
		hInfoText[3] = GetDlgItem(hDlg, IDC_MVS_TEXTCOMMENT1 + nSlot);

		for (int j = 0; j < 4; j++) {
			if (hInfoText[j]) {
				SendMessage(hInfoText[j], WM_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
			}
			if (hInfoLabel[j]) {
				EnableWindow(hInfoLabel[j], FALSE);
			}
		}

		nBurnDrvActive = nBurnDrvSelect[nSlot];
		if (nBurnDrvActive < nBurnDrvCount) {

			FILE* fp = OpenPreview(0, szAppTitlesPath);
			if (fp) {
				hMVSpreview[nSlot] = PNGLoadBitmap(hDlg, fp, 72, 54, 5);
				fclose(fp);
			} else {
				hMVSpreview[nSlot] = PNGLoadBitmap(hDlg, NULL, 72, 54, 4);
			}

			UpdateInfoROMInfo();
			UpdateInfoRelease();
			UpdateInfoGameInfo();
			nPrevDrvSelect[nSlot] = nBurnDrvActive;

		} else {
			hMVSpreview[nSlot] = PNGLoadBitmap(hDlg, NULL, 72, 54, 4);
			SendMessage(hInfoText[0], WM_SETTEXT, (WPARAM)0, (LPARAM)FBALoadStringEx(hAppInst, IDS_EMPTY, true));
		}

		SendDlgItemMessage(hDlg, IDC_MVS_CART1 + nSlot, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hMVSpreview[nSlot]);
	}

	nBurnDrvActive = nOldSelect;

	return 0;
}

static int MVSpreviewEndDialog()
{
	for (int i = 0; i < 6; i++) {
		DeleteObject((HGDIOBJ)hMVSpreview[i]);
		hMVSpreview[i] = NULL;
	}

	return 0;
}

static INT_PTR CALLBACK MVSpreviewProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM)
{
	switch (Msg) {
		case WM_INITDIALOG: {

			nDialogSelect = ~0U;

			for (int i = 0; i < 6; i++) {
				nPrevDrvSelect[i] = nBurnDrvSelect[i];
				MVSpreviewUpdateSlot(i, hDlg);
			}

			WndInMid(hDlg, hScrnWnd);

			return TRUE;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_VALUE_CLOSE) {
				SendMessage(hDlg, WM_CLOSE, 0, 0);
				break;
			}
			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == IDOK) {
					if (nPrevDrvSelect[0] == ~0U) {
						MessageBox(hSelDlg, FBALoadStringEx(hAppInst, IDS_ERR_NO_DRIVER_SEL_SLOT1, true), FBALoadStringEx(hAppInst, IDS_ERR_ERROR, true), MB_OK);
						break;
					}
					MVSpreviewEndDialog();
					for (int i = 0; i < 6; i++) {
						nBurnDrvSelect[i] = nPrevDrvSelect[i];
					}
					EndDialog(hDlg, 0);
					break;
				}
				if (LOWORD(wParam) == IDCANCEL) {
					SendMessage(hDlg, WM_CLOSE, 0, 0);
					break;
				}

				if (LOWORD(wParam) >= IDC_MVS_CLEAR1 && LOWORD(wParam) <= IDC_MVS_CLEAR6) {
					int nSlot = LOWORD(wParam) - IDC_MVS_CLEAR1;

					nBurnDrvSelect[nSlot] = ~0U;
					nPrevDrvSelect[nSlot] = ~0U;
					MVSpreviewUpdateSlot(nSlot, hDlg);
					break;
				}

				if (LOWORD(wParam) >= IDC_MVS_SELECT1 && LOWORD(wParam) <= IDC_MVS_SELECT6) {
					int nSlot = LOWORD(wParam) - IDC_MVS_SELECT1;

					nBurnDrvSelect[nSlot] = SelDialog(HARDWARE_PREFIX_CARTRIDGE | HARDWARE_SNK_NEOGEO, hDlg);
					MVSpreviewUpdateSlot(nSlot, hDlg);
					break;
				}
			}
			break;

		case WM_CLOSE: {
			MVSpreviewEndDialog();
			for (int i = 0; i < 6; i++) {
				nBurnDrvSelect[i] = nPrevDrvSelect[i];
			}
			EndDialog(hDlg, 1);
			break;
		}
	}

	return 0;
}

int SelMVSDialog()
{
	return FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_MVS_SELECT_CARTS), hScrnWnd, (DLGPROC)MVSpreviewProc);
}

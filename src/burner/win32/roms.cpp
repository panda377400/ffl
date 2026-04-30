#include "burner.h"
#include <shlobj.h>
#include <process.h>
#include <set>
#include "unzip.h"

#ifdef INCLUDE_7Z_SUPPORT
#include "un7z.h"
#endif

static HWND hTabControl = NULL;
static HWND hParent     = NULL;

HWND hRomsDlg = NULL;

char* gameAv = NULL;
bool avOk    = false;

bool bSkipStartupCheck = false;

static UINT32 ScanThreadId = 0;
static HANDLE hScanThread  = NULL;
static INT32 nOldSelect    = 0;

static HANDLE hEvent = NULL;

struct RomScanRomEntry {
	UINT32 nLen;
	UINT32 nCrc;
	UINT32 nType;
	std::vector<std::basic_string<TCHAR> > aliases;
};

struct RomScanDriverEntry {
	UINT32 nDriverIndex;
	UINT32 nHardwareMask;
	std::vector<std::basic_string<TCHAR> > archiveNames;
	std::vector<RomScanRomEntry> roms;
	std::basic_string<TCHAR> hddName;
	std::basic_string<TCHAR> hddFolderName;
};

static std::vector<RomScanDriverEntry> g_RomScanDrivers;
static LONG g_nRomScanNextIndex = 0;
static LONG g_nRomScanCompleted = 0;
static CRITICAL_SECTION g_RomScanCacheLock;
static bool g_bRomScanCacheLockInit = false;
static std::set<std::basic_string<TCHAR> > g_RomScanExistingArchives;
static std::set<std::basic_string<TCHAR> > g_RomScanMissingArchives;

static const TCHAR szAppDefaultPaths[DIRS_MAX][MAX_PATH] = {
	{ _T("roms/")			},
	{ _T("roms/arcade/")	},
	{ _T("roms/megadrive/")	},
	{ _T("roms/pce/")		},
	{ _T("roms/sgx/")		},
	{ _T("roms/tg16/")		},
	{ _T("roms/coleco/")	},
	{ _T("roms/sg1000/")	},
	{ _T("roms/gamegear/")	},
	{ _T("roms/sms/")		},
	{ _T("roms/msx/")		},
	{ _T("roms/spectrum/")	},
	{ _T("roms/snes/")		},
	{ _T("roms/fds/")		},
	{ _T("roms/nes/")		},
	{ _T("roms/ngp/")		},
	{ _T("roms/channelf/")	},
	{ _T("roms/romdata/")	},
	{ _T("roms/gba/")		},
	{ _T("roms/igs/")		},
	{ _T("roms/my/")		},
	{ _T("")				}
};

INT32 nRomsDlgWidth  = 0x1d2;
INT32 nRomsDlgHeight = 0x291;
static INT32 nDlgCaptionHeight;
static INT32 nDlgInitialWidth;
static INT32 nDlgInitialHeight;
static INT32 nDlgTabCtrlInitialPos[4];
static INT32 nDlgGroupCtrlInitialPos[4];
static INT32 nDlgOKBtnInitialPos[4];
static INT32 nDlgCancelBtnInitialPos[4];
static INT32 nDlgDefaultsBtnInitialPos[4];
static INT32 nDlgTextCtrlInitialPos[20][4];
static INT32 nDlgEditCtrlInitialPos[20][4];
static INT32 nDlgBtnCtrlInitialPos[20][4];

// Dialog sizing support functions and macros (everything working in client co-ords)
#define GetInititalControlPos(a, b)								\
	GetWindowRect(GetDlgItem(hRomsDlg, a), &rect);				\
	memset(&point, 0, sizeof(POINT));							\
	point.x = rect.left;										\
	point.y = rect.top;											\
	ScreenToClient(hRomsDlg, &point);							\
	b[0] = point.x;												\
	b[1] = point.y;												\
	GetClientRect(GetDlgItem(hRomsDlg, a), &rect);				\
	b[2] = rect.right;											\
	b[3] = rect.bottom;

#define SetControlPosAlignTopLeft(a, b)							\
	SetWindowPos(GetDlgItem(hRomsDlg, a), hRomsDlg, b[0], b[1], 0, 0, SWP_NOZORDER | SWP_NOSIZE);

#define SetControlPosAlignTopLeftResizeHor(a, b)				\
	SetWindowPos(GetDlgItem(hRomsDlg, a), hRomsDlg, b[0], b[1], b[2] + 4 - xDelta, b[3] + 4, SWP_NOZORDER);

#define SetControlPosAlignTopLeftResizeHorVert(a, b)			\
	SetWindowPos(GetDlgItem(hRomsDlg, a), hRomsDlg, b[0], b[1], b[2] - xDelta, b[3] - yDelta, SWP_NOZORDER);

#define SetControlPosAlignTopRight(a, b)						\
	SetWindowPos(GetDlgItem(hRomsDlg, a), hRomsDlg, b[0] - xDelta, b[1], 0, 0, SWP_NOZORDER | SWP_NOSIZE);

#define SetControlPosAlignBottomLeft(a, b)						\
	SetWindowPos(GetDlgItem(hRomsDlg, a), hRomsDlg, b[0], b[1] - yDelta, b[2], b[3], SWP_NOZORDER);

#define SetControlPosAlignBottomRight(a, b)						\
	SetWindowPos(GetDlgItem(hRomsDlg, a), hRomsDlg, b[0] - xDelta, b[1] - yDelta, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

static INT32 GetCaptionHight()
{
	RECT rect;

	GetWindowRect(hRomsDlg, &rect);
	const INT32 nWndHight = rect.bottom - rect.top;

	GetClientRect(hRomsDlg, &rect);

	return nWndHight - (rect.bottom - rect.top);
}

static void GetInitialPositions()
{
	RECT rect;
	POINT point;

	nDlgCaptionHeight = GetCaptionHight();

	GetClientRect(hRomsDlg, &rect);
	nDlgInitialWidth  = rect.right;
	nDlgInitialHeight = rect.bottom;

	GetInititalControlPos(IDC_ROMPATH_TAB, nDlgTabCtrlInitialPos);
	GetInititalControlPos(IDC_STATIC1,     nDlgGroupCtrlInitialPos);
	GetInititalControlPos(IDOK,            nDlgOKBtnInitialPos);
	GetInititalControlPos(IDCANCEL,        nDlgCancelBtnInitialPos);
	GetInititalControlPos(IDDEFAULT,       nDlgDefaultsBtnInitialPos);

	for (INT32 i = 0; i < 20; i++) {
		GetInititalControlPos(IDC_ROMSDIR_TEXT1 + i, nDlgTextCtrlInitialPos[i]);
		GetInititalControlPos(IDC_ROMSDIR_EDIT1 + i, nDlgEditCtrlInitialPos[i]);
		GetInititalControlPos(IDC_ROMSDIR_BR1   + i, nDlgBtnCtrlInitialPos[i]);
	}
}

static void CreateRomDatName(TCHAR* szRomDat)
{
	_stprintf(szRomDat, _T("config/%s.roms.dat"), szAppExeName);

	return;
}

static bool IsRelativePath(const TCHAR* pszPath)
{
	// Invalid input
	if ((NULL == pszPath) || (_T('\0' == pszPath[0]))) {
		return false;
	}

	// Check for absolute path flags (drive letter or root / network path)
	if (
		((_tcslen(pszPath) >= 3) && _istalpha(pszPath[0]) && (_T(':') == pszPath[1]) && (_T('\\' == pszPath[2]) || (_T('/') == pszPath[2]))) ||
		((_T('\\') == pszPath[0]) || (_T('/') == pszPath[0]))
	) {
		return false;
	}

	// Check that it is not a '.' or '..' starts with (e.g. . \folder or . \parent)
	if (
		(pszPath[0] == _T('.') && (pszPath[1] == _T('\0') || pszPath[1] == _T('\\') || pszPath[1] == _T('/'))) ||
		(pszPath[0] == _T('.') && pszPath[1] == _T('.') && (pszPath[2] == _T('\0') || pszPath[2] == _T('\\') || pszPath[2] == _T('/')))
	) {
		return true;
	}

	return true;
}

static UINT32 GetAppDirectory(TCHAR* pszAppPath)
{
	TCHAR szExePath[MAX_PATH] = { 0 };

	// Get executable file path
	if (0 == GetModuleFileName(NULL, szExePath, MAX_PATH))
		return 0;

	// Find last slash or backslash
	TCHAR* pLastSlash = _tcsrchr(szExePath, _T('\\'));
	if (NULL == pLastSlash) {
		pLastSlash = _tcsrchr(szExePath, _T('/'));
		if (NULL == pLastSlash)
			return 0;
	}

	// Calculate directory length (with slash or backslash)
	UINT32 nLen = pLastSlash - szExePath + 1;
	if (nLen >= MAX_PATH)
		return 0;

	if (NULL != pszAppPath) {
		_tcsncpy(pszAppPath, szExePath, nLen);
		pszAppPath[nLen] = _T('\0');
	}

	return nLen;
}

static INT32 ConvertToAbsolutePath(const TCHAR* pszPath, TCHAR* pszAbsolutePath)
{
	if (NULL == pszPath)
		return -1;

	INT32 nRet = -1;
//	TCHAR szAbsolutePath[MAX_PATH] = { 0 };

	if (IsRelativePath(pszPath)) {
		TCHAR szAppPath[MAX_PATH] = { 0 };
		UINT32 nAppPathLen = GetAppDirectory(szAppPath);
		if (0 == nAppPathLen)
			return -1;

		UINT32 nPathLen = _tcslen(pszPath);
		if ((nRet = nAppPathLen + nPathLen) >= MAX_PATH)
			return -1;

		if (NULL != pszAbsolutePath) {
			_stprintf(pszAbsolutePath, _T("%s%s"), szAppPath, pszPath);

			for (UINT32 i = 0; _T('\0') != pszAbsolutePath[i]; i++) {
				if (pszAbsolutePath[i] == _T('/')) pszAbsolutePath[i] = _T('\\');
			}
		}

		return nRet;
	}

	if ((nRet = _tcslen(pszPath)) >= MAX_PATH)
		return -1;

	if (NULL != pszAbsolutePath) {
		_tcscpy(pszAbsolutePath, pszPath);
	}

	return nRet;
}

static INT32 CALLBACK BRProc(HWND hWnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED) {
		SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, (LPARAM)lpData);
	}

	return 0;
}

//Select Directory Dialog//////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK DefInpProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HFONT  hFont         = NULL;
	HBRUSH hWhiteBGBrush = NULL;
	TCHAR szAbsolutePath[MAX_PATH] = {0};

	INT32 var;
	static bool chOk;

	switch (Msg) {
		case WM_INITDIALOG: {
			hRomsDlg = hDlg;
			chOk = false;

			// add WS_MAXIMIZEBOX button;
			SetWindowLongPtr(hDlg, GWL_STYLE, GetWindowLongPtr(hDlg, GWL_STYLE) | WS_MAXIMIZEBOX);

			HICON hIcon = LoadIcon(hAppInst, MAKEINTRESOURCE(IDI_APP));
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

			hWhiteBGBrush = CreateSolidBrush(RGB(0xff, 0xff, 0xff));

			TCHAR szDialogTitle[200];
			_stprintf(szDialogTitle, FBALoadStringEx(hAppInst, IDS_ROMS_EDIT_PATHS, true), _T(APP_TITLE), _T(SEPERATOR_1));
			SetWindowText(hDlg, szDialogTitle);

			// Setup edit controls values (ROMs Paths)
			for (INT32 x = 0; x < 20; x++) {
				SetDlgItemText(hDlg, IDC_ROMSDIR_EDIT1 + x, szAppRomPaths[x]);
			}

			// Setup the tabs
			hTabControl = GetDlgItem(hDlg, IDC_ROMPATH_TAB);
			TC_ITEM tcItem = { 0 };
			tcItem.mask = TCIF_TEXT;
			tcItem.pszText = FBALoadStringEx(hAppInst, IDS_ROMS_PATHS, true);
			TabCtrl_InsertItem(hTabControl, 0, &tcItem);

			// Font
			LOGFONT lf = { 0 };
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(LOGFONT), &lf, 0);
			lf.lfHeight = -MulDiv(9, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
			lf.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect(&lf);
			SendMessage(hTabControl, WM_SETFONT, (WPARAM)hFont, TRUE);

			GetInitialPositions();
			SetWindowPos(hDlg, NULL, 0, 0, nRomsDlgWidth, nRomsDlgHeight, SWP_NOZORDER);

			UpdateWindow(hDlg);
			WndInMid(hDlg, hParent);
			SetFocus(hDlg);														// Enable Esc=close
			break;
		}
		case WM_CTLCOLORSTATIC: {
			HDC hDc = (HDC)wParam;
			for (int x = 0; x < 20; x++) {
				if ((HWND)lParam == GetDlgItem(hDlg, IDC_ROMSDIR_TEXT1 + x)) {
					SetBkMode(hDc, TRANSPARENT);								// LTEXT control with transparent background
					return (LRESULT)GetStockObject(HOLLOW_BRUSH);				// Return to empty brush
				}
			}
			break;
		}
		case WM_GETMINMAXINFO: {
			MINMAXINFO *info = (MINMAXINFO*)lParam;
			info->ptMinTrackSize.x = nDlgInitialWidth;
			info->ptMinTrackSize.y = nDlgInitialHeight + nDlgCaptionHeight;
			return 0;
		}
		case WM_SIZE: {
			if (nDlgInitialWidth == 0 || nDlgInitialHeight == 0) return 0;

			RECT rc;
			GetClientRect(hDlg, &rc);

			const INT32 xDelta = nDlgInitialWidth  - rc.right;
			const INT32 yDelta = nDlgInitialHeight - rc.bottom;
			if (xDelta == 0 && yDelta == 0) return 0;

			SetControlPosAlignTopLeftResizeHorVert(IDC_ROMPATH_TAB, nDlgTabCtrlInitialPos);
			SetControlPosAlignTopLeftResizeHorVert(IDC_STATIC1,     nDlgGroupCtrlInitialPos);

			SetControlPosAlignBottomRight(IDOK,     nDlgOKBtnInitialPos);
			SetControlPosAlignBottomRight(IDCANCEL, nDlgCancelBtnInitialPos);

			SetControlPosAlignBottomLeft(IDDEFAULT, nDlgDefaultsBtnInitialPos);

			for (INT32 i = 0; i < 20; i++) {
				SetControlPosAlignTopLeft(IDC_ROMSDIR_TEXT1          + i, nDlgTextCtrlInitialPos[i]);
				SetControlPosAlignTopLeftResizeHor(IDC_ROMSDIR_EDIT1 + i, nDlgEditCtrlInitialPos[i]);
				SetControlPosAlignTopRight(IDC_ROMSDIR_BR1           + i, nDlgBtnCtrlInitialPos[i]);
			}

			InvalidateRect(hDlg, NULL, true);
			UpdateWindow(hDlg);

			return 0;
		}
		case WM_COMMAND: {
			LPMALLOC pMalloc = NULL;
			BROWSEINFO bInfo;
			ITEMIDLIST* pItemIDList = NULL;
			TCHAR buffer[MAX_PATH];

			if (LOWORD(wParam) == IDOK) {
				for (int i = 0; i < 20; i++) {
//					if (GetDlgItemText(hDlg, IDC_ROMSDIR_EDIT1 + i, buffer, sizeof(buffer)) && lstrcmp(szAppRomPaths[i], buffer)) {
					GetDlgItemText(hDlg, IDC_ROMSDIR_EDIT1 + i, buffer, sizeof(buffer));
					if (lstrcmp(szAppRomPaths[i], buffer)) chOk = true;

					// add trailing backslash
					INT32 strLen = _tcslen(buffer);
					if (strLen) {
						if (buffer[strLen - 1] != _T('\\') && buffer[strLen - 1] != _T('/') ) {
							buffer[strLen + 0]  = _T('\\');
							buffer[strLen + 1]  = _T('\0');
						}
					}
					_tcscpy(szAppRomPaths[i], buffer);
//					}
				}

				SendMessage(hDlg, WM_CLOSE, 0, 0);
				break;
			} else {
				if (LOWORD(wParam) >= IDC_ROMSDIR_BR1 && LOWORD(wParam) <= IDC_ROMSDIR_BR20) {
					var = IDC_ROMSDIR_EDIT1 + LOWORD(wParam) - IDC_ROMSDIR_BR1;

					TCHAR szPath[MAX_PATH] = { 0 };
					GetDlgItemText(hDlg, var, szPath, sizeof(szPath));
					ConvertToAbsolutePath(szPath, szAbsolutePath);
				} else {
					if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDCANCEL) {
						SendMessage(hDlg, WM_CLOSE, 0, 0);
					}
					else
					if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDDEFAULT) {
						if (IDOK == MessageBox(
							hDlg,
							FBALoadStringEx(hAppInst, IDS_EDIT_DEFAULTS, true),
							FBALoadStringEx(hAppInst, IDS_ERR_WARNING,   true),
							MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2)
						) {
							for (INT32 x = 0; x < 20; x++) {
								if (lstrcmp(szAppRomPaths[x], szAppDefaultPaths[x])) chOk = true;
								_tcscpy(szAppRomPaths[x], szAppDefaultPaths[x]);
								SetDlgItemText(hDlg, IDC_ROMSDIR_EDIT1 + x, szAppRomPaths[x]);
							}

							UpdateWindow(hDlg);
						}
					}
					break;
				}
			}

		    SHGetMalloc(&pMalloc);

			memset(&bInfo, 0, sizeof(bInfo));
			bInfo.hwndOwner      = hDlg;
			bInfo.pszDisplayName = buffer;
			bInfo.lpszTitle      = FBALoadStringEx(hAppInst, IDS_SELECT_DIR, true);
			bInfo.ulFlags        = BIF_EDITBOX | BIF_RETURNONLYFSDIRS;
			if (S_OK == nCOMInit) {
				bInfo.ulFlags   |= BIF_NEWDIALOGSTYLE;	// Caller needs to call CoInitialize() / OleInitialize() before using API (main.cpp)
			}
			bInfo.lpfn           = BRProc;
			bInfo.lParam         = (LPARAM)szAbsolutePath;

			pItemIDList = SHBrowseForFolder(&bInfo);

			if (pItemIDList) {
				if (SHGetPathFromIDList(pItemIDList, buffer)) {
					int strLen = _tcslen(buffer);
					if (strLen) {
						if (buffer[strLen - 1] != _T('\\')) {
							buffer[strLen]      = _T('\\');
							buffer[strLen + 1]  = _T('\0');
						}
						SetDlgItemText(hDlg, var, buffer);
					}
				}
				pMalloc->Free(pItemIDList);
			}
			pMalloc->Release();
			break;
		}
		case WM_CLOSE: {
			RECT rect;

			GetWindowRect(hDlg, &rect);
			nRomsDlgWidth  = rect.right - rect.left;
			nRomsDlgHeight = rect.bottom - rect.top;

			EndDialog(hDlg, 0);
			if (chOk && bSkipStartupCheck == false) {
				bRescanRoms = true;
				CreateROMInfo(hDlg);
			}
			if (NULL != hFont) {
				DeleteObject(hFont);         hFont         = NULL;
			}
			if (NULL != hWhiteBGBrush) {
				DeleteObject(hWhiteBGBrush); hWhiteBGBrush = NULL;
			}
			hParent  = NULL;
			hRomsDlg = NULL;
		}
	}

	return 0;
}


INT32 RomsDirCreate(HWND hParentWND)
{
	hParent = hParentWND;

	FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_ROMSDIR), hParent, (DLGPROC)DefInpProc);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Check Romsets Dialog/////////////////////////////////////////////////////////////////////////////

INT32 WriteGameAvb()
{
	TCHAR szRomDat[MAX_PATH];
	FILE* h;

	CreateRomDatName(szRomDat);

	if ((h = _tfopen(szRomDat, _T("wt"))) == NULL) {
		return 1;
	}

	_ftprintf(h, _T(APP_TITLE) _T(" v%.20s ROMs"), szAppBurnVer);	// identifier
	_ftprintf(h, _T(" 0x%04X "), nBurnDrvCount);					// no of games

	for (unsigned int i = 0; i < nBurnDrvCount; i++) {
		if (gameAv[i] & 2) {
			_fputtc(_T('*'), h);
		} else {
			if (gameAv[i] & 1) {
				_fputtc(_T('+'), h);
			} else {
				_fputtc(_T('-'), h);
			}
		}
	}

	_ftprintf(h, _T(" END"));									// end marker

	fclose(h);

	return 0;
}

static int DoCheck(TCHAR* buffPos)
{
	TCHAR label[256];

	// Check identifier
	memset(label, 0, sizeof(label));
	_stprintf(label, _T(APP_TITLE) _T(" v%.20s ROMs"), szAppBurnVer);
	if ((buffPos = LabelCheck(buffPos, label)) == NULL) {
		return 1;
	}

	// Check no of supported games
	memset(label, 0, sizeof(label));
	memcpy(label, buffPos, 16);
	buffPos += 8;
	unsigned int n = _tcstol(label, NULL, 0);
	if (n != nBurnDrvCount) {
		return 1;
	}

	for (unsigned int i = 0; i < nBurnDrvCount; i++) {
		if (*buffPos == _T('*')) {
			gameAv[i] = 3;
		} else {
			if (*buffPos == _T('+')) {
				gameAv[i] = 1;
			} else {
				if (*buffPos == _T('-')) {
					gameAv[i] = 0;
				} else {
					return 1;
				}
			}
		}

		buffPos++;
	}

	memset(label, 0, sizeof(label));
	_stprintf(label, _T(" END"));
	if (LabelCheck(buffPos, label) == NULL) {
		avOk = true;
		return 0;
	} else {
		return 1;
	}
}

int CheckGameAvb()
{
	TCHAR szRomDat[MAX_PATH];
	FILE* h;
	int bOK;
	int nBufferSize = nBurnDrvCount + 256;
	TCHAR* buffer = (TCHAR*)malloc(nBufferSize * sizeof(TCHAR));
	if (buffer == NULL) {
		return 1;
	}

	memset(buffer, 0, nBufferSize * sizeof(TCHAR));
	CreateRomDatName(szRomDat);

	if ((h = _tfopen(szRomDat, _T("r"))) == NULL) {
		if (buffer)
		{
			free(buffer);
			buffer = NULL;
		}
		return 1;
	}

	_fgetts(buffer, nBufferSize, h);
	fclose(h);

	bOK = DoCheck(buffer);

	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
	return bOK;
}

static void RomScanEnsureCacheLock()
{
	if (!g_bRomScanCacheLockInit) {
		InitializeCriticalSection(&g_RomScanCacheLock);
		g_bRomScanCacheLockInit = true;
	}
}

static void RomScanClearArchiveExistenceCache()
{
	if (!g_bRomScanCacheLockInit) {
		return;
	}

	EnterCriticalSection(&g_RomScanCacheLock);
	g_RomScanExistingArchives.clear();
	g_RomScanMissingArchives.clear();
	LeaveCriticalSection(&g_RomScanCacheLock);
}

static bool RomScanFileExists(const TCHAR* pszName)
{
	if (pszName == NULL || pszName[0] == _T('\0')) {
		return false;
	}

	DWORD dwAttrib = GetFileAttributes(pszName);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static bool RomScanArchiveExists(const TCHAR* pszName)
{
	if (pszName == NULL || pszName[0] == _T('\0')) {
		return false;
	}

	RomScanEnsureCacheLock();
	std::basic_string<TCHAR> key(pszName);

	EnterCriticalSection(&g_RomScanCacheLock);
	if (g_RomScanExistingArchives.find(key) != g_RomScanExistingArchives.end()) {
		LeaveCriticalSection(&g_RomScanCacheLock);
		return true;
	}
	if (g_RomScanMissingArchives.find(key) != g_RomScanMissingArchives.end()) {
		LeaveCriticalSection(&g_RomScanCacheLock);
		return false;
	}
	LeaveCriticalSection(&g_RomScanCacheLock);

	TCHAR szFileName[MAX_PATH];
	_stprintf(szFileName, _T("%s.zip"), pszName);
	bool bFound = RomScanFileExists(szFileName);

#ifdef INCLUDE_7Z_SUPPORT
	if (!bFound) {
		_stprintf(szFileName, _T("%s.7z"), pszName);
		bFound = RomScanFileExists(szFileName);
	}
#endif

	EnterCriticalSection(&g_RomScanCacheLock);
	if (bFound) {
		g_RomScanExistingArchives.insert(key);
	} else {
		g_RomScanMissingArchives.insert(key);
	}
	LeaveCriticalSection(&g_RomScanCacheLock);

	return bFound;
}

static const TCHAR* RomScanGetFilename(const TCHAR* pszFull)
{
	if (pszFull == NULL) {
		return _T("");
	}

	INT32 nLen = _tcslen(pszFull);
	for (INT32 i = nLen - 1; i >= 0; i--) {
		if (pszFull[i] == _T('\\') || pszFull[i] == _T('/')) {
			return pszFull + i + 1;
		}
	}

	return pszFull;
}

static void RomScanFreeZipList(struct ZipEntry* pList, INT32 nListCount)
{
	if (pList == NULL) {
		return;
	}

	for (INT32 i = 0; i < nListCount; i++) {
		if (pList[i].szName) {
			free(pList[i].szName);
			pList[i].szName = NULL;
		}
	}

	free(pList);
}

static INT32 RomScanGetArchiveList(const TCHAR* pszArchiveName, struct ZipEntry** pList, INT32* pnListCount)
{
	if (pList == NULL || pszArchiveName == NULL || pszArchiveName[0] == _T('\0')) {
		return 1;
	}

	*pList = NULL;
	if (pnListCount) {
		*pnListCount = 0;
	}

	char szArchiveA[(MAX_PATH * 4) + 8] = { 0 };
	char szFileName[(MAX_PATH * 4) + 8] = { 0 };
	TCHARToANSI(pszArchiveName, szArchiveA, sizeof(szArchiveA));

	size_t nArchiveLen = strlen(szArchiveA);
	if ((nArchiveLen + 5) >= sizeof(szFileName)) {
		return 1;
	}
	memcpy(szFileName, szArchiveA, nArchiveLen);
	memcpy(szFileName + nArchiveLen, ".zip", 5);
	unzFile zip = unzOpen(szFileName);
	if (zip != NULL) {
		unz_global_info ZipGlobalInfo;
		memset(&ZipGlobalInfo, 0, sizeof(ZipGlobalInfo));
		unzGetGlobalInfo(zip, &ZipGlobalInfo);

		INT32 nListLen = ZipGlobalInfo.number_entry;
		struct ZipEntry* List = (struct ZipEntry*)malloc(nListLen * sizeof(struct ZipEntry));
		if (List == NULL) {
			unzClose(zip);
			return 1;
		}
		memset(List, 0, nListLen * sizeof(struct ZipEntry));

		if (unzGoToFirstFile(zip) != UNZ_OK) {
			RomScanFreeZipList(List, nListLen);
			unzClose(zip);
			return 1;
		}

		for (INT32 i = 0, nNextRet = UNZ_OK; i < nListLen && nNextRet == UNZ_OK; i++, nNextRet = unzGoToNextFile(zip)) {
			unz_file_info FileInfo;
			memset(&FileInfo, 0, sizeof(FileInfo));

			if (unzGetCurrentFileInfo(zip, &FileInfo, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK) {
				continue;
			}

			char* szName = (char*)malloc(FileInfo.size_filename + 1);
			if (szName == NULL) {
				continue;
			}

			if (unzGetCurrentFileInfo(zip, &FileInfo, szName, FileInfo.size_filename + 1, NULL, 0, NULL, 0) != UNZ_OK) {
				free(szName);
				continue;
			}

			List[i].szName = szName;
			List[i].nLen = FileInfo.uncompressed_size;
			List[i].nCrc = FileInfo.crc;
		}

		*pList = List;
		if (pnListCount) {
			*pnListCount = nListLen;
		}
		unzClose(zip);
		return 0;
	}

#ifdef INCLUDE_7Z_SUPPORT
	_7z_file* p7zFile = NULL;
	memset(szFileName, 0, sizeof(szFileName));
	if ((nArchiveLen + 4) >= sizeof(szFileName)) {
		return 1;
	}
	memcpy(szFileName, szArchiveA, nArchiveLen);
	memcpy(szFileName + nArchiveLen, ".7z", 4);
	if (_7z_file_open(szFileName, &p7zFile) == _7ZERR_NONE) {
		UInt16* temp = NULL;
		size_t tempSize = 0;
		INT32 nListLen = p7zFile->db.NumFiles;
		struct ZipEntry* List = (struct ZipEntry*)malloc(nListLen * sizeof(struct ZipEntry));
		if (List == NULL) {
			_7z_file_close(p7zFile);
			return 1;
		}
		memset(List, 0, nListLen * sizeof(struct ZipEntry));

		for (UINT32 i = 0; i < p7zFile->db.NumFiles; i++) {
			size_t len = SzArEx_GetFileNameUtf16(&p7zFile->db, i, NULL);
			if (len > tempSize) {
				SZipFree(NULL, temp);
				tempSize = len;
				temp = (UInt16*)SZipAlloc(NULL, tempSize * sizeof(temp[0]));
				if (temp == NULL) {
					RomScanFreeZipList(List, nListLen);
					_7z_file_close(p7zFile);
					return 1;
				}
			}

			SzArEx_GetFileNameUtf16(&p7zFile->db, i, temp);
			char* szName = (char*)malloc(len * 2 * sizeof(char));
			if (szName == NULL) {
				continue;
			}

			for (UINT32 j = 0; j < len; j++) {
				szName[j + 0] = temp[j] & 0xff;
				szName[j + 1] = temp[j] >> 8;
			}

			List[i].szName = szName;
			List[i].nLen = (UINT32)SzArEx_GetFileSize(&p7zFile->db, i);
			List[i].nCrc = p7zFile->db.CRCs.Vals[i];
		}

		SZipFree(NULL, temp);
		_7z_file_close(p7zFile);

		*pList = List;
		if (pnListCount) {
			*pnListCount = nListLen;
		}
		return 0;
	}
#endif

	return 1;
}

static INT32 RomScanFindRomByCrc(UINT32 nCrc, struct ZipEntry* pList, INT32 nListCount)
{
	for (INT32 i = 0; i < nListCount; i++) {
		if (pList[i].nCrc == nCrc) {
			return i;
		}
	}

	return -1;
}

static INT32 RomScanFindRomByName(const TCHAR* pszName, struct ZipEntry* pList, INT32 nListCount)
{
	if (pszName == NULL || pszName[0] == _T('\0')) {
		return -1;
	}

	for (INT32 i = 0; i < nListCount; i++) {
		if (pList[i].szName == NULL) {
			continue;
		}

		TCHAR szCurrentName[MAX_PATH] = { 0 };
		if (_tcsicmp(pszName, RomScanGetFilename(ANSIToTCHAR(pList[i].szName, szCurrentName, MAX_PATH))) == 0) {
			return i;
		}
	}

	return -1;
}

static INT32 RomScanFindRom(const RomScanRomEntry& rom, struct ZipEntry* pList, INT32 nListCount)
{
	if (rom.nCrc) {
		INT32 nRet = RomScanFindRomByCrc(rom.nCrc, pList, nListCount);
		if (nRet >= 0) {
			return nRet;
		}
	}

	for (size_t i = 0; i < rom.aliases.size(); i++) {
		INT32 nRet = RomScanFindRomByName(rom.aliases[i].c_str(), pList, nListCount);
		if (nRet >= 0) {
			return nRet;
		}
	}

	return -1;
}

static bool RomScanIsMgbaHardware(UINT32 nHardwareMask)
{
	return nHardwareMask == HARDWARE_GBA || nHardwareMask == HARDWARE_GB || nHardwareMask == HARDWARE_GBC;
}

static INT32 RomScanGetMgbaFallbackSubdirs(UINT32 nHardwareMask, const TCHAR** subdirs, INT32 maxCount)
{
	if (subdirs == NULL || maxCount <= 0) {
		return 0;
	}

	INT32 count = 0;
	switch (nHardwareMask) {
		case HARDWARE_GBA:
			if (count < maxCount) subdirs[count++] = _T("mgba/");
			break;
		case HARDWARE_GB:
			if (count < maxCount) subdirs[count++] = _T("mgb/");
			if (count < maxCount) subdirs[count++] = _T("mgba/");
			break;
		case HARDWARE_GBC:
			if (count < maxCount) subdirs[count++] = _T("mgbc/");
			if (count < maxCount) subdirs[count++] = _T("mgba/");
			break;
	}

	return count;
}

static bool RomScanBuildMgbaFallbackPath(TCHAR* dst, INT32 nDstCount, const TCHAR* basePath, const TCHAR* subdir, const TCHAR* archiveName)
{
	if (dst == NULL || nDstCount <= 0 || basePath == NULL || subdir == NULL || archiveName == NULL) {
		return false;
	}

	if (_tcslen(basePath) == 0) {
		return false;
	}

	size_t nBaseLen = _tcslen(basePath);
	_stprintf(dst, _T("%s%s%s"), basePath, subdir, archiveName);

	if (dst[nBaseLen] != _T('\\') && dst[nBaseLen] != _T('/')) {
		return false;
	}

	return true;
}

static void RomScanAddUniqueArchive(std::vector<std::basic_string<TCHAR> >& archives, const TCHAR* pszArchive)
{
	if (pszArchive == NULL || pszArchive[0] == _T('\0')) {
		return;
	}

	for (size_t i = 0; i < archives.size(); i++) {
		if (_tcsicmp(archives[i].c_str(), pszArchive) == 0) {
			return;
		}
	}

	archives.push_back(std::basic_string<TCHAR>(pszArchive));
}

static void RomScanBuildArchiveCandidates(const RomScanDriverEntry& driver, std::vector<std::basic_string<TCHAR> >& archives)
{
	const INT32 nAppRomPaths = (nQuickOpen > 0) ? DIRS_MAX + 1 : DIRS_MAX;
	const bool bScanSubDirs = (nLoadMenuShowY & (1 << 26)) != 0;

	for (size_t y = 0; y < driver.archiveNames.size(); y++) {
		const TCHAR* pszArchiveName = driver.archiveNames[y].c_str();

		for (INT32 d = 0; d < nAppRomPaths; d++) {
			const TCHAR* pszBasePath = (d < DIRS_MAX) ? szAppRomPaths[d] : szAppQuickPath;
			if (pszBasePath == NULL || pszBasePath[0] == _T('\0')) {
				continue;
			}

			if (bScanSubDirs && d < DIRS_MAX) {
				for (UINT32 nCount = 0; nCount < _SubDirInfo[d].nCount; nCount++) {
					TCHAR szFullName[MAX_PATH] = { 0 };
					if ((_tcslen(_SubDirInfo[d].SubDirs[nCount]) + _tcslen(pszArchiveName)) > (MAX_PATH - 5)) {
						continue;
					}

					_stprintf(szFullName, _T("%s%s"), _SubDirInfo[d].SubDirs[nCount], pszArchiveName);
					if (RomScanArchiveExists(szFullName)) {
						RomScanAddUniqueArchive(archives, szFullName);
					}
				}
			}

			TCHAR szFullName[MAX_PATH] = { 0 };
			_stprintf(szFullName, _T("%s%s"), pszBasePath, pszArchiveName);
			if (RomScanArchiveExists(szFullName)) {
				RomScanAddUniqueArchive(archives, szFullName);
			}

			if (RomScanIsMgbaHardware(driver.nHardwareMask)) {
				const TCHAR* fallbackSubdirs[4] = { NULL, };
				const INT32 fallbackCount = RomScanGetMgbaFallbackSubdirs(driver.nHardwareMask, fallbackSubdirs, 4);
				for (INT32 f = 0; f < fallbackCount; f++) {
					TCHAR szFallback[MAX_PATH] = { 0 };
					if (RomScanBuildMgbaFallbackPath(szFallback, _countof(szFallback), pszBasePath, fallbackSubdirs[f], pszArchiveName) && RomScanArchiveExists(szFallback)) {
						RomScanAddUniqueArchive(archives, szFallback);
					}
				}
			}
		}
	}
}

static void RomScanBuildDriverSnapshot(RomScanDriverEntry& entry, UINT32 nDrvIndex)
{
	entry.nDriverIndex = nDrvIndex;
	entry.archiveNames.clear();
	entry.roms.clear();
	entry.hddName.clear();
	entry.hddFolderName.clear();

	const INT32 nOldActive = nBurnDrvActive;
	nBurnDrvActive = nDrvIndex;

	entry.nHardwareMask = BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK;

	for (INT32 y = 0; y < BZIP_MAX; y++) {
		char* szName = NULL;
		if (BurnDrvGetZipName(&szName, y)) {
			break;
		}

		TCHAR szNameT[MAX_PATH] = { 0 };
		ANSIToTCHAR(szName, szNameT, MAX_PATH);
		entry.archiveNames.push_back(std::basic_string<TCHAR>(szNameT));
	}

	for (INT32 i = 0; ; i++) {
		BurnRomInfo ri;
		memset(&ri, 0, sizeof(ri));
		if (BurnDrvGetRomInfo(&ri, i)) {
			break;
		}

		RomScanRomEntry rom;
		rom.nLen = ri.nLen;
		rom.nCrc = ri.nCrc;
		rom.nType = ri.nType;

		for (INT32 nAka = 0; nAka < 0x10000; nAka++) {
			char* szAlias = NULL;
			if (BurnDrvGetRomName(&szAlias, i, nAka)) {
				break;
			}

			TCHAR szAliasT[MAX_PATH] = { 0 };
			ANSIToTCHAR(szAlias, szAliasT, MAX_PATH);
			rom.aliases.push_back(std::basic_string<TCHAR>(szAliasT));
		}

		entry.roms.push_back(rom);
	}

	char* szHddName = NULL;
	BurnDrvGetHDDName(&szHddName, 0, 0);
	if (szHddName != NULL) {
		TCHAR szHddNameT[MAX_PATH] = { 0 };
		ANSIToTCHAR(szHddName, szHddNameT, MAX_PATH);
		entry.hddName = std::basic_string<TCHAR>(szHddNameT);

		const TCHAR* pszFolder = BurnDrvGetText(DRV_PARENT);
		if (pszFolder == NULL) {
			pszFolder = BurnDrvGetText(DRV_NAME);
		}
		if (pszFolder != NULL) {
			entry.hddFolderName = std::basic_string<TCHAR>(pszFolder);
		}
	}

	nBurnDrvActive = nOldActive;
}

static void RomScanBuildSnapshot()
{
	g_RomScanDrivers.clear();
	g_RomScanDrivers.reserve(nBurnDrvCount);

	for (UINT32 i = 0; i < nBurnDrvCount; i++) {
		if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
			return;
		}

		RomScanDriverEntry entry;
		RomScanBuildDriverSnapshot(entry, i);
		g_RomScanDrivers.push_back(entry);
	}
}

static INT32 RomScanCheckDriverHdd(const RomScanDriverEntry& driver)
{
	if (driver.hddName.empty() || driver.hddFolderName.empty()) {
		return 0;
	}

	TCHAR szHddPath[MAX_PATH] = { 0 };
	_stprintf(szHddPath, _T("%s%s/%s"), szAppHDDPath, driver.hddFolderName.c_str(), driver.hddName.c_str());
	return RomScanFileExists(szHddPath) ? 0 : 1;
}

static INT32 RomScanAnalyzeDriver(const RomScanDriverEntry& driver)
{
	if (driver.roms.empty()) {
		return 1;
	}

	std::vector<std::basic_string<TCHAR> > archives;
	RomScanBuildArchiveCandidates(driver, archives);

	std::vector<INT32> romStates(driver.roms.size(), 0);

	for (size_t a = 0; a < archives.size(); a++) {
		if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
			return -1;
		}

		struct ZipEntry* pList = NULL;
		INT32 nListCount = 0;
		if (RomScanGetArchiveList(archives[a].c_str(), &pList, &nListCount) != 0) {
			continue;
		}

		for (size_t i = 0; i < driver.roms.size(); i++) {
			if (romStates[i] == 1) {
				continue;
			}

			INT32 nFind = RomScanFindRom(driver.roms[i], pList, nListCount);
			if (nFind < 0) {
				continue;
			}

			romStates[i] = 1;
			if (pList[nFind].nLen == driver.roms[i].nLen) {
				if ((nLoadMenuShowY & DISABLECRCVERIFY) == 0 && driver.roms[i].nCrc && pList[nFind].nCrc != driver.roms[i].nCrc) {
					romStates[i] = 2;
				}
			} else {
				romStates[i] = (pList[nFind].nLen < driver.roms[i].nLen) ? 3 : 4;
			}
		}

		RomScanFreeZipList(pList, nListCount);
	}

	if (RomScanCheckDriverHdd(driver) != 0) {
		return 1;
	}

	INT32 nRet = 0;
	for (size_t i = 0; i < driver.roms.size(); i++) {
		INT32 nState = romStates[i];
		if ((nLoadMenuShowY & DISABLECRCVERIFY) && nState == 2) {
			nState = 1;
		}

		if (nState != 1 && driver.roms[i].nType && driver.roms[i].nCrc) {
			if ((driver.roms[i].nType & BRF_OPT) == 0 && (driver.roms[i].nType & BRF_NODUMP) == 0) {
				return 1;
			}
			nRet = 2;
		}
	}

	return nRet;
}

static unsigned __stdcall AnalyzeRomWorker(void*)
{
	for (;;) {
		if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
			return 0;
		}

		LONG nIndex = InterlockedIncrement(&g_nRomScanNextIndex) - 1;
		if (nIndex < 0 || nIndex >= (LONG)g_RomScanDrivers.size()) {
			return 0;
		}

		const RomScanDriverEntry& driver = g_RomScanDrivers[(size_t)nIndex];
		INT32 nResult = RomScanAnalyzeDriver(driver);
		if (nResult >= 0) {
			switch (nResult) {
				case 0: gameAv[driver.nDriverIndex] = 3; break;
				case 2: gameAv[driver.nDriverIndex] = 1; break;
				default: gameAv[driver.nDriverIndex] = 0; break;
			}
		}

		LONG nCompleted = InterlockedIncrement(&g_nRomScanCompleted);
		if (((nCompleted % 32) == 0 || nCompleted == (LONG)nBurnDrvCount) && hRomsDlg != NULL) {
			SendDlgItemMessage(hRomsDlg, IDC_WAIT_PROG, PBM_SETPOS, (WPARAM)nCompleted, 0);
		}
	}
}

static int QuitRomsScan()
{
	DWORD dwExitCode;

	GetExitCodeThread(hScanThread, &dwExitCode);

	if (dwExitCode == STILL_ACTIVE) {

		// Signal the scan thread to abort
		SetEvent(hEvent);

		// Wait for the thread to finish
		if (WaitForSingleObject(hScanThread, 10000) != WAIT_OBJECT_0) {
			// If the thread doesn't finish within 10 seconds, forcibly kill it
			TerminateThread(hScanThread, 1);
		}

		CloseHandle(hScanThread);
	}

	CloseHandle(hEvent);

	hEvent = NULL;

	hScanThread = NULL;
	ScanThreadId = 0;

	BzipClose();
	BzipEnableArchiveExistenceCache(false);
	RomScanClearArchiveExistenceCache();
	g_RomScanDrivers.clear();

	nBurnDrvActive = nOldSelect;
	nOldSelect = 0;
	bRescanRoms  = false;

	if (avOk) {
		WriteGameAvb();
	}

	return 1;
}

static unsigned __stdcall AnalyzingRoms(void*)
{
	RomScanBuildSnapshot();
	if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
		return 0;
	}

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	UINT32 nWorkerCount = sysInfo.dwNumberOfProcessors;
	if (nWorkerCount < 1) {
		nWorkerCount = 1;
	}
	if (nWorkerCount > 8) {
		nWorkerCount = 8;
	}
	if (nWorkerCount > nBurnDrvCount) {
		nWorkerCount = nBurnDrvCount;
	}
	if (nWorkerCount < 1) {
		nWorkerCount = 1;
	}

	g_nRomScanNextIndex = 0;
	g_nRomScanCompleted = 0;
	std::vector<HANDLE> workerThreads;
	workerThreads.reserve(nWorkerCount);

	for (UINT32 i = 0; i < nWorkerCount; i++) {
		UINT32 workerId = 0;
		HANDLE hWorker = (HANDLE)_beginthreadex(NULL, 0, AnalyzeRomWorker, NULL, 0, &workerId);
		if (hWorker != NULL) {
			workerThreads.push_back(hWorker);
		}
	}

	if (workerThreads.empty()) {
		AnalyzeRomWorker(NULL);
	} else {
		WaitForMultipleObjects((DWORD)workerThreads.size(), &workerThreads[0], TRUE, INFINITE);
		for (size_t i = 0; i < workerThreads.size(); i++) {
			CloseHandle(workerThreads[i]);
		}
	}

	g_RomScanDrivers.clear();
	RomScanClearArchiveExistenceCache();

	if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
		return 0;
	}

	avOk = true;
	PostMessage(hRomsDlg, WM_CLOSE, 0, 0);

	return 0;
}

//Add these two static variables
static int xClick;
static int yClick;

static INT_PTR CALLBACK WaitProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)		// LPARAM lParam
{
	switch (Msg) {
		case WM_INITDIALOG: {
			hRomsDlg = hDlg;
			nOldSelect = nBurnDrvActive;
			memset(gameAv, 0, nBurnDrvCount); // clear game list
			SendDlgItemMessage(hDlg, IDC_WAIT_PROG, PBM_SETRANGE, 0, MAKELPARAM(0, nBurnDrvCount));
			SendDlgItemMessage(hDlg, IDC_WAIT_PROG, PBM_SETSTEP, (WPARAM)1, 0);

			ShowWindow(GetDlgItem(hDlg, IDC_WAIT_LABEL_A), TRUE);
			SendMessage(GetDlgItem(hDlg, IDC_WAIT_LABEL_A), WM_SETTEXT, (WPARAM)0, (LPARAM)FBALoadStringEx(hAppInst, IDS_SCANNING_ROMS, true));
			ShowWindow(GetDlgItem(hDlg, IDCANCEL), TRUE);

			avOk = false;
			hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			BzipEnableArchiveExistenceCache(true);
			hScanThread = (HANDLE)_beginthreadex(NULL, 0, AnalyzingRoms, NULL, 0, &ScanThreadId);

			if (hParent == NULL) {
#if 0
				RECT rect;
				int x, y;

				SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

				x = 315 + GetSystemMetrics(SM_CXDLGFRAME) * 2 + 6;
				y = 74 + GetSystemMetrics(SM_CYDLGFRAME) * 2 + 6;

				SetForegroundWindow(hDlg);
				SetWindowPos(hDlg, HWND_TOPMOST, (rect.right - rect.left) / 2 - x / 2, (rect.bottom - rect.top) / 2 - y / 2, x, y, 0);
				RedrawWindow(hDlg, NULL, NULL, 0);
				ShowWindow(hDlg, SW_SHOWNORMAL);
#endif
				WndInMid(hDlg, hParent);
				SetFocus(hDlg);		// Enable Esc=close
			} else {
				WndInMid(hDlg, hParent);
				SetFocus(hDlg);		// Enable Esc=close
			}

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
				GetWindowRect(hDlg,&rcWindow);

				int xMouse = GET_X_LPARAM(lParam);
				int yMouse = GET_Y_LPARAM(lParam);
				int xWindow = rcWindow.left + xMouse - xClick;
				int yWindow = rcWindow.top + yMouse - yClick;

				SetWindowPos(hDlg,NULL,xWindow,yWindow,0,0,SWP_NOSIZE|SWP_NOZORDER);
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
			QuitRomsScan();
			EndDialog(hDlg, 0);
			hRomsDlg = NULL;
			hParent = NULL;
		}
	}

	return 0;
}

INT32 CreateROMInfo(HWND hParentWND)
{
	SubDirThreadExit();

	hParent = hParentWND;
	bool bStarting = 0;

	if (gameAv == NULL) {
		gameAv = (char*)malloc(nBurnDrvCount);
		memset(gameAv, 0, nBurnDrvCount);
		bStarting = 1;
	}

	if (gameAv) {
		if (CheckGameAvb() || bRescanRoms) {
			if ((bStarting && bSkipStartupCheck == false) || bRescanRoms) {
				FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_WAIT), hParent, (DLGPROC)WaitProc);
			}
		}
	}

	return 1;
}

void FreeROMInfo()
{
	if (gameAv) {
		free(gameAv);
		gameAv = NULL;
	}
}

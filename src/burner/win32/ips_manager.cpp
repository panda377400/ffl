// FB Neo IPS Mangler^H^H^H^H^H^H^HManager
#include "burner.h"

#define NUM_LANGUAGES		12
#define MAX_NODES			1024
#define MAX_ACTIVE_PATCHES	1024
#define IDC_IPS_CTX_EDIT	0x5401
#define IDC_IPS_CTX_OPEN	0x5402
#define IDC_IPS_CTX_DELETE	0x5403

static HWND hIpsDlg			= NULL;
static HWND hParent			= NULL;
static HWND hIpsList		= NULL;
static bool bIpsHostedMode  = false;
static HWND hIpsHostedPreview = NULL;
static HWND hIpsHostedDesc    = NULL;
static HWND hIpsHostedLang    = NULL;

INT32 nIpsSelectedLanguage	= 0;
static TCHAR szFullName[1024];
static TCHAR szLanguages[NUM_LANGUAGES][32];
static TCHAR szLanguageCodes[NUM_LANGUAGES][6];
static TCHAR szPngName[MAX_PATH];

static HTREEITEM hItemHandles[MAX_NODES];

static INT32 nPatchIndex	= 0;
static INT32 nNumPatches	= 0;
static HTREEITEM hPatchHandlesIndex[MAX_NODES];
static TCHAR szPatchFileNames[MAX_NODES][MAX_PATH];

static HBRUSH hWhiteBGBrush;
static HBITMAP hBmp			= NULL;
static HBITMAP hPreview		= NULL;
static LONG_PTR nIpsHostedOriginalTreeStyle = 0;
static bool bIpsHostedHaveOriginalTreeStyle = false;

static TCHAR szStorageName[MAX_PATH];
static TCHAR szHostedStorageName[MAX_PATH];

INT32 nRomOffset			= 0;
UINT32 nIpsDrvDefine		= 0, nIpsMemExpLen[SND2_ROM + 1] = { 0 };

static TCHAR szIpsActivePatches[MAX_ACTIVE_PATCHES][MAX_PATH];
static TCHAR szIpsSearchRaw[256] = { 0 };
static TCHAR szIpsSearchNormalized[256] = { 0 };
static void RefreshPatch();
static void FillListBox();
static void CheckActivePatches();
static void SavePatches();

void IpsSetHostedStorageName(const TCHAR* pszStorageName)
{
	if (pszStorageName && pszStorageName[0] != _T('\0')) {
		_tcsncpy(szHostedStorageName, pszStorageName, _countof(szHostedStorageName) - 1);
		szHostedStorageName[_countof(szHostedStorageName) - 1] = _T('\0');
	} else {
		szHostedStorageName[0] = _T('\0');
	}
}

static bool IpsResolveStorageName(TCHAR* pszOut, INT32 nOutLen)
{
	if (pszOut == NULL || nOutLen <= 0) {
		return false;
	}

	pszOut[0] = _T('\0');

	if (nQuickOpen == 2 && szStorageName[0] != _T('\0')) {
		_tcsncpy(pszOut, szStorageName, nOutLen - 1);
		pszOut[nOutLen - 1] = _T('\0');
		return true;
	}

	if (szHostedStorageName[0] != _T('\0')) {
		_tcsncpy(pszOut, szHostedStorageName, nOutLen - 1);
		pszOut[nOutLen - 1] = _T('\0');
		return true;
	}

	char* pszZipName = NULL;
	if (BurnDrvGetZipName(&pszZipName, 0) == 0 && pszZipName && pszZipName[0] != '\0') {
		ANSIToTCHAR(pszZipName, pszOut, nOutLen);
		pszOut[nOutLen - 1] = _T('\0');
		return (pszOut[0] != _T('\0'));
	}

	const TCHAR* pszDrvName = BurnDrvGetText(DRV_NAME);
	if (pszDrvName && pszDrvName[0] != _T('\0')) {
		_tcsncpy(pszOut, pszDrvName, nOutLen - 1);
		pszOut[nOutLen - 1] = _T('\0');
		return true;
	}

	return false;
}

static bool IpsGetSelectedPatchFile(TCHAR* pszOut, INT32 nOutLen)
{
	if (pszOut == NULL || nOutLen <= 0 || hIpsList == NULL) {
		return false;
	}

	pszOut[0] = _T('\0');
	HTREEITEM hSelectHandle = TreeView_GetSelection(hIpsList);
	if (hSelectHandle == NULL) {
		return false;
	}

	for (INT32 i = 0; i < nNumPatches; i++) {
		if (hPatchHandlesIndex[i] == hSelectHandle && szPatchFileNames[i][0]) {
			_tcsncpy(pszOut, szPatchFileNames[i], nOutLen - 1);
			pszOut[nOutLen - 1] = _T('\0');
			return true;
		}
	}

	return false;
}

static void IpsSelectItemFromScreenPoint(const POINT* pptScreen)
{
	if (pptScreen == NULL || hIpsList == NULL) {
		return;
	}

	POINT ptClient = *pptScreen;
	ScreenToClient(hIpsList, &ptClient);

	TVHITTESTINFO thi;
	memset(&thi, 0, sizeof(thi));
	thi.pt = ptClient;
	TreeView_HitTest(hIpsList, &thi);

	if (thi.hItem && (thi.flags & (TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMRIGHT | TVHT_ONITEMSTATEICON | TVHT_ONITEMINDENT))) {
		TreeView_SelectItem(hIpsList, thi.hItem);
	}
}

static void IpsNormalizePath(const TCHAR* pszSrc, TCHAR* pszDst, INT32 nDstLen)
{
	if (pszDst == NULL || nDstLen <= 0) {
		return;
	}

	pszDst[0] = _T('\0');
	if (pszSrc == NULL || pszSrc[0] == _T('\0')) {
		return;
	}

	DWORD nRet = GetFullPathName(pszSrc, nDstLen, pszDst, NULL);
	if (nRet == 0 || nRet >= (DWORD)nDstLen) {
		_tcsncpy(pszDst, pszSrc, nDstLen - 1);
		pszDst[nDstLen - 1] = _T('\0');
	}

	for (TCHAR* p = pszDst; *p; p++) {
		if (*p == _T('/')) {
			*p = _T('\\');
		}
	}
}

static void IpsRefreshHostedView()
{
	if (hIpsList == NULL) {
		return;
	}

	TreeView_DeleteAllItems(hIpsList);
	memset(hItemHandles, 0, sizeof(hItemHandles));
	memset(hPatchHandlesIndex, 0, sizeof(hPatchHandlesIndex));
	nPatchIndex = 0;
	nNumPatches = 0;

	for (INT32 i = 0; i < MAX_NODES; i++) {
		szPatchFileNames[i][0] = _T('\0');
	}

	FillListBox();
	CheckActivePatches();

	if (nNumPatches > 0 && hPatchHandlesIndex[0]) {
		TreeView_SelectItem(hIpsList, hPatchHandlesIndex[0]);
		TreeView_EnsureVisible(hIpsList, hPatchHandlesIndex[0]);
	} else if (TreeView_GetCount(hIpsList) > 0) {
		HTREEITEM hFirst = TreeView_GetRoot(hIpsList);
		if (hFirst) {
			TreeView_SelectItem(hIpsList, hFirst);
			TreeView_EnsureVisible(hIpsList, hFirst);
		}
	}

	RefreshPatch();
	UpdateWindow(hIpsList);
}

static void IpsEditFile(const TCHAR* pszPatchFile)
{
	if (pszPatchFile == NULL || pszPatchFile[0] == _T('\0')) {
		return;
	}

	INT_PTR nRet = (INT_PTR)ShellExecute(NULL, _T("edit"), pszPatchFile, NULL, NULL, SW_SHOWNORMAL);
	if (nRet > 32) {
		return;
	}

	nRet = (INT_PTR)ShellExecute(NULL, _T("open"), pszPatchFile, NULL, NULL, SW_SHOWNORMAL);
	if (nRet > 32) {
		return;
	}

	TCHAR szParams[MAX_PATH * 2] = { 0 };
	_stprintf(szParams, _T("\"%s\""), pszPatchFile);
	ShellExecute(NULL, NULL, _T("notepad.exe"), szParams, NULL, SW_SHOWNORMAL);
}

static void IpsOpenContainingFolder(const TCHAR* pszPatchFile)
{
	if (pszPatchFile == NULL || pszPatchFile[0] == _T('\0')) {
		return;
	}

	TCHAR szFullPath[MAX_PATH] = { 0 };
	IpsNormalizePath(pszPatchFile, szFullPath, _countof(szFullPath));

	TCHAR szParams[MAX_PATH * 2] = { 0 };
	_stprintf(szParams, _T("/select,\"%s\""), szFullPath[0] ? szFullPath : pszPatchFile);

	INT_PTR nRet = (INT_PTR)ShellExecute(NULL, _T("open"), _T("explorer.exe"), szParams, NULL, SW_SHOWNORMAL);
	if (nRet > 32) {
		return;
	}

	TCHAR szDir[MAX_PATH] = { 0 };
	_tcscpy(szDir, szFullPath[0] ? szFullPath : pszPatchFile);
	TCHAR* pSlash = _tcsrchr(szDir, _T('\\'));
	if (pSlash) {
		*pSlash = _T('\0');
	}

	ShellExecute(NULL, _T("open"), szDir, NULL, NULL, SW_SHOWNORMAL);
}

static void IpsDeleteFile(HWND hOwner, const TCHAR* pszPatchFile)
{
	if (pszPatchFile == NULL || pszPatchFile[0] == _T('\0')) {
		return;
	}

	TCHAR szFullPath[MAX_PATH] = { 0 };
	IpsNormalizePath(pszPatchFile, szFullPath, _countof(szFullPath));

	TCHAR szMessage[MAX_PATH * 2] = { 0 };
	_stprintf(szMessage, _T("\x786E\x5B9A\x5220\x9664\x8FD9\x4E2A IPS \x6587\x4EF6\x5417\xFF1F\n\n%s"), szFullPath[0] ? szFullPath : pszPatchFile);
	if (MessageBox(hOwner, szMessage, _T("IPS"), MB_ICONQUESTION | MB_YESNO) != IDYES) {
		return;
	}

	const TCHAR* pszTargetPath = szFullPath[0] ? szFullPath : pszPatchFile;
	SetFileAttributes(pszTargetPath, FILE_ATTRIBUTE_NORMAL);

	if (!DeleteFile(pszTargetPath)) {
		DWORD nDeleteError = GetLastError();
		TCHAR szError[MAX_PATH * 2] = { 0 };
		_stprintf(szError, _T("\x5220\x9664\x5931\x8D25\xFF1A\n\n%s\n\nWin32 Error: %lu"), pszTargetPath, (unsigned long)nDeleteError);
		MessageBox(hOwner, szError, _T("IPS"), MB_ICONERROR | MB_OK);
		return;
	}

	IpsRefreshHostedView();
	SavePatches();
}

static void IpsShowContextMenu(HWND hOwner)
{
	if (hIpsList == NULL) {
		return;
	}

	DWORD dwPos = GetMessagePos();
	POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
	IpsSelectItemFromScreenPoint(&pt);

	TCHAR szPatchFile[MAX_PATH] = { 0 };
	if (!IpsGetSelectedPatchFile(szPatchFile, _countof(szPatchFile))) {
		return;
	}

	HMENU hPopupMenu = CreatePopupMenu();
	if (hPopupMenu == NULL) {
		return;
	}

	AppendMenu(hPopupMenu, MF_STRING, IDC_IPS_CTX_EDIT,   _T("\x7F16\x8F91"));
	AppendMenu(hPopupMenu, MF_STRING, IDC_IPS_CTX_OPEN,   _T("\x6253\x5F00\x6587\x4EF6\x6240\x5728\x76EE\x5F55"));
	AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hPopupMenu, MF_STRING, IDC_IPS_CTX_DELETE, _T("\x5220\x9664"));

	const UINT nCmd = TrackPopupMenu(hPopupMenu,
		TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hOwner ? hOwner : hIpsDlg, NULL);

	DestroyMenu(hPopupMenu);

	switch (nCmd) {
		case IDC_IPS_CTX_EDIT:
			IpsEditFile(szPatchFile);
			break;

		case IDC_IPS_CTX_OPEN:
			IpsOpenContainingFolder(szPatchFile);
			break;

		case IDC_IPS_CTX_DELETE:
			IpsDeleteFile(hOwner ? hOwner : hIpsDlg, szPatchFile);
			break;

		default:
			break;
	}
}

static HWND IpsPreviewControl()
{
	return bIpsHostedMode ? hIpsHostedPreview : GetDlgItem(hIpsDlg, IDC_SCREENSHOT_H);
}

static HWND IpsDescControl()
{
	return bIpsHostedMode ? hIpsHostedDesc : GetDlgItem(hIpsDlg, IDC_TEXTCOMMENT);
}

static HWND IpsLanguageControl()
{
	return bIpsHostedMode ? hIpsHostedLang : GetDlgItem(hIpsDlg, IDC_CHOOSE_LIST);
}

static HBITMAP IpsLoadPreviewBitmap(FILE* fp, INT32 nFlags)
{
	INT32 nWidth = 304;
	INT32 nHeight = 228;
	HWND hCtrl = IpsPreviewControl();
	RECT rc = { 0 };

	if (bIpsHostedMode && hIpsDlg) {
		HWND hFrameCtrl = GetDlgItem(hIpsDlg, IDC_STATIC2);
		if (hFrameCtrl && GetClientRect(hFrameCtrl, &rc)) {
			nWidth = rc.right - rc.left - 20;
			nHeight = rc.bottom - rc.top - 38;
		}
	} else if (hCtrl && GetClientRect(hCtrl, &rc)) {
		nWidth = rc.right - rc.left;
		nHeight = rc.bottom - rc.top;
	}

	if (nWidth < 8) nWidth = 304;
	if (nHeight < 8) nHeight = 228;

	return PNGLoadBitmap(hIpsDlg, fp, nWidth, nHeight, nFlags);
}

static bool IpsIsSearchSeparator(TCHAR c)
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

static void IpsNormalizeSearchText(const TCHAR* pszSrc, TCHAR* pszDst, size_t nDstCount)
{
	if (pszDst == NULL || nDstCount == 0) return;

	pszDst[0] = _T('\0');
	if (pszSrc == NULL) return;

	size_t j = 0;
	for (size_t i = 0; pszSrc[i] && j + 1 < nDstCount; i++) {
		TCHAR c = _totlower(pszSrc[i]);
		if (IpsIsSearchSeparator(c)) continue;
		pszDst[j++] = c;
	}

	pszDst[j] = _T('\0');
}

static TCHAR IpsGetPinyinInitial(TCHAR c)
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

static void IpsBuildInitialsSearchText(const TCHAR* pszSrc, TCHAR* pszDst, size_t nDstCount)
{
	if (pszDst == NULL || nDstCount == 0) return;

	pszDst[0] = _T('\0');
	if (pszSrc == NULL) return;

	size_t j = 0;
	bool bNewWord = true;

	for (size_t i = 0; pszSrc[i] && j + 1 < nDstCount; i++) {
		TCHAR c = pszSrc[i];

		if (IpsIsSearchSeparator(c)) {
			bNewWord = true;
			continue;
		}

		TCHAR initial = IpsGetPinyinInitial(c);
		if (initial == 0) {
			bNewWord = false;
			continue;
		}

		if (c < 0x80) {
			if (bNewWord) {
				pszDst[j++] = initial;
			}
			bNewWord = false;
		} else {
			pszDst[j++] = initial;
			bNewWord = false;
		}
	}

	pszDst[j] = _T('\0');
}

static bool IpsStringMatchesSearch(const TCHAR* pszValue)
{
	if (szIpsSearchNormalized[0] == _T('\0')) return true;
	if (pszValue == NULL || pszValue[0] == _T('\0')) return false;

	TCHAR szNormalized[1024] = { 0 };
	IpsNormalizeSearchText(pszValue, szNormalized, _countof(szNormalized));
	if (_tcsstr(szNormalized, szIpsSearchNormalized)) return true;

	TCHAR szInitials[1024] = { 0 };
	IpsBuildInitialsSearchText(pszValue, szInitials, _countof(szInitials));
	return _tcsstr(szInitials, szIpsSearchNormalized) != NULL;
}

static bool IpsPatchMatchesSearch(const TCHAR* pszPatchName, const TCHAR* pszPatchDesc, const TCHAR* pszFileName)
{
	if (szIpsSearchNormalized[0] == _T('\0')) return true;

	return IpsStringMatchesSearch(pszPatchName)
		|| IpsStringMatchesSearch(pszPatchDesc)
		|| IpsStringMatchesSearch(pszFileName);
}

struct IpsExtraPatch {
	TCHAR** pszDat;
	UINT32 nCount;
};

// When including #include *.dat, inserts a timely patch depending on where it is before or after the patch
static IpsExtraPatch _IpsEarly = { 0 }, _IpsLast = { 0 };

// GCC doesn't seem to define these correctly.....
#define _TreeView_SetItemState(hwndTV, hti, data, _mask)			\
{																	\
	TVITEM _ms_TVi;													\
	_ms_TVi.mask      = TVIF_STATE;									\
	_ms_TVi.hItem     = hti;										\
	_ms_TVi.stateMask = _mask;										\
	_ms_TVi.state     = data;										\
	SNDMSG((hwndTV), TVM_SETITEM, 0, (LPARAM)(TV_ITEM *)&_ms_TVi);	\
}

#define _TreeView_SetCheckState(hwndTV, hti, fCheck) \
  _TreeView_SetItemState(hwndTV, hti, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), TVIS_STATEIMAGEMASK)

#define _TreeView_GetCheckState(hwndTV, hti) \
   ((((UINT)(SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), TVIS_STATEIMAGEMASK))) >> 12) -1)

static INT32 FileExists(const TCHAR* pszName)
{
	DWORD dwAttrib = GetFileAttributes(pszName);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static TCHAR* GameIpsConfigName()
{
	// Return the path of the config file for this game
	static TCHAR szName[MAX_PATH];
	TCHAR szCurrentStorageName[MAX_PATH] = { 0 };
	IpsResolveStorageName(szCurrentStorageName, _countof(szCurrentStorageName));
	_stprintf(szName, _T("config\\ips\\%s.ini"), szCurrentStorageName);
	return szName;
}

INT32 GetIpsNumPatches()
{
	WIN32_FIND_DATA wfd;
	HANDLE hSearch;
	TCHAR szFilePath[MAX_PATH];
	INT32 Count = 0;
	TCHAR szCurrentStorageName[MAX_PATH] = { 0 };

	if (!IpsResolveStorageName(szCurrentStorageName, _countof(szCurrentStorageName)) || szCurrentStorageName[0] == _T('\0')) {
		return 0;
	}

	_stprintf(szFilePath, _T("%s%s\\*.dat"), szAppIpsPath, szCurrentStorageName);

	hSearch = FindFirstFile(szFilePath, &wfd);

	if (hSearch != INVALID_HANDLE_VALUE) {
		INT32 Done = 0;

		while (!Done ) {
			Count++;
			Done = !FindNextFile(hSearch, &wfd);
		}

		FindClose(hSearch);
	}

	return Count;
}

static TCHAR* GetPatchDescByLangcode(FILE* fp, INT32 nLang)
{
	static TCHAR* result = NULL;
	TCHAR* desc = NULL, langtag[10] = { 0 };

	_stprintf(langtag, _T("[%s]"), szLanguageCodes[nLang]);

	fseek(fp, 0, SEEK_SET);

	while (!feof(fp)) {
		TCHAR s[4096];

		if (_fgetts(s, sizeof(s), fp) != NULL) {
			if (_tcsncmp(langtag, s, _tcslen(langtag)) != 0)
				continue;

			while (_fgetts(s, sizeof(s), fp) != NULL) {
				TCHAR* p;

				if (*s == _T('[')) {
					if (desc) {
						result = (TCHAR*)malloc((_tcslen(desc) + 1) * sizeof(TCHAR));
						_tcscpy(result, desc);
						free(desc); desc = NULL;
						return result;
					} else
						return NULL;
				}

				for (p = s; *p; p++) {
					if (*p == _T('\r') || *p == _T('\n')) {
						*p = _T('\0');
						break;
					}
				}

				if (desc) {
					TCHAR* p1 = NULL;
					INT32 len = _tcslen(desc);

					len += _tcslen(s) + 2;
					p1 = (TCHAR*)malloc((len + 1) * sizeof(TCHAR));
					_stprintf(p1, _T("%s\r\n%s"), desc, s);
					free(desc);
					desc = p1;
				} else {
					desc = (TCHAR*)malloc((_tcslen(s) + 1) * sizeof(TCHAR));
					if (desc != NULL)
						_tcscpy(desc, s);
				}
			}
		}
	}

	if (desc) {
		result = (TCHAR*)malloc((_tcslen(desc) + 1) * sizeof(TCHAR));
		_tcscpy(result, desc);
		free(desc); desc = NULL;
		return result;
	} else
		return NULL;
}

static void FillListBox()
{
	WIN32_FIND_DATA wfd;
	HANDLE hSearch;
	TCHAR szFilePath[MAX_PATH]       = { 0 };
	TCHAR szFilePathSearch[MAX_PATH] = { 0 };
	TCHAR szFileName[MAX_PATH]       = { 0 };
	TCHAR PatchName[256]             = { 0 };
	TCHAR* PatchDesc = NULL;
	INT32 nHandlePos = 0;

	TV_INSERTSTRUCT TvItem;

	memset(&TvItem, 0, sizeof(TvItem));
	TvItem.item.mask    = TVIF_TEXT | TVIF_PARAM;
	TvItem.hInsertAfter = TVI_LAST;

	if (szStorageName[0] == _T('\0') && !IpsResolveStorageName(szStorageName, _countof(szStorageName))) {
		nNumPatches = 0;
		return;
	}

	_stprintf(szFilePath, _T("%s%s\\"), szAppIpsPath, szStorageName);
	_stprintf(szFilePathSearch, _T("%s*.dat"), szFilePath);

	hSearch = FindFirstFile(szFilePathSearch, &wfd);

	if (hSearch != INVALID_HANDLE_VALUE) {
		INT32 Done = 0;

		while (!Done ) {
			memset(szFileName, '\0', MAX_PATH * sizeof(TCHAR));
			_stprintf(szFileName, _T("%s%s"), szFilePath, wfd.cFileName);

			const TCHAR* pszReadMode = AdaptiveEncodingReads(szFileName);
			if (NULL == pszReadMode) continue;

			FILE *fp = _tfopen(szFileName, pszReadMode);
			if (fp) {
				PatchDesc = NULL;
				memset(PatchName, '\0', 256 * sizeof(TCHAR));

				PatchDesc = GetPatchDescByLangcode(fp, nIpsSelectedLanguage);
				// If not available - try English first
				if (PatchDesc == NULL) PatchDesc = GetPatchDescByLangcode(fp, 0);
				// Simplified Chinese is the reference language (should always be available!!)
				if (PatchDesc == NULL) PatchDesc = GetPatchDescByLangcode(fp, 1);
				if (PatchDesc == NULL) {
					PatchDesc = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
					if (NULL == PatchDesc) {
						fclose(fp); fp = NULL;
						continue;
					}
					memset(PatchDesc, 0, MAX_PATH * sizeof(TCHAR));
					_tcscpy(PatchDesc, wfd.cFileName);
				}

				bprintf(0, _T("PatchDesc [%s]\n"), PatchDesc);

				for (UINT32 i = 0; i < _tcslen(PatchDesc); i++) {
					if (PatchDesc[i] == _T('\r') || PatchDesc[i] == _T('\n')) break;
					PatchName[i] = PatchDesc[i];
				}
				PatchName[_countof(PatchName) - 1] = _T('\0');

				if (!IpsPatchMatchesSearch(PatchName, PatchDesc, wfd.cFileName)) {
					if (NULL != PatchDesc) {
						free(PatchDesc);
						PatchDesc = NULL;
					}
					fclose(fp);
					fp = NULL;
					Done = !FindNextFile(hSearch, &wfd);
					continue;
				}

				if (NULL != PatchDesc) {
					free(PatchDesc); PatchDesc = NULL;
				}

				// Check for categories
				TCHAR *Tokens;
				INT32 nNumTokens = 0, nNumNodes = 0;
				TCHAR szCategory[256];
				UINT32 nPatchNameLength = _tcslen(PatchName);

				Tokens = _tcstok(PatchName, _T("/"));
				while (Tokens != NULL) {
					if (nNumTokens == 0) {
						INT32 bAddItem = 1;
						// Check if item already exists
						nNumNodes = SendMessage(hIpsList, TVM_GETCOUNT, (WPARAM)0, (LPARAM)0);
						for (INT32 i = 0; i < nNumNodes; i++) {
							TCHAR Temp[256];
							TVITEM Tvi;
							memset(&Tvi, 0, sizeof(Tvi));
							Tvi.hItem      = hItemHandles[i];
							Tvi.mask       = TVIF_TEXT | TVIF_HANDLE;
							Tvi.pszText    = Temp;
							Tvi.cchTextMax = 256;
							SendMessage(hIpsList, TVM_GETITEM, (WPARAM)0, (LPARAM)&Tvi);

							if (!_tcsicmp(Tvi.pszText, Tokens)) bAddItem = 0;
						}
						if (bAddItem) {
							TvItem.hParent           = TVI_ROOT;
							TvItem.item.pszText      = Tokens;
							hItemHandles[nHandlePos] = (HTREEITEM)SendMessage(hIpsList, TVM_INSERTITEM, 0, (LPARAM)&TvItem);
							nHandlePos++;
						}
						if (_tcslen(Tokens) == nPatchNameLength) {
							hPatchHandlesIndex[nPatchIndex] = hItemHandles[nHandlePos - 1];
							_tcscpy(szPatchFileNames[nPatchIndex], szFileName);

							nPatchIndex++;
						}
						_tcscpy(szCategory, Tokens);
					} else {
						HTREEITEM hNode = TVI_ROOT;
						// See which category we should be in
						nNumNodes = SendMessage(hIpsList, TVM_GETCOUNT, (WPARAM)0, (LPARAM)0);
						for (INT32 i = 0; i < nNumNodes; i++) {
							TCHAR Temp[256];
							TVITEM Tvi;
							memset(&Tvi, 0, sizeof(Tvi));
							Tvi.hItem      = hItemHandles[i];
							Tvi.mask       = TVIF_TEXT | TVIF_HANDLE;
							Tvi.pszText    = Temp;
							Tvi.cchTextMax = 256;
							SendMessage(hIpsList, TVM_GETITEM, (WPARAM)0, (LPARAM)&Tvi);

							if (!_tcsicmp(Tvi.pszText, szCategory)) hNode = Tvi.hItem;
						}

						TvItem.hParent           = hNode;
						TvItem.item.pszText      = Tokens;
						hItemHandles[nHandlePos] = (HTREEITEM)SendMessage(hIpsList, TVM_INSERTITEM, 0, (LPARAM)&TvItem);

						hPatchHandlesIndex[nPatchIndex] = hItemHandles[nHandlePos];
						_tcscpy(szPatchFileNames[nPatchIndex], szFileName);

						nHandlePos++;
						nPatchIndex++;
					}

					// Only one file path can be bound to a DAT file.
					// A maximum of root and secondary nodes are required.
					// The use of '/' here will potentially create useless multi-level nodes.
					Tokens = _tcstok(NULL, _T("\0"));
					nNumTokens++;
				}

				fclose(fp);
			}

			Done = !FindNextFile(hSearch, &wfd);
		}

		FindClose(hSearch);
	}

	nNumPatches = nPatchIndex;

	// Expand all branches
	INT32 nNumNodes = SendMessage(hIpsList, TVM_GETCOUNT, (WPARAM)0, (LPARAM)0);;
	for (INT32 i = 0; i < nNumNodes; i++) {
		SendMessage(hIpsList, TVM_EXPAND, TVE_EXPAND, (LPARAM)hItemHandles[i]);
	}
}

static INT32 GetIpsNumActivePatches()
{
	INT32 nActivePatches = 0;

	for (INT32 i = 0; i < MAX_ACTIVE_PATCHES; i++) {
		if (_tcsicmp(szIpsActivePatches[i], _T(""))) nActivePatches++;
	}

	return nActivePatches;
}

INT32 LoadIpsActivePatches()
{
	if (!IpsResolveStorageName(szStorageName, _countof(szStorageName))) {
		szStorageName[0] = _T('\0');
	}

	for (INT32 i = 0; i < MAX_ACTIVE_PATCHES; i++) {
		_stprintf(szIpsActivePatches[i], _T(""));
	}

	if (szStorageName[0] == _T('\0')) {
		return 0;
	}

	FILE* fp = _tfopen(GameIpsConfigName(), _T("rt"));
	TCHAR szLine[MAX_PATH];
	INT32 nActivePatches = 0;

	if (fp) {
		while (_fgetts(szLine, MAX_PATH, fp)) {
			INT32 nLen = _tcslen(szLine);

			// Get rid of the linefeed at the end
			if (nLen > 0 && szLine[nLen - 1] == 10) {
				szLine[nLen - 1] = 0;
				nLen--;
			}

			if (!_tcsnicmp(szLine, _T("//"), 2)) continue;
			if (!_tcsicmp(szLine, _T("")))       continue;

			_stprintf(szIpsActivePatches[nActivePatches], _T("%s%s\\%s"), szAppIpsPath, szStorageName, szLine);
			nActivePatches++;
		}
		fclose(fp);
	}

	return nActivePatches;
}

static void CheckActivePatches()
{
	INT32 nActivePatches = LoadIpsActivePatches();

	for (INT32 i = 0; i < nActivePatches; i++) {
		for (INT32 j = 0; j < nNumPatches; j++) {
			if (!_tcsicmp(szIpsActivePatches[i], szPatchFileNames[j])) {
				_TreeView_SetCheckState(hIpsList, hPatchHandlesIndex[j], TRUE);
			}
		}
	}
}

static INT32 IpsManagerInit()
{
	// Get the games full name
	TCHAR szText[1024] = _T("");
	TCHAR* pszPosition = szText;
	TCHAR* pszName     = BurnDrvGetText(DRV_FULLNAME);

	pszPosition += _sntprintf(szText, 1024, pszName);

	pszName = BurnDrvGetText(DRV_FULLNAME);
	while ((pszName = BurnDrvGetText(DRV_NEXTNAME | DRV_FULLNAME)) != NULL) {
		if (pszPosition + _tcslen(pszName) - 1024 > szText) {
			break;
		}
		pszPosition += _stprintf(pszPosition, _T(SEPERATOR_2) _T("%s"), pszName);
	}

	_tcscpy(szFullName, szText);

	_stprintf(szText, _T("%s") _T(SEPERATOR_1) _T("%s"), FBALoadStringEx(hAppInst, IDS_IPSMANAGER_TITLE, true), szFullName);

	if (!bIpsHostedMode) {
		// Set the window caption
		SetWindowText(hIpsDlg, szText);
	}

	// Fill the combo box
	_stprintf(szLanguages[ 0], FBALoadStringEx(hAppInst, IDS_LANG_ENGLISH_US,   true));
	_stprintf(szLanguages[ 1], FBALoadStringEx(hAppInst, IDS_LANG_SIMP_CHINESE, true));
	_stprintf(szLanguages[ 2], FBALoadStringEx(hAppInst, IDS_LANG_TRAD_CHINESE, true));
	_stprintf(szLanguages[ 3], FBALoadStringEx(hAppInst, IDS_LANG_JAPANESE,     true));
	_stprintf(szLanguages[ 4], FBALoadStringEx(hAppInst, IDS_LANG_KOREAN,       true));
	_stprintf(szLanguages[ 5], FBALoadStringEx(hAppInst, IDS_LANG_FRENCH,       true));
	_stprintf(szLanguages[ 6], FBALoadStringEx(hAppInst, IDS_LANG_SPANISH,      true));
	_stprintf(szLanguages[ 7], FBALoadStringEx(hAppInst, IDS_LANG_ITALIAN,      true));
	_stprintf(szLanguages[ 8], FBALoadStringEx(hAppInst, IDS_LANG_GERMAN,       true));
	_stprintf(szLanguages[ 9], FBALoadStringEx(hAppInst, IDS_LANG_PORTUGUESE,   true));
	_stprintf(szLanguages[10], FBALoadStringEx(hAppInst, IDS_LANG_POLISH,       true));
	_stprintf(szLanguages[11], FBALoadStringEx(hAppInst, IDS_LANG_HUNGARIAN,    true));

	_stprintf(szLanguageCodes[ 0], _T("en_US"));
	_stprintf(szLanguageCodes[ 1], _T("zh_CN"));
	_stprintf(szLanguageCodes[ 2], _T("zh_TW"));
	_stprintf(szLanguageCodes[ 3], _T("ja_JP"));
	_stprintf(szLanguageCodes[ 4], _T("ko_KR"));
	_stprintf(szLanguageCodes[ 5], _T("fr_FR"));
	_stprintf(szLanguageCodes[ 6], _T("es_ES"));
	_stprintf(szLanguageCodes[ 7], _T("it_IT"));
	_stprintf(szLanguageCodes[ 8], _T("de_DE"));
	_stprintf(szLanguageCodes[ 9], _T("pt_BR"));
	_stprintf(szLanguageCodes[10], _T("pl_PL"));
	_stprintf(szLanguageCodes[11], _T("hu_HU"));

	HWND hLang = IpsLanguageControl();
	if (hLang) {
		SendMessage(hLang, CB_RESETCONTENT, 0, 0);
	}
	for (INT32 i = 0; i < NUM_LANGUAGES; i++) {
		if (hLang) {
			SendMessage(hLang, CB_ADDSTRING, 0, (LPARAM)&szLanguages[i]);
		}
	}

	if (hLang) {
		SendMessage(hLang, CB_SETCURSEL, (WPARAM)nIpsSelectedLanguage, (LPARAM)0);
	}

	if (!bIpsHostedMode) {
		hIpsList = GetDlgItem(hIpsDlg, IDC_TREE1);
	}

	if (!IpsResolveStorageName(szStorageName, _countof(szStorageName))) {
		szStorageName[0] = _T('\0');
	}

	FillListBox();
	CheckActivePatches();
	RefreshPatch();

	return 0;
}

static void RefreshPatch()
{
	szPngName[0] = _T('\0');  // Reset the file name of the preview picture
	HWND hDescCtrl = IpsDescControl();
	HWND hPreviewCtrl = IpsPreviewControl();
	if (hDescCtrl) {
		SendMessage(hDescCtrl, WM_SETTEXT, (WPARAM)0, (LPARAM)NULL);
	}
	if (hPreviewCtrl) {
		SendMessage(hPreviewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hPreview);
	}

	HTREEITEM hSelectHandle = (HTREEITEM)SendMessage(hIpsList, TVM_GETNEXTITEM, TVGN_CARET, ~0U);

	if (hBmp) {
		DeleteObject((HGDIOBJ)hBmp);
		hBmp = NULL;
	}

	for (INT32 i = 0; i < nNumPatches; i++) {
		if (hSelectHandle == hPatchHandlesIndex[i]) {
			TCHAR *PatchDesc = NULL;

			const TCHAR* pszReadMode = AdaptiveEncodingReads(szPatchFileNames[i]);
			if (NULL == pszReadMode) continue;

			FILE *fp = _tfopen(szPatchFileNames[i], pszReadMode);
			if (fp) {
				PatchDesc = GetPatchDescByLangcode(fp, nIpsSelectedLanguage);
				// If not available - try English first
				if (PatchDesc == NULL) PatchDesc = GetPatchDescByLangcode(fp, 0);
				// Simplified Chinese is the reference language (should always be available!!)
				if (PatchDesc == NULL) PatchDesc = GetPatchDescByLangcode(fp, 1);
				if (PatchDesc == NULL) {
					PatchDesc = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
					if (NULL == PatchDesc) {
						fclose(fp); fp = NULL;
						continue;
					}
					memset(PatchDesc, 0, MAX_PATH * sizeof(TCHAR));
					_tcscpy(PatchDesc, szPatchFileNames[i]);
				}

				if (hDescCtrl) {
					SendMessage(hDescCtrl, WM_SETTEXT, (WPARAM)0, (LPARAM)PatchDesc);
				}

				if (NULL != PatchDesc) {
					free(PatchDesc); PatchDesc = NULL;
				}
				fclose(fp);
			}
			fp = NULL;

			TCHAR szImageFileName[MAX_PATH];
			szImageFileName[0] = _T('\0');

			_tcscpy(szImageFileName, szPatchFileNames[i]);
			szImageFileName[_tcslen(szImageFileName) - 3] = _T('p');
			szImageFileName[_tcslen(szImageFileName) - 2] = _T('n');
			szImageFileName[_tcslen(szImageFileName) - 1] = _T('g');

			fp = _tfopen(szImageFileName, _T("rb"));
			HBITMAP hNewImage = NULL;
			if (fp) {
				_tcscpy(szPngName, szImageFileName);  // Associated preview picture
				hNewImage = IpsLoadPreviewBitmap(fp, 3);
				fclose(fp);
			}
			if (hNewImage) {
				DeleteObject((HGDIOBJ)hBmp);
				hBmp = hNewImage;
				if (hPreviewCtrl) {
					SendMessage(hPreviewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
				}
			} else {
				if (hPreviewCtrl) {
					SendMessage(hPreviewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hPreview);
				}
			}
		}
	}
}

static void SavePatches()
{
	INT32 nActivePatches = 0;

	for (INT32 i = 0; i < MAX_ACTIVE_PATCHES; i++) {
		_stprintf(szIpsActivePatches[i], _T(""));
	}

	for (INT32 i = 0; i < nNumPatches; i++) {
		INT32 nChecked = _TreeView_GetCheckState(hIpsList, hPatchHandlesIndex[i]);

		if (nChecked) {
			_tcscpy(szIpsActivePatches[nActivePatches], szPatchFileNames[i]);
			nActivePatches++;
		}
	}

	FILE* fp = _tfopen(GameIpsConfigName(), _T("wt"));

	if (fp) {
		_ftprintf(fp, _T("// ") _T(APP_TITLE) _T(" v%s --- IPS Config File for %s (%s)\n\n"), szAppBurnVer, szStorageName, szFullName);
		for (INT32 i = 0; i < nActivePatches; i++) {
			const TCHAR* pszFileName = _tcsrchr(szIpsActivePatches[i], _T('\\'));
			pszFileName = (pszFileName != NULL) ? (pszFileName + 1) : szIpsActivePatches[i];
			_ftprintf(fp, _T("%s\n"), pszFileName);
		}
		fclose(fp);
	}
}

static void IpsResetState()
{
	for (INT32 i = 0; i < NUM_LANGUAGES; i++) {
		szLanguages[i][0]     = _T('\0');
		szLanguageCodes[i][0] = _T('\0');
	}

	memset(hItemHandles,       0, MAX_NODES * sizeof(HTREEITEM));
	memset(hPatchHandlesIndex, 0, MAX_NODES * sizeof(HTREEITEM));

	nPatchIndex = 0;
	nNumPatches = 0;

	for (INT32 i = 0; i < MAX_NODES; i++) {
		szPatchFileNames[i][0] = _T('\0');
	}

	szPngName[0] = _T('\0');
}

static void IpsManagerExit()
{
	HWND hPreviewCtrl = IpsPreviewControl();
	HWND hDescCtrl = IpsDescControl();
	HWND hLangCtrl = IpsLanguageControl();

	if (hPreviewCtrl) {
		SendMessage(hPreviewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
	}
	if (hDescCtrl) {
		SendMessage(hDescCtrl, WM_SETTEXT, (WPARAM)0, (LPARAM)NULL);
	}
	if (hLangCtrl) {
		SendMessage(hLangCtrl, CB_RESETCONTENT, 0, 0);
	}
	if (hIpsList) {
		TreeView_DeleteAllItems(hIpsList);
	}

	IpsResetState();

	if (hBmp) {
		DeleteObject((HGDIOBJ)hBmp);
		hBmp = NULL;
	}
	if (hPreview) {
		DeleteObject((HGDIOBJ)hPreview);
		hPreview = NULL;
	}

	hParent = NULL;
	if (hWhiteBGBrush) {
		DeleteObject(hWhiteBGBrush);
		hWhiteBGBrush = NULL;
	}
	hIpsHostedPreview = NULL;
	hIpsHostedDesc = NULL;
	hIpsHostedLang = NULL;
	hIpsList = NULL;
	bIpsHostedMode = false;
	EndDialog(hIpsDlg, 0);
}

static void IpsOkay()
{
	SavePatches();
	IpsManagerExit();
}

static INT_PTR CALLBACK DefInpProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_INITDIALOG: {
			hIpsDlg = hDlg;
			bIpsHostedMode = false;
			hIpsHostedPreview = NULL;
			hIpsHostedDesc = NULL;
			hIpsHostedLang = NULL;

			hWhiteBGBrush = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));
			hPreview = IpsLoadPreviewBitmap(NULL, 2);
			SendMessage(IpsPreviewControl(), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hPreview);

			LONG_PTR Style;
			Style = GetWindowLongPtr (GetDlgItem(hIpsDlg, IDC_TREE1), GWL_STYLE);
			Style |= TVS_CHECKBOXES;
			SetWindowLongPtr (GetDlgItem(hIpsDlg, IDC_TREE1), GWL_STYLE, Style);

			IpsManagerInit();
			INT32 nBurnDrvActiveOld = nBurnDrvActive;	// RockyWall Add
			WndInMid(hDlg, hScrnWnd);
			SetFocus(hDlg);								// Enable Esc=close
			nBurnDrvActive = nBurnDrvActiveOld;			// RockyWall Add
			break;
		}

		case WM_LBUTTONDBLCLK: {
			RECT PreviewRect;
			POINT Point;

			memset(&PreviewRect, 0, sizeof(RECT));
			memset(&Point,       0, sizeof(POINT));

			if (GetCursorPos(&Point) && GetWindowRect(GetDlgItem(hIpsDlg, IDC_SCREENSHOT_H), &PreviewRect)) {
				if (PtInRect(&PreviewRect, Point)) {
					FILE* fp = NULL;

					fp = _tfopen(szPngName, _T("rb"));
					if (fp) {
						fclose(fp);
						ShellExecute(  // Open the image with the associated program
							GetDlgItem(hIpsDlg, IDC_SCREENSHOT_H),
							NULL, szPngName, NULL, NULL, SW_SHOWNORMAL
						);
					}
				}
			}
			return 0;
		}

		case WM_COMMAND: {
			INT32 wID = LOWORD(wParam), Notify = HIWORD(wParam);

			if (Notify == BN_CLICKED) {
				switch (wID) {
					case IDOK: {
						IpsOkay();
						break;
					}

					case IDCANCEL: {
						SendMessage(hDlg, WM_CLOSE, 0, 0);
						return 0;
					}

					case IDC_IPSMAN_DESELECTALL: {
						for (INT32 i = 0; i < nNumPatches; i++) {
							for (INT32 j = 0; j < nNumPatches; j++) {
								_TreeView_SetCheckState(hIpsList, hPatchHandlesIndex[j], FALSE);
							}
						}
						break;
					}
				}
			}

			if (wID == IDC_CHOOSE_LIST && Notify == CBN_SELCHANGE) {
				nIpsSelectedLanguage = SendMessage(GetDlgItem(hIpsDlg, IDC_CHOOSE_LIST), CB_GETCURSEL, 0, 0);
				TreeView_DeleteAllItems(hIpsList);
				FillListBox();
				RefreshPatch();
				return 0;
			}
			break;
		}

		case WM_NOTIFY: {
			NMHDR* pNmHdr = (NMHDR*)lParam;

			if (LOWORD(wParam) == IDC_TREE1 && pNmHdr->code == TVN_SELCHANGED) {
				RefreshPatch();
				return 1;
			}

			if (LOWORD(wParam) == IDC_TREE1 && pNmHdr->code == NM_RCLICK) {
				IpsShowContextMenu(hIpsDlg);
				return 1;
			}

			if (LOWORD(wParam) == IDC_TREE1 && pNmHdr->code == NM_DBLCLK) {
				// disable double-click node-expand
				SetWindowLongPtr(hIpsDlg, DWLP_MSGRESULT, 1);
				return 1;
			}

			if (LOWORD(wParam) == IDC_TREE1 && pNmHdr->code == NM_CLICK) {
				POINT cursorPos;
				GetCursorPos(&cursorPos);
				ScreenToClient(hIpsList, &cursorPos);

				TVHITTESTINFO thi;
				thi.pt = cursorPos;
				TreeView_HitTest(hIpsList, &thi);

				if (thi.flags == TVHT_ONITEMSTATEICON) {
					TreeView_SelectItem(hIpsList, thi.hItem);
				}

				return 1;
			}

			if (LOWORD(wParam) == IDC_CHOOSE_LIST && pNmHdr->code == NM_DBLCLK) {
				// disable double-click node-expand
				SetWindowLongPtr(hIpsDlg, DWLP_MSGRESULT, 1);
				return 1;
			}

			SetWindowLongPtr(hIpsDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
			return 1;
		}

		case WM_CTLCOLORSTATIC: {
			if ((HWND)lParam == GetDlgItem(hIpsDlg, IDC_TEXTCOMMENT)) {
				return (INT_PTR)hWhiteBGBrush;
			}
			break;
		}

		case WM_CLOSE: {
			IpsManagerExit();
			break;
		}
	}

	return 0;
}

INT32 IpsManagerCreate(HWND hParentWND)
{
	hParent = hParentWND;

	FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_IPS_MANAGER), hParent, (DLGPROC)DefInpProc);
	return 1;
}

INT32 IpsHostedInit(HWND hHostWnd, HWND hTree, HWND hPreviewCtrl, HWND hDescCtrl, HWND hLangCtrl)
{
	if (hTree == NULL || hPreviewCtrl == NULL || hDescCtrl == NULL) {
		return 0;
	}

	if (hBmp) {
		DeleteObject((HGDIOBJ)hBmp);
		hBmp = NULL;
	}
	if (hPreview) {
		DeleteObject((HGDIOBJ)hPreview);
		hPreview = NULL;
	}
	if (hWhiteBGBrush) {
		DeleteObject(hWhiteBGBrush);
		hWhiteBGBrush = NULL;
	}

	bIpsHostedMode = true;
	hIpsDlg = hHostWnd;
	hIpsList = hTree;
	hIpsHostedPreview = hPreviewCtrl;
	hIpsHostedDesc = hDescCtrl;
	hIpsHostedLang = hLangCtrl;
	hWhiteBGBrush = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));

	TreeView_DeleteAllItems(hIpsList);

	LONG_PTR Style = GetWindowLongPtr(hIpsList, GWL_STYLE);
	nIpsHostedOriginalTreeStyle = Style;
	bIpsHostedHaveOriginalTreeStyle = true;
	Style |= TVS_CHECKBOXES;
	SetWindowLongPtr(hIpsList, GWL_STYLE, Style);
	SetWindowPos(hIpsList, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	hPreview = IpsLoadPreviewBitmap(NULL, 2);
	SendMessage(IpsPreviewControl(), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hPreview);
	SendMessage(IpsDescControl(), WM_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
	TreeView_SetBkColor(hIpsList, RGB(0xFF, 0xFF, 0xFF));
	TreeView_SetTextColor(hIpsList, RGB(0x00, 0x00, 0x00));
	TreeView_SetLineColor(hIpsList, RGB(0x80, 0x80, 0x80));

	IpsResetState();
	IpsManagerInit();

	if (nNumPatches > 0 && hPatchHandlesIndex[0]) {
		TreeView_SelectItem(hIpsList, hPatchHandlesIndex[0]);
		TreeView_EnsureVisible(hIpsList, hPatchHandlesIndex[0]);
	} else if (TreeView_GetCount(hIpsList) > 0) {
		HTREEITEM hFirst = TreeView_GetRoot(hIpsList);
		if (hFirst) {
			TreeView_SelectItem(hIpsList, hFirst);
			TreeView_EnsureVisible(hIpsList, hFirst);
		}
	}

	UpdateWindow(hIpsList);
	return nNumPatches;
}

void IpsHostedExit(INT32 bSave)
{
	if (!bIpsHostedMode) {
		return;
	}

	HWND hPreviewCtrl = IpsPreviewControl();
	HWND hDescCtrl = IpsDescControl();
	HWND hLangCtrl = IpsLanguageControl();
	HWND hTree = hIpsList;

	if (bSave) {
		SavePatches();
	}

	if (hPreviewCtrl) {
		SendMessage(hPreviewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
	}
	if (hDescCtrl) {
		SendMessage(hDescCtrl, WM_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
	}
	if (hLangCtrl) {
		SendMessage(hLangCtrl, CB_RESETCONTENT, 0, 0);
	}
	if (hTree) {
		TreeView_DeleteAllItems(hTree);
		LONG_PTR Style = GetWindowLongPtr(hTree, GWL_STYLE);
		if (bIpsHostedHaveOriginalTreeStyle) {
			Style = nIpsHostedOriginalTreeStyle;
		} else {
			Style &= ~TVS_CHECKBOXES;
		}
		SetWindowLongPtr(hTree, GWL_STYLE, Style & ~TVS_CHECKBOXES);
		TreeView_SetImageList(hTree, NULL, TVSIL_STATE);
		SetWindowPos(hTree, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	IpsResetState();

	if (hBmp) {
		DeleteObject((HGDIOBJ)hBmp);
		hBmp = NULL;
	}
	if (hPreview) {
		DeleteObject((HGDIOBJ)hPreview);
		hPreview = NULL;
	}
	if (hWhiteBGBrush) {
		DeleteObject(hWhiteBGBrush);
		hWhiteBGBrush = NULL;
	}

	hIpsHostedPreview = NULL;
	hIpsHostedDesc = NULL;
	hIpsHostedLang = NULL;
	hIpsList = NULL;
	hIpsDlg = NULL;
	bIpsHostedHaveOriginalTreeStyle = false;
	nIpsHostedOriginalTreeStyle = 0;
	bIpsHostedMode = false;
}

INT32 IpsHostedHandleNotify(NMHDR* pNmHdr)
{
	if (!bIpsHostedMode || pNmHdr == NULL || pNmHdr->hwndFrom != hIpsList) {
		return 0;
	}

	if (pNmHdr->code == TVN_SELCHANGED) {
		RefreshPatch();
		return 1;
	}

	if (pNmHdr->code == NM_RCLICK) {
		IpsShowContextMenu(hIpsDlg);
		return 1;
	}

	if (pNmHdr->code == NM_DBLCLK) {
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		IpsSelectItemFromScreenPoint(&cursorPos);

		TCHAR szPatchFile[MAX_PATH] = { 0 };
		if (IpsGetSelectedPatchFile(szPatchFile, _countof(szPatchFile))) {
			SavePatches();
			return 3;
		}
		return 2;
	}

	if (pNmHdr->code == NM_CLICK) {
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(hIpsList, &cursorPos);

		TVHITTESTINFO thi;
		thi.pt = cursorPos;
		TreeView_HitTest(hIpsList, &thi);

		if ((thi.flags & TVHT_ONITEMSTATEICON) && thi.hItem) {
			TreeView_SelectItem(hIpsList, thi.hItem);
			_TreeView_SetCheckState(hIpsList, thi.hItem, !_TreeView_GetCheckState(hIpsList, thi.hItem));
			RefreshPatch();
			SavePatches();
			return 2;
		}

		return 0;
	}

	return 0;
}

void IpsHostedSetLanguage(INT32 nLanguage)
{
	if (!bIpsHostedMode) {
		return;
	}

	nIpsSelectedLanguage = nLanguage;
	TreeView_DeleteAllItems(hIpsList);
	memset(hItemHandles,       0, MAX_NODES * sizeof(HTREEITEM));
	memset(hPatchHandlesIndex, 0, MAX_NODES * sizeof(HTREEITEM));
	nPatchIndex = 0;
	nNumPatches = 0;
	for (INT32 i = 0; i < MAX_NODES; i++) {
		szPatchFileNames[i][0] = _T('\0');
	}
	FillListBox();
	CheckActivePatches();
	RefreshPatch();
}

void IpsHostedSetSearchText(const TCHAR* pszSearch)
{
	if (!bIpsHostedMode) {
		return;
	}

	_tcscpy(szIpsSearchRaw, pszSearch ? pszSearch : _T(""));
	IpsNormalizeSearchText(szIpsSearchRaw, szIpsSearchNormalized, _countof(szIpsSearchNormalized));

	TreeView_DeleteAllItems(hIpsList);
	memset(hItemHandles,       0, MAX_NODES * sizeof(HTREEITEM));
	memset(hPatchHandlesIndex, 0, MAX_NODES * sizeof(HTREEITEM));
	nPatchIndex = 0;
	nNumPatches = 0;
	for (INT32 i = 0; i < MAX_NODES; i++) {
		szPatchFileNames[i][0] = _T('\0');
	}

	FillListBox();
	CheckActivePatches();

	if (nNumPatches > 0 && hPatchHandlesIndex[0]) {
		TreeView_SelectItem(hIpsList, hPatchHandlesIndex[0]);
		TreeView_EnsureVisible(hIpsList, hPatchHandlesIndex[0]);
	}
	RefreshPatch();
	UpdateWindow(hIpsList);
}

void IpsHostedDeselectAll()
{
	if (!bIpsHostedMode) {
		return;
	}

	for (INT32 j = 0; j < nNumPatches; j++) {
		_TreeView_SetCheckState(hIpsList, hPatchHandlesIndex[j], FALSE);
	}
	RefreshPatch();
	SavePatches();
}

INT32 IpsHostedGetCheckedCount()
{
	if (!bIpsHostedMode) {
		return 0;
	}

	INT32 nActivePatches = 0;
	for (INT32 i = 0; i < nNumPatches; i++) {
		if (_TreeView_GetCheckState(hIpsList, hPatchHandlesIndex[i])) {
			nActivePatches++;
		}
	}
	return nActivePatches;
}

// Game patching
/*
#define UTF8_SIGNATURE "\xef\xbb\xbf"
*/
#define IPS_SIGNATURE  "PATCH"
#define IPS_TAG_EOF    "EOF"
#define IPS_EXT        _T(".ips")

#define BYTE3_TO_UINT(bp)						\
	 (((UINT32)(bp)[0] << 16) & 0x00FF0000) |	\
	 (((UINT32)(bp)[1] <<  8) & 0x0000FF00) |	\
	 (( UINT32)(bp)[2]        & 0x000000FF)

#define BYTE2_TO_UINT(bp)						\
	(((UINT32)(bp)[0] << 8) & 0xFF00) |			\
	(( UINT32)(bp)[1]       & 0x00FF)

bool bDoIpsPatch = false;

static void PatchFile(const char* ips_path, UINT8* base, bool readonly)
{
	char buf[6];
	FILE* f      = NULL;
	INT32 Offset = 0, Size = 0;
	UINT8* mem8  = NULL;

	if (NULL == (f = fopen(ips_path, "rb"))) {
		bprintf(0, _T("IPS - Can't open file %S!  Aborting.\n"), ips_path);
		return;
	}

	memset(buf, 0, sizeof(buf));
	fread(buf, 1, 5, f);
	if (strcmp(buf, IPS_SIGNATURE)) {
		bprintf(0, _T("IPS - Bad IPS-Signature in: %S.\n"), ips_path);
		if (f)
		{
			fclose(f);
		}
		return;
	} else {
		bprintf(0, _T("IPS - Patching with: %S. (%S)\n"), ips_path, (readonly) ? "Read-Only" : "Write");
		UINT8 ch   = 0;
		INT32 bRLE = 0;
		while (!feof(f)) {
			// read patch address offset
			fread(buf, 1, 3, f);
			buf[3] = 0;
			if (strcmp(buf, IPS_TAG_EOF) == 0)
				break;

			Offset = BYTE3_TO_UINT(buf);

			// read patch length
			fread(buf, 1, 2, f);
			Size = BYTE2_TO_UINT(buf);

			bRLE = (Size == 0);
			if (bRLE) {
				fread(buf, 1, 2, f);
				Size = BYTE2_TO_UINT(buf);
				ch   = fgetc(f);
			}

			while (Size--) {
				if (!readonly) mem8 = base + Offset + nRomOffset;
				Offset++;
				if (readonly) {
					if (!bRLE) fgetc(f);
				} else {
					*mem8 = bRLE ? ch : fgetc(f);
				}
			}
		}
	}

	// Avoid memory out-of-bounds due to ips offset greater than rom length.
	if (readonly && (0 == nIpsMemExpLen[EXP_FLAG])) {	// Unspecified length.
		nIpsMemExpLen[LOAD_ROM] = Offset;
	}

	fclose(f);
}

static char* stristr_int(const char* str1, const char* str2)
{
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = (!*p2) ? str1 : NULL;

	while (*p1 && *p2) {
		if (tolower((UINT8)*p1) == tolower((UINT8)*p2)) {
			if (!r) {
				r = p1;
			}

			p2++;
		} else {
			p2 = str2;
			if (r) {
				p1 = r + 1;
			}

			if (tolower((UINT8)*p1) == tolower((UINT8)*p2)) {
				r = p1;
				p2++;
			} else {
				r = NULL;
			}
		}

		p1++;
	}

	return (*p2) ? NULL : (char*)r;
}

static UINT32 hexto32(const char *s)
{
	UINT32 val = 0;
	char c;

	while ((c = *s++)) {
		UINT8 v = ((c & 0xf) + (c >> 6)) | ((c >> 3) & 0x8);
		val = (val << 4) | (UINT32)v;
	}

	return val;
}

// strqtoken() - functionally identicle to strtok() w/ special treatment for
// quoted strings.  -dink april 2023
static char *strqtoken(char *s, const char *delims)
{
	static char *prev_str = NULL;
	char *token = NULL;

	if (!s) s = prev_str;

	s += strspn(s, delims);
	if (s[0] == '\0') {
		prev_str = s;
		return NULL;
	}

	if (s[0] == '\"') { // time to grab quoted string!
		token = ++s;
		if ((s = strpbrk(token, "\""))) {
			*(s++) = '\0';
		}
	} else {
		token = s;
	}

	if ((s = strpbrk(s, delims))) {
		*(s++) = '\0';
		prev_str = s;
	} else {
		// we're at the end of the road
		prev_str = (char*)memchr((void *)token, '\0', MAX_PATH);
	}

	return token;
}

static void DoPatchGame(const TCHAR* patch_name, const TCHAR* game_name, const UINT32 crc, UINT8* base, bool readonly)
{
	TCHAR s[MAX_PATH] = { 0 };
	TCHAR* p          = NULL;
	TCHAR* rom_name   = NULL;
	TCHAR* ips_name   = NULL;
	TCHAR* ips_offs   = NULL;
	TCHAR* ips_crc    = NULL;
	UINT32 nIps_crc   = 0;
	FILE* fp          = NULL;

	//bprintf(0, _T("DoPatchGame [%S][%S]\n"), patch_name, game_name);

	const TCHAR* pszReadMode = AdaptiveEncodingReads(patch_name);
	if (NULL == pszReadMode) return;

	const TCHAR* pszAppRomPaths = (2 == nQuickOpen) ? szAppQuickPath : szAppIpsPath;
	TCHAR szCurrentStorageName[MAX_PATH] = { 0 };
	if (!IpsResolveStorageName(szCurrentStorageName, _countof(szCurrentStorageName)) || szCurrentStorageName[0] == _T('\0')) {
		return;
	}

	if ((fp = _tfopen(patch_name, pszReadMode)) != NULL) {
		bool bTarget = false;

		while (!feof(fp)) {
			if (_fgetts(s, sizeof(s), fp) != NULL) {
				p = s;
#if 0
				// skip UTF-8 sig
				if (strncmp(p, UTF8_SIGNATURE, strlen(UTF8_SIGNATURE)) == 0)
					p += strlen(UTF8_SIGNATURE);
#endif // 0
				if (p[0] == _T('['))	// reached info-section of .dat file, time to leave.
					break;

				// Can support linetypes: (space or tab)
				// "rom name.bin" "patch file.ips" CRC(abcd1234)
				// romname.bin patchfile CRC(abcd1234)
#define DELIM_TOKENS_NAME _T(" \t\r\n")
#define DELIM_TOKENS      _T(" \t\r\n()")

				rom_name = _strqtoken(p, DELIM_TOKENS_NAME);

				if (!rom_name)
					continue;
				if (*rom_name == _T('#'))
					continue;

				ips_name = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (!ips_name)
					continue;

				nIps_crc = 0;
				nRomOffset = 0; // Reset to 0
				if (NULL != (ips_offs = _strqtoken(NULL, DELIM_TOKENS))) {	// Parameters of the offset increment
					if (     0 == _tcscmp(ips_offs, _T("IPS_OFFSET_016"))) nRomOffset = 0x1000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_032"))) nRomOffset = 0x2000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_048"))) nRomOffset = 0x3000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_064"))) nRomOffset = 0x4000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_080"))) nRomOffset = 0x5000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_096"))) nRomOffset = 0x6000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_112"))) nRomOffset = 0x7000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_128"))) nRomOffset = 0x8000000;
					else if (0 == _tcscmp(ips_offs, _T("IPS_OFFSET_144"))) nRomOffset = 0x9000000;

					if (nRomOffset != 0) { // better get next token (crc)
						ips_offs = _strqtoken(NULL, DELIM_TOKENS);
					}
				}

				if ((ips_offs != NULL) && stristr_int(_TtoA(ips_offs), "crc")) {
					ips_crc = _strqtoken(NULL, DELIM_TOKENS);
					if (ips_crc) {
						nIps_crc = hexto32(_TtoA(ips_crc));
					}
				}

#undef DELIM_TOKENS_NAME
#undef DELIM_TOKENS

				char* has_ext = stristr_int(_TtoA(ips_name), ".ips");

				if (_tcsicmp(rom_name, game_name))	// name don't match?
					if (nIps_crc != crc)			// crc don't match?
						continue;					// not our file. next!

				bTarget = true;

				if (!readonly) {
					bprintf(0, _T("ips name:[%S]\n"), ips_name);
					bprintf(0, _T("rom name:[%S]\n"), rom_name);
					bprintf(0, _T("rom crc :[%x]\n"), nIps_crc);
				}

				bool bHasDir = false;	// The IPS file and DAT file are in the same directory
				TCHAR* str = ips_name;

				while (_T('\0') != *str) {
					if ((_T('\\') == *str) || (_T('/') == *str)) {
						bHasDir = true;	// The IPS file contains directory paths
						str = NULL;
						break;
					}
					str++;
				}

				TCHAR ips_path[MAX_PATH] = { 0 };

				if (bHasDir) {
					// Customize drv_name
					// support/ips/ips_name
					_stprintf(ips_path, _T("%s%s%s"), pszAppRomPaths, ips_name, (has_ext) ? _T("") : IPS_EXT);
				} else {
					// Default drv_name
					// support/ips/drv_name/ips_name
					_stprintf(ips_path, _T("%s%s/%s%s"), pszAppRomPaths, szCurrentStorageName, ips_name, (has_ext) ? _T("") : IPS_EXT);
				}

				PatchFile(_TtoA(ips_path), base, readonly);
			}
		}
		fclose(fp);

		if (!bTarget && (0 == nIpsMemExpLen[EXP_FLAG])) {
			// Must be reset to 0!
			nIpsMemExpLen[LOAD_ROM] = 0;
		}
	}
}

static INT32 IpsVerifyDat(const TCHAR* pszDatFile)
{
	TCHAR s[MAX_PATH] = { 0 }, * p = NULL, * rom_name = NULL, * ips_name = NULL;
	FILE* fp = NULL;

	const TCHAR* pszReadMode = AdaptiveEncodingReads(pszDatFile);
	if (NULL == pszReadMode) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS: %s\n\n"), pszDatFile);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_ENCODING), pszDatFile);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -1;
	}
	if (NULL== (fp = _tfopen(pszDatFile, pszReadMode))) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS: %s\n\n"), pszDatFile);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_OPEN), pszDatFile);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -2;
	}

	const TCHAR* pszAppRomPaths = (2 == nQuickOpen) ? szAppQuickPath : szAppIpsPath;
	TCHAR szCurrentStorageName[MAX_PATH] = { 0 };
	if (!IpsResolveStorageName(szCurrentStorageName, _countof(szCurrentStorageName)) || szCurrentStorageName[0] == _T('\0')) {
		fclose(fp);
		return -4;
	}
	INT32 nLoop = 0, nError = 0, nFind = 0;

	while (!feof(fp)) {
		if (_fgetts(s, sizeof(s), fp) != NULL) {
			p = s;

			if (p[0] == _T('[')) {
				if (0 == nLoop) {
					nError++;
					FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS: %s\n\n"), pszDatFile);
					FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_CONTENT), pszDatFile);
				}
				break;
			}

#define DELIM_TOKENS_NAME _T(" \t\r\n")
#define DELIM_TOKENS      _T(" \t\r\n()")

			rom_name = _strqtoken(p, DELIM_TOKENS_NAME);
			if (!rom_name)
				continue;
			if (*rom_name == _T('#'))
				continue;

			ips_name = _strqtoken(NULL, DELIM_TOKENS_NAME);
			if (!ips_name)
				continue;

#undef DELIM_TOKENS_NAME
#undef DELIM_TOKENS

			char* has_ext = stristr_int(_TtoA(ips_name), ".ips");
			TCHAR ips_path[MAX_PATH] = { 0 };
			_stprintf(ips_path, _T("%s%s"), ips_name, (has_ext) ? _T("") : IPS_EXT);

			if (!FileExists(ips_path)) {
				_stprintf(ips_path, _T(""));

				if ((_T('\\') == ips_name[0]) || (_T('/') == ips_name[0])) {
					_stprintf(ips_path, _T("%s%s%s"), pszAppRomPaths, ips_name + 1, (has_ext) ? _T("") : IPS_EXT);
				} else {
					_stprintf(ips_path, _T("%s%s\\%s%s"), pszAppRomPaths, szCurrentStorageName, ips_name, (has_ext) ? _T("") : IPS_EXT);
				}
				if (!FileExists(ips_path)) {
					nError++;
					if (1 == nError) {
						FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS: %s\n\n"), pszDatFile);
					}
					FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXIST), ips_path);
				} else {
					nFind++;
				}
			} else {
				nFind++;
			}
		}
		nLoop++;
	}
	fclose(fp);

	if (nError > 0) {
		FBAPopupDisplay(PUF_TYPE_ERROR);
	}

	return (nFind > 0) ? 0 : -3;
}

#undef IPS_EXT
#undef IPS_TAG_EOF
#undef IPS_SIGNATURE

static UINT32 GetIpsDefineExpValue(TCHAR* pszTmp)
{
	if (NULL == (pszTmp = _strqtoken(NULL, _T(" \t\r\n"))))
		return 0U;

	INT32 nRet = 0;

	if      (0 == _tcscmp(pszTmp, _T("EXP_VALUE_001"))) nRet = 0x0010000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_002"))) nRet = 0x0020000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_003"))) nRet = 0x0030000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_004"))) nRet = 0x0040000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_005"))) nRet = 0x0050000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_006"))) nRet = 0x0060000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_007"))) nRet = 0x0070000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_008"))) nRet = 0x0080000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_010"))) nRet = 0x0100000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_020"))) nRet = 0x0200000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_030"))) nRet = 0x0300000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_040"))) nRet = 0x0400000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_050"))) nRet = 0x0500000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_060"))) nRet = 0x0600000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_070"))) nRet = 0x0700000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_080"))) nRet = 0x0800000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_100"))) nRet = 0x1000000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_200"))) nRet = 0x2000000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_300"))) nRet = 0x3000000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_400"))) nRet = 0x4000000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_500"))) nRet = 0x5000000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_600"))) nRet = 0x6000000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_700"))) nRet = 0x7000000;
	else if (0 == _tcscmp(pszTmp, _T("EXP_VALUE_800"))) nRet = 0x8000000;
	else if (EOF != (_stscanf(pszTmp, _T("%x"), &nRet))) return nRet;

	return nRet;
}

static void ExtraPatchesExit()
{
	if (_IpsEarly.nCount > 0) {
		for (UINT32 i = 0; i < _IpsEarly.nCount; i++) {
			if (NULL != _IpsEarly.pszDat[i]) {
				free(_IpsEarly.pszDat[i]);
				_IpsEarly.pszDat[i] = NULL;
			}
		}
	}
	free(_IpsEarly.pszDat);
	_IpsEarly.pszDat = NULL;
	_IpsEarly.nCount = 0;

	if (_IpsLast.nCount > 0) {
		for (UINT32 i = 0; i < _IpsLast.nCount; i++) {
			if (NULL != _IpsLast.pszDat[i]) {
				free(_IpsLast.pszDat[i]);
				_IpsLast.pszDat[i] = NULL;
			}
		}
	}
	free(_IpsLast.pszDat);
	_IpsLast.pszDat = NULL;
	_IpsLast.nCount = 0;
}

#if 0
static bool NormalizePath(const TCHAR* pSrcPath, TCHAR* pDestPath, UINT32 nDestSize) {
	if ((NULL == pSrcPath) || (NULL == pDestPath) || (0 == nDestSize))
		return false;

	TCHAR szFullPath[MAX_PATH] = { 0 };
	DWORD dwRet = GetFullPathName(pSrcPath, MAX_PATH, szFullPath, NULL);
	if ((0 == dwRet) || (dwRet >= MAX_PATH))
		return false;

	for (UINT32 i = 0; i < dwRet; i++) {
		if (_T('\\') == szFullPath[i])
			pDestPath[i] = _T('/');
		else
			pDestPath[i] = _tolower(szFullPath[i]);
	}
	pDestPath[dwRet] = _T('\0');
	return true;
}

static bool CompareTwoPaths(const TCHAR* pszPathA, const TCHAR* pszPathB)
{
	TCHAR szPathA[MAX_PATH] = { 0 }, szPathB[MAX_PATH] = { 0 };

	if (!NormalizePath(pszPathA, szPathA, MAX_PATH) ||
		!NormalizePath(pszPathB, szPathB, MAX_PATH))
		return false;

	return (0 == _tcsicmp(szPathA, szPathB));
}
#endif

static INT32 IpsDatPathGetInfo(const TCHAR* pszSelDat, TCHAR** pszDrvName, TCHAR** pszIpsPath)
{
	*pszDrvName = *pszIpsPath = NULL;

	if ((NULL == pszSelDat) || !FileExists(pszSelDat)) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS %s:\n\n"), pszSelDat);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXIST), pszSelDat);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -2;
	}

	const TCHAR* pszExt = _tcsrchr(pszSelDat, _T('.'));
	if (NULL == pszExt || (0 != _tcsicmp(_T(".dat"), pszExt))) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS %s:\n\n"), pszSelDat);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXTENSION), pszSelDat, _T(".dat"));
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -5;
	}

	const TCHAR* p = pszSelDat + _tcslen(pszSelDat), * drv_start = NULL, * drv_end = NULL, * dir_end = NULL;
	INT32 nCount = 0;
	while (p > pszSelDat) {
		if ((_T('/') == *p) || (_T('\\') == *p)) {
			TCHAR c = *(p - 1);
			if ((_T('/') == c) ||
				(_T('\\') == c)) {		// xxxx//ssss\\...
				FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS %s:\n\n"), pszSelDat);
				FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_FILE_EXIST), pszSelDat);
				FBAPopupDisplay(PUF_TYPE_ERROR);
				return -2;
			}

			nCount++;
			if (1 == nCount) {
				drv_end = p;			// Intentionally add 1
			}
			if (2 == nCount) {
				dir_end = drv_start = p + 1;
			}
		}
		p--;
	}

	TCHAR szDrvName[64] = { 0 }, szIpsPath[MAX_PATH] = { 0 };
	switch (nCount) {
		case 0:							// xxx.dat
			FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS %s:\n\n"), pszSelDat);
			FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_DRIVER_NOT_EXIST), pszSelDat);
			FBAPopupDisplay(PUF_TYPE_ERROR);
			return -4;
		case 1:							// drv_name/xxx.dat
			drv_start = dir_end = p;
			_tcscpy(szIpsPath, _T(""));
			break;
		default:
			_tcsncpy(szIpsPath, pszSelDat, dir_end - pszSelDat);
			break;
	}

	const UINT32 nDrvLen = drv_end - drv_start;
	if (0 == nDrvLen) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS %s:\n\n"), pszSelDat);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_DRIVER_NOT_EXIST), pszSelDat);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -4;
	}

	_tcsncpy(szDrvName, drv_start, nDrvLen);

	const INT32 nDrvIdx = BurnDrvGetIndex(_TtoA(szDrvName));
	if (nDrvIdx < 0) {
		FBAPopupAddText(PUF_TEXT_DEFAULT, _T("IPS %s:\n\n"), pszSelDat);
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_DRIVER_NOT_EXIST), pszSelDat);
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return -4;
	}

	*pszDrvName = _tcsdup(szDrvName);	// The caller needs to free it
	*pszIpsPath = _tcsdup(szIpsPath);	// free(pszIpsPath)

	return nDrvIdx;
}

INT32 IpsGetDrvForQuickOpen(const TCHAR* pszSelDat)
{
	TCHAR* pszDrv, * pszPath;
	const INT32 nDrvIdx = IpsDatPathGetInfo(pszSelDat, &pszDrv, &pszPath);
	if (nDrvIdx < 0)
		return nDrvIdx;

	_tcscpy(szStorageName,  pszDrv);  free(pszDrv);
	_tcscpy(szAppQuickPath, pszPath); free(pszPath);

	INT32 nVerify = IpsVerifyDat(pszSelDat);
	if (0 != nVerify) {
		memset(szStorageName, 0, sizeof(szStorageName));
		return nVerify;
	}

	_tcscpy(szIpsActivePatches[0], pszSelDat);

	for (INT32 i = 1; i < MAX_ACTIVE_PATCHES; i++) {
		_stprintf(szIpsActivePatches[i], _T(""));
	}

	return nDrvIdx;
}

static void ExtraPatchesInit(const INT32 nActivePatches)
{
	ExtraPatchesExit();

	const TCHAR* pszAppRomPaths = (2 == nQuickOpen) ? szAppQuickPath : szAppIpsPath;
	TCHAR szCurrentStorageName[MAX_PATH] = { 0 };
	if (!IpsResolveStorageName(szCurrentStorageName, _countof(szCurrentStorageName)) || szCurrentStorageName[0] == _T('\0')) {
		return;
	}

	for (INT32 i = 0; i < nActivePatches; i++) {
		TCHAR str[MAX_PATH] = { 0 }, * ptr = NULL, * tmp = NULL;
		FILE* fp = NULL;
		INT32 nNumbers = 0;

		const TCHAR* pszReadMode = AdaptiveEncodingReads(szIpsActivePatches[i]);
		if (NULL == pszReadMode) continue;

		if (NULL != (fp = _tfopen(szIpsActivePatches[i], pszReadMode))) {
			while (!feof(fp)) {
				if (NULL != _fgetts(str, sizeof(str), fp)) {
					ptr = str;

					if (NULL == (tmp = _strqtoken(ptr, _T(" \t\r\n"))))
						continue;
					if (_T('[') == tmp[0])
						break;
					if ((_T('/') == tmp[0]) && (_T('/') == tmp[1]))
						continue;
					if ((0 != _tcscmp(tmp, _T("#define"))) && (0 != _tcscmp(tmp, _T("#include")))) {
						nNumbers++;
						continue;
					} else {
						if (0 == _tcscmp(tmp, _T("#define"))) {
							continue;
						}
						if (0 == _tcscmp(tmp, _T("#include"))) {
							if (NULL == (tmp = _strqtoken(NULL, _T(" \t\r\n"))))
								continue;
							/*
								#inclede "kof97"
								#inclede "kof97.dat"
							*/
							// Only dat files in the same directory are allowed
							// Only accepts ips with the same driver
							if ((NULL != _tcsrchr(tmp, _T('/'))) || (NULL != _tcsrchr(tmp, _T('\\'))))
								continue;

							// ips_path/drv_name/game.dat
							TCHAR szInclde[MAX_PATH] = { 0 };
							const TCHAR* pszFormat = (NULL == _tcsrchr(tmp, _T('.'))) ? _T("%s%s\\%s.dat") : _T("%s%s\\%s");
							_stprintf(szInclde, pszFormat, pszAppRomPaths, szCurrentStorageName, tmp);

							// Check if the file exists
							if (!FileExists(szInclde))
								continue;

							TCHAR** newArray = (TCHAR**)realloc((nNumbers > 0) ? _IpsLast.pszDat : _IpsEarly.pszDat, (((nNumbers > 0) ? _IpsLast.nCount : _IpsEarly.nCount) + 1) * sizeof(TCHAR*));
							((nNumbers > 0) ? _IpsLast.pszDat : _IpsEarly.pszDat) = newArray;

							((nNumbers > 0) ? _IpsLast.pszDat : _IpsEarly.pszDat)[(nNumbers > 0) ? _IpsLast.nCount : _IpsEarly.nCount] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
							_tcscpy(((nNumbers > 0) ? _IpsLast.pszDat : _IpsEarly.pszDat)[(nNumbers > 0) ? _IpsLast.nCount : _IpsEarly.nCount], szInclde);
							((nNumbers > 0) ? _IpsLast.nCount : _IpsEarly.nCount)++;
						}
					}
				}
			}
			fclose(fp);
		}
	}
}

// Run once to get the definition & definition values of the DAT files.
// Suppress CPU usage caused by multiple runs.
// Two entry points: cmdline Launch & SelOkay.
static void GetIpsDrvDefine()
{
	if (!bDoIpsPatch)
		return;

	nIpsDrvDefine = 0;
	memset(nIpsMemExpLen, 0, sizeof(nIpsMemExpLen));

	INT32 nActivePatches = GetIpsNumActivePatches();
	ExtraPatchesInit(nActivePatches);

	INT32 nCount = _IpsEarly.nCount + nActivePatches + _IpsLast.nCount;

	for (INT32 i = 0; i < nCount; i++) {
		TCHAR str[MAX_PATH] = { 0 }, * ptr = NULL, * tmp = NULL, * pszDatName = szIpsActivePatches[i];
		FILE* fp = NULL;

		if ((_IpsEarly.nCount) > 0 || (_IpsLast.nCount > 0)) {
			if ((i >= 0) && (i < _IpsEarly.nCount)) {
				pszDatName = _IpsEarly.pszDat[i];
			}
			if ((i >= _IpsEarly.nCount) && (i < (_IpsEarly.nCount + nActivePatches))) {
				pszDatName = szIpsActivePatches[i - _IpsEarly.nCount];
			}
			if (i >= (_IpsEarly.nCount + nActivePatches)) {
				pszDatName = _IpsLast.pszDat[i - (_IpsEarly.nCount + nActivePatches)];
			}
		}

		const TCHAR* pszReadMode = AdaptiveEncodingReads(pszDatName);
		if (NULL == pszReadMode) continue;

		if (NULL != (fp = _tfopen(pszDatName, pszReadMode))) {
			while (!feof(fp)) {
				if (NULL != _fgetts(str, sizeof(str), fp)) {
					ptr = str;
#if 0
					// skip UTF-8 sig
					if (0 == strncmp(ptr, UTF8_SIGNATURE, strlen(UTF8_SIGNATURE)))
						ptr += strlen(UTF8_SIGNATURE);
#endif // 0
					if (NULL == (tmp = _strqtoken(ptr,  _T(" \t\r\n"))))
						continue;
					if (_T('[') == tmp[0])
						break;
					if ((_T('/') == tmp[0]) && (_T('/') == tmp[1]))
						continue;
					if (0 == _tcscmp(tmp, _T("#define"))) {
						if (NULL == (tmp = _strqtoken(NULL, _T(" \t\r\n"))))
							continue;

						UINT32 nNewValue = 0;

						if (0 == _tcscmp(tmp, _T("IPS_NOT_PROTECT"))) {
							nIpsDrvDefine |= IPS_NOT_PROTECT;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_PGM_SPRHACK"))) {
							nIpsDrvDefine |= IPS_PGM_SPRHACK;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_PGM_MAPHACK"))) {
							nIpsDrvDefine |= IPS_PGM_MAPHACK;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_PGM_SNDOFFS"))) {
							nIpsDrvDefine |= IPS_PGM_SNDOFFS;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_SNES_VRAMHK"))) {
							nIpsDrvDefine |= IPS_SNES_VRAMHK;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_NEO_RAMHACK"))) {
							nIpsDrvDefine |= IPS_NEO_RAMHACK;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_LOAD_EXPAND"))) {
							nIpsDrvDefine |= IPS_LOAD_EXPAND;
							nIpsMemExpLen[EXP_FLAG] = 1;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[LOAD_ROM])
								nIpsMemExpLen[LOAD_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_EXTROM_INCL"))) {
							nIpsDrvDefine |= IPS_EXTROM_INCL;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[EXTR_ROM])
								nIpsMemExpLen[EXTR_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_PRG1_EXPAND"))) {
							nIpsDrvDefine |= IPS_PRG1_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[PRG1_ROM])
								nIpsMemExpLen[PRG1_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_PRG2_EXPAND"))) {
							nIpsDrvDefine |= IPS_PRG2_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[PRG2_ROM])
								nIpsMemExpLen[PRG2_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_GRA1_EXPAND"))) {
							nIpsDrvDefine |= IPS_GRA1_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[GRA1_ROM])
								nIpsMemExpLen[GRA1_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_GRA2_EXPAND"))) {
							nIpsDrvDefine |= IPS_GRA2_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[GRA2_ROM])
								nIpsMemExpLen[GRA2_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_GRA3_EXPAND"))) {
							nIpsDrvDefine |= IPS_GRA3_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[GRA3_ROM])
								nIpsMemExpLen[GRA3_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_ACPU_EXPAND"))) {
							nIpsDrvDefine |= IPS_ACPU_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[ACPU_ROM])
								nIpsMemExpLen[ACPU_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_SND1_EXPAND"))) {
							nIpsDrvDefine |= IPS_SND1_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[SND1_ROM])
								nIpsMemExpLen[SND1_ROM] = nNewValue;
							continue;
						}
						if (0 == _tcscmp(tmp, _T("IPS_SND2_EXPAND"))) {
							nIpsDrvDefine |= IPS_SND2_EXPAND;
							if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[SND2_ROM])
								nIpsMemExpLen[SND2_ROM] = nNewValue;
							continue;
						}
					}
				}
			}
			fclose(fp);
		}
	}
}

void IpsApplyPatches(UINT8* base, char* rom_name, UINT32 crc, bool readonly)
{
	if (!bDoIpsPatch)
		return;

	INT32 nActivePatches = GetIpsNumActivePatches();
	INT32 nCount = _IpsEarly.nCount + nActivePatches + _IpsLast.nCount;

	for (INT32 i = 0; i < nCount; i++) {
		TCHAR* pszDatName = szIpsActivePatches[i];
		if ((_IpsEarly.nCount) > 0 || (_IpsLast.nCount > 0)) {
			if ((i >= 0) && (i < _IpsEarly.nCount)) {
				pszDatName = _IpsEarly.pszDat[i];
			}
			if ((i >= _IpsEarly.nCount) && (i < (_IpsEarly.nCount + nActivePatches))) {
				pszDatName = szIpsActivePatches[i - _IpsEarly.nCount];
			}
			if (i >= (_IpsEarly.nCount + nActivePatches)) {
				pszDatName = _IpsLast.pszDat[i - (_IpsEarly.nCount + nActivePatches)];
			}
		}
		DoPatchGame(pszDatName, _AtoT(rom_name), crc, base, readonly);
	}
}

void IpsPatchInit()
{
	GetIpsDrvDefine();
}

void IpsPatchExit()
{
	ExtraPatchesExit();
	memset(nIpsMemExpLen, 0, sizeof(nIpsMemExpLen));

	nIpsDrvDefine = 0;
	bDoIpsPatch   = false;
}

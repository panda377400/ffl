// Memory card support module

#include "burner.h"
#include "../../burn/drv/pgm2/pgm2_memcard.h"

static TCHAR szMemoryCardFile[MAX_PATH];
static TCHAR szPgm2MemoryCardFile[4][MAX_PATH];
static TCHAR szPgm2MemcardFamily[32];
static TCHAR szPgm2MemcardDirectory[MAX_PATH];

int nMemoryCardStatus = 0;
int nMemoryCardSize;
static int nPgm2MemoryCardStatus[4];
static INT32 nPgm2MemcardSlotCount = 0;

static int nMinVersion;
static bool bMemCardFC1Format;
static bool bPgm2MemCardFC1Format[4];
static bool bPgm2MemcardAutoInsert[4];

static bool ends_with_ignore_case_a(const char* value, const char* suffix)
{
	if (!value || !suffix) {
		return false;
	}

	size_t value_len = strlen(value);
	size_t suffix_len = strlen(suffix);
	if (value_len < suffix_len) {
		return false;
	}

	return _stricmp(value + value_len - suffix_len, suffix) == 0;
}

static UINT32 GetAppDirectoryLocal(TCHAR* pszAppPath)
{
	TCHAR szExePath[MAX_PATH] = { 0 };

	if (GetModuleFileName(NULL, szExePath, MAX_PATH) == 0) {
		return 0;
	}

	TCHAR* pLastSlash = _tcsrchr(szExePath, _T('\\'));
	if (pLastSlash == NULL) {
		pLastSlash = _tcsrchr(szExePath, _T('/'));
		if (pLastSlash == NULL) {
			return 0;
		}
	}

	UINT32 nLen = (UINT32)(pLastSlash - szExePath + 1);
	if (nLen >= MAX_PATH) {
		return 0;
	}

	if (pszAppPath != NULL) {
		_tcsncpy(pszAppPath, szExePath, nLen);
		pszAppPath[nLen] = _T('\0');
	}

	return nLen;
}

static bool FileExistsT(const TCHAR* pszPath)
{
	if (pszPath == NULL || pszPath[0] == _T('\0')) {
		return false;
	}

	DWORD attr = GetFileAttributes(pszPath);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool DirectoryExistsT(const TCHAR* pszPath)
{
	if (pszPath == NULL || pszPath[0] == _T('\0')) {
		return false;
	}

	DWORD attr = GetFileAttributes(pszPath);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static bool EnsureDirectoryT(const TCHAR* pszPath)
{
	if (DirectoryExistsT(pszPath)) {
		return true;
	}

	if (CreateDirectory(pszPath, NULL)) {
		return true;
	}

	return GetLastError() == ERROR_ALREADY_EXISTS && DirectoryExistsT(pszPath);
}

static void TrimLineEnding(TCHAR* pszText)
{
	if (pszText == NULL) {
		return;
	}

	for (INT32 i = 0; pszText[i] != _T('\0'); i++) {
		if (pszText[i] == _T('\r') || pszText[i] == _T('\n')) {
			pszText[i] = _T('\0');
			break;
		}
	}
}

static bool SaveTextLineT(const TCHAR* pszPath, const TCHAR* pszText)
{
	FILE* fp = _tfopen(pszPath, _T("wb"));
	if (fp == NULL) {
		return false;
	}

	if (pszText && pszText[0]) {
		_fputts(pszText, fp);
	}

	fclose(fp);
	return true;
}

static bool LoadTextLineT(const TCHAR* pszPath, TCHAR* pszText, size_t nCount)
{
	if (pszText == NULL || nCount == 0) {
		return false;
	}

	pszText[0] = _T('\0');

	FILE* fp = _tfopen(pszPath, _T("rb"));
	if (fp == NULL) {
		return false;
	}

	if (_fgetts(pszText, (int)nCount, fp) == NULL) {
		fclose(fp);
		pszText[0] = _T('\0');
		return false;
	}

	fclose(fp);
	TrimLineEnding(pszText);
	return true;
}

static void Pgm2MemcardClearRuntimeState()
{
	memset(szPgm2MemoryCardFile, 0, sizeof(szPgm2MemoryCardFile));
	memset(nPgm2MemoryCardStatus, 0, sizeof(nPgm2MemoryCardStatus));
	memset(bPgm2MemCardFC1Format, 0, sizeof(bPgm2MemCardFC1Format));
	memset(bPgm2MemcardAutoInsert, 0, sizeof(bPgm2MemcardAutoInsert));
	szPgm2MemcardFamily[0] = _T('\0');
	szPgm2MemcardDirectory[0] = _T('\0');
	nPgm2MemcardSlotCount = 0;
	nMemoryCardStatus = 0;
}

static bool DetectPgm2MemcardConfig(TCHAR* pszFamily, size_t nFamilyCount, INT32* pnSlotCount)
{
	const char* pszDrv = BurnDrvGetTextA(DRV_NAME);
	if (pszFamily == NULL || nFamilyCount == 0 || pnSlotCount == NULL || pszDrv == NULL) {
		return false;
	}

	pszFamily[0] = _T('\0');
	*pnSlotCount = 0;

	if ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) != HARDWARE_IGS_PGM2) {
		return false;
	}

	if (strncmp(pszDrv, "kov2nl", 6) == 0) {
		_tcsncpy(pszFamily, _T("kov2nl"), nFamilyCount - 1);
		pszFamily[nFamilyCount - 1] = _T('\0');
		*pnSlotCount = 4;
		return true;
	}

	if (strncmp(pszDrv, "kov3", 4) == 0) {
		_tcsncpy(pszFamily, _T("kov3"), nFamilyCount - 1);
		pszFamily[nFamilyCount - 1] = _T('\0');
		*pnSlotCount = 2;
		return true;
	}

	if (strncmp(pszDrv, "orleg2", 6) == 0 &&
		(ends_with_ignore_case_a(pszDrv, "cn") || ends_with_ignore_case_a(pszDrv, "hk") || ends_with_ignore_case_a(pszDrv, "tw"))) {
		_tcsncpy(pszFamily, _T("orleg2"), nFamilyCount - 1);
		pszFamily[nFamilyCount - 1] = _T('\0');
		*pnSlotCount = 4;
		return true;
	}

	return false;
}

static bool EnsurePgm2MemcardDirectory()
{
	TCHAR szFamily[32];
	INT32 nSlotCount = 0;
	TCHAR szAppPath[MAX_PATH];
	TCHAR szBaseDir[MAX_PATH];
	TCHAR szFamilyDir[MAX_PATH];

	if (!DetectPgm2MemcardConfig(szFamily, _countof(szFamily), &nSlotCount)) {
		return false;
	}

	if (GetAppDirectoryLocal(szAppPath) == 0) {
		return false;
	}

	_stprintf(szBaseDir, _T("%smemcard"), szAppPath);
	_stprintf(szFamilyDir, _T("%smemcard\\%s"), szAppPath, szFamily);

	if (!EnsureDirectoryT(szBaseDir)) {
		return false;
	}

	if (!EnsureDirectoryT(szFamilyDir)) {
		return false;
	}

	_tcsncpy(szPgm2MemcardFamily, szFamily, _countof(szPgm2MemcardFamily) - 1);
	szPgm2MemcardFamily[_countof(szPgm2MemcardFamily) - 1] = _T('\0');
	_stprintf(szPgm2MemcardDirectory, _T("%s\\"), szFamilyDir);
	nPgm2MemcardSlotCount = nSlotCount;
	if (nPgm2MemcardSlotCount > 4) {
		nPgm2MemcardSlotCount = 4;
	}

	return true;
}

static void GetPgm2MemcardMetaPath(UINT32 slot, const TCHAR* pszPrefix, TCHAR* pszPath)
{
	_stprintf(pszPath, _T("%s%s_p%u.txt"), szPgm2MemcardDirectory, pszPrefix, slot + 1);
}

static void Pgm2MemcardPersistSlot(UINT32 slot)
{
	slot &= 3;
	if (!EnsurePgm2MemcardDirectory()) {
		return;
	}

	TCHAR szMetaPath[MAX_PATH];

	GetPgm2MemcardMetaPath(slot, _T("last_card"), szMetaPath);
	if (szPgm2MemoryCardFile[slot][0]) {
		SaveTextLineT(szMetaPath, szPgm2MemoryCardFile[slot]);
	} else {
		DeleteFile(szMetaPath);
	}

	GetPgm2MemcardMetaPath(slot, _T("auto_insert"), szMetaPath);
	if (bPgm2MemcardAutoInsert[slot] && szPgm2MemoryCardFile[slot][0]) {
		SaveTextLineT(szMetaPath, _T("1"));
	} else {
		DeleteFile(szMetaPath);
	}
}

static void Pgm2MemcardLoadPersistedSlot(UINT32 slot)
{
	slot &= 3;
	if (!EnsurePgm2MemcardDirectory()) {
		return;
	}

	TCHAR szMetaPath[MAX_PATH];
	TCHAR szValue[MAX_PATH];

	szPgm2MemoryCardFile[slot][0] = _T('\0');
	bPgm2MemCardFC1Format[slot] = false;
	bPgm2MemcardAutoInsert[slot] = false;

	GetPgm2MemcardMetaPath(slot, _T("last_card"), szMetaPath);
	if (LoadTextLineT(szMetaPath, szValue, _countof(szValue)) && FileExistsT(szValue)) {
		_tcsncpy(szPgm2MemoryCardFile[slot], szValue, _countof(szPgm2MemoryCardFile[slot]) - 1);
		szPgm2MemoryCardFile[slot][_countof(szPgm2MemoryCardFile[slot]) - 1] = _T('\0');
	}

	GetPgm2MemcardMetaPath(slot, _T("auto_insert"), szMetaPath);
	if (szPgm2MemoryCardFile[slot][0] && LoadTextLineT(szMetaPath, szValue, _countof(szValue))) {
		bPgm2MemcardAutoInsert[slot] = (_tcscmp(szValue, _T("1")) == 0);
	}

	nPgm2MemoryCardStatus[slot] = szPgm2MemoryCardFile[slot][0] ? 1 : 0;
}

bool DriverUsesPgm2RawMemcard()
{
	TCHAR szFamily[32];
	INT32 nSlotCount = 0;
	return DetectPgm2MemcardConfig(szFamily, _countof(szFamily), &nSlotCount);
}

bool DriverSupportsMemcard()
{
	if ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_SNK_NEOGEO) {
		return true;
	}

	return DriverUsesPgm2RawMemcard();
}

int MemCardPgm2SlotCount()
{
	if (!DriverUsesPgm2RawMemcard()) {
		return 0;
	}

	if (nPgm2MemcardSlotCount <= 0) {
		EnsurePgm2MemcardDirectory();
	}

	return nPgm2MemcardSlotCount;
}

static int MakeOfn()
{
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hScrnWnd;
	ofn.lpstrFile = szMemoryCardFile;
	ofn.nMaxFile = sizeof(szMemoryCardFile);
	ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = DriverUsesPgm2RawMemcard() ? _T("pg2") : _T("fc");

	if (DriverUsesPgm2RawMemcard() && EnsurePgm2MemcardDirectory()) {
		ofn.lpstrInitialDir = szPgm2MemcardDirectory;
	} else {
		ofn.lpstrInitialDir = _T(".");
	}

	return 0;
}

static int MemCardRead(TCHAR* szFilename, unsigned char* pData, int nSize, bool* pbFC1Format)
{
	const char* szHeader  = "FB1 FC1 ";
	char szReadHeader[8] = "";
	bool* pbFormat = pbFC1Format ? pbFC1Format : &bMemCardFC1Format;

	*pbFormat = false;

	FILE* fp = _tfopen(szFilename, _T("rb"));
	if (fp == NULL) {
		return 1;
	}

	fread(szReadHeader, 1, 8, fp);
	if (memcmp(szReadHeader, szHeader, 8) == 0) {

		int nChunkSize = 0;
		int nVersion = 0;

		*pbFormat = true;

		fread(&nChunkSize, 1, 4, fp);
		if (nSize < nChunkSize - 32) {
			fclose(fp);
			return 1;
		}

		fread(&nVersion, 1, 4, fp);
		if (nVersion < nMinVersion) {
			fclose(fp);
			return 1;
		}
		fread(&nVersion, 1, 4, fp);

		fseek(fp, 0x0C, SEEK_CUR);
		fread(pData, 1, nChunkSize - 32, fp);
	} else {
		if (DriverUsesPgm2RawMemcard()) {
			long nFileSize;

			fseek(fp, 0, SEEK_END);
			nFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			if (nFileSize != nSize) {
				fclose(fp);
				return 1;
			}

			fread(pData, 1, nSize, fp);
			fclose(fp);
			return 0;
		}

		unsigned char* pTemp = (unsigned char*)malloc(nSize >> 1);

		memset(pData, 0, nSize);
		fseek(fp, 0x00, SEEK_SET);

		if (pTemp) {
			fread(pTemp, 1, nSize >> 1, fp);

			for (int i = 1; i < nSize; i += 2) {
				pData[i] = pTemp[i >> 1];
			}

			free(pTemp);
			pTemp = NULL;
		}
	}

	fclose(fp);

	return 0;
}

static int MemCardWrite(TCHAR* szFilename, unsigned char* pData, int nSize, bool bFC1Format)
{
	FILE* fp = _tfopen(szFilename, _T("wb"));
	if (fp == NULL) {
		return 1;
	}

	if (DriverUsesPgm2RawMemcard() && !bFC1Format) {
		fwrite(pData, 1, nSize, fp);
	} else if (bFC1Format) {

		const char* szFileHeader  = "FB1 ";
		const char* szChunkHeader = "FC1 ";
		const int nZero = 0;
		int nChunkSize = nSize + 32;

		fwrite(szFileHeader, 1, 4, fp);
		fwrite(szChunkHeader, 1, 4, fp);
		fwrite(&nChunkSize, 1, 4, fp);
		fwrite(&nBurnVer, 1, 4, fp);
		fwrite(&nMinVersion, 1, 4, fp);
		fwrite(&nZero, 1, 4, fp);
		fwrite(&nZero, 1, 4, fp);
		fwrite(&nZero, 1, 4, fp);
		fwrite(pData, 1, nSize, fp);
	} else {

		unsigned char* pTemp = (unsigned char*)malloc(nSize >> 1);
		if (pTemp) {
			for (int i = 1; i < nSize; i += 2) {
				pTemp[i >> 1] = pData[i];
			}

			fwrite(pTemp, 1, nSize >> 1, fp);
			free(pTemp);
			pTemp = NULL;
		}
	}

	fclose(fp);

	return 0;
}

static void Pgm2MemcardSyncStatus()
{
	nMemoryCardStatus = nPgm2MemoryCardStatus[0];
}

static int Pgm2MemcardGetSize()
{
	nMemoryCardSize = (int)pgm2MemcardGetSize(0);
	return nMemoryCardSize > 0 ? 0 : 1;
}

static int __cdecl MemCardDoGetSize(struct BurnArea* pba)
{
	nMemoryCardSize = pba->nLen;
	return 0;
}

static int MemCardGetSize()
{
	if (DriverUsesPgm2RawMemcard()) {
		return Pgm2MemcardGetSize();
	}

	BurnAcb = MemCardDoGetSize;
	BurnAreaScan(ACB_MEMCARD | ACB_MEMCARD_ACTION, &nMinVersion);

	return 0;
}

static int Pgm2MemcardStoreSlot(UINT32 slot)
{
	slot &= 3;

	if ((nPgm2MemoryCardStatus[slot] & 2) == 0 || szPgm2MemoryCardFile[slot][0] == 0) {
		return 0;
	}

	if (Pgm2MemcardGetSize()) {
		return 1;
	}

	unsigned char* pCard = (unsigned char*)malloc(nMemoryCardSize);
	if (pCard == NULL) {
		return 1;
	}

	const int nRet =
		pgm2MemcardSaveImageSlot(slot, pCard, nMemoryCardSize) ||
		MemCardWrite(szPgm2MemoryCardFile[slot], pCard, nMemoryCardSize, bPgm2MemCardFC1Format[slot]);

	free(pCard);
	return nRet;
}

static int Pgm2MemcardInsertSelectedSlot(UINT32 slot)
{
	slot &= 3;

	if (!DriverUsesPgm2RawMemcard() || slot >= (UINT32)MemCardPgm2SlotCount()) {
		return 1;
	}

	if ((nPgm2MemoryCardStatus[slot] & 2) != 0) {
		return 0;
	}

	if ((nPgm2MemoryCardStatus[slot] & 1) == 0) {
		Pgm2MemcardLoadPersistedSlot(slot);
	}

	if (szPgm2MemoryCardFile[slot][0] == 0 || !FileExistsT(szPgm2MemoryCardFile[slot])) {
		szPgm2MemoryCardFile[slot][0] = _T('\0');
		nPgm2MemoryCardStatus[slot] = 0;
		bPgm2MemcardAutoInsert[slot] = false;
		bPgm2MemCardFC1Format[slot] = false;
		Pgm2MemcardPersistSlot(slot);
		Pgm2MemcardSyncStatus();
		MenuEnableItems();
		return 1;
	}

	if (Pgm2MemcardGetSize()) {
		return 1;
	}

	unsigned char* pCard = (unsigned char*)malloc(nMemoryCardSize);
	bool bFC1Format = false;
	if (pCard == NULL) {
		return 1;
	}

	if (MemCardRead(szPgm2MemoryCardFile[slot], pCard, nMemoryCardSize, &bFC1Format)) {
		free(pCard);
		return 1;
	}

	const int nLoadRet = pgm2MemcardLoadImageSlot(slot, pCard, nMemoryCardSize);
	free(pCard);

	if (nLoadRet != 0) {
		return 1;
	}

	bPgm2MemCardFC1Format[slot] = bFC1Format;
	nPgm2MemoryCardStatus[slot] = 3;
	Pgm2MemcardSyncStatus();
	MenuEnableItems();

	return 0;
}

static int Pgm2MemcardRuntimeEjectSlot(UINT32 slot, bool bDisableAutoInsert)
{
	slot &= 3;

	if (!DriverUsesPgm2RawMemcard() || slot >= (UINT32)MemCardPgm2SlotCount()) {
		return 1;
	}

	if ((nPgm2MemoryCardStatus[slot] & 2) == 0) {
		if (bDisableAutoInsert) {
			bPgm2MemcardAutoInsert[slot] = false;
			Pgm2MemcardPersistSlot(slot);
			MenuEnableItems();
		}
		return 0;
	}

	if (Pgm2MemcardStoreSlot(slot)) {
		return 1;
	}

	pgm2MemcardEjectSlot(slot);
	nPgm2MemoryCardStatus[slot] = szPgm2MemoryCardFile[slot][0] ? 1 : 0;
	if (bDisableAutoInsert) {
		bPgm2MemcardAutoInsert[slot] = false;
	}
	Pgm2MemcardPersistSlot(slot);
	Pgm2MemcardSyncStatus();
	MenuEnableItems();

	return 0;
}

static void Pgm2MemcardSetSelectedPath(UINT32 slot, const TCHAR* pszFilename)
{
	slot &= 3;

	if (pszFilename && pszFilename[0]) {
		_tcsncpy(szPgm2MemoryCardFile[slot], pszFilename, _countof(szPgm2MemoryCardFile[slot]) - 1);
		szPgm2MemoryCardFile[slot][_countof(szPgm2MemoryCardFile[slot]) - 1] = _T('\0');
		nPgm2MemoryCardStatus[slot] = 1;
	} else {
		szPgm2MemoryCardFile[slot][0] = _T('\0');
		nPgm2MemoryCardStatus[slot] = 0;
		bPgm2MemCardFC1Format[slot] = false;
	}

	Pgm2MemcardSyncStatus();
	MenuEnableItems();
}

static bool Pgm2MemcardBuildNewPath(UINT32 slot, TCHAR* pszPath)
{
	if (!EnsurePgm2MemcardDirectory() || pszPath == NULL) {
		return false;
	}

	SYSTEMTIME st;
	GetLocalTime(&st);

	for (INT32 attempt = 0; attempt < 1000; attempt++) {
		if (attempt == 0) {
			_stprintf(pszPath, _T("%sp%u_memorycard_%04u%02u%02u_%02u%02u%02u.pg2"),
				szPgm2MemcardDirectory, slot + 1,
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond);
		} else {
			_stprintf(pszPath, _T("%sp%u_memorycard_%04u%02u%02u_%02u%02u%02u_%03d.pg2"),
				szPgm2MemcardDirectory, slot + 1,
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond, attempt + 1);
		}

		if (!FileExistsT(pszPath)) {
			return true;
		}
	}

	return false;
}

int MemCardStatusSlot(UINT32 slot)
{
	slot &= 3;

	if (DriverUsesPgm2RawMemcard()) {
		return nPgm2MemoryCardStatus[slot];
	}

	return (slot == 0) ? nMemoryCardStatus : 0;
}

void MemCardGameInit()
{
	if (!DriverUsesPgm2RawMemcard()) {
		Pgm2MemcardClearRuntimeState();
		return;
	}

	Pgm2MemcardClearRuntimeState();
	if (!EnsurePgm2MemcardDirectory()) {
		return;
	}

	for (INT32 slot = 0; slot < MemCardPgm2SlotCount() && slot < 4; slot++) {
		Pgm2MemcardLoadPersistedSlot(slot);

		if (bPgm2MemcardAutoInsert[slot] && (nPgm2MemoryCardStatus[slot] & 1)) {
			if (Pgm2MemcardInsertSelectedSlot(slot) != 0) {
				bPgm2MemcardAutoInsert[slot] = false;
				Pgm2MemcardPersistSlot(slot);
			}
		}
	}

	Pgm2MemcardSyncStatus();
	MenuEnableItems();
}

void MemCardGameExit()
{
	if (!DriverUsesPgm2RawMemcard()) {
		return;
	}

	for (INT32 slot = 0; slot < MemCardPgm2SlotCount() && slot < 4; slot++) {
		if (nPgm2MemoryCardStatus[slot] & 2) {
			Pgm2MemcardRuntimeEjectSlot(slot, false);
		}
	}
}

int MemCardCreate()
{
	TCHAR szFilter[1024];
	int nRet;

	if (DriverUsesPgm2RawMemcard()) {
		return MemCardCreateSlot(0);
	}

	_stprintf(szFilter, FBALoadStringEx(hAppInst, IDS_DISK_FILE_CARD, true), _T(APP_TITLE));
	{
		static const TCHAR szFcCreateFilter[] = _T(" (*.fc)\0*.fc\0\0");
		memcpy(szFilter + _tcslen(szFilter), szFcCreateFilter, sizeof(szFcCreateFilter));
	}

	_stprintf(szMemoryCardFile, _T("memorycard"));
	MakeOfn();
	ofn.lpstrTitle = FBALoadStringEx(hAppInst, IDS_MEMCARD_CREATE, true);
	ofn.lpstrFilter = szFilter;
	ofn.Flags |= OFN_OVERWRITEPROMPT;

	int bOldPause = bRunPause;
	bRunPause = 1;
	nRet = GetSaveFileName(&ofn);
	bRunPause = bOldPause;

	if (nRet == 0) {
		return 1;
	}

	unsigned char* pCard;

	MemCardGetSize();

	pCard = (unsigned char*)malloc(nMemoryCardSize);
	memset(pCard, 0, nMemoryCardSize);

	bMemCardFC1Format = true;

	if (MemCardWrite(szMemoryCardFile, pCard, nMemoryCardSize, bMemCardFC1Format)) {
		free(pCard);
		return 1;
	}

	free(pCard);

	nMemoryCardStatus = 1;
	MenuEnableItems();

	return 0;
}

int MemCardCreateSlot(UINT32 slot)
{
	slot &= 3;

	if (!DriverUsesPgm2RawMemcard()) {
		return (slot == 0) ? MemCardCreate() : 1;
	}

	if (slot >= (UINT32)MemCardPgm2SlotCount() || Pgm2MemcardGetSize()) {
		return 1;
	}

	if (nPgm2MemoryCardStatus[slot] & 2) {
		if (Pgm2MemcardRuntimeEjectSlot(slot, false)) {
			return 1;
		}
	}

	TCHAR szNewPath[MAX_PATH];
	if (!Pgm2MemcardBuildNewPath(slot, szNewPath)) {
		return 1;
	}

	unsigned char* pCard = (unsigned char*)malloc(nMemoryCardSize);
	if (pCard == NULL) {
		return 1;
	}

	if (pgm2MemcardCreateImage(pCard, nMemoryCardSize) != 0) {
		memset(pCard, 0xFF, nMemoryCardSize);
	}

	if (MemCardWrite(szNewPath, pCard, nMemoryCardSize, false)) {
		free(pCard);
		return 1;
	}

	free(pCard);

	Pgm2MemcardSetSelectedPath(slot, szNewPath);
	bPgm2MemcardAutoInsert[slot] = false;
	Pgm2MemcardPersistSlot(slot);

	if (Pgm2MemcardInsertSelectedSlot(slot) == 0) {
		bPgm2MemcardAutoInsert[slot] = true;
		Pgm2MemcardPersistSlot(slot);
		return 0;
	}

	return 1;
}

int MemCardSelect()
{
	TCHAR szFilter[1024];
	TCHAR* pszTemp = szFilter;
	int nRet;

	if (DriverUsesPgm2RawMemcard()) {
		return MemCardSelectSlot(0);
	}

	static const TCHAR szAllFilter[] = _T(" (*.fc, MEMCARD.\\?\\?\\?)\0*.fc;MEMCARD.\\?\\?\\?\0");
	static const TCHAR szFcFilter[] = _T(" (*.fc)\0*.fc\0");
	static const TCHAR szMameFilter[] = _T(" (MEMCARD.\\?\\?\\?)\0MEMCARD.\\?\\?\\?\0\0");
	pszTemp += _stprintf(pszTemp, FBALoadStringEx(hAppInst, IDS_DISK_ALL_CARD, true));
	memcpy(pszTemp, szAllFilter, sizeof(szAllFilter));
	pszTemp += sizeof(szAllFilter) / sizeof(TCHAR);
	pszTemp += _stprintf(pszTemp, FBALoadStringEx(hAppInst, IDS_DISK_FILE_CARD, true), _T(APP_TITLE));
	memcpy(pszTemp, szFcFilter, sizeof(szFcFilter));
	pszTemp += sizeof(szFcFilter) / sizeof(TCHAR);
	pszTemp += _stprintf(pszTemp, FBALoadStringEx(hAppInst, IDS_DISK_FILE_CARD, true), _T("MAME"));
	memcpy(pszTemp, szMameFilter, sizeof(szMameFilter));

	MakeOfn();
	ofn.lpstrTitle = FBALoadStringEx(hAppInst, IDS_MEMCARD_SELECT, true);
	ofn.lpstrFilter = szFilter;

	int bOldPause = bRunPause;
	bRunPause = 1;
	nRet = GetOpenFileName(&ofn);
	bRunPause = bOldPause;

	if (nRet == 0) {
		return 1;
	}

	MemCardGetSize();

	if (nMemoryCardSize <= 0) {
		return 1;
	}

	nMemoryCardStatus = 1;
	bprintf(PRINT_IMPORTANT, _T("[MEMCARD] selected %s size=%d\n"), szMemoryCardFile, nMemoryCardSize);
	MenuEnableItems();

	return 0;
}

int MemCardSelectSlot(UINT32 slot)
{
	slot &= 3;

	if (!DriverUsesPgm2RawMemcard()) {
		return (slot == 0) ? MemCardSelect() : 1;
	}

	if (slot >= (UINT32)MemCardPgm2SlotCount() || Pgm2MemcardGetSize()) {
		return 1;
	}

	TCHAR szFilter[1024];
	TCHAR* pszTemp = szFilter;
	TCHAR szTitle[128];

	static const TCHAR szRawAllFilter[] = _T(" (*.pg2, *.bin, *.mem, *.fc)\0*.pg2;*.bin;*.mem;*.fc\0");
	static const TCHAR szRawFileFilter[] = _T(" (*.pg2;*.bin;*.mem;*.fc)\0*.pg2;*.bin;*.mem;*.fc\0\0");
	pszTemp += _stprintf(pszTemp, FBALoadStringEx(hAppInst, IDS_DISK_ALL_CARD, true));
	memcpy(pszTemp, szRawAllFilter, sizeof(szRawAllFilter));
	pszTemp += sizeof(szRawAllFilter) / sizeof(TCHAR);
	pszTemp += _stprintf(pszTemp, FBALoadStringEx(hAppInst, IDS_DISK_FILE_CARD, true), _T("PGM2"));
	memcpy(pszTemp, szRawFileFilter, sizeof(szRawFileFilter));

	if (szPgm2MemoryCardFile[slot][0]) {
		_tcscpy(szMemoryCardFile, szPgm2MemoryCardFile[slot]);
	} else {
		szMemoryCardFile[0] = 0;
	}

	MakeOfn();
	_stprintf(szTitle, _T("%uP %s"), slot + 1, FBALoadStringEx(hAppInst, IDS_MEMCARD_SELECT, true));
	ofn.lpstrTitle = szTitle;
	ofn.lpstrFilter = szFilter;

	int bOldPause = bRunPause;
	bRunPause = 1;
	const int nRet = GetOpenFileName(&ofn);
	bRunPause = bOldPause;

	if (nRet == 0) {
		DWORD nDlgErr = CommDlgExtendedError();
		if (nDlgErr != 0) {
			bprintf(PRINT_ERROR, _T("[PGM2][MEMCARD] slot=%u open dialog failed err=%lu\n"), slot + 1, (unsigned long)nDlgErr);
		}
		return 1;
	}

	if (nPgm2MemoryCardStatus[slot] & 2) {
		if (Pgm2MemcardRuntimeEjectSlot(slot, false)) {
			return 1;
		}
	}

	Pgm2MemcardSetSelectedPath(slot, szMemoryCardFile);
	bPgm2MemcardAutoInsert[slot] = false;
	Pgm2MemcardPersistSlot(slot);

	if (Pgm2MemcardInsertSelectedSlot(slot) == 0) {
		bPgm2MemcardAutoInsert[slot] = true;
		Pgm2MemcardPersistSlot(slot);
		bprintf(PRINT_IMPORTANT, _T("[PGM2][MEMCARD] slot=%u selected %s\n"), slot + 1, szMemoryCardFile);
		return 0;
	}

	return 1;
}

static int __cdecl MemCardDoInsert(struct BurnArea* pba)
{
	if (MemCardRead(szMemoryCardFile, (unsigned char*)pba->Data, pba->nLen, &bMemCardFC1Format)) {
		return 1;
	}

	nMemoryCardStatus |= 2;
	MenuEnableItems();

	return 0;
}

int MemCardInsert()
{
	if (DriverUsesPgm2RawMemcard()) {
		int nRet = 1;
		for (UINT32 slot = 0; slot < (UINT32)MemCardPgm2SlotCount() && slot < 4; slot++) {
			if ((nPgm2MemoryCardStatus[slot] & 1) && (nPgm2MemoryCardStatus[slot] & 2) == 0) {
				if (Pgm2MemcardInsertSelectedSlot(slot) == 0) {
					bPgm2MemcardAutoInsert[slot] = true;
					Pgm2MemcardPersistSlot(slot);
					nRet = 0;
				}
			}
		}
		return nRet;
	}

	if ((nMemoryCardStatus & 1) && (nMemoryCardStatus & 2) == 0) {
		bprintf(PRINT_IMPORTANT, _T("[MEMCARD] insert requested %s\n"), szMemoryCardFile);
		BurnAcb = MemCardDoInsert;
		BurnAreaScan(ACB_WRITE | ACB_MEMCARD | ACB_MEMCARD_ACTION, &nMinVersion);
	}

	return 0;
}

static int __cdecl MemCardDoEject(struct BurnArea* pba)
{
	if (MemCardWrite(szMemoryCardFile, (unsigned char*)pba->Data, pba->nLen, bMemCardFC1Format) == 0) {
		nMemoryCardStatus &= ~2;
		MenuEnableItems();
		return 0;
	}

	return 1;
}

int MemCardEjectSlot(UINT32 slot)
{
	slot &= 3;

	if (!DriverUsesPgm2RawMemcard()) {
		return (slot == 0) ? MemCardEject() : 1;
	}

	return Pgm2MemcardRuntimeEjectSlot(slot, true);
}

int MemCardEject()
{
	if (DriverUsesPgm2RawMemcard()) {
		int nRet = 1;
		for (UINT32 slot = 0; slot < (UINT32)MemCardPgm2SlotCount() && slot < 4; slot++) {
			if (nPgm2MemoryCardStatus[slot] & 2) {
				if (Pgm2MemcardRuntimeEjectSlot(slot, true) == 0) {
					nRet = 0;
				}
			}
		}
		return nRet;
	}

	if ((nMemoryCardStatus & 1) && (nMemoryCardStatus & 2)) {
		bprintf(PRINT_IMPORTANT, _T("[MEMCARD] eject requested %s\n"), szMemoryCardFile);
		BurnAcb = MemCardDoEject;
		nMinVersion = 0;
		BurnAreaScan(ACB_READ | ACB_MEMCARD | ACB_MEMCARD_ACTION, &nMinVersion);
	}

	return 0;
}

int MemCardToggle()
{
	if (DriverUsesPgm2RawMemcard()) {
		for (UINT32 slot = 0; slot < (UINT32)MemCardPgm2SlotCount() && slot < 4; slot++) {
			if (nPgm2MemoryCardStatus[slot] & 2) {
				return MemCardEject();
			}
		}
		return MemCardInsert();
	}

	if (nMemoryCardStatus & 1) {
		if (nMemoryCardStatus & 2) {
			return MemCardEject();
		} else {
			return MemCardInsert();
		}
	}

	return 1;
}

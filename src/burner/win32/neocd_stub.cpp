#include "burner.h"
#include "neocdlist.h"

static TCHAR szNeoCdStubText[] = _T("");

bool bNeoCDListScanSub = false;
bool bNeoCDListScanOnlyISO = false;
TCHAR szNeoCDCoverDir[MAX_PATH] = _T("support/neocdz/");
TCHAR szNeoCDPreviewDir[MAX_PATH] = _T("support/neocdzpreviews/");
TCHAR szNeoCDGamesDir[MAX_PATH] = _T("neocdiso/");

NGCDGAME* GetNeoGeoCDInfo(unsigned int)
{
	return NULL;
}

int GetNeoCDTitle(unsigned int)
{
	return 1;
}

int GetNeoGeoCD_Identifier()
{
	return 0;
}

int NeoCDList_Init()
{
	return 0;
}

bool IsNeoGeoCD()
{
	return false;
}

int NeoCDInfo_Init()
{
	return 0;
}

TCHAR* NeoCDInfo_Text(int)
{
	return szNeoCdStubText;
}

int NeoCDInfo_ID()
{
	return 0;
}

void NeoCDInfo_SetTitle()
{
}

void NeoCDInfo_Exit()
{
}

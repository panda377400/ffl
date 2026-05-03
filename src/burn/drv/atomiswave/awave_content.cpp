#include "awave_content.h"
#include <cstring>

static INT32 NormalizeAwaveBurnRomIndex(const NaomiGameConfig* config, INT32 index)
{
	/*
	 * d_awave.cpp currently stores Atomiswave game ROM indexes as if the
	 * three awbios ROMs were prepended to each game set.  BurnDrvGetRomInfo()
	 * and BurnLoadRom() address only the active game's BurnRomDesc, so convert
	 * those merged indexes back to local game indexes here.
	 */
	if (config != NULL && config->platform == AWAVE_PLATFORM_ATOMISWAVE && index >= 3) {
		return index - 3;
	}

	return index;
}

static INT32 LoadBurnRomToVector(INT32 index, std::vector<UINT8>& out)
{
	struct BurnRomInfo ri;
	memset(&ri, 0, sizeof(ri));

	if (BurnDrvGetRomInfo(&ri, index)) {
		return 1;
	}

	if (ri.nLen <= 0) {
		return 1;
	}

	out.resize(ri.nLen);

	if (BurnLoadRom(out.data(), index, 1)) {
		out.clear();
		return 1;
	}

	return 0;
}

INT32 AwaveBuildContentPackage(const NaomiGameConfig* config, AwaveContentPackage& outPackage)
{
	AwaveFreeContentPackage(outPackage);

	if (config == NULL || config->zipEntries == NULL) {
		return 1;
	}

	outPackage.driverName = config->driverName;
	outPackage.zipName = config->zipName;
	outPackage.biosZipName = config->biosZipName;

	for (INT32 i = 0; config->zipEntries[i].filename != NULL; i++) {
		AwaveContentEntry entry;
		entry.filename = config->zipEntries[i].filename;

		const INT32 originalIndex = config->zipEntries[i].burnRomIndex;
		const INT32 burnIndex = NormalizeAwaveBurnRomIndex(config, originalIndex);

		if (LoadBurnRomToVector(burnIndex, entry.data)) {
			bprintf(0, _T("atomiswave: failed to load ROM index %d normalized from %d for %S\n"), burnIndex, originalIndex, entry.filename);
			AwaveFreeContentPackage(outPackage);
			return 1;
		}

		outPackage.entries.push_back(entry);
	}

	return outPackage.entries.empty() ? 1 : 0;
}

void AwaveFreeContentPackage(AwaveContentPackage& package)
{
	package.driverName = NULL;
	package.zipName = NULL;
	package.biosZipName = NULL;
	package.entries.clear();
}

#include "awave_content.h"
#include <cstring>

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

		// Important: Atomiswave/NAOMI BurnRomDesc entries include BIOS rows first.
		// The burnRomIndex stored in d_awave.cpp/d_naomi.cpp is already the real
		// BurnLoadRom() index. Do not normalize 3->0 here, or game ROM names will
		// be paired with BIOS bytes and Flycast will reject the package.
		if (LoadBurnRomToVector(config->zipEntries[i].burnRomIndex, entry.data)) {
			bprintf(0, _T("atomiswave: failed to load ROM index %d for %S\n"), config->zipEntries[i].burnRomIndex, entry.filename);
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

#include "awave_content.h"

#include <cstring>

static INT32 LoadBurnRomToVector(INT32 index, std::vector<UINT8>& out)
{
    struct BurnRomInfo ri;
    char* romName = NULL;

    std::memset(&ri, 0, sizeof(ri));

    INT32 nameRet = BurnDrvGetRomName(&romName, index, 0);
    INT32 infoRet = BurnDrvGetRomInfo(&ri, index);

    bprintf(0, _T("[AW] ROM index=%d name=%S nameRet=%d infoRet=%d len=%08x type=%08x\n"),
        index,
        romName ? romName : "(null)",
        nameRet,
        infoRet,
        ri.nLen,
        ri.nType
    );

    if (infoRet || ri.nLen <= 0) {
        return 1;
    }

    out.resize(ri.nLen);

    bprintf(0, _T("[AW] before BurnLoadRom index=%d name=%S\n"),
        index,
        romName ? romName : "(null)"
    );

    if (BurnLoadRom(out.data(), index, 1)) {
        bprintf(0, _T("[AW] BurnLoadRom failed index=%d name=%S\n"),
            index,
            romName ? romName : "(null)"
        );
        out.clear();
        return 1;
    }

    bprintf(0, _T("[AW] after BurnLoadRom index=%d size=%08x\n"), index, ri.nLen);
    return 0;
}

static INT32 CountBurnRoms()
{
    INT32 count = 0;

    for (;;) {
        struct BurnRomInfo ri;
        char* romName = NULL;

        std::memset(&ri, 0, sizeof(ri));

        if (BurnDrvGetRomName(&romName, count, 0)) {
            break;
        }

        if (BurnDrvGetRomInfo(&ri, count)) {
            break;
        }

        if (romName == NULL || ri.nLen <= 0) {
            break;
        }

        count++;
    }

    return count;
}

static void DumpBurnRomTable()
{
    bprintf(0, _T("[AW] Burn ROM table begin\n"));

    for (INT32 i = 0; ; i++) {
        struct BurnRomInfo ri;
        char* romName = NULL;

        std::memset(&ri, 0, sizeof(ri));

        if (BurnDrvGetRomName(&romName, i, 0)) {
            break;
        }

        if (BurnDrvGetRomInfo(&ri, i)) {
            break;
        }

        bprintf(0, _T("[AW] rom[%d] name=%S len=%08x type=%08x\n"),
            i,
            romName ? romName : "(null)",
            ri.nLen,
            ri.nType
        );
    }

    bprintf(0, _T("[AW] Burn ROM table end\n"));
}

INT32 AwaveBuildContentPackage(const NaomiGameConfig* config, AwaveContentPackage& outPackage)
{
    AwaveFreeContentPackage(outPackage);

    if (config == NULL) {
        return 1;
    }

    outPackage.driverName = config->driverName;
    outPackage.zipName = config->zipName;
    outPackage.biosZipName = config->biosZipName;

    bprintf(0, _T("[AW] AwaveBuildContentPackage auto driver=%S zip=%S bios=%S\n"),
        config->driverName ? config->driverName : "(null)",
        config->zipName ? config->zipName : "(null)",
        config->biosZipName ? config->biosZipName : "(null)"
    );

    DumpBurnRomTable();

    const INT32 romCount = CountBurnRoms();
    if (romCount <= 0) {
        bprintf(0, _T("atomiswave: Burn ROM table is empty.\n"));
        return 1;
    }

    outPackage.entries.reserve(romCount);

    for (INT32 index = 0; index < romCount; index++) {
        struct BurnRomInfo ri;
        char* romName = NULL;

        std::memset(&ri, 0, sizeof(ri));

        if (BurnDrvGetRomName(&romName, index, 0) || BurnDrvGetRomInfo(&ri, index)) {
            bprintf(0, _T("atomiswave: unable to query ROM index %d.\n"), index);
            AwaveFreeContentPackage(outPackage);
            return 1;
        }

        if (romName == NULL || ri.nLen <= 0) {
            bprintf(0, _T("atomiswave: invalid ROM entry index %d.\n"), index);
            AwaveFreeContentPackage(outPackage);
            return 1;
        }

        AwaveContentEntry entry;
        entry.filename = romName;

        if (LoadBurnRomToVector(index, entry.data)) {
            bprintf(0, _T("atomiswave: failed to auto-load ROM index %d name=%S\n"),
                index,
                romName ? romName : "(null)"
            );
            AwaveFreeContentPackage(outPackage);
            return 1;
        }

        outPackage.entries.push_back(entry);
    }

    bprintf(0, _T("[AW] AwaveBuildContentPackage auto done entries=%u\n"),
        (unsigned)outPackage.entries.size()
    );

    return outPackage.entries.empty() ? 1 : 0;
}

void AwaveFreeContentPackage(AwaveContentPackage& package)
{
    package.driverName = NULL;
    package.zipName = NULL;
    package.biosZipName = NULL;
    package.entries.clear();
}

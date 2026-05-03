#pragma once
#include "burnint.h"
#include "awave_core.h"
#include <vector>

struct AwaveContentEntry {
    const char* filename;
    std::vector<UINT8> data;
};

struct AwaveContentPackage {
    const char* driverName;
    const char* zipName;
    const char* biosZipName;
    std::vector<AwaveContentEntry> entries;
};

INT32 AwaveBuildContentPackage(const NaomiGameConfig* config, AwaveContentPackage& outPackage);
void AwaveFreeContentPackage(AwaveContentPackage& package);

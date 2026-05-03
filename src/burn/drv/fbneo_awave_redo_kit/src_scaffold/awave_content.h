#pragma once

#include "awave_driverdb.h"
#include <vector>

struct AwaveContentResult {
    const char* path;
    bool usesTemporaryFile;
};

bool AwaveContentPrepare(const AwaveGameMeta* game, AwaveContentResult* out);
void AwaveContentCleanup();

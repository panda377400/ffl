#pragma once

#include "awave_driverdb.h"
#include <libretro.h>

struct AwaveHostConfig {
    const AwaveGameMeta* game;
    const char* systemDir;
    const char* saveDir;
    const char* contentPath;
    bool directTexturePreferred;
};

void AwaveHostInit(const AwaveHostConfig* cfg);
void AwaveHostShutdown();
bool AwaveHostEnvironment(unsigned cmd, void* data);
const char* AwaveHostGetOption(const char* key);

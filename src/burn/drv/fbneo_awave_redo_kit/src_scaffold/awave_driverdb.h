#pragma once

#include "burnint.h"

enum AwaveBoardClass {
    AWAVE_BOARD_ATOMISWAVE = 0,
    AWAVE_BOARD_NAOMI = 1,
    AWAVE_BOARD_NAOMI2 = 2,
    AWAVE_BOARD_DREAMCAST = 3,
};

enum AwaveContentType {
    AWAVE_CONTENT_ARCADE_ZIP = 0,
    AWAVE_CONTENT_DIRECT_ARCHIVE = 1,
    AWAVE_CONTENT_EXTERNAL_DISC = 2,
};

enum AwaveInputProfile {
    AWAVE_INPUT_PROFILE_AW_DIGITAL = 0,
    AWAVE_INPUT_PROFILE_AW_LIGHTGUN,
    AWAVE_INPUT_PROFILE_AW_RACING,
    AWAVE_INPUT_PROFILE_NAOMI_DIGITAL,
    AWAVE_INPUT_PROFILE_NAOMI_LIGHTGUN,
    AWAVE_INPUT_PROFILE_NAOMI_RACING,
    AWAVE_INPUT_PROFILE_DREAMCAST_PAD,
};

struct AwaveRomEntry {
    const char* flycastName;
    INT32 burnRomIndex;
};

struct AwaveGameMeta {
    const char* driverName;
    const char* parentName;
    const char* biosZip;
    const char* runtimeSubdir;
    AwaveBoardClass board;
    AwaveContentType contentType;
    AwaveInputProfile inputProfile;
    const AwaveRomEntry* roms;
    UINT32 romCount;
    const char* externalContentPath;
};

const AwaveGameMeta* AwaveFindGameMeta(const char* driverName);
const char* AwaveBoardName(AwaveBoardClass board);
const char* AwaveSystemOption(AwaveBoardClass board);

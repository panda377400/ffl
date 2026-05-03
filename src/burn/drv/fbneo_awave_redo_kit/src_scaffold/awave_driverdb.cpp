#include "awave_driverdb.h"
#include <cstring>

const char* AwaveBoardName(AwaveBoardClass board)
{
    switch (board) {
        case AWAVE_BOARD_DREAMCAST: return "dreamcast";
        case AWAVE_BOARD_NAOMI2: return "naomi2";
        case AWAVE_BOARD_NAOMI: return "naomi";
        case AWAVE_BOARD_ATOMISWAVE:
        default: return "atomiswave";
    }
}

const char* AwaveSystemOption(AwaveBoardClass board)
{
    return AwaveBoardName(board);
}

// Move generated game metadata here in Phase 1.
static const AwaveGameMeta kGames[] = {
    // Example only:
    // { "kofxi", NULL, "awbios", "atomiswave", AWAVE_BOARD_ATOMISWAVE,
    //   AWAVE_CONTENT_DIRECT_ARCHIVE, AWAVE_INPUT_PROFILE_AW_DIGITAL,
    //   KofxiRoms, KofxiRomCount, NULL },
};

const AwaveGameMeta* AwaveFindGameMeta(const char* driverName)
{
    if (driverName == NULL) return NULL;
    for (UINT32 i = 0; i < sizeof(kGames) / sizeof(kGames[0]); i++) {
        if (kGames[i].driverName && strcmp(kGames[i].driverName, driverName) == 0) {
            return &kGames[i];
        }
    }
    return NULL;
}

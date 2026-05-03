#pragma once

#include "burnint.h"

// Public hardware IDs for the clean Atomiswave/NAOMI bridge.
// Upstream FBNeo does not currently define public NAOMI/Atomiswave hardware IDs,
// so keep these local fallbacks. If you also patch src/burn/burn.h with the same
// names, these guards prevent redefinition.
#ifndef HARDWARE_SAMMY_ATOMISWAVE
#define HARDWARE_SAMMY_ATOMISWAVE (HARDWARE_PREFIX_MISC_POST90S | 0x00010000)
#endif

#ifndef HARDWARE_SEGA_NAOMI
#define HARDWARE_SEGA_NAOMI (HARDWARE_PREFIX_SEGA | 0x000d0000)
#endif

#ifndef HARDWARE_SEGA_NAOMI2
#define HARDWARE_SEGA_NAOMI2 (HARDWARE_PREFIX_SEGA | 0x000e0000)
#endif

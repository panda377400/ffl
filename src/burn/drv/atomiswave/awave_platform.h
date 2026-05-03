// Stable hardware metadata for FBNeo filters.
// Keep these values inside existing public prefixes so old frontends do not drop them.
#pragma once

#ifndef HARDWARE_SAMMY_ATOMISWAVE
#define HARDWARE_SAMMY_ATOMISWAVE (HARDWARE_PREFIX_MISC_POST90S | 0x00010000)
#endif

#ifndef HARDWARE_SEGA_NAOMI
#define HARDWARE_SEGA_NAOMI       (HARDWARE_PREFIX_SEGA | 0x000d0000)
#endif

#ifndef HARDWARE_SEGA_NAOMI2
#define HARDWARE_SEGA_NAOMI2      (HARDWARE_PREFIX_SEGA | 0x000e0000)
#endif

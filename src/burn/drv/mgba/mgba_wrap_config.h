#ifndef FBNEO_MGBA_WRAP_CONFIG_H
#define FBNEO_MGBA_WRAP_CONFIG_H

#ifndef BUILD_STATIC
#define BUILD_STATIC
#endif

#ifndef DISABLE_THREADING
#define DISABLE_THREADING
#endif

#ifndef MGBA_STANDALONE
#define MGBA_STANDALONE
#endif

#ifndef MINIMAL_CORE
#define MINIMAL_CORE 2
#endif

#ifndef M_CORE_GBA
#define M_CORE_GBA
#endif

#ifndef M_CORE_GB
#define M_CORE_GB
#endif

#ifndef COLOR_16_BIT
#define COLOR_16_BIT
#endif

#ifndef COLOR_5_6_5
#define COLOR_5_6_5
#endif

#ifndef HAVE_CRC32
#define HAVE_CRC32 1
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _In_opt_
#define _In_opt_
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef _Out_opt_
#define _Out_opt_
#endif

#ifndef _Inout_
#define _Inout_
#endif

#ifndef _Inout_opt_
#define _Inout_opt_
#endif

#include "generated/include/mgba/flags.h"

#endif

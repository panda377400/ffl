# V4.1 compile fix

This package fixes the V4 build failure where `awave_core.cpp` included `awave_wrap_config.h` before `burnint.h`.

`awave_wrap_config.h` defines Flycast/libretro wrapper macros such as `__LIBRETRO__`. When it is included before FBNeo internal headers, `burnint.h` can enter the wrong conditional path and reference `kNetGame` without a declaration.

V4.1 changes the include order to:

```cpp
#include "burnint.h"
#include "awave_wrap_config.h"
#include "awave_core.h"
```

It also stops defining `ARRAY_SIZE` in the wrapper config to avoid conflicts with libretro-common.

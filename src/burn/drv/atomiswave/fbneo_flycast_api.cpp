// FBNeo makefile compatibility wrapper.
//
// makefile.burn_rules lists object names, and the MinGW makefile searches
// the normal atomiswave driver directory for fbneo_flycast_api.cpp.  The real
// implementation lives in flycast_shim/ to keep the shim self-contained, so
// this wrapper makes the existing fbneo_flycast_api.o target resolvable without
// changing makefile.mingw/makefile.burn_rules.

#include "flycast_shim/fbneo_flycast_api.cpp"

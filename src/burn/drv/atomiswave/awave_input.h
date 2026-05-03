#pragma once
#include "burnint.h"
#include "awave_core.h"

void AwaveInputResetRuntime();
void AwaveInputSetPad(INT32 port, UINT32 state);
void AwaveInputSetAnalog(INT32 port, INT32 axis, INT16 value);
void AwaveInputSetAnalogButton(INT32 port, INT32 button, INT16 value);
void AwaveInputSetLightgun(INT32 port, INT16 x, INT16 y, UINT8 offscreen, UINT8 reload);
UINT32 AwaveInputGetPad(INT32 port);
INT16 AwaveInputGetAnalog(INT32 port, INT32 axis);
INT16 AwaveInputGetAnalogButton(INT32 port, INT32 button);
void AwaveInputGetLightgun(INT32 port, INT16* x, INT16* y, UINT8* offscreen, UINT8* reload);

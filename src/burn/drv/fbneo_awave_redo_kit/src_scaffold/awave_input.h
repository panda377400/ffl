#pragma once

#include "awave_driverdb.h"
#include "burnint.h"

void AwaveInputReset(AwaveInputProfile profile);
void AwaveInputSetDigital(INT32 port, UINT32 mask);
void AwaveInputSetAnalog(INT32 port, INT32 axis, INT16 value);
void AwaveInputSetTrigger(INT32 port, INT32 trigger, INT16 value);
void AwaveInputSetLightgun(INT32 port, INT16 x, INT16 y, UINT8 offscreen, UINT8 reload);
int16_t AwaveInputRetroState(unsigned port, unsigned device, unsigned index, unsigned id);

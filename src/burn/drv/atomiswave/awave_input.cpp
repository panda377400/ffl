#include "awave_input.h"
#include <cstring>

static UINT32 g_pad[4];
static INT16 g_analog[4][8];
static INT16 g_analogButton[4][4];
static INT16 g_gunX[4];
static INT16 g_gunY[4];
static UINT8 g_gunOffscreen[4];
static UINT8 g_gunReload[4];

void AwaveInputResetRuntime()
{
    memset(g_pad, 0, sizeof(g_pad));
    memset(g_analog, 0, sizeof(g_analog));
    memset(g_analogButton, 0, sizeof(g_analogButton));
    memset(g_gunX, 0, sizeof(g_gunX));
    memset(g_gunY, 0, sizeof(g_gunY));
    memset(g_gunOffscreen, 0, sizeof(g_gunOffscreen));
    memset(g_gunReload, 0, sizeof(g_gunReload));
}

void AwaveInputSetPad(INT32 port, UINT32 state)
{
    if (port < 0 || port >= 4) return;
    g_pad[port] = state;
}

void AwaveInputSetAnalog(INT32 port, INT32 axis, INT16 value)
{
    if (port < 0 || port >= 4 || axis < 0 || axis >= 8) return;
    g_analog[port][axis] = value;
}

void AwaveInputSetAnalogButton(INT32 port, INT32 button, INT16 value)
{
    if (port < 0 || port >= 4 || button < 0 || button >= 4) return;
    g_analogButton[port][button] = value;
}

void AwaveInputSetLightgun(INT32 port, INT16 x, INT16 y, UINT8 offscreen, UINT8 reload)
{
    if (port < 0 || port >= 4) return;
    g_gunX[port] = x;
    g_gunY[port] = y;
    g_gunOffscreen[port] = offscreen;
    g_gunReload[port] = reload;
}

UINT32 AwaveInputGetPad(INT32 port)
{
    if (port < 0 || port >= 4) return 0;
    return g_pad[port];
}

INT16 AwaveInputGetAnalog(INT32 port, INT32 axis)
{
    if (port < 0 || port >= 4 || axis < 0 || axis >= 8) return 0;
    return g_analog[port][axis];
}

INT16 AwaveInputGetAnalogButton(INT32 port, INT32 button)
{
    if (port < 0 || port >= 4 || button < 0 || button >= 4) return 0;
    return g_analogButton[port][button];
}

void AwaveInputGetLightgun(INT32 port, INT16* x, INT16* y, UINT8* offscreen, UINT8* reload)
{
    if (x) *x = 0;
    if (y) *y = 0;
    if (offscreen) *offscreen = 0;
    if (reload) *reload = 0;
    if (port < 0 || port >= 4) return;
    if (x) *x = g_gunX[port];
    if (y) *y = g_gunY[port];
    if (offscreen) *offscreen = g_gunOffscreen[port];
    if (reload) *reload = g_gunReload[port];
}

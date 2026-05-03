#pragma once

#include "burnint.h"

struct AwaveVideoFrame {
    UINT32 texture;
    UINT32 framebuffer;
    INT32 width;
    INT32 height;
    INT32 upsideDown;
    bool valid;
};

void AwaveVideoReset();
bool AwaveVideoBeginFrame();
void AwaveVideoEndFrame();
bool AwaveVideoGetDirectFrame(AwaveVideoFrame* out);
bool AwaveVideoReadbackToBurn(INT32 nBurnBpp);

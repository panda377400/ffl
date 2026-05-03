#pragma once
#include "burnint.h"
#include <vector>

void AwaveAudioInit(INT32 sampleRate, INT32 framesOfLatency);
void AwaveAudioExit();
void AwaveAudioReset();
void AwaveAudioPush(const INT16* stereo, INT32 frames);
void AwaveAudioRender(INT16* out, INT32 frames);
INT32 AwaveAudioBufferedFrames();

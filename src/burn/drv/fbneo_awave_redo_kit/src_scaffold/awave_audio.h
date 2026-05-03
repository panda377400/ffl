#pragma once

#include "burnint.h"

void AwaveAudioReset(INT32 sourceRate);
void AwaveAudioPush(const INT16* samples, size_t frames);
void AwaveAudioDrainToBurn(INT16* out, INT32 frames, INT32 sinkRate);
void AwaveAudioSetTargetLatencyFrames(INT32 frames);

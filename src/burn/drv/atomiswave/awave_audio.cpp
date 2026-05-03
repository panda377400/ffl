#include "awave_audio.h"
#include <algorithm>
#include <cstring>

static std::vector<INT16> g_fifo;
static INT32 g_read = 0;
static INT32 g_write = 0;
static INT32 g_countFrames = 0;
static INT16 g_last[2] = {0, 0};

void AwaveAudioInit(INT32 sampleRate, INT32 framesOfLatency)
{
    if (sampleRate <= 0) sampleRate = 44100;
    if (framesOfLatency < 2) framesOfLatency = 2;
    const INT32 capacityFrames = sampleRate * framesOfLatency / 30;
    g_fifo.assign(capacityFrames * 2, 0);
    g_read = g_write = g_countFrames = 0;
    g_last[0] = g_last[1] = 0;
}

void AwaveAudioExit()
{
    g_fifo.clear();
    g_read = g_write = g_countFrames = 0;
}

void AwaveAudioReset()
{
    std::fill(g_fifo.begin(), g_fifo.end(), 0);
    g_read = g_write = g_countFrames = 0;
    g_last[0] = g_last[1] = 0;
}

void AwaveAudioPush(const INT16* stereo, INT32 frames)
{
    if (stereo == NULL || frames <= 0 || g_fifo.empty()) return;
    const INT32 cap = (INT32)g_fifo.size() / 2;
    for (INT32 i = 0; i < frames; i++) {
        if (g_countFrames == cap) {
            g_read = (g_read + 1) % cap;
            g_countFrames--;
        }
        g_fifo[g_write * 2 + 0] = stereo[i * 2 + 0];
        g_fifo[g_write * 2 + 1] = stereo[i * 2 + 1];
        g_last[0] = stereo[i * 2 + 0];
        g_last[1] = stereo[i * 2 + 1];
        g_write = (g_write + 1) % cap;
        g_countFrames++;
    }
}

void AwaveAudioRender(INT16* out, INT32 frames)
{
    if (out == NULL || frames <= 0) return;
    if (g_fifo.empty()) {
        memset(out, 0, frames * 2 * sizeof(INT16));
        return;
    }
    const INT32 cap = (INT32)g_fifo.size() / 2;
    for (INT32 i = 0; i < frames; i++) {
        if (g_countFrames > 0) {
            out[i * 2 + 0] = g_fifo[g_read * 2 + 0];
            out[i * 2 + 1] = g_fifo[g_read * 2 + 1];
            g_last[0] = out[i * 2 + 0];
            g_last[1] = out[i * 2 + 1];
            g_read = (g_read + 1) % cap;
            g_countFrames--;
        } else {
            out[i * 2 + 0] = g_last[0];
            out[i * 2 + 1] = g_last[1];
        }
    }
}

INT32 AwaveAudioBufferedFrames()
{
    return g_countFrames;
}

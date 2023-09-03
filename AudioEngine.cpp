
#include "AudioEngine.h"
#include <cmath>
#include <atomic>
#include <algorithm>
#include <spdlog/spdlog.h>

const double SAMPLERATE = 44100.0;

float vol = 0.5;
float vol_step = 0;
float frequency = 400.0;
float target_vol = vol;

bool vol_changing = false;
void change_volume(float amount, float period) {
    vol_step = amount / (SAMPLERATE*period/1000); //(/ms)
    target_vol = std::clamp(vol + amount, 0.0f, 1.0f);
    vol_changing = true;
    spdlog::debug("target vol = {:.2f} | step = {:.2f} ", target_vol, vol_step*1000);
}

void update_volume() {
    if (vol_changing) {
        // Compute the new volume
        float new_vol = vol + vol_step;

        // Check if the new volume crosses or reaches the target volume
        if ((vol_step > 0 && new_vol >= target_vol) || (vol_step < 0 && new_vol <= target_vol)) {
            vol = target_vol;
            vol_changing = false;
            spdlog::debug("vol == target vol");
        } else {
            vol = new_vol;
        }
    }
}

int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData)
{
    float *out = (float *)outputBuffer;
    double phaseIncrement = 2.0 * M_PI * frequency / SAMPLERATE;
    static double phase = 0.0;

    for (unsigned long i = 0; i < framesPerBuffer; ++i)
    {
        update_volume();
        float sample = (float)(sin(phase));
        *out++ = sample * vol; // Left channel
        *out++ = sample * vol; // Right channel
        phase += phaseIncrement;
        phase = std::fmod(phase, 2.0 * M_PI);
    }
    return paContinue;
}



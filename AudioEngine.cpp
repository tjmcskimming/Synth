
#include "AudioEngine.h"
#include <cmath>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>

const double SAMPLERATE = 44100.0;

float vol = 0.5;
float vol_step = 0;
std::vector<float> freqs_to_add = {}; // Initial freqs
std::vector<float> freqs_to_rem = {};
std::vector<float> freqs = {}; // Initial freqs
std::vector<float> phases = {}; // Phases for each frequency

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

std::string vec_to_string(const std::vector<float>& vec) {
    std::string result;
    result.reserve(vec.size() * 6);
    for (const auto& elem : vec) {
        result +=  fmt::format("{:3.1f} ", elem);
    }
    return result;
}
void frequency_on(float freq) {
    spdlog::debug("frequency on {:.2f}", freq);
    freqs_to_add.push_back(freq);
}

void frequency_off(float freq) {
    spdlog::debug("frequency off {:.2f}", freq);
    freqs_to_rem.push_back(freq);
}

void update_freqs() {
    for (unsigned short i = 0; i < freqs_to_add.size(); ++i){
        auto it = std::find(
                freqs.begin(), freqs.end(), freqs_to_add[i]);
        freqs_to_add.pop_back();
        if (it == freqs.end()) {
            spdlog::debug("adding {:.2f}", freqs_to_add[i]);
            freqs.push_back(freqs_to_add[i]);
            phases.push_back(0);
            spdlog::debug(vec_to_string(freqs));
        }
    }
    for (unsigned short i = 0; i < freqs_to_rem.size(); ++i){
        auto it = std::find(
                freqs.begin(), freqs.end(), freqs_to_rem[i]);
        freqs_to_rem.pop_back();
        if (it != freqs.end()) {
            size_t index = std::distance(freqs.begin(), it);
            freqs.erase(it);
            phases.erase(phases.begin() + index);
            spdlog::debug(vec_to_string(freqs));
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

    update_volume();
    update_freqs();

    for (unsigned long i = 0; i < framesPerBuffer; ++i)
    {
        float sample = 0.0;
        for (size_t j = 0; j < freqs.size(); ++j)
        {
            double phaseIncrement = 2.0 * M_PI * freqs[j] / SAMPLERATE;
            sample += (float)sin(phases[j]);
            phases[j] += phaseIncrement;
            phases[j] = std::fmod(phases[j], 2.0 * M_PI);
        }

        *out++ = sample * vol; // Left channel
        *out++ = sample * vol; // Right channel
    }

    return paContinue;
}



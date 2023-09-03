
#include "AudioEngine.h"
#include <cmath>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>
#include <map>
#include <array>

struct Sound {
    float frequency;
    float phase;
    float vol;
};

const double SAMPLERATE = 44100.0;

std::array<float, 256> key_freq_map = {};
std::vector<Note> note_on_buffer = {};
std::vector<Note> note_off_buffer = {};
std::array<std::optional<Sound>, 256> playing_notes;

void initialize_key_freq_map(std::array<float, 256> map) {
    key_freq_map = map;
}

float gain = 0.5;
float target_gain = gain;
float gain_step = 0;
bool gain_changing = false;

std::string note_buffer_to_string(std::vector<Note> note_buffer){
    std::string nb_string = "";
    for (int i = 0; i < note_buffer.size(); ++i) {
        nb_string = fmt::format("{} {}", nb_string, note_buffer[i].key);
    }
    return nb_string;
}

void note_on(Note note) {
    spdlog::debug("note on key:{} vel:{:.2f}", note.key, note.velocity);
    if (note.key >= 0 && note.key < 256) {
        note_on_buffer.push_back(note);
        spdlog::debug("+ {}", note_buffer_to_string(note_on_buffer));
        spdlog::debug("- {}", note_buffer_to_string(note_off_buffer));
    }
}

void note_off(Note note) {
    spdlog::debug("note off key:{}", note.key, note.velocity);
    if (note.key >= 0 && note.key < 256) {
        note_off_buffer.push_back(note);
        spdlog::debug("+ {}", note_buffer_to_string(note_on_buffer));
        spdlog::debug("- {}", note_buffer_to_string(note_off_buffer));
    }
}

void update_sounds() {
    unsigned short on_buffer_size = note_on_buffer.size();
    unsigned short off_buffer_size = note_off_buffer.size();

    for (unsigned short i = 0; i < on_buffer_size; ++i) {
        if (!playing_notes[note_on_buffer[i].key].has_value()){
            playing_notes[note_on_buffer[i].key] = {
                    .frequency = key_freq_map[note_on_buffer[i].key],
                    .vol = note_on_buffer[i].velocity / 255,
                    .phase = 0
            };
        }
    }

    for (unsigned short i = 0; i < off_buffer_size; ++i) {
        playing_notes[note_off_buffer[i].key] = {};
    }

    // Remove the frequencies that were processed
    note_on_buffer.erase(note_on_buffer.begin(), note_on_buffer.begin() + on_buffer_size);
    note_off_buffer.erase(note_off_buffer.begin(), note_off_buffer.begin() + off_buffer_size);
}

void change_volume(float amount, float period) {
    gain_step = amount / (SAMPLERATE * period / 1000); //(1/ms)
    target_gain = std::clamp(gain + amount, 0.0f, 1.0f);
    gain_changing = true;
    spdlog::debug("target gain = {:.2f} | step = {:.2f} ", target_gain, gain_step * 1000);
}

void update_volume() {
    if (gain_changing) {
        float new_vol = gain + gain_step;

        if ((gain_step > 0 && new_vol >= target_gain) || (gain_step < 0 && new_vol <= target_gain)) {
            gain = target_gain;
            gain_changing = false;
            spdlog::debug("gain == target gain");
        } else {
            gain = new_vol;
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
    update_sounds();

    for (unsigned long i = 0; i < framesPerBuffer; ++i)
    {
        float sample = 0.0;
        for (size_t j = 0; j < playing_notes.size(); ++j)
        {
            double phaseIncrement = 2.0 * M_PI * playing_notes[j]->frequency / SAMPLERATE;
            sample += playing_notes[j]->vol *(float)sin(playing_notes[j]->phase);
            playing_notes[j]->phase += phaseIncrement;
            playing_notes[j]->phase = std::fmod(playing_notes[j]->phase, 2.0 * M_PI);
        }

        *out++ = sample * gain; // Left channel
        *out++ = sample * gain; // Right channel
    }

    return paContinue;
}



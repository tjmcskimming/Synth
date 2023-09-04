#include "AudioEngine.h"
#include <cmath>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>
#include <map>
#include <array>
#include <algorithm>

enum EnvelopeStage {
    ATTACK, DECAY, SUSTAIN, RELEASE
};

struct Sound {
    float frequency;
    float phase;
    float max_vol;
    float vol;
    float vol_step;
    bool note_on;
    EnvelopeStage stage;
};


const float SAMPLERATE = 44100.0;

std::array<float, 256> key_freq_map = {};
std::vector<Note> note_on_buffer = {};
std::vector<Note> note_off_buffer = {};
std::array<std::optional<Sound>, 256> playing_sounds;
Envelope envelope = {.attack=0, .decay=0,.sustain=0,.release=0};

void initialize_key_freq_map(std::array<float, 256> map) {
    key_freq_map = map;
}

void set_envelope(Envelope env) {
    envelope = env;
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


void change_volume(float amount, float period) {
    gain_step = amount / (SAMPLERATE * period / 1000); //(1/ms)
    target_gain = std::clamp(gain + amount, 0.0f, 1.0f);
    gain_changing = true;
    //spdlog::debug("target gain = {:.2f} | step = {:.2f} ", target_gain, gain_step * 1000);
}

void update_volume() {
    if (gain_changing) {
        float new_vol = gain + gain_step;

        if ((gain_step > 0 && new_vol >= target_gain) || (gain_step < 0 && new_vol <= target_gain)) {
            gain = target_gain;
            gain_changing = false;
            //spdlog::debug("gain == target gain");
        } else {
            gain = new_vol;
        }
    }
}

void note_on(Note note) {
    //spdlog::debug("note on key:{} vel:{:.2f}", note.key, note.velocity);
    if (note.key >= 0 && note.key < 256) {
        note_on_buffer.push_back(note);
        //spdlog::debug("+ {}", note_buffer_to_string(note_on_buffer));
        //spdlog::debug("- {}", note_buffer_to_string(note_off_buffer));
    }
}

void note_off(Note note) {
    //spdlog::debug("note off key:{}", note.key, note.velocity);
    if (note.key >= 0 && note.key < 256) {
        note_off_buffer.push_back(note);
        //spdlog::debug("+ {}", note_buffer_to_string(note_on_buffer));
        //spdlog::debug("- {}", note_buffer_to_string(note_off_buffer));
    }
}

void update_sounds() {
    unsigned short on_buffer_size = note_on_buffer.size();
    unsigned short off_buffer_size = note_off_buffer.size();

    for (unsigned short i = 0; i < on_buffer_size; ++i) {
        if (!playing_sounds[note_on_buffer[i].key].has_value()) {
            float max_vol = note_on_buffer[i].velocity / 255;
            Sound sound = {
                    .frequency = key_freq_map[note_on_buffer[i].key],
                    .vol = 0,
                    .phase = 0,
                    .max_vol = max_vol,
                    .vol_step = max_vol / ((SAMPLERATE / 1000) * envelope.decay),
                    .note_on = true,
                    .stage = ATTACK
            };
            playing_sounds[note_on_buffer[i].key] = sound;
            //spdlog::debug("enter ATTACK, vol_step {} ", sound.vol_step);
        } else if (!playing_sounds[note_on_buffer[i].key].value().note_on) {
            Sound& sound = playing_sounds[note_on_buffer[i].key].value();
            float max_vol = note_on_buffer[i].velocity / 255;
            // leave phase and volume alone
            sound.frequency = key_freq_map[note_on_buffer[i].key];
            sound.max_vol = max_vol;
            sound.vol_step = max_vol / ((SAMPLERATE / 1000) * envelope.decay);
            sound.note_on = true;
            sound.stage = ATTACK;
        }
    }

    for (unsigned short i = 0; i < off_buffer_size; ++i) {
        playing_sounds[note_off_buffer[i].key]->note_on = false;
    }

    // Remove the frequencies that were processed
    note_on_buffer.erase(note_on_buffer.begin(), note_on_buffer.begin() + on_buffer_size);
    note_off_buffer.erase(note_off_buffer.begin(), note_off_buffer.begin() + off_buffer_size);
}

int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData) {

    float *out = (float *)outputBuffer;
    update_volume();
    update_sounds();

    std::vector<size_t> toRemove;
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        float sample = 0.0;
        for (size_t j = 0; j < playing_sounds.size(); ++j) {  // Changed the loop to use index
            auto& opt_sound = playing_sounds[j];
            if (!opt_sound.has_value()) continue;
            Sound& sound = opt_sound.value();
            double phaseIncrement = 2.0 * M_PI * sound.frequency / SAMPLERATE;
            sample += sound.vol * static_cast<float>(std::sin(sound.phase));
            sound.phase += phaseIncrement;
            sound.phase = std::fmod(sound.phase, 2.0 * M_PI);
            sound.vol = std::clamp(sound.vol + sound.vol_step, 0.0f, sound.max_vol);
            switch(sound.stage){
                case ATTACK:
                     if (sound.vol >= sound.max_vol) {
                        sound.stage = DECAY;
                        sound.vol_step = -sound.max_vol*(1-(envelope.sustain))
                                /((SAMPLERATE/1000)*envelope.decay);
                        //spdlog::debug("enter DECAY, vol_step {} ", sound.vol_step);
                    }
                     break;
                case DECAY:
                    if (sound.vol <= sound.max_vol*envelope.sustain) {
                        sound.stage = SUSTAIN;
                        sound.vol_step = 0;
                        //spdlog::debug("enter SUSTAIN, vol_step {} ", sound.vol_step);
                    }
                    break;
                case SUSTAIN:
                    if (!sound.note_on) {
                        sound.stage = RELEASE;
                        sound.vol_step = -(sound.max_vol*envelope.sustain)
                                         /((SAMPLERATE/1000)*envelope.release);
                        //spdlog::debug("enter RELEASE, vol_step {} ", sound.vol_step);
                    }
                    break;
                case RELEASE:
                    if (sound.vol <= 0) {
                        toRemove.push_back(j);  // Add this line to mark for removal
                        // spdlog::debug("note over");
                    }
                    break;
            }
        }
        *out++ = sample * gain; // Left channel
        *out++ = sample * gain; // Right channel
    }

    for (auto index : toRemove) {
        playing_sounds[index] = std::nullopt;
    }
    return paContinue;
}



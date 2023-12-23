#include "AudioEngine.h"
#include <cmath>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>
#include <map>
#include <array>
#include "public.sdk/source/vst2.x/audioeffectx.h"

enum EnvelopeStage {
    ATTACK, DECAY, SUSTAIN, RELEASE
};

struct Note {
    int key;
    int velocity;
};

struct Envelope {
    float attack;
    float decay;
    float sustain;
    float release;
};

struct LFO {
    float frequency;
    float amplitude;
    float phase;
};

struct Filter {
    float cutoff;
    float Q;
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

std::array<float, 256> key_freq_map = {};
std::vector<Note> note_on_buffer = {};
std::vector<unsigned short> note_off_buffer = {};
std::array<std::optional<Sound>, 256> playing_sounds;

Envelope envelope = {.attack=0, .decay=0,.sustain=0,.release=0};
LFO vol_lfo = {.frequency = 0, .amplitude = 0, .phase = 0 };
LFO filter_cutoff_lfo = {.frequency = 0.001, .amplitude = 0.3, .phase = 0 };
Filter filter {.cutoff=10, .Q=1};

float gain = 0.5;
float target_gain = gain;
float gain_step = 0;
bool gain_changing = false;

float p_saw = 0;
float p_sin = 0;
float p_square = 1;

// filter coefficients (should be calculated based on set cutoff and Q)
float a0 = 0.1;
float a1 = 0.1;
float a2 = 0.1;
float b0 = 0.1;
float b1 = 0.1;
float b2 = 0.1;

void note_on(unsigned short key, unsigned short velocity) {
    //spdlog::debug("note on key:{} vel:{:.2f}", note.key, note.velocity);
    if (key >= 0 && key < 256) {
        note_on_buffer.push_back({.key=key, .velocity=velocity});
        //spdlog::debug("+ {}", note_buffer_to_string(note_on_buffer));
        //spdlog::debug("- {}", note_buffer_to_string(note_off_buffer));
    }
}

void note_off(unsigned short key) {
    //spdlog::debug("note off key:{}", note.key, note.velocity);
    if (key >= 0 && key < 256) {
        note_off_buffer.push_back(key);
        //spdlog::debug("+ {}", note_buffer_to_string(note_on_buffer));
        //spdlog::debug("- {}", note_buffer_to_string(note_off_buffer));
    }
}

void update_filter_coefficients() {
    float omega = 2.0 * M_PI * filter.cutoff / SAMPLERATE_kHz;  // Angular frequency
    omega += filter_cutoff_lfo.amplitude * static_cast<float>(std::sin(filter_cutoff_lfo.phase)) * omega;

    float alpha = std::sin(omega) / (2.0 * filter.Q);  // Bandwidth
    // Calculate the coefficients
    b0 = (1 - std::cos(omega)) / 2.0;
    b1 = 1 - std::cos(omega);
    b2 = (1 - std::cos(omega)) / 2.0;
    a0 = 1 + alpha;
    a1 = -2 * std::cos(omega);
    a2 = 1 - alpha;

    // Normalize the coefficients
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    a0 = 1.0;

    //spdlog::debug("{:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} ", a0, a1, a2, b0, b1, b2);
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

// function updates sounds, get called at top of buffer
void update_sounds() {
    unsigned short on_buffer_size = note_on_buffer.size();
    unsigned short off_buffer_size = note_off_buffer.size();

    for (unsigned short i = 0; i < on_buffer_size; ++i) {
        // Check if sound is active
        if (!playing_sounds[note_on_buffer[i].key].has_value()) {
            // Sound is not active, start sound
            float max_vol = note_on_buffer[i].velocity / 255.0f;
            playing_sounds[note_on_buffer[i].key] = {
                    .note_on = true, // key is pressed (distinct from sound is playing)
                    .stage = ATTACK, // attack phase
                    .frequency = key_freq_map[note_on_buffer[i].key], // look up frequency
                    .vol = 0,   // current/starting volume
                    .phase = 0, // current phase
                    .max_vol = max_vol, // peak volume for this sound
                    .vol_step = max_vol / (SAMPLERATE_kHz * envelope.attack) // amount to increase per sample
            };
            //spdlog::debug("enter ATTACK, vol_step {} ", sound.vol_step);
        } else if (!playing_sounds[note_on_buffer[i].key].value().note_on) {
            // Sound is active, but key is not pressed. Restart sound
            // without doing a step function on the volume or phase
            Sound& sound = *playing_sounds[note_on_buffer[i].key];
            float max_vol = note_on_buffer[i].velocity / 255.0f;
            // leave phase and volume alone
            sound.note_on = true;
            sound.stage = ATTACK;
            sound.frequency = key_freq_map[note_on_buffer[i].key];
            sound.max_vol = max_vol;
            sound.vol_step = max_vol / (SAMPLERATE_kHz * envelope.attack);
        }
    }

    for (unsigned short i = 0; i < off_buffer_size; ++i) {
        // set note on to false,
        // leave volume and phase, frequency, max_vol
        // enter release phase
        if (playing_sounds[note_off_buffer[i]].has_value()) {
            Sound &sound = *playing_sounds[note_off_buffer[i]];
            sound.note_on = false;
            sound.stage = RELEASE;
            sound.vol_step = -(sound.max_vol * envelope.sustain) / (SAMPLERATE_kHz * envelope.release);
            spdlog::debug("enter RELEASE, vol_step {}", sound.vol_step);
        } else {
            spdlog::debug("note in off_buffer had no value");
        }
    }

    // state transitions
    std::vector<size_t> toRemove;
    for (size_t j = 0; j < playing_sounds.size(); ++j) {  // Changed the loop to use index
        auto& opt_sound = playing_sounds[j];
        if (!opt_sound.has_value()) continue;
        Sound& sound = opt_sound.value();
        if (!sound.note_on) {
            sound.stage = RELEASE;
            sound.vol_step = -(sound.max_vol*envelope.sustain)
                             /(SAMPLERATE_kHz * envelope.release);
            //spdlog::debug("enter RELEASE, vol_step {} ", sound.vol_step);
        }
        switch(sound.stage){
            case ATTACK:
                if (sound.vol >= sound.max_vol) {
                    sound.stage = DECAY;
                    sound.vol_step = -sound.max_vol*(1-(envelope.sustain))
                                     /(SAMPLERATE_kHz * envelope.decay);
                    //spdlog::debug("enter DECAY, vol_step {} ", sound.vol_step);
                }
                break;
            case DECAY:
                if (sound.vol <= sound.max_vol*envelope.sustain) {
                    sound.stage = SUSTAIN;
                    sound.vol_step = 0;
                    spdlog::debug("enter SUSTAIN, vol_step {} ", sound.vol_step);
                }
                break;
            case SUSTAIN:

                break;
            case RELEASE:
                if (sound.vol <= 0) {
                    toRemove.push_back(j);  // Add this line to mark for removal
                    //spdlog::debug("note over");
                }
                break;
        }
    }

    // Remove the frequencies that were processed
    note_on_buffer.erase(note_on_buffer.begin(), note_on_buffer.begin() + on_buffer_size);
    note_off_buffer.erase(note_off_buffer.begin(), note_off_buffer.begin() + off_buffer_size);
    for (auto index : toRemove) {
        playing_sounds[index] = std::nullopt;
    }
}

// Initialize variables to keep track of previous inputs and outputs
static float last_input = 0.0f;
static float last_input2 = 0.0f;
static float last_output = 0.0f;
static float last_output2 = 0.0f;


int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData) {

    double lfo_phase_increment = 2.0 * M_PI * vol_lfo.frequency / SAMPLERATE_kHz;
    double filter_cutoff_lfo_phase_increment = 2.0 * M_PI * filter_cutoff_lfo.frequency / SAMPLERATE_kHz;

    float *out = (float *)outputBuffer;

    update_volume();
    update_sounds();
    update_filter_coefficients();

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        float sample = 0.0;
        for (size_t j = 0; j < playing_sounds.size(); ++j) {
            auto& opt_sound = playing_sounds[j];
            if (!opt_sound.has_value()){
                continue;
            }
            Sound& sound = opt_sound.value();
            sound.vol = std::clamp(sound.vol + sound.vol_step, 0.0f, sound.max_vol);
            sample += p_sin * sound.vol * static_cast<float>(std::sin(sound.phase));
            sample += p_saw * sound.vol * static_cast<float>((2.0 / M_PI) * (sound.phase - M_PI));
            sample += p_square * sound.vol * static_cast<float>((sound.phase < M_PI) ? 1.0 : -1.0);
            double phase_increment = 2.0 * M_PI * sound.frequency / SAMPLERATE_kHz;
            sound.phase += phase_increment;
            sound.phase = std::fmod(sound.phase, 2.0 * M_PI);
        }

        // Apply LFO to volume
        sample = sample + sample * vol_lfo.amplitude * static_cast<float>(std::sin(vol_lfo.phase));

        // Apply second-order low-pass filter
        sample = (b0 / a0) * sample + (b1 / a0) * last_input + (b2 / a0) * last_input2
                       - (a1 / a0) * last_output - (a2 / a0) * last_output2;
        // Update previous inputs and outputs
        last_input2 = last_input;
        last_input = sample;  // Assuming 'sample' was the input
        last_output2 = last_output;
        last_output = sample;

        vol_lfo.phase += lfo_phase_increment;
        vol_lfo.phase = std::fmod(vol_lfo.phase, 2.0 * M_PI);
        filter_cutoff_lfo.phase += filter_cutoff_lfo_phase_increment;
        filter_cutoff_lfo.phase = std::fmod(filter_cutoff_lfo.phase, 2.0 * M_PI);

        *out++ = sample * gain; // Left channel
        *out++ = sample * gain; // Right channel
    }

    return paContinue;
}





void set_LFO(float frequency, float magnitude) {
    vol_lfo.frequency = frequency;
    vol_lfo.amplitude = magnitude;
}

void set_key_freq_map(std::array<float, 256> map) {
    key_freq_map = map;
}

void set_envelope(float attack, float decay, float sustain, float release) {
    envelope.attack = attack;
    envelope.decay = decay;
    envelope.sustain = sustain;
    envelope.release = release;
}

void set_waveform(float sin, float saw, float square) {
    p_sin = sin;
    p_square = square;
    p_saw = saw;
}

void set_filter(float cutoff_freq, float Q) {
    filter.cutoff = cutoff_freq;
    filter.Q = Q;
    update_filter_coefficients();
}

void set_cutoff_lfo(float frequency, float magnitude) {
    filter_cutoff_lfo.frequency = frequency;
    filter_cutoff_lfo.amplitude = magnitude;
}

std::string note_buffer_to_string(std::vector<Note> note_buffer){
    std::string nb_string = "";
    for (int i = 0; i < note_buffer.size(); ++i) {
        nb_string = fmt::format("{} {}", nb_string, note_buffer[i].key);
    }
    return nb_string;
}


void change_volume(float amount, float period) {
    gain_step = amount / (SAMPLERATE_kHz * period / 1000); //(1/ms)
    target_gain = std::clamp(gain + amount, 0.0f, 1.0f);
    gain_changing = true;
    //spdlog::debug("target gain = {:.2f} | step = {:.2f} ", target_gain, gain_step * 1000);
}


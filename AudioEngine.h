#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <portaudio.h>
#include <array>

struct Note {
    int key;
    float velocity;
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

void change_volume(float amount, float period);

void note_on(Note note);

void note_off(Note frequency);

void initialize_key_freq_map(std::array<float, 256> map);

void set_envelope(Envelope envelope);

void set_LFO(float frequency, float magnitude);

void set_waveform(float p_sin, float p_saw, float s_square);

int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData);

extern float frequency;  // External global variable


#endif  // AUDIO_ENGINE_H

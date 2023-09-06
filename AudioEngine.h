#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <portaudio.h>
#include <array>


const float SAMPLERATE_kHz = 44.1;

void change_volume(float amount, float period);

void note_on(unsigned short key, unsigned short velocity);

void note_off(unsigned short key);

void initialize_key_freq_map(std::array<float, 256> map);

void set_envelope(float attack, float decay, float sustain, float release);

void set_LFO(float frequency, float magnitude);

void set_cutoff_lfo(float frequency, float magnitude);

void set_waveform(float p_sin, float p_saw, float s_square);

void set_filter(float cutoff, float Q);

int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData);

extern float frequency;  // External global variable


#endif  // AUDIO_ENGINE_H

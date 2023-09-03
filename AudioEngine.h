#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <portaudio.h>

void change_volume(float amount, float period);

void frequency_on(float frequency);

void frequency_off(float frequency);

int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData);

extern float frequency;  // External global variable


#endif  // AUDIO_ENGINE_H

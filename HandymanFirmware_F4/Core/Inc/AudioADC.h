#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H
#include "main.h"

#define FFT_SIZE 2048

// converts a right-aligned 12 bit value from the ADC to a 
// 32 bit float in the -1 to 1 range
float AudioADC_12BitToFloat(uint32_t value);

// call this once at boot to initialise the two FFT buffers
void AudioADC_InitFFTBuffers();

// push the next value into the current FFT buffer
void AudioADC_PushFFTValue(float value);

#endif
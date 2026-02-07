#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H
#include "main.h"

#define FFT_SIZE 2048

// converts a right-aligned 12 bit value from the ADC to a 
// 32 bit float in the -1 to 1 range
float AudioADC_12BitToFloat(uint16_t value);
// call this at the end of a conversion to load a chunk of samples from teh ADC buffer into the FFT buffer
void AudioADC_LoadToFFTBuffer(uint16_t* buf);



#endif
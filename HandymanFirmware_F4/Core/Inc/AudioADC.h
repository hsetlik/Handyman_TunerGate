#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H
#include "main.h"

#include <stdbool.h>
#define FFT_SIZE 2048

// converts a right-aligned 12 bit value from the ADC to a 
// 32 bit float in the -1 to 1 range
float AudioADC_12BitToFloat(uint16_t value);
// in tuner mode: call this at the end of a conversion to load a chunk of samples from the ADC buffer into the FFT buffer
void AudioADC_LoadToFFTBuffer(uint16_t* buf);
// not in tuner mode: call this to load the next set of values into the RMS level meter
void AudioADC_LoadToRMSBuffer(uint16_t* buf);
// call this in the main while loop to check if it's time to perform an FFT for the next tuning error
bool AudioADC_ShouldPerformFFT();



#endif
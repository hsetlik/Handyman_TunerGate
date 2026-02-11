#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H
#include "main.h"

// not in tuner mode: call this to load the next set of values into the RMS level meter
void AudioADC_LoadToRMSBuffer(uint16_t* buf);




#endif
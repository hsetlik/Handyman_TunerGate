#include "AudioADC.h"
#include <stdbool.h>

// buffers for our FFT data
float fftBuf[FFT_SIZE * 2];

bool timeForFFT = false;

float AudioADC_12BitToFloat(uint16_t value){
    float fVal = (float)value - 2048.0f;
    return fVal / 2048.0f;
}


void AudioADC_LoadToFFTBuffer(uint16_t* buf) {
    for(uint32_t i = 0; i < FFT_SIZE; ++i){
        fftBuf[i] = AudioADC_12BitToFloat(buf[i]);
    }
    timeForFFT = true;
}



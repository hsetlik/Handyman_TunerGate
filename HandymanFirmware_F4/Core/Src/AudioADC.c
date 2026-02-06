#include "AudioADC.h"
#include <stdbool.h>

// buffers for our FFT data
float fftBufA[FFT_SIZE * 2];
float fftBufB[FFT_SIZE * 2];
// pointers for the ready and waiting FFT buffers
float* pendingBuf = fftBufA;
float* readyBuf = fftBufB;
uint32_t pendingBufPos = 0;
bool timeForFFT = false;

float AudioADC_12BitToFloat(uint32_t value){
    float fVal = (float)value - 2048.0f;
    return fVal / 2048.0f;
}



void AudioADC_InitFFTBuffers(){
    for(uint32_t i = 0; i < FFT_SIZE * 2; ++i){
        fftBufA[i] = 0.0f;
        fftBufB[i] = 0.0f;
    }
}


void AudioADC_PushFFTValue(float value){
    pendingBuf[pendingBufPos] = value;
    ++pendingBufPos;
    // time to swap the buffers
    if(pendingBufPos >= FFT_SIZE){
        float* prevPending = pendingBuf;
        pendingBuf = readyBuf;
        readyBuf = prevPending;
        for(uint32_t i = 0; i < FFT_SIZE * 2; ++i){
            pendingBuf[i] = 0.0f;
        }
        pendingBufPos = 0;
        timeForFFT = true;
    }
}

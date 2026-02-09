#include "AudioADC.h"

// buffer for our FFT data
float ACFBuf[WINDOW_SIZE];
bool timeForACF = false;

// buffer for our envelope follower

float AudioADC_12BitToFloat(uint16_t value){
    float fVal = (float)value - 2048.0f;
    return fVal / 2048.0f;
}


void AudioADC_LoadToACFBuffer(uint16_t* buf) {
    for(uint32_t i = 0; i < WINDOW_SIZE; ++i){
        ACFBuf[i] = AudioADC_12BitToFloat(buf[i * 3]);
    }
    timeForACF = true;
}


bool AudioADC_ShouldPerformACF(){
    return timeForACF;
}

// this is a cooley lewis and welch type FFT that takes two buffers of length FFT_SIZE
// for the real and imaginary components

//-----------------------------------------------------------------
void AudioADC_LoadToRMSBuffer(uint16_t* buf){
    //TODO
}



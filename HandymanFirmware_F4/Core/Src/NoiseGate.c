#include "NoiseGate.h"
#include "main.h"

static int16_t abs16(int16_t val){
    if(val < 0){
        return -val;
    }
    return val;
}

float Gate_sampleMagnitude(uint16_t val) {
    int16_t iVal = abs16((int16_t)val - 2048);
    return (float)iVal / 2048.0f;
}

static const float fAttack = 0.8f;
float fRelease = 0.95f;
float envLevel = 0.0f;
float noiseThresh = 0.35f;
bool isGateClosed = false;
float Gate_pushSample(uint16_t val){
    const float vMag = Gate_sampleMagnitude(val);
    if(vMag > envLevel){
        envLevel = fAttack * envLevel + (1.0f - fAttack) * vMag;
    } else {
        envLevel = fRelease * envLevel + (1.0f - fRelease) * vMag;
    }
    return envLevel;
}


void Gate_processChunk(uint16_t* buf, uint32_t length){
    const uint32_t chunkLength = length / 64;
    if(length % 64 != 0){
        Error_Handler();
    }
    uint32_t bufIdx = 0;
    while(bufIdx < length){
        // 1. load the chunk into the env follower
        for(uint32_t i = 0; i < chunkLength; ++i){
            Gate_pushSample(buf[bufIdx + i]);
        }
        // 2. check if we should open/close the gate
        if(!isGateClosed && envLevel < noiseThresh){
            setNoiseGateClosed(true);
            isGateClosed = true;
        } else if (isGateClosed && envLevel >= noiseThresh){
            setNoiseGateClosed(false);
            isGateClosed = false;
        }
        // 3. increment bufIdx
        bufIdx += chunkLength;

    }
}
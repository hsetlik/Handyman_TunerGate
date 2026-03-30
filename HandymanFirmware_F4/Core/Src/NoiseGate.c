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
    return (float)iVal / 1024.0f;
}

static const float fAttack = 0.8f;
float fRelease = 0.95f;
float envLevel = 0.0f;
float noiseThresh = 0.35f;
bool isGateClosed = false;
volatile bool wantsPotReadings = false;

float Gate_pushChunkLevel(float vMag){
    if(vMag > envLevel){
        envLevel = fAttack * envLevel + (1.0f - fAttack) * vMag;
    } else {
        envLevel = fRelease * envLevel + (1.0f - fRelease) * vMag;
    }
    return envLevel;
}

static float Gate_getChunkLevel(uint16_t* buf, uint32_t length){
    float chunkSum = 0.0f;
    for(uint32_t i = 0; i < length; ++i){
        chunkSum +=  Gate_sampleMagnitude(buf[i]);
    }
    return (chunkSum / (float)length);
}


void Gate_processChunk(uint16_t* buf, uint32_t length){
    const uint32_t chunkLength = length / 4;
    if(length % 4 != 0){
        Error_Handler();
    }
    uint32_t bufIdx = 0;
    while(bufIdx < length){
        // 1. load the chunk into the env follower
        const float chunkLvl = Gate_getChunkLevel(&buf[bufIdx], chunkLength);
        Gate_pushChunkLevel(chunkLvl);
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


void Gate_requestPotReadings(){
    wantsPotReadings = true;
}


bool Gate_isAwaitingPotReadings() {
    return wantsPotReadings;
}

static float lerp12Bit(float minVal, float maxVal, uint16_t pos){
    const float fPos = (float)pos / 4096.0f;
    return minVal + (fPos * (maxVal - minVal));
}


void Gate_updatePotReadings(uint16_t threshVal, uint16_t releaseVal) {
    fRelease = lerp12Bit(RELEASE_MIN, RELEASE_MAX, releaseVal);
    noiseThresh = lerp12Bit(THRESH_MIN, THRESH_MAX, threshVal);
    wantsPotReadings = false;
}
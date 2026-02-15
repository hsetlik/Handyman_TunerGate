#include "NoiseGate.h"

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
float Gate_pushSample(uint16_t val){
    const float vMag = Gate_sampleMagnitude(val);
    if(vMag > envLevel){
        envLevel = fAttack * envLevel + (1.0f - fAttack) * vMag;
    } else {
        envLevel = fRelease * envLevel + (1.0f - fRelease) * vMag;
    }
    return envLevel;
}
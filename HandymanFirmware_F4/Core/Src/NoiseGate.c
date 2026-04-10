#include "NoiseGate.h"
#include "main.h"
#include <math.h>

#define SAMPLE_RATE 48000.0f
#define TIME_CONST -2197.22457724f

// minLevel = r^rSamples
// or r = minLevel^(1/rSamples)
noise_gate_t ng;
float sampleTimeMs;


static inline int16_t abs16(int16_t val){
    if(val < 0){
        return -val;
    }
    return val;
}

static inline float Gate_sampleMagnitude(uint16_t val) {
    int16_t iVal = abs16((int16_t)val - 2048);
    return (float)iVal;
}

static float getCoefficientForMs(float ms){
    return expf(TIME_CONST / (SAMPLE_RATE * ms));
}



void Gate_initNoiseGate(){
    ng.threshold = THRESH_MIN;
    ng.attackCoeff = getCoefficientForMs(ATTACK_MS_MIN);
    ng.releaseCoeff = getCoefficientForMs(RELEASE_MS);
    ng.attackCounter = 0.0f;
    ng.smoothedGain = 1.0f;
    sampleTimeMs = 1000.0f / SAMPLE_RATE;
}

static void Gate_processSample(uint16_t value){
    // 1. grip the magnitude of the current sample
    const float mag = Gate_sampleMagnitude(value);
    // 2. determine whether we should smooth the gain towards 0 or 1
    const float gain = (mag >= ng.threshold) ? 1.0f : 0.0f;

    // 3. if the smoothed gain is above our target, either:
    if(gain <= ng.smoothedGain){
        // a. lerp towards the target gain w the appropriate filter coefficient
        if(ng.attackCounter > HOLD_TIME_MS){
            ng.smoothedGain = (ng.attackCoeff * ng.smoothedGain) + ((1.0f - ng.attackCoeff) * gain);
        // or b. increment the counter until it's time to do a.
        } else {
            ng.attackCounter += sampleTimeMs;
        }
    // 4. if our smoothed gain is below the target, smooth w the release coefficient
    } else {
        ng.smoothedGain = (ng.releaseCoeff * ng.smoothedGain) + ((1.0f - ng.releaseCoeff) * gain);
        // the attackCounter gets reset here since we're in the release phase
        ng.attackCounter = 0.0f;
    }
}

static void Gate_processChunk(uint16_t* buf, uint32_t length){
    for(uint32_t i = 0; i < length; ++i){
        Gate_processSample(buf[i]);
    }
}

void Gate_processWindow(uint16_t* buf, uint32_t length){
    const uint32_t chunkLength = length / 4;
    uint32_t bufIdx = 0;
    /* split the buffer up into four chunks, each will be 32 samples long.
       after processing each chunk, we set the GPIO pin that controls the analog 
       gate circuit based on the status of the smoothed gain
    */
    while(bufIdx < length){
        Gate_processChunk(&buf[bufIdx], chunkLength);
        setNoiseGateClosed(ng.smoothedGain < 0.5f);
        bufIdx += chunkLength;
    }
}

//=====================================================================================
volatile bool wantsPotReadings = true;
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


void Gate_updatePotReadings(uint16_t threshVal, uint16_t attackVal) {
    ng.threshold = lerp12Bit(THRESH_MIN, THRESH_MAX, threshVal);
    const float attackMs = lerp12Bit(ATTACK_MS_MIN, ATTACK_MS_MAX, attackVal);
    ng.attackCoeff = getCoefficientForMs(attackMs);
    wantsPotReadings = false;
}
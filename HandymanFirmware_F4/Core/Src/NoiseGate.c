#include "NoiseGate.h"
#include "main.h"
#include <math.h>

#define SAMPLE_RATE 48000.0f
#define TIME_CONST -2197.22457724f

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


float threshold = THRESH_MIN;
float attackCoeff;
float releaseCoeff;
float attackCounter = 0.0f;
float envLevel = 0.0f;
const float sampleTimeMs = 1000.0f / SAMPLE_RATE;


#ifdef NOISE_DEBUG
#define RMS_LENGTH 1024
float rmsBuf[RMS_LENGTH];
uint16_t rmsHead = 0;
float rmsSum = 0.0f;
float rmsLevel = 0.0f;
static void pushRMS(float mag){
    // 1. subtract the oldest sample
    rmsSum -= rmsBuf[rmsHead];
    // 2. add the newest sample
    rmsSum += mag;
    // 3. overwrite the oldest sample
    rmsBuf[rmsHead] = mag;
    rmsHead = (rmsHead + 1) % RMS_LENGTH;
    // 4. calculate the mean
    rmsLevel = rmsSum / (float)RMS_LENGTH;
}
#endif

void Gate_initNoiseGate(){
attackCoeff = getCoefficientForMs(ATTACK_MS_MIN);
releaseCoeff = getCoefficientForMs(RELEASE_MS);
#ifdef NOISE_DEBUG
for(uint16_t i = 0; i < RMS_LENGTH; ++i){
    rmsBuf[i] = 0.0f;
}
#endif
}

static void Gate_processSample(uint16_t value){
    // 1. grip the magnitude of the current sample
    const float mag = Gate_sampleMagnitude(value);
    #ifdef NOISE_DEBUG
    pushRMS(mag);
    #endif
    // 2. determine whether we should smooth the gain towards 0 or 1
    const float gain = (mag >= threshold) ? 1.0f : 0.0f;

    // 3. if the smoothed gain is above our target, either:
    if(gain <= envLevel){
        // a. lerp towards the target gain w the appropriate filter coefficient
        if(attackCounter > HOLD_TIME_MS){
            envLevel = (attackCoeff * envLevel) + ((1.0f - attackCoeff) * gain);
        // or b. increment the counter until it's time to do a.
        } else {
            attackCounter += sampleTimeMs;
        }
    // 4. if our smoothed gain is below the target, smooth w the release coefficient
    } else {
        envLevel = (releaseCoeff * envLevel) + ((1.0f - releaseCoeff) * gain);
        // the attackCounter gets reset here since we're in the release phase
        attackCounter = 0.0f;
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
    /* split the buffer up into four chunks, each will be 32 samples lo
       after processing each chunk, we set the GPIO pin that controls the analog 
       gate circuit based on the status of the smoothed gain
    */
    while(bufIdx < length){
        Gate_processChunk(&buf[bufIdx], chunkLength);
        setNoiseGateClosed(envLevel < 0.5f);
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


static float mapThreshCurve(float norm) {
    static const float skew = logf(0.5f) / logf((THRESH_CENTER - THRESH_MIN) / (THRESH_MAX - THRESH_MIN));
    const float proportion = expf(logf(norm) / skew);
    return THRESH_MIN + ((THRESH_MAX - THRESH_MIN) * proportion);
}

static float lerp12Bit(float minVal, float maxVal, uint16_t pos){
    const float fPos = (float)pos / 4096.0f;
    return minVal + (fPos * (maxVal - minVal));
}


void Gate_updatePotReadings(uint16_t threshVal, uint16_t attackVal) {
    const float threshValNorm = (float)threshVal / 4096.0f;

    threshold = mapThreshCurve(threshValNorm);
    const float attackMs = lerp12Bit(ATTACK_MS_MIN, ATTACK_MS_MAX, attackVal);
    attackCoeff = getCoefficientForMs(attackMs);
    wantsPotReadings = false;
}
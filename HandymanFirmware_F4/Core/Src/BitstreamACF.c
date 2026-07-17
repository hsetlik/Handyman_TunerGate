#include "BitstreamACF.h"
#include "IIR.h"
#include "main.h"
#include <stdint.h>
#include <stdlib.h>

static const float BAC_sampleRate = 48000.0f;
static const float BAC_maxFreq = 1460.0f;
//static const float BAC_minFreq = 32.7f;
static const uint32_t minPeriod = (uint32_t)(BAC_sampleRate/ BAC_maxFreq);
#define BAC_numBits 32
static const bitval_t BAC_arraySize = TUNING_WINDOW_SIZE / BAC_numBits;
volatile bool bacRunning = false;
volatile bool bitstreamLoaded = false;
volatile bool hasValidSignal = false;
bool zeroCrossState = false;

// the main array of bitstream values
bitval_t bits[TUNING_WINDOW_SIZE / BAC_numBits];
//stores the correlations for each offset
bitval_t corBuffer[TUNING_WINDOW_SIZE / 2];
// raw normalized values from the input
float signalBuf[TUNING_WINDOW_SIZE];
// input filter
static iir_t filter;

// hold the min/max values and min index
uint32_t minDifference = 0xFFFF;
uint32_t maxDifference = 0;
uint32_t minIdx = 0;


void BAC_initBitArray(){
    for(bitval_t i = 0; i < BAC_arraySize; ++i){
        bits[i] = 0;
    }
    for(bitval_t i = 0; i < TUNING_WINDOW_SIZE / 2; ++i){
        corBuffer[i] = 0;
    }
    filter.alpha = iir_getAlpha(INPUT_CUTOFF_FREQ);
    filter.y1 = 0.0f;
}

bool BAC_hasTuningSignal(){
    return hasValidSignal;
}


void BAC_finishedWithCurrentHz(){
    bitstreamLoaded = false;
    hasValidSignal = false;
}

bool BAC_get(uint32_t i){
    bitval_t mask = 1 << (i % BAC_numBits);
    return (bits[i / BAC_numBits] & mask) != 0;
}

void BAC_set(uint32_t i, bool val){
    bitval_t mask = 1 << (i % BAC_numBits);
    bitval_t* ref = &bits[i / BAC_numBits];
    if(val){
        bits[i / BAC_numBits] = *ref | mask;
    } else {
        bits[i / BAC_numBits] = *ref & ~(mask);
    }
}

bool BAC_isZeroCross(bool* prevState, float value){
    if(value < -0.1f){
        *prevState = false;
    } else if (value > 0.0f){
        *prevState = true;
    }
    return *prevState;
}

static inline float getSampleMagnitude(uint16_t sample){
    int16_t val = (int16_t)(sample - 2048);
    if(val < 0){
       val = -val; 
    }
    return (float)val;
}

static inline void BAC_fillSignalBuf(uint16_t* adcBuf){
    for(uint16_t i = 0; i < TUNING_WINDOW_SIZE; ++i){
        float fVal = (float)(adcBuf[i] - 2048) / 2048.0f;
        #ifdef BAC_PREFILTER
        signalBuf[i] = iir_process(&filter, fVal);
        #else
        signalBuf[i] = fVal;
        #endif
    }
}

void BAC_loadBitstream(uint16_t* adcBuf){
    if(bitstreamLoaded && hasValidSignal){
        return;
    }
    hasValidSignal = false;
    BAC_fillSignalBuf(adcBuf);
    bool prev = BAC_isZeroCross(&zeroCrossState, signalBuf[0]);
    float sum = getSampleMagnitude(adcBuf[0]);
    BAC_set(0, prev);
    for(uint32_t i = 1; i < TUNING_WINDOW_SIZE; ++i){
        bool current =  BAC_isZeroCross(&zeroCrossState, signalBuf[i]);
        sum += getSampleMagnitude(signalBuf[i]);
        BAC_set(i, current);
    }
    const float avgMag = sum / (float)TUNING_WINDOW_SIZE;

    /* only update the tuning display if we have at least one zero crossing
     AND the magnitude of this chunk is above some threshold (60 seems about right for the 
     pickups/guitars I've tested this with)
    */ 
    hasValidSignal = avgMag >= 60.0f;
    bitstreamLoaded = true;
}



bool BAC_isBitstreamLoaded(){
    return bitstreamLoaded;
}

bool BAC_isWorking(){
    return bacRunning;
}

static void BAC_clearCorBuf(){
    for(uint32_t i = 0; i < (TUNING_WINDOW_SIZE / 2); ++i){
        corBuffer[i] = 0;
    }
}

void BAC_autoCorrelate(uint32_t startPos){
    bacRunning = true;
    BAC_clearCorBuf();
    const uint32_t midArray = (BAC_arraySize / 2) - 1;
    const uint32_t midPoint = TUNING_WINDOW_SIZE / 2;
    uint32_t index = startPos / BAC_numBits;
    uint32_t shift = startPos % BAC_numBits;

    for(uint32_t pos = startPos; pos < midPoint; ++pos){
        bitval_t* p1 = &bits[0];
        bitval_t* p2 = &bits[index];
        uint32_t count = 0;
        if(shift == 0){
            for(uint32_t i = 0; i < midArray; ++i){
                count += __builtin_popcount(*p1++ ^ *p2++);
            }
        } else {
            uint32_t shift2 = BAC_numBits - shift;
            
            for(uint32_t i = 0; i < midArray; ++i){
                uint32_t v = *p2++ >> shift;
                v |= *p2 << shift2;
                count += __builtin_popcount(*p1++ ^ v);
            }
        }
        ++shift;
        if(shift == BAC_numBits){
            shift = 0;
            ++index;
        }
        corBuffer[pos] = count;
    }
    bacRunning = false;
    bitstreamLoaded = false;
}

// convert the correlation bin index to a frequency in hertz
static float BAC_hzForIndex(uint32_t index){
    return BAC_sampleRate / (float)index;
}

// finds the minimum value of the correlation buffer in the given range
static void BAC_findMinMaxCorrelations(uint32_t startBin, uint32_t endBin){
    minDifference = 0xFFFF;
    minIdx = 0;
    maxDifference = 0;
    for(uint32_t i = startBin; i <= endBin; ++i){
        if(corBuffer[i] < minDifference){
            minDifference = corBuffer[i];
            minIdx = i;
        }
        if(corBuffer[i] > maxDifference){
            maxDifference = corBuffer[i];
        }
    }
}

float BAC_getCurrentHz(){
    static const uint32_t endBin = (TUNING_WINDOW_SIZE / 2) - 1;
    // 1. run the main autocorrelation function
    BAC_autoCorrelate(minPeriod);
    // 2. calculate the min & max correlations
    BAC_findMinMaxCorrelations(minPeriod, endBin);
    // 3. adjust the minimum index to account for any harmonics
    const uint32_t subThresh = (uint32_t)((float)maxDifference * 0.15f);
    const uint32_t maxDiv = minIdx / minPeriod;
    for(uint32_t div = maxDiv; div > 0; --div){
        bool allStrong = true;
        const float mul = 1.0f / (float)div;
        for(uint32_t k = 1; k < div; ++k){
            uint32_t subPeriod = (uint32_t)((float)k * (float)minIdx * mul);
            if(corBuffer[subPeriod] > subThresh){
                allStrong = false;
                break;
            }
        }
        if(allStrong){
            minIdx = (uint32_t)((float)minIdx * mul);
            break;
        }
    }
    return BAC_hzForIndex(minIdx);
}
#include "BitstreamACF.h"
#include "main.h"
#include <stdlib.h>
static bitval_t smallestPow2Recur(bitval_t n, bitval_t m){
    return (m < n) ? smallestPow2Recur(n, m << 1) : m;
}

bitval_t smallestPow2(bitval_t n){
    return smallestPow2Recur(n, 1);
}

static const float BAC_sampleRate = 40000.0f;
#define BAC_numBits 32
static const bitval_t BAC_arraySize = WINDOW_SIZE / BAC_numBits;
volatile bool bacRunning = false;
volatile bool bitstreamLoaded = false;
volatile bool hasValidSignal = false;
volatile uint32_t currentRisingEdge = 0;

// the main array of bitstream values
bitval_t bits[WINDOW_SIZE / BAC_numBits];
bitval_t corBuffer[WINDOW_SIZE / 2];


void BAC_initBitArray(){
    for(bitval_t i = 0; i < BAC_arraySize; ++i){
        bits[i] = 0;
    }
    for(bitval_t i = 0; i < WINDOW_SIZE / 2; ++i){
        corBuffer[i] = 0;
    }
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

bool BAC_isZeroCross(uint16_t value){
    return value >= 2048;
}

static uint32_t BAC_firstRisingEdge(uint16_t* buf){
    for(uint32_t i = 1; i < (WINDOW_SIZE / 2); ++i){
        if(buf[i - 1] < 2048 && buf[i] >= 2048){
            hasValidSignal = true;
            return i;
        }
    }
    hasValidSignal = false;
    return 0;
}

void BAC_loadBitstream(uint16_t* adcBuf, uint32_t spacing){
    if(bitstreamLoaded && hasValidSignal){
        return;
    }
    // 1. find the first rising edge, ensuring that it falls before the midway point of the input buffer
    currentRisingEdge = BAC_firstRisingEdge(adcBuf);
    // 2. copy the relevant portion of the adc buffer into the bitstream
    for(uint32_t i = 0; i < WINDOW_SIZE; ++i){
        BAC_set(i, BAC_isZeroCross(adcBuf[i * spacing]));
    }
    bitstreamLoaded = true;
}



bool BAC_isBitstreamLoaded(){
    return bitstreamLoaded && !bacRunning;
}

bool BAC_isWorking(){
    return bacRunning;
}

static void BAC_clearCorBuf(){
    for(uint32_t i = 0; i < (WINDOW_SIZE / 2); ++i){
        corBuffer[i] = 0;
    }
}

void BAC_autoCorrelate(uint32_t startPos){
    bacRunning = true;
    BAC_clearCorBuf();
    const uint32_t midArray = (BAC_arraySize / 2) - 1;
    const uint32_t midPoint = WINDOW_SIZE / 2;
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
static uint32_t BAC_minCorrelationIndex(uint32_t startBin, uint32_t endBin){
    uint32_t minDifference = 0xFFFF;
    uint32_t minIdx = 0;
    for(uint32_t i = startBin; i <= endBin; ++i){
        if(corBuffer[i] < minDifference){
            minDifference = corBuffer[i];
            minIdx = i;
        }
    }
    return minIdx;
}

float BAC_getCurrentHz(){
    BAC_autoCorrelate(currentRisingEdge);
    const uint32_t startBin = (currentRisingEdge > 55) ? currentRisingEdge : 55;
    return BAC_hzForIndex(BAC_minCorrelationIndex(startBin, (WINDOW_SIZE / 2) - 1));
}
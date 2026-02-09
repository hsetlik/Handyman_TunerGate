#include "BitstreamACF.h"
#include <stdlib.h>
static bitval_t smallestPow2Recur(bitval_t n, bitval_t m){
    return (m < n) ? smallestPow2Recur(n, m << 1) : m;
}

bitval_t smallestPow2(bitval_t n){
    return smallestPow2Recur(n, 1);
}


static const bitval_t BAC_numBits = 8 * sizeof(bitval_t);
static const bitval_t BAC_arraySize = WINDOW_SIZE / BAC_numBits;

// the main array of bitstream values
bitval_t* bits;

bitval_t BAC_countBits(bitval_t n) {
    bitval_t count = 0;
    for(bitval_t i = 0; i < BAC_numBits; ++i){
        if((n & (1 << i)) > 0){
            ++count;
        }
    }
    return count;
}

void BAC_initBitArray(){
    bits = (bitval_t*)malloc(BAC_arraySize * sizeof(bitval_t));
    for(bitval_t i = 0; i < BAC_arraySize; ++i){
        bits[i] = 0;
    }
}


bool BAC_get(uint32_t i){
    bitval_t mask = 1 << (i % BAC_numBits);
    return (bits[i / BAC_numBits] & mask) != 0;
}


void BAC_set(uint32_t i, bool val){
    bitval_t mask = 1 << (i % BAC_numBits);
    bitval_t* ref = &bits[i / BAC_numBits];
    if(val){
        *ref = *ref | mask;
    } else {
        *ref &= ~(mask);
    }
}


bool BAC_isZeroCross(uint16_t value){
    return value >= 2048;
}

void BAC_loadBitstream(uint16_t* adcBuf, uint32_t spacing){
    for(uint32_t i = 0; i < WINDOW_SIZE; ++i){
        BAC_set(i, BAC_isZeroCross(adcBuf[i * spacing]));
    }
}


void BAC_autoCorrelate(bitval_t* corBuf){
    corBuf[0] = 0;
    const uint32_t midArray = (BAC_arraySize / 2) - 1;
    const uint32_t midPoint = WINDOW_SIZE / 2;
    uint32_t index = 0;
    uint32_t shift = 1;

    for(uint32_t pos = 1; pos != midPoint; ++pos){
        bitval_t* p1 = &bits[0];
        bitval_t* p2 = &bits[index];
        uint32_t count = 0;
        if(shift == 0){
            for(uint32_t i = 0; i != midArray; ++i){
                count += BAC_countBits(*p1++ ^ *p2++);
            }
        } else {
            uint32_t shift2 = BAC_numBits - shift;
            
            for(uint32_t i = 0; i != midArray; ++i){
                uint32_t v = *p2++ >> shift;
                v |= *p2 << shift2;
                count += BAC_countBits(*p1++ ^ v);
            }
        }
        ++shift;
        if(shift == BAC_numBits){
            shift = 0;
            ++index;
        }
        corBuf[pos] = count;
    }
}
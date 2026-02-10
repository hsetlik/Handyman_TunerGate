#ifndef BITSTREAM_ACF_H
#define BITSTREAM_ACF_H
#include "main.h"
#include <stdbool.h>
// the type we'll use to store the bitstreams
#define bitval_t uint32_t
#define WINDOW_SIZE 1024
/* This is based on Joel de Guzman's Bitstream Autocorrelation concept, explained
in his 2018 blog post here:
https://www.cycfi.com/2018/03/fast-and-efficient-pitch-detection-bitstream-autocorrelation/
Code works more or less the same as Guzman's C++ implementation here:
https://github.com/cycfi/bitstream_autocorrelation
but in C (duh) and without a couple compiler specific things
*/
// return the smallest power of 2 greater than n
bitval_t smallestPow2(bitval_t n);

// returns the number of set bits in the input n
bitval_t BAC_countBits(bitval_t n);

//call this at startup
void BAC_initBitArray();

// get/set for the bit array
bool BAC_get(uint32_t i);
void BAC_set(uint32_t i, bool val);

// check if this ADC value is above the common mode voltage
bool BAC_isZeroCross(uint16_t value);

// call this on the filled ADC buffer to load data into the bitstream
void BAC_loadBitstream(uint16_t* adcBuf, uint32_t spacing);

// the actual autocorellation happens here. the input is a buffer of half the window size
void BAC_autoCorrelate();

// check if we're ready to run the algorithm
bool BAC_isBitstreamLoaded();

// return the current best guess for the fundamental frequency in the correlation buffer
float BAC_getCurrentHz();

#endif
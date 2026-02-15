#ifndef NOISE_GATE_H
#define NOISE_GATE_H
#include "main.h"

// get the floating point magnitude of this unsigned sample relative to the "zero" line (2048 in the case of our 12-bit ADC)
float Gate_sampleMagnitude(uint16_t val);

// push one sample into our IIR envelope follower and return the current envelope level
float Gate_pushSample(uint16_t val);


#endif
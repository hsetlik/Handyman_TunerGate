#ifndef NOISE_GATE_H
#define NOISE_GATE_H
#include "main.h"

// get the floating point magnitude of this unsigned sample relative to the "zero" line (2048 in the case of our 12-bit ADC)
float Gate_sampleMagnitude(uint16_t val);

// push one sample into our IIR envelope follower and return the current envelope level
float Gate_pushSample(uint16_t val);

// load a chunk of the DMA buffer into the envelope follower and open/close the noise gate as appropriate
void Gate_processChunk(uint16_t* buf, uint32_t length);

//TODO updating and ADC reading for the release and threshold knobs

#endif
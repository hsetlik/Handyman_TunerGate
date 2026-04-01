#ifndef NOISE_GATE_H
#define NOISE_GATE_H
#include "main.h"
#define GATE_WINDOW_SIZE 128
//#define RELEASE_MIN 0.941f
//#define RELEASE_MAX 0.998f
#define RELEASE_MIN_SAMPLES 2000.0
#define RELEASE_MAX_SAMPLES 42000.0
#define THRESH_MIN 35.0f
#define THRESH_MAX 120.0f

// this calculates the release coefficients (call once during setup)

void Gate_prepareNoiseGate();
// get the floating point magnitude of this unsigned sample relative to the "zero" line (2048 in the case of our 12-bit ADC)
float Gate_sampleMagnitude(uint16_t val);

float Gate_pushChunkLevel(float val);

// load a chunk of the DMA buffer into the envelope follower and open/close the noise gate as appropriate
void Gate_processChunk(uint16_t* buf, uint32_t length);

//TODO updating and ADC reading for the release and threshold knobs
void Gate_requestPotReadings();

bool Gate_isAwaitingPotReadings();

void Gate_updatePotReadings(uint16_t threshVal, uint16_t releaseVal);

#endif
#ifndef NOISE_GATE_H
#define NOISE_GATE_H
#include "main.h"
#include "IIR.h"
#define GATE_WINDOW_SIZE 128
#define THRESH_MIN 35.0f
#define THRESH_CENTER 140.0f
#define THRESH_MAX 285.0f
#define INPUT_CUTOFF 12000.0f

//#define NOISE_DEBUG


#define ATTACK_MS_MIN 25.0f
#define ATTACK_MS_MAX 1500.0f
#define RELEASE_MS 6.0f
#define HOLD_TIME_MS 8.0f

// call once at startup
void Gate_initNoiseGate();

// load a chunk of the DMA buffer into the envelope follower and open/close the noise gate as appropriate
void Gate_processWindow(uint16_t* buf, uint32_t length);

void Gate_requestPotReadings();

bool Gate_isAwaitingPotReadings();

void Gate_updatePotReadings(uint16_t threshVal, uint16_t releaseVal);

#endif
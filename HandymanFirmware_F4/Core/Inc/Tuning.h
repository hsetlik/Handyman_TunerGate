#ifndef TUNING_H
#define TUNING_H
#include "main.h"

// get a fractional midi number for a given frequency in hertz
float Tune_midiNoteForFreq(float hz);

typedef struct {
    uint16_t midiNote;
    int16_t errorCents;
} tuning_error_t;

tuning_error_t Tune_getErrorForFreq(float hz);



#endif
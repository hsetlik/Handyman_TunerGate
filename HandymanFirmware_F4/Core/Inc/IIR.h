#ifndef IIR_H
#define IIR_H
#include "main.h"
#include <math.h>
/*
Super simple first-order IIR lowpass filter.
Relevant math is explained here: https://www.dsprelated.com/showarticle/1769.php
*/

typedef struct {
    float alpha;
    float y1;
} iir_t;

float iir_getAlpha(float fCutoff);

float iir_process(iir_t* filter, float input);


#endif
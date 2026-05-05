#include "IIR.h"

float iir_getAlpha(float fc){
    const float denom = 1.0f + (SAMPLE_RATE / (2.0f * M_PI * fc));
    return 1.0f / denom;
}

float iir_process(iir_t* filter, float input){
    float output = (filter->alpha * (input - filter->y1)) + filter->y1;
    filter->y1 = output;
    return output;
}
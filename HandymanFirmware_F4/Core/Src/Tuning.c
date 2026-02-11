#include "Tuning.h"
#include <math.h>

float Tune_midiNoteForFreq(float hz){
    return 69.0f + (12.0f * (logf(hz/440.0f) / logf(2.0f)));
}


tuning_error_t Tune_getErrorForFreq(float hz){
    const float midiVal = Tune_midiNoteForFreq(hz);
    const float noteBelow = floorf(midiVal);
    uint16_t midiNote;
    int16_t cents;
    if((midiVal - noteBelow) <= 0.5f){ // closer to the note below, sharp
        midiNote = (uint16_t)noteBelow;
        cents = (int16_t)((midiVal - noteBelow) * 100.0f);
    } else { // otherwise flat
        
        midiNote = (uint16_t)(noteBelow) + 1;
        cents = (int16_t)((1.0f - (midiVal - noteBelow)) * -100.0f);
    }
    return (tuning_error_t){midiNote, cents};
}
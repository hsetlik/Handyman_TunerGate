/*
 * tuning.h
 *
 *  Created on: Feb 9, 2025
 *      Author: hayden
 */

#ifndef INC_TUNING_H_
#define INC_TUNING_H_
#include "main.h"


#ifdef __cplusplus
extern "C" {
#endif

#define TUNING_BUF_SIZE 150
#define SEMITONE_RATIO 1.05946309436f
#define IN_TUNE_THRESHOLD 4
#define TICK_RATE 4000
#define NUM_MIDI_NOTES 127

#define TICK_OVERFLOW_SAFE

// store the nearest note and error in cents
typedef struct {
	uint8_t note;
	int16_t cents;
} tuning_error_t;



// call this on the rising edge interrupt for the TUNE_IN pin
void tuning_rising_edge();

void tuning_update_error(tuning_error_t* err);








#ifdef __cplusplus
}
#endif

#endif /* INC_TUNING_H_ */

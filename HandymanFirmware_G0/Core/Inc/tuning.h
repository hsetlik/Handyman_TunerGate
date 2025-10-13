/*
 * tuning.h
 *
 *  Created on: Feb 9, 2025
 *      Author: hayden
 */

#ifndef INC_TUNING_H_
#define INC_TUNING_H_
#include "main.h"
#include <ssd1306_fonts.h>
#include <math.h>
#include <stdbool.h>
#include <ssd1306.h>


#ifdef __cplusplus
extern "C" {
#endif

#define TUNING_BUF_SIZE 128
#define SEMITONE_RATIO 1.05946309436f
#define IN_TUNE_THRESHOLD 4
#define TICK_RATE 5000
#define NUM_MIDI_NOTES 127

#define TICK_OVERFLOW_SAFE

// store the nearest note and error in cents
typedef struct {
	uint8_t note;
	int16_t cents;
} tuning_error_t;



// call this on the rising edge interrupt for the TUNE_IN pin
void tuning_rising_edge(uint32_t tick);

void tuning_falling_edge(uint32_t tick);

void tuning_start_listening();
void tuning_stop_listening();

void tuning_update_error(tuning_error_t* err);

void tuning_update_display(tuning_error_t* err);







#ifdef __cplusplus
}
#endif

#endif /* INC_TUNING_H_ */

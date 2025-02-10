/*
 * tuning.c
 *
 *  Created on: Feb 9, 2025
 *      Author: hayden
 */
#include "tuning.h"
#include <math.h>

// shared vars
static uint32_t tuningBuf[TUNING_BUF_SIZE];
static uint16_t head = 0;
static uint8_t pitchesInitialized = 0;
static float midiNotePitches[NUM_MIDI_NOTES];

void tuning_rising_edge(){
	tuningBuf[head] = *TICK_COUNT;
	head = (head + 1) % TUNING_BUF_SIZE;
}

// helper for converting to ms and handling timer overflow
static float tick_distance_ms(uint32_t first, uint32_t second){
	static const float tickMs = 1000.0f / (float)TICK_RATE;
#ifdef TICK_OVERFLOW_SAFE
	if(first <= second){
		return tickMs * (float)(second - first);
	}
	return tickMs * (float)(second + (4294967295 - first));
#else
	return tickMs * (float)(second - first);
#endif
}

float tuning_get_current_hz(){
	float totalMs = 0.0f;
	uint32_t current, prev;
	for(uint16_t i = 1; i < TUNING_BUF_SIZE; i++){
		prev = tuningBuf[(head + i - 1) % TUNING_BUF_SIZE];
		current = tuningBuf[(head + i) % TUNING_BUF_SIZE];
		totalMs += tick_distance_ms(prev, current);
	}
	return (totalMs / (float)(TUNING_BUF_SIZE - 1)) / 1000.0f;
}

// note/error stuff-------------------
void initMidiPitches(){
	for(uint8_t note = 0; note < NUM_MIDI_NOTES; ++note){
		midiNotePitches[note] = 440.0f * powf(SEMITONE_RATIO, (float)note - 69.0f);
	}
}

// get the note nearest to a given pitch in hz
uint8_t closestNote(float hz){
	if(hz <= midiNotePitches[0])
		return 0;
	if(hz >= midiNotePitches[NUM_MIDI_NOTES - 1])
		return midiNotePitches[NUM_MIDI_NOTES - 1];
	// binary search
	uint8_t i = 0;
	uint8_t j = NUM_MIDI_NOTES;
	uint8_t mid = 0;
	while (i < j) {
		mid = (i + j) / 2;
		// check left half
		if(hz < midiNotePitches[mid]) {
			if(mid > 0 && hz >= midiNotePitches[mid - 1]) {
				if(hz - midiNotePitches[mid - 1] >= midiNotePitches[mid] - hz)
					return mid - 1;
				return mid;
			}
			j = mid;
		} else { // check right half
			if(mid < (NUM_MIDI_NOTES - 1) && hz < midiNotePitches[mid + 1]) {
				if(hz - midiNotePitches[mid] >= midiNotePitches[mid + 1] - hz)
					return mid;
				return mid + 1;
			}
			i = mid + 1;
		}
	}
	return mid;
}

// get the error in cents between the note's pitch and the given frequency
int16_t getErrorHz(uint8_t midiTarget, float hz){
	if(hz <= midiNotePitches[midiTarget]){ // we're flat
		float diff = midiNotePitches[midiTarget] - hz;
		float halfstep = midiNotePitches[midiTarget] - midiNotePitches[midiTarget - 1];
		return (int16_t)((diff / halfstep) * -100.0f);
	} else {
		float diff = hz - midiNotePitches[midiTarget];
		float halfstep = midiNotePitches[midiTarget + 1] - midiNotePitches[midiTarget];
		return (int16_t)((diff / halfstep) * 100.0f);
	}
}

void tuning_update_error(tuning_error_t* err){
	if(!pitchesInitialized){
		initMidiPitches();
		pitchesInitialized = 0xFF;
	}
	float pitch = tuning_get_current_hz();
	err->note = closestNote(pitch);
	err->cents = getErrorHz(err->note, pitch);

}

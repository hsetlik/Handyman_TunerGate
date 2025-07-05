/*
 * tuning.c
 *
 *  Created on: Feb 9, 2025
 *      Author: hayden
 */
#include "tuning.h"


// shared vars
static uint32_t tuningBuf[TUNING_BUF_SIZE];
static uint16_t head = 0;
static uint8_t pitchesInitialized = 0;
static float midiNotePitches[NUM_MIDI_NOTES];

void tuning_rising_edge(uint32_t tick){
	tuningBuf[head] = tick;
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

static float mean_distance_ms(){
	float totalMs = 0.0f;
	float denom = 0.0f;
	uint32_t prev;
	uint32_t current;
	for(uint16_t i = 1; i < TUNING_BUF_SIZE; ++i){
		prev = tuningBuf[(head + i - 1)% TUNING_BUF_SIZE];
		current = tuningBuf[(i + head) % TUNING_BUF_SIZE];
		if(prev < current){
			totalMs += tick_distance_ms(prev, current);
			denom += 1.0f;
		}
	}
	return totalMs / denom;
}

float tuning_get_current_hz(){
	return 1000.0f / mean_distance_ms();
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


void tuning_update_display(tuning_error_t* err){
	static char* noteNames[] = {
			"A",
			"Bb",
			"B",
			"C",
			"Db",
			"D",
			"Eb",
			"E",
			"F",
			"Gb",
			"G"
	};

	const uint8_t x = (128 - 16) / 2;
	const uint8_t y = (64 - 26) / 2;
	// draw the note name
	char* name = noteNames[err->note % 12];


	//display the error
	uint8_t x1, y1, x2, y2;
	if(err->cents < (0 - IN_TUNE_THRESHOLD)) { // flat
		ssd1306_Fill(Black);
		ssd1306_SetCursor(x, y);
		ssd1306_WriteString(name, Font_16x24, White);
		x2 = SSD1306_WIDTH / 2;
		float fDist = (float)(err->cents * -1) / 100.0f;
		x1 = x2 - (uint8_t)(fDist * (float)x2);
		y1 = y + 25;
		y2 = SSD1306_HEIGHT;

		ssd1306_FillRectangle(x1, y1, x2, y2, White);
	}
	else if (err->cents > IN_TUNE_THRESHOLD){ // sharp
		ssd1306_Fill(Black);
		ssd1306_SetCursor(x, y);
		ssd1306_WriteString(name, Font_16x24, White);
		x1 = SSD1306_WIDTH / 2;
		float fDist = (float)(err->cents) / 100.0f;
		x2 = x1 + (uint8_t)(fDist * (float)x1);
		y1 = y + 25;
		y2 = SSD1306_HEIGHT;
		ssd1306_FillRectangle(x1, y1, x2, y2, White);

	} else {
		ssd1306_Fill(White);
		ssd1306_SetCursor(x, y);
		ssd1306_WriteString(name, Font_16x24, Black);
	}
	ssd1306_UpdateScreen();
}

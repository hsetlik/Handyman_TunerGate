/*
 * tuning.c
 *
 *  Created on: Feb 9, 2025
 *      Author: hayden
 */
#include "tuning.h"

// shared vars

// two buffers for rising edges, two for falling
uint32_t risingBuf1[TUNING_BUF_SIZE] = { 0 };
uint32_t risingBuf2[TUNING_BUF_SIZE] = { 0 };
uint32_t fallingBuf1[TUNING_BUF_SIZE] = { 0 };
uint32_t fallingBuf2[TUNING_BUF_SIZE] = { 0 };
static uint32_t *risingWriteBuf = risingBuf1;
#ifdef USE_DOUBLE_BUF
static uint32_t *risingReadBuf = risingBuf2;
#else
static uint32_t *risingReadBuf = risingBuf1;

#endif
static uint16_t risingHead = 0;
static uint32_t *fallingWriteBuf = fallingBuf1;
static uint32_t *fallingReadBuf = fallingBuf2;
static uint16_t fallingHead = 0;
static bool pitchesInitialized = false;
static bool tunerListening = false;
volatile bool readBufferReady = false;
static uint32_t listeningStartIdx = 0;
static float midiNotePitches[NUM_MIDI_NOTES];
static float currentHz = 220.0f;

static float freq_diff_semitones(float freq1, float freq2) {
	return 12.0f * (logf(freq2 / freq1) / logf(2.0f));
}

void tuning_rising_edge(uint32_t tick) {
	risingWriteBuf[risingHead] = tick;
	risingHead = (risingHead + 1) % TUNING_BUF_SIZE;
#ifdef USE_DOUBLE_BUF
	// swap to the read & write buffers
	if (risingHead == 0) {
		uint32_t *prevRead = risingReadBuf;
		risingReadBuf = risingWriteBuf;
		risingWriteBuf = prevRead;
		readBufferReady = true;
	}
#endif
}

void tuning_falling_edge(uint32_t tick) {
	fallingWriteBuf[fallingHead] = tick;
	fallingHead = (fallingHead + 1) % TUNING_BUF_SIZE;
	if (fallingHead == 0) {
		uint32_t *prevRead = fallingReadBuf;
		fallingReadBuf = fallingWriteBuf;
		fallingWriteBuf = prevRead;
	}
}

// helper for converting to ms and handling timer overflow
static float tick_distance_ms(uint32_t first, uint32_t second) {
	static const float tickMs = 1000.0f / (float) TICK_RATE;
#ifdef TICK_OVERFLOW_SAFE
	if (first <= second) {
		return tickMs * (float) (second - first);
	}
	return tickMs * (float) (second + (4294967295 - first));
#else
	return tickMs * (float)(second - first);
#endif
}

void tuning_start_listening() {
	tunerListening = true;
	listeningStartIdx = risingHead;
}
void tuning_stop_listening() {
	tunerListening = false;
}

// returns the avg deviation from the length of the first period of given length starting from a given index
float get_pattern_deviation(uint32_t startIdx, uint32_t length,
		uint32_t *edgeBuf) {
	uint32_t idx1 = startIdx;
	uint32_t idx2 = idx1 + length;
	const float firstPeriod = tick_distance_ms(edgeBuf[idx1], edgeBuf[idx2]);
	float devSum = 0.0f;
	uint16_t numCycles = 0;
	idx1 = idx2;
	idx2 = idx1 + length;
	while (idx2 < TUNING_BUF_SIZE) {
		float period = tick_distance_ms(edgeBuf[idx1], edgeBuf[idx2]);
		float deviation = fabs(firstPeriod - period) / firstPeriod;
		devSum += deviation;
		++numCycles;
		idx1 = idx2;
		idx2 = idx1 + length;
	}
	return devSum / (float) numCycles;
}

// gets the edge pattern with the least deviation with the given startIdx and length range
edge_pattern_t first_valid_pattern(uint32_t maxStartIdx, uint32_t maxLength,
		uint32_t *buf) {
	static const float maxValidDeviation = 0.04;
	for (uint32_t start = 0; start < maxStartIdx; ++start) {
		for (uint32_t length = 1; length <= maxLength; ++length) {
			float dev = get_pattern_deviation(start, length, buf);
			if (dev < maxValidDeviation) {
				return (edge_pattern_t) { start, length };
			}

		}
	}
	return (edge_pattern_t){0, 0};
}

// gives a frequency in hz for a given edge pattern
float hz_for_pattern(edge_pattern_t pattern, uint32_t *edgeBuf) {
	//handle the case of an invalid pattern

	float periodSum = 0.0f;
	uint16_t numCycles = 0;
	uint32_t idx1 = pattern.startIdx;
	uint32_t idx2 = idx1 + pattern.length;
	while (idx2 < TUNING_BUF_SIZE) {
		periodSum += tick_distance_ms(edgeBuf[idx1], edgeBuf[idx2]);
		++numCycles;
		idx1 = idx2;
		idx2 = idx1 + pattern.length;
	}
	float meanPeriod = periodSum / 1000.0f / (float) numCycles;
	return 1.0f / meanPeriod;
}

float tuning_get_current_hz() {
	edge_pattern_t pattern = first_valid_pattern(10, 10, risingReadBuf);
	// only calculate hz if our patterh is valid
	if (pattern.length >= 1) {
		currentHz = hz_for_pattern(pattern, risingReadBuf);
	}
	return currentHz;
}

// note/error stuff-------------------
void initMidiPitches() {
	for (uint8_t note = 0; note < NUM_MIDI_NOTES; ++note) {
		midiNotePitches[note] = 440.0f
				* powf(SEMITONE_RATIO, (float) note - 69.0f);
	}
}

// get the note nearest to a given pitch in hz
uint8_t closestNote(float hz) {
	if (hz <= midiNotePitches[0])
		return 0;
	if (hz >= midiNotePitches[NUM_MIDI_NOTES - 1])
		return midiNotePitches[NUM_MIDI_NOTES - 1];
	// binary search
	uint8_t i = 0;
	uint8_t j = NUM_MIDI_NOTES;
	uint8_t mid = 0;
	while (i < j) {
		mid = (i + j) / 2;
		// check left half
		if (hz < midiNotePitches[mid]) {
			if (mid > 0 && hz >= midiNotePitches[mid - 1]) {
				if (hz - midiNotePitches[mid - 1] < midiNotePitches[mid] - hz)
					return mid - 1;
				return mid;
			}
			j = mid;
		} else { // check right half
			if (mid < (NUM_MIDI_NOTES - 1) && hz < midiNotePitches[mid + 1]) {
				if (hz - midiNotePitches[mid] < midiNotePitches[mid + 1] - hz)
					return mid;
				return mid + 1;
			}
			i = mid + 1;
		}
	}
	return mid;
}

// get the error in cents between the note's pitch and the given frequency
int16_t getErrorHz(uint8_t midiTarget, float hz) {
	float targetHz = midiNotePitches[midiTarget];
	return (int16_t) (freq_diff_semitones(hz, targetHz) * 100.0f);
}

void tuning_update_error(tuning_error_t *err) {
	if (!pitchesInitialized) {
		initMidiPitches();
		pitchesInitialized = true;
	}

	if (readBufferReady) {
		float pitch = tuning_get_current_hz();
		err->note = closestNote(pitch);
		err->cents = getErrorHz(err->note, pitch);
		readBufferReady = false;
	}

}

void tuning_update_display(tuning_error_t *err) {
	static char *noteNames[] = { "A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F",
			"Gb", "G", "Ab" };

	const uint8_t x = (128 - 16) / 2;
	const uint8_t y = (64 - 26) / 2;
	// draw the note name
	char *name = noteNames[err->note % 12];

	//display the error
	uint8_t x1, y1, x2, y2;
	if (err->cents < (0 - IN_TUNE_THRESHOLD)) { // flat
		ssd1306_Fill(Black);
		ssd1306_SetCursor(x, y);
		ssd1306_WriteString(name, Font_16x24, White);
		x2 = SSD1306_WIDTH / 2;
		float fDist = (float) (err->cents * -1) / 50.0f;
		x1 = x2 - (uint8_t) (fDist * (float) x2);
		y1 = y + 25;
		y2 = SSD1306_HEIGHT;

		ssd1306_FillRectangle(x1, y1, x2, y2, White);
	} else if (err->cents > IN_TUNE_THRESHOLD) { // sharp
		ssd1306_Fill(Black);
		ssd1306_SetCursor(x, y);
		ssd1306_WriteString(name, Font_16x24, White);
		x1 = SSD1306_WIDTH / 2;
		float fDist = (float) (err->cents) / 50.0f;
		x2 = x1 + (uint8_t) (fDist * (float) x1);
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

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



/*
Copyright 2015 Jeff Hamm <jeff.hamm@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _AUDIO_STRUCTURES_H
#define _AUDIO_STRUCTURES_H

#include "Arduino.h"
#include "FastLED.h"
#include "sqrt_integer.h"

#undef SHOW_REFRESH_RATE
#undef PRINT_DEBUG
#define FADE_RATE 17
#define BIT_MASK(n) ((1<<n)-1)
#define CHAR_BIT 8

// The number of amplitude bits per audio bucket, max 8.
#define RESOLUTION	8

#define MIN_PEAK_VALUE 2

enum DisplayFunction {
	Lin,
	Log,
	Sq,
	Sqrt
};


typedef struct {
	int16_t startFFTBin;
	int16_t endFFTBin;
	int16_t startLEDNum;
	int16_t endLEDNum;
	DisplayFunction displayFunction;
} DisplayBin;

template<int FREQ_BINS>
struct FFTBinData {
	uint8_t peak;
	uint8_t binValues[FREQ_BINS];
	// Sets the peak value, ensures it is in a valid range.
	void setPeak(int peakValue) {
		peak = min(255, max(MIN_PEAK_VALUE, peakValue));
	}

	void print() {
		Serial.print("Peak: ");
		Serial.print(peak);
		Serial.print("; Values(");
		for(int i = 0; i < FREQ_BINS; i++) {
			Serial.print(binValues[i]);
			Serial.print(", ");
		}
		Serial.println(")");
	}
};

float applyFunction(DisplayFunction f, uint16_t value) {
	switch(f) {
	case DisplayFunction::Log:
		return log(value);
	case DisplayFunction::Sq:
		return pow(value,2);
	case DisplayFunction::Sqrt:
		return sqrt(value);
	case DisplayFunction::Lin:
	default:
		return value;
	}
}

struct DisplayBinState {
	DisplayBin * configuration; 
	CRGB * leds;
	uint8_t num_leds;
	uint8_t * brightness;
	uint8_t value;
	float avgV;
	int avgCount;
	float applyDisplayFunction(uint16_t value) {
		return applyFunction(configuration->displayFunction, value);
	}

};


const float MAX_LOG = applyFunction(DisplayFunction::Log, _BV(RESOLUTION)-1);
const float MAX_SQ = applyFunction(DisplayFunction::Sq, _BV(RESOLUTION)-1);
const float MAX_SQRT = applyFunction(DisplayFunction::Sqrt, _BV(RESOLUTION)-1);
const float MAX_LIN = applyFunction(DisplayFunction::Lin, _BV(RESOLUTION)-1);

float functionMaxValue(DisplayFunction f) {
	switch(f) {
	case DisplayFunction::Log:
		return MAX_LOG;
	case DisplayFunction::Sq:
		return MAX_SQ;
	case DisplayFunction::Sqrt:
		return MAX_SQRT;
	case DisplayFunction::Lin:
	default:
		return MAX_LIN;
	};
}


#endif


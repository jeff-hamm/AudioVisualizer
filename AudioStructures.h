// RFCommon.h

#ifndef _AUDIO_STRUCTURES_H
#define _AUDIO_STRUCTURES_H

#undef SHOW_REFRESH_RATE
#undef PRINT_DEBUG
#define FADE_RATE 17
#define BIT_MASK(n) ((1<<n)-1)
#define CHAR_BIT 8

// The number of amplitude bits per audio bucket, max 8.
#ifndef RESOLUTION
#define RESOLUTION				4
#endif

// The number of frequency buckets we're transmitting
#ifndef FREQ_BINS				
#define FREQ_BINS				8
#endif

#define MIN_PEAK_VALUE 2

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


#endif


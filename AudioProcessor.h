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

#ifndef _AUDIOPROCESSOR_H
#define _AUDIOPROCESSOR_H
#ifndef __MKL26Z64__
#include <Audio.h>
#define FFT_OUTPUT_SIZE 512
#else
#include "LCAnalyzeFFT.h"
#include "LC_ADC.h"
#endif
#include "AudioRenderer.h" 
#include "EEPROM.h"

#define MAX_FFT_HZ ((float)AUDIO_SAMPLE_RATE/2.0)
#define FFT_BIN_SIZE_HZ (MAX_FFT_HZ / (float)FFT_OUTPUT_SIZE)
#define HZ_TO_NEAREST_FFT_BIN(h) (round((float)h/FFT_BIN_SIZE_HZ))

template<int FREQ_BINS = 8, int MAX_VISUALIZERS = 16, int BUILD_NUM = 0x01>
class AudioProcessor
{
public:
#ifndef __MKL26Z64__
	AudioProcessor(AudioAnalyzeFFT1024  & myFFT) : myFFT(myFFT) {
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			visualizer[i] = NULL;
	}
	AudioProcessor(AudioAnalyzeFFT1024  * myFFT, bool uniformScale) : myFFT(myFFT), uniformScale(uniformScale) {
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			visualizer[i] = NULL;
	}
#else
	AudioProcessor(int inputPin, int averaging=8, int resolution=12, uint8_t analogReferenceType = INTERNAL) {
		myFFT.init(inputPin, averaging, resolution, analogReferenceType);
		myFFT.enable();
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			visualizer[i] = NULL;
	}
#endif
	void init(DisplayBin * bins) {
#ifdef __MKL26Z64__
		calibrateADC();
#endif
		enableDebugFFT = false;
		enableDebugAutoscale = false;

		for(int i = 0; i < FREQ_BINS; i++)
			autoScaleValue[i] = INITIAL_AUTOSCALE;
		configureBins(bins);
	}
#ifdef __MKL26Z64__
	void calibrateADC(bool force = false) {
		uint16_t offset;
		if(force || EEPROM.read(0) != BUILD_NUM) {
			delay(5000);
			offset = getDCOffset();
			EEPROM.write(1, offset & 0xFF);
			EEPROM.write(2, offset >> 8);
			EEPROM.write(0, BUILD_NUM);
		}
		else {
			offset = EEPROM.read(2);
			offset <<= 8;
			offset |= EEPROM.read(1);
		}
		setDCOffset(offset);
		Serial.print("DC Offset: ");
		Serial.println(offset);
	}
	
	void enableADC()  {
		myFFT.enable();
	}

	void disableADC() {
		myFFT.disable();
	}
#endif

	void configureBins(DisplayBin * bins) {
		for(int i = 0; i < FREQ_BINS; i++) {
			BIN_SUM_COUNTS[i]= bins[i].endFFTBin - bins[i].startFFTBin;
		}
	}

	bool connectAudioRenderer(AudioRenderer<FREQ_BINS> * visualizer) {
		for(int i = 0; i <MAX_VISUALIZERS; i++)
			if(this->visualizer[i] == NULL) {
				this->visualizer[i] = visualizer;
				return true;
				}
		return false;
	}



	int analyzeData(float scale =-1.0f) {
		if (myFFT.available()) {
			uint16_t sums[FREQ_BINS];
			memset(sums, 0, FREQ_BINS*sizeof(uint16_t));
			int peak = 0;
			int n=0;
			int count=0;
			int avg = 0;
			if(enableDebugFFT)
				Serial.println("FFT:");
			for (int i=0; i<FFT_OUTPUT_SIZE; i++) {
				sums[n] = sums[n] + myFFT.output[i];
				if(enableDebugFFT) {
					Serial.printf("[%2u]",myFFT.output[i]);
				}
				count++;
				if (count >= BIN_SUM_COUNTS[n]) {
					if(enableDebugFFT)
						Serial.printf("\nFilled bin %2u, value: %6u\n", n, sums[n]);
					avg += sums[n]/FREQ_BINS;
					n++;
					if (n >= FREQ_BINS) 
						break;
					count = 0;
				}
			}
			if(enableDebugFFT)
				Serial.println();

			for(int i = 0 ; i<FREQ_BINS; i++) {
				float rScale;
				if(scale < 0.0f)
					rScale = autoScaleValue[i];
				else
					rScale = scale;
				data.binValues[i] = min(MAX_BIN_VALUE,max(0,sums[i]/rScale));
				peak = (max(peak, data.binValues[i]));
				if(enableDebugAutoscale) {
					Serial.printf("Freq Bin %2u: [Value: %3u, Scale: ",i, sums[i]);
					// no printf float support!
					Serial.print(rScale);
					Serial.printf(", Scaled Value: %3u]\n", data.binValues[i]);
				}
			}
			if(enableDebugAutoscale)
				Serial.println();
			data.setPeak(peak);
			if(scale < 0.0f)
				updateAutoScale(data);
			visualize(&data);
			return avg;
		}
		else {
			visualize(NULL);
			return -1;
		}
	}

	void visualize(FFTBinData<FREQ_BINS> * data) {
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			if(visualizer[i] != NULL)
				visualizer[i]->update(data);
	}

	// debug flags
	bool enableDebugFFT;
	bool enableDebugAutoscale;
private:
	const float AUTOSCALE_INCREMENT = 0.1f;
    const int MAX_BIN_VALUE = _BV(RESOLUTION)-1;
	const int HALF_MAX_BIN_VALUE = MAX_BIN_VALUE/4;
	const float INITIAL_AUTOSCALE = 4;
	const float MINIMUM_SCALE = 1;
	
#ifndef __MKL26Z64__
	AudioAnalyzeFFT1024  & myFFT;
#else
	LCAnalyzeFFT  myFFT;
#endif
	FFTBinData<FREQ_BINS> data;
	AudioRenderer<FREQ_BINS> * visualizer[MAX_VISUALIZERS];
	int BIN_SUM_COUNTS[FREQ_BINS];
	float autoScaleValue[FREQ_BINS];
	bool uniformScale;



	// autoscale is pretty dumb -- we're aiming for values in the range 0-_BV(RESOLUTION). 
	// lets assume that the last autoscale value was pretty good, 
	// if most of the values are over halfway, lets increment a bit. If most of them are under halfway, lets decrement a bit.
	// increment faster for clipped values
	void updateAutoScale(FFTBinData<FREQ_BINS> & data) {
		for(int i = 0; i < FREQ_BINS; i++) {
			if(data.binValues[i] < HALF_MAX_BIN_VALUE) {
				if(autoScaleValue[i] >= AUTOSCALE_INCREMENT && autoScaleValue[i] > MINIMUM_SCALE)
					autoScaleValue[i] -= AUTOSCALE_INCREMENT;
				else
					autoScaleValue[i] = MINIMUM_SCALE;
			}
			else if (data.binValues[i] > HALF_MAX_BIN_VALUE) {
				if(data.binValues[i] >= MAX_BIN_VALUE) {
					autoScaleValue[i] += 4*AUTOSCALE_INCREMENT;
				}
				autoScaleValue[i] += AUTOSCALE_INCREMENT;
				
			}
		}
	}
	};

#endif


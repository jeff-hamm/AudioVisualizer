// AudioFFT.h

#ifndef _AUDIOPROCESSOR_H
#define _AUDIOPROCESSOR_H
#ifndef __MKL26Z64__
#include <Audio.h>
#define FFT_OUTPUT_SIZE 512
#else
#include "LCAnalyzeFFT.h"
#endif
#include "AudioVisualizer.h" 

class AudioProcessor
{
	const float AUTOSCALE_INCREMENT = 0.01f;

    const int MAX_BIN_VALUE = _BV(RESOLUTION)-1;
	const int HALF_MAX_BIN_VALUE = MAX_BIN_VALUE/4;
	const float INITIAL_AUTOSCALE = 4;
	const float MINIMUM_SCALE = 1.6;
	const static int MAX_VISUALIZERS = 32;
	
#ifndef __MKL26Z64__
	AudioAnalyzeFFT1024  * myFFT;
#else
	LCAnalyzeFFT * myFFT;
#endif
	AudioVisualizer * visualizer[MAX_VISUALIZERS];
	int BIN_SUM_COUNTS[FREQ_BINS];
	float autoScaleValue[FREQ_BINS];
	bool uniformScale;
public:
#ifndef __MKL26Z64__
	AudioProcessor(AudioAnalyzeFFT1024  * myFFT) {
		this->myFFT = myFFT;
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			visualizer[i] = NULL;
	}
	AudioProcessor(AudioAnalyzeFFT1024  * myFFT, bool uniformScale) : uniformScale(uniformScale){
			this->myFFT = myFFT;
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			visualizer[i] = NULL;
	}
#else
	AudioProcessor(LCAnalyzeFFT * myFFT) {
		this->myFFT = myFFT;
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			visualizer[i] = NULL;
	}
#endif
	void init() {
		delay(1000);

		for(int i = 0; i < FREQ_BINS; i++)
			autoScaleValue[i] = INITIAL_AUTOSCALE;
		
		// cheat a little to put a 1 in the initial frequency bin. (this makes the first two octaves equal to the first two bins rather than the first 3);
		BIN_SUM_COUNTS[0] = 1;
		for(int i = 0; i < (FREQ_BINS-1); i++) {
			BIN_SUM_COUNTS[i+1] = pow(2, i);
		}
		BIN_SUM_COUNTS[FREQ_BINS-1]--;
	}
	
	void setBinCounts(uint8_t * counts) {
		for(int i = 0; i < FREQ_BINS; i++)
			BIN_SUM_COUNTS[i]= counts[i];
	}

	bool connectAudioVisualizer(AudioVisualizer * visualizer) {
		for(int i = 0; i <MAX_VISUALIZERS; i++)
			if(this->visualizer[i] == NULL) {
				this->visualizer[i] = visualizer;
				return true;
				}
		return false;
	}

	int analyzeData(FFTBinData & data, float scale =-1.0f) {
		if (myFFT->available()) {
			uint16_t sums[FREQ_BINS];
			memset(sums, 0, FREQ_BINS*sizeof(uint16_t));
			int peak = 0;
			int n=0;
			int count=0;
			int avg = 0;
			for (int i=0; i<FFT_OUTPUT_SIZE; i++) {
				sums[n] = sums[n] + myFFT->output[i];
				count++;
				if (count >= BIN_SUM_COUNTS[n]) {
					avg += sums[n]/FREQ_BINS;
					n++;
					if (n >= FREQ_BINS) 
						break;
					count = 0;
				}
			}

			for(int i = 0 ; i<FREQ_BINS; i++) {
				float rScale;
				if(scale < 0.0f)
					rScale = autoScaleValue[i];
				else
					rScale = scale;
				Serial.print("(");
				Serial.print(rScale);
				Serial.print(";");
				Serial.print(sums[i]);
				Serial.print(";");
				Serial.print(sums[i]/rScale);
				Serial.print("), ");
				data.binValues[i] = min(MAX_BIN_VALUE,max(0,sums[i]/rScale));
				peak = (max(peak, data.binValues[i]));
			}
			Serial.println();
			data.setPeak(peak);
			if(scale < 0.0f)
				updateAutoScale(data);
			return avg;
		}
		return -1;
	}

	void visualize(FFTBinData data) {
		for(int i = 0; i < MAX_VISUALIZERS; i++)
			if(visualizer[i] != NULL)
				visualizer[i]->update(data);
	}

private:
	// autoscale is pretty dumb -- we're aiming for values in the range 0-_BV(RESOLUTION). 
	// lets assume that the last autoscale value was pretty good, 
	// if most of the values are over halfway, lets increment a bit. If most of them are under halfway, lets decrement a bit.
	// increment faster for clipped values
	void updateAutoScale(FFTBinData & data) {
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


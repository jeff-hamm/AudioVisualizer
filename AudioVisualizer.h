// AudioVisualizer.h

#ifndef _AUDIOVISUALIZER_h
#define _AUDIOVISUALIZER_h

#include "FastLED.h"
#include "AudioStructures.h"
#include "hsv2rgb.h"
struct VisBin {
	CRGB * leds;
	uint8_t num_leds;
	uint8_t value;
	uint8_t fft_start;
	uint8_t fft_end;
	int fadeCount = 0;
	uint8_t * brightness;
};

class AudioVisualizer {
public:
#define SQ(x) (sqrt(x))
	virtual void update(FFTBinData & data) = 0;
};

class LEDStripAudioVisualizer : public AudioVisualizer
{
protected:
	CRGB * leds;
	int numLEDs;
	// the begining hue of the color sweep (default 0, values [0 1))
	float startHue;
	// the final hue of the color sweep (default 1, values (0 1])
	float endHue;
	// How quickly in percentage/100 per update tick we perform the hue sweep
	float hueDelta;
	// How quickly in brightness units per update tick we fade out pixels that should no longer be lit
	int fadeSpeed;
	// whether we are performing a forward color sweep (1) or backwards (0) [Datatype float to avoid excessive casting)
	float hueSign;
	// The current hue in the hue sweep
	float hue;
	// a value from 0-255 indicating how saturated we should be.
	int saturation;
	// the current RGB color of the strip
	CRGB currentColor;
	//
	int fadeCount = 1;

	const float AVG_COUNT = 1024;
	float avgValue[FREQ_BINS];
	VisBin *bins;
	uint8_t numBins;
	VisBin defaultBins[FREQ_BINS];
public:
	LEDStripAudioVisualizer()
	{
		setColorParameters(0.0f, 1.0f, 230);
		setSpeed(16,.00005);
		bins = defaultBins;
		numBins = FREQ_BINS;
		buildDefaultBins();
	}

	// Initializes the visualizer and connects to the LEDs
	void init(CRGB * leds, int numLEDs) {
		this->leds = leds;
		this->numLEDs = numLEDs;
		if(bins == defaultBins) {
			int binSize = numLEDs/FREQ_BINS;
			for(int i = 0; i < numBins; i++) {
				bins[i].num_leds = binSize;
				bins[i].leds = &leds[i*binSize];
				bins[i].brightness = new uint8_t[binSize];
			}
		}
	}

	VisBin * getBins() {
		return bins;
	}

	void setBins(VisBin * bins, uint8_t numBins) {
		this->bins = bins;
		this->numBins = numBins;
	}

	// Sets the speed of the fade transition
	// fadeSpeed: How quickly in brightness units per update tick we fade out pixels that should no longer be lit
	// hueDelta:  How quickly in percentage/100 per update tick we perform the hue sweep
	void setSpeed(int fadeSpeed, float hueDelta) {
		this->fadeSpeed = fadeSpeed;
		this->hueDelta = hueDelta;
		// reset colors due to the huedelta changing.
		setColorParameters(startHue,endHue,(float)saturation/255.0f);
	}


	// Sets the range of colors that we're using for the fade. Defaults to the whole spectrum
	void setColorParameters(float startHue, float endHue, float saturation) 
	{
		if(saturation < 0.0f)
			saturation = 0.0f;
		else if (saturation > 1.0f)
			saturation = 1.0f;
		// convert it back to a byte. We're just exposing the float for consistency
		this->saturation = saturation * 255;
		if(startHue < 0.0f) {
			startHue = 0.0f;
		}
		else if(startHue > 1.0f)
			startHue = 1.0f - hueDelta;
		if(endHue < startHue) {
			endHue = endHue + 1.0f;
		}
		if (endHue > 2.0f)
			endHue = 2.0f;
		this->startHue = startHue;
		this->endHue = endHue;
		hueSign = 1.0f;
		this->hue = startHue;
		currentColor = CHSV(hue*255,this->saturation,255);
	}


	float avgV(int index, int value) {
		avgValue[index] -= (avgValue[index]/AVG_COUNT);
		avgValue[index] += (float)value/AVG_COUNT;
		return avgValue[index];
	}

	// Updates the strip with the spcified frequency data.
	void update(FFTBinData & data) {
		for(int i = 0; i < FREQ_BINS; i++) {
			int v = SQ(data.binValues[i]);
			float a = avgV(i,v);
			v -= a/1.5;
			/*			if(i == 0 ) {
			Serial.print("input: ");
			Serial.println(data.binValues[i]);
			Serial.print("v: ");
			Serial.println(v);
			Serial.print("a: ");
			Serial.println(a);
			}*/
			//			Serial.print("p: ");
			//			Serial.println(abs(v - a));
			int m = SQ((_BV(RESOLUTION)-1))-(a/1.5);
			int fV = max(0,map(v,0,m,0,255));

			/*			if(i == 0) {
			Serial.print("m: ");
			Serial.println(m);
			Serial.print("fv: ");
			Serial.println(fV);
			}
			*/
			drawValue(i, fV);
		}
		updateHue();
	}



	void buildDefaultBins() {
		for(int i = 0; i < FREQ_BINS; i++) {
			bins[i].value = 0;
			bins[i].fft_start=i;
			bins[i].fft_end=i+1;
		}
	}

	void clearValue(int freqBin) {
		for(int i = 0; i < numBins; i++)
			if(freqBin >= bins[i].fft_start && freqBin < bins[i].fft_end) 
				bins[i].value = 0;

	}

#define GET_BRIGHTNESS(rgb)  (max(rgb.r,max(rgb.g, rgb.b)))

	void drawValue(int freqBin, int newValue) {
		for(int i = 0; i < numBins; i++) {
			if(freqBin >= bins[i].fft_start && freqBin < bins[i].fft_end) {
				VisBin * b = &bins[i];
				newValue = map(newValue, 0, 255, 0, b->num_leds);
				newValue = constrain(newValue,0,b->num_leds);
				b->value = newValue;
				b->fadeCount++;
				bool fade = b->fadeCount == fadeCount;
				if(fade)
					b->fadeCount = 0;
				for(int j = 0; j < b->num_leds; j++) {
					// The current values are full bright
					if(j < b->value) {
						b->leds[j] = currentColor;
						b->brightness[j] = 255;
					}
					else 
					{
						if(b->brightness[j] > fadeSpeed) {
							int realSpeed = fadeSpeed;
							if(b->brightness[j] > 200) {
								realSpeed = 2*fadeSpeed;
							}
							b->brightness[j] -= realSpeed;
							b->leds[j] = CHSV(hue*255.0,saturation,b->brightness[j]);
						}
						else {
							b->leds[j] = CRGB::Black;

						}
					}
				}
			}
		}
	}




	void updateHue() {
		hue += hueSign * hueDelta;
		if(hue > endHue && hueSign > 0)  {
			float e = endHue;
			endHue = startHue;
			startHue = e;
			hueSign = -1;
		}
		else if( hue < endHue && hueSign < 0) {
			float e = endHue;
			endHue = startHue;
			startHue = e;
			hueSign = 1;
		}
		float h = hue;
		if(h > 1.0)
			h-= 1.0;
		currentColor = CHSV(h*255.0, saturation,255);
	}
};

#endif


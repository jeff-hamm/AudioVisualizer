// AudioVisualizer.h

#ifndef _AUDIORENDERER_h
#define _AUDIORENDERER_h

#include "FastLED.h"
#include "AudioStructures.h"
#include "hsv2rgb.h"

//#define PRINT_DEBUG
template<int DISPLAY_BINS>
class AudioRenderer {
public:
	virtual void update(FFTBinData<DISPLAY_BINS> * data) = 0;
};

template<int DISPLAY_BINS>
class LEDStripAudioRenderer : public AudioRenderer<DISPLAY_BINS>
{
protected:
	int NUM_LEDS;
	CRGB * leds;
	// the begining hue of the color sweep (default 0, values [0 1))
	float startHue;
	// the final hue of the color sweep (default 1, values (0 1])
	float endHue;
	// The speed of the hue sweep in hue8/millisecond
	float hueDelta;
	// The total amount of time to sweep the hue range
	uint16_t hueSweepTime;
	// How quickly in microseconds between fade ticks
	uint16_t fadeSpeed;
	// How quickly in microseconds between fade ticks
	uint16_t newValFadeSpeed;
	// The time of the last fade tick
	unsigned long lastFade;
	// whether we are performing a forward color sweep (1) or backwards (0) [Datatype float to avoid excessive casting)
	float hueSign;
	// The current hue in the hue sweep
	float hue;
	// a value from 0-255 indicating how saturated we should be.
	int saturation;
	// the current RGB color of the strip
	CRGB currentColor;

	const float AVG_COUNT = 512;
	DisplayBinState binStates[DISPLAY_BINS];


public:
	// enables rendering debug messages
	bool enableDebug;

	LEDStripAudioRenderer() : enableDebug(false)
	{
		setSpeed(2000,10000, 20000);
		setColorSweep(0,255,255);
	}

	// Initializes the visualizer and connects to the LEDs
	void init(CRGB * leds, int numLeds, DisplayBin * bins) {
		NUM_LEDS = numLeds;
		this->leds = leds;
		// configure the display bins to their initial state
		for(int i = 0; i < DISPLAY_BINS; i++) {
			DisplayBinState * bs = &binStates[i];
			bs->configuration = &bins[i];
			bs->num_leds = bins[i].endLEDNum - bins[i].startLEDNum;
			bs->leds = &leds[bins[i].startLEDNum];
			bs->brightness = new uint8_t[bs->num_leds];
			bs->value = 0;
		}
	}

	CRGB * getLEDS() {
		return leds; 
	}

	DisplayBinState * getBinState() {
		return binStates;
	}

	// Sets the speed of the fade transition
	// fadeSpeed: the number of microseconds for a full LED to fade out.
	// hueSweepSpeed: the number of milliseconds for a full sweep from start to end hue
	void setSpeed(int edgeFadeSpeed, int newValFadeSpeed, uint16_t sweepTime) {
		// divide by the render resolution
		this->fadeSpeed = (edgeFadeSpeed)/RESOLUTION;
		this->newValFadeSpeed = (newValFadeSpeed)/RESOLUTION;
		this->lastFade = 0;
		this->hueSweepTime = sweepTime;
		this->hueDelta = abs(this->endHue - this->startHue)/(float)sweepTime;
	}


	// Sets the range of colors that we're using for the fade. Defaults to the whole spectrum
	// The FastLED/pixeltypes.h HSVHue enum has some good reference values
	// Set reverse = true if you want the hue sweep to traverse backwards around the color wheel
	void setColorSweep(uint8_t startHue8, uint8_t endHue8, uint8_t saturation8 = 255, bool reverseWheel = false) 
	{
		this->saturation = saturation8;
		this->startHue = (float)startHue8/255.0f;
		this->endHue = (float)endHue8/255.0f;
		this->hue = startHue;

		// change the endpoint relative to the wheel direction we are heading
		if(reverseWheel) {
			hueSign = -1.0f;
			if(startHue < endHue) {
				endHue -= 1.0f;
			}
		}
		else {
			hueSign = 1.0f;
			if(endHue < startHue) {
				endHue += 1.0;
			}
		}

		currentColor = CHSV(hue*255,this->saturation,255);
		// recalculate the hue delta
		this->hueDelta = abs(this->endHue - this->startHue)/(float)hueSweepTime;

	}


	float avgV(DisplayBinState * bs, float value) {
		if(bs->avgCount < AVG_COUNT)
			bs->avgCount++;

		bs->avgV -= bs->avgV/(float)bs->avgCount;
		bs->avgV += (float)value/(float)bs->avgCount;
		return bs->avgV;
	}

	// Updates the strip with the spcified frequency data.
	void update(FFTBinData<DISPLAY_BINS> * data) {
		unsigned long currentMicros = micros();
		unsigned long microsSinceFade = currentMicros - lastFade;
		int fadeAmount = microsSinceFade / fadeSpeed;
		int newValFadeAmount = microsSinceFade / newValFadeSpeed;
		if(fadeAmount) {
			lastFade = currentMicros;
		}

		for(int i = 0; i < DISPLAY_BINS; i++) {
			DisplayBinState * bs = &binStates[i];
			if(data != NULL) {
				float v = bs->applyDisplayFunction(data->binValues[i]);
				float a = avgV(bs, v)/1.25;
				if(v > a )
					v -= a;
				else v = 0;
				float m = functionMaxValue(bs->configuration->displayFunction) - (a);
				int fV = max(0,map(v,0,m,0,255));			
				fV= map(fV, 0, 255, 0, bs->num_leds);
				fV = constrain(fV,0,bs->num_leds);
				bs->value = fV;
				if(enableDebug && i == 0 ) {
					Serial.printf("Display %u: \n", i);
					Serial.printf("\tinput: %3u",data->binValues[i]);
					Serial.print("\toutput: ");
					Serial.print(bs->applyDisplayFunction(data->binValues[i]));
					Serial.print("\tavg: ");
					Serial.print(a);
					Serial.print("\toffset v: ");
					Serial.print(v);
					Serial.print("\tmax v: ");
					Serial.print(m);
					Serial.printf("\tnum leds: %u\n", fV);
				}
			}
			// just update the bin so it fades
			renderBin(bs, fadeAmount,newValFadeAmount, data != NULL);

		}
		updateHue(microsSinceFade/1000);
	}

	void renderBin(DisplayBinState * b, int fadeAmount, int newValFadeAmount, bool newValue) {
		int vOffset = (b->num_leds - b->value)/2;
		for(int j = 0; j < b->num_leds; j++) {
			// The current values are full bright
			if(j > vOffset && j < (b->value+vOffset) && newValue) {
				b->leds[j] = currentColor;
				b->brightness[j] = 255;
			}
			else if(fadeAmount > 0)
			{
				uint8_t & pixelBrightness = b->brightness[j];
				if(pixelBrightness > 80 ) {
					pixelBrightness -= newValFadeAmount;
				}
				// fade the edges 
				else if(pixelBrightness > fadeAmount) {
					if((j == 0 || j == b->num_leds-1 || (b->brightness[j-1] == 0) || (b->brightness[j+1] == 0)))
						pixelBrightness -= fadeAmount;
				}
				else {
					pixelBrightness = 0;
				}
				b->leds[j] = CHSV(hue*255.0,saturation,pixelBrightness);
			}
		}
	}

	void updateHue(uint32_t millisSinceUpdate) {
		hue += hueSign * (hueDelta * (float)millisSinceUpdate);

		if(hue >= endHue && hueSign > 0)  {
			float e = endHue;
			endHue = startHue;
			startHue = e;
			hueSign = -1;
		}
		else if( hue <= endHue && hueSign < 0) {
			float e = endHue;
			endHue = startHue;
			startHue = e;
			hueSign = 1;
		}

		// valid hue values are between -1.0 and 2.0, 
		// uint8_t oferflow/underflow works in our favor here.
		currentColor = CHSV((uint8_t)(hue*255), saturation,255);
	}

};

#endif


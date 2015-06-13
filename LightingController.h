// LightingController.h

#ifndef _LIGHTINGCONTROLLER_h
#define _LIGHTINGCONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
#include "FastLED.h"
#include "LedFxUtilities.h"
template<uint8_t BOX_COUNT>
	class LightingControllerClass
{
private:
	static const int MAX_FADE = _BV(15);
 protected:
	 CRGB * leds;
	 int numLeds;
	 int boxSize;
	 CRGB destination[BOX_COUNT];
	 uint16_t fadePosition[BOX_COUNT];
	 int16_t fadeSpeed;

	 public:
	void init(CRGB * leds, int numLEDS, uint16_t fadeSpeed) {
		this->leds = leds;
		this->numLeds = numLEDS;
		this->boxSize = numLEDS/BOX_COUNT;
		this->fadeSpeed = fadeSpeed;
		memset(destination, 0, sizeof(CRGB)*BOX_COUNT);
		memset(fadePosition, 0, sizeof(uint16_t)*BOX_COUNT);
	}

	void setColor(CRGB color, bool fade = true) {
		for(int i = 0; i < BOX_COUNT;i++) {
			setColor(i, color, fade);
		}
	}
	CRGB getColor(int box) {
		return leds[box*boxSize];
	}
	void setColor(int box, CRGB color, bool fade = true) {
		destination[box] = color;
		fadePosition[box] = fade ? 0 : MAX_FADE;
	}

	bool isRendered(int box) {
		return getColor(box) == destination[box];

	}

	void render() {
		CRGB boxColor;
		for(int i = 0; i < BOX_COUNT; i++) {
			if(fadePosition[i] >= MAX_FADE) {
				boxColor = destination[i];
			}
			else {
				boxColor = scaleColor(leds[i*boxSize],destination[i], fadePosition[i]);
				fadePosition[i] += (uint16_t)fadeSpeed;
			}
			for(int j = i*boxSize; j < (i*boxSize)+boxSize; j++)
				leds[j] = boxColor;
		}
		LEDS.show();
	}

	CRGB  scaleColor(CRGB  from, CRGB to, uint16_t scale) {
		//  current+((destionation-current)/(steps-step))
		CRGB retval(from);
		uint16_t divisor = (MAX_FADE/fadeSpeed)-(scale/fadeSpeed);
		retval.r += (to.r - retval.r)/divisor;
		retval.g += (to.g - retval.g)/divisor;
		retval.b += (to.b - retval.b)/divisor;
		return retval;
	}
};


#endif


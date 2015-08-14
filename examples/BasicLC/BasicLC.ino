#include "AudioVisualizer.h"
#include "FastLED.h"
#include "ADC.h"
#include "EEPROM.h"
#include "pixeltypes.h"
#include "LCAnalyzeFFT.h"

#define FFT_BINS 1
#define NUM_LEDS 60
#define LED_PIN 17
#define BRIGHTNESS_KNOB_PIN A6
#define INPUT_PIN A2

CRGB leds[NUM_LEDS] = {0};

AudioVisualizer<NUM_LEDS, FFT_BINS> visualizer(INPUT_PIN);
DisplayBin * bins;
	

void setup() {
	Serial.begin(9600);
	
	// Built-in LED as power status
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

	pinMode(BRIGHTNESS_KNOB_PIN, INPUT);

	LEDS.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS);
	LEDS.show();
	configureBins();
	
	visualizer.init(leds, bins);
	visualizer.renderer.setSpeed(2000,11000,10000);
	visualizer.renderer.setColorSweep(HUE_BLUE, HUE_PINK, 240);
	visualizer.enableSerialCommands();
	Serial.println("Setup Complete");
}

int binSizes[] = {5};
int ledSizes[] = {NUM_LEDS};
void configureBins() {
	bins = visualizer.getDefaultBins();
	int binIndex = 0;
	int ledIndex = 0;
	for(int i = 0; i <FFT_BINS; i++ ) {
		bins[i].startFFTBin = binIndex;
		binIndex += binSizes[i];
		bins[i].endFFTBin = binIndex;
		bins[i].displayFunction = DisplayFunction::Sqrt;
		bins[i].startLEDNum = ledIndex;
		ledIndex += ledSizes[i];
		bins[i].endLEDNum = ledIndex;
	}
}

void loop() {
	checkBrightnessKnob();

	if(visualizer.update()) {
		LEDS.show();
	}
}

void checkBrightnessKnob() {
	static unsigned long lastADCCheck = 0;
	unsigned long currentTime = millis();
	if((currentTime - lastADCCheck) > 100) {
		visualizer.processor.disableADC();
		LEDS.setBrightness(map(analogRead(BRIGHTNESS_KNOB_PIN),0,_BV(12),0,256));
		visualizer.processor.enableADC();
		lastADCCheck = currentTime;
	}
}


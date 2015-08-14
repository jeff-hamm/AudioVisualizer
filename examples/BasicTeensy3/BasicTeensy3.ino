#include "AudioVisualizer.h"
#include "FastLED.h"
#include "ADC.h"
#include "EEPROM.h"
#include "pixeltypes.h"
#include "SD.h"
#include "SPI.h"
#include "Wire.h"
#include "Audio.h"

#define FFT_BINS 3
#define NUM_LEDS 168
#define LED_PIN 17
#define BRIGHTNESS_KNOB_PIN A6

CRGB leds[NUM_LEDS] = {0};

// Default connection to A2
AudioInputAnalog  audioInput;         
AudioAnalyzeFFT1024  myFFT;
// Create Audio connections between the components
AudioConnection c2(audioInput, 0, myFFT, 0);

AudioVisualizer<NUM_LEDS, FFT_BINS> visualizer(myFFT);
DisplayBin * bins;
	

void setup() {
	Serial.begin(9600);
	
	// Built-in LED as power status
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

	pinMode(BRIGHTNESS_KNOB_PIN, INPUT);

	// Audio library setup
	AudioMemory(4);

	// FastLED setup
	LEDS.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS);
	LEDS.show();
	configureBins();
	
	visualizer.init(leds, bins);
	visualizer.renderer.setSpeed(2000,11000,10000);
	visualizer.renderer.setColorSweep(HUE_BLUE, HUE_PINK, 240);
	visualizer.enableSerialCommands();
	Serial.println("Setup Complete");
}

int binSizes[] = {3,7,31};
int ledSizes[] = {72,24,72};

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
		LEDS.setBrightness(map(analogRead(BRIGHTNESS_KNOB_PIN),0,_BV(12),0,256));
		lastADCCheck = currentTime;
	}
}


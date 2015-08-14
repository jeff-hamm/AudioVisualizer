#ifndef _AUDIOVISUALIZER_h
#define _AUDIOVISUALIZER_h

#include "AudioStructures.h"
#include "AudioProcessor.h"
#include "AudioRenderer.h"
#include "LightingController.h"
#include "FastLED.h"



template<int NUM_LEDS, int DISPLAY_BINS = 8,  int AUDIO_PIN = A2>
class AudioVisualizer {
public:
	AudioProcessor<DISPLAY_BINS, AUDIO_PIN, 1> processor;
	LEDStripAudioRenderer<DISPLAY_BINS> renderer;
	LightingControllerClass<DISPLAY_BINS> controller;

#ifdef __MKL26Z64__
	AudioVisualizer() : 
		processor(8, 12, EXTERNAL), 
		enableSerialCMD(false) {
	}
#else
	AudioVisualizer(AudioAnalyzeFFT1024  & myFFT) : 
		processor(myFFT), 
		enableSerialCMD(false) {
	}

#endif

	void init(CRGB * leds, DisplayBin * bins = NULL) {
		if(bins == NULL)
			bins = getDefaultBins();
		printBins();
		processor.init(bins);

		renderer.init(leds, NUM_LEDS, bins);
		processor.connectAudioRenderer(&renderer);
		controller.init(leds, NUM_LEDS, _BV(7));
	}

	void enableSerialCommands() {
		enableSerialCMD = true;
	}

	void disableSerialCommands() {
		enableSerialCMD = false;
	}

	void printBins() {
		Serial.println("Display bin Configuration:");
		for(int i = 0; i < DISPLAY_BINS; i++) {
			Serial.printf("Display Bin: %u\n", i);
			Serial.printf("\tApplied to FFT Bins %u-%u\n",bins[i].startFFTBin,bins[i].endFFTBin);
			Serial.printf("\tMapped over LEDS %u-%u\n",bins[i].startLEDNum,bins[i].endLEDNum);
		}
	}

	bool update() {
		if(enableSerialCMD)
			checkSerial();
		return processor.analyzeData() > 0;

	}

	DisplayBin* getDefaultBins() {
		return getOctaveBins();
	}

	DisplayBin * getOctaveBins() {
		float functionBase =  pow(MAX_FFT_HZ, 1.0/(float)DISPLAY_BINS);
		delay(1000);
		Serial.println(functionBase);
		float ledsPerBin = (float)NUM_LEDS/(float)DISPLAY_BINS;
		for(int i = 0; i < DISPLAY_BINS; i++) {
			bins[i].startFFTBin = HZ_TO_NEAREST_FFT_BIN(pow(functionBase, i));
			bins[i].endFFTBin = HZ_TO_NEAREST_FFT_BIN(pow(functionBase,i+1));
			if(bins[i].endFFTBin == bins[i].startFFTBin)
				bins[i].endFFTBin++;
			if(bins[i].startFFTBin > (FFT_OUTPUT_SIZE-1))
				bins[i].startFFTBin = FFT_OUTPUT_SIZE-1;
			if(bins[i].endFFTBin > (FFT_OUTPUT_SIZE-1))
				bins[i].endFFTBin = FFT_OUTPUT_SIZE-1;
			bins[i].startLEDNum = i * ledsPerBin;
			bins[i].endLEDNum = (i+1) * ledsPerBin;
			bins[i].displayFunction = DisplayFunction::Lin;
		}
		return bins;
	}

	DisplayBin * getLinearBins() { 
		float displaySize = FFT_OUTPUT_SIZE/(float)DISPLAY_BINS;
		float ledsPerBin = (float)NUM_LEDS/(float)DISPLAY_BINS;
		for(int i = 0; i < DISPLAY_BINS; i++) {
			bins[i].startFFTBin = i * displaySize;
			bins[i].endFFTBin = (i+1.0) * displaySize;
			bins[i].startLEDNum = i * ledsPerBin;
			bins[i].endLEDNum = (i+1) * ledsPerBin;
		}
		return bins;
	}

private:
	bool enableSerialCMD;

	void disableDebug() {
		processor.enableDebugAutoscale = false;
		processor.enableDebugFFT = false;
		renderer.enableDebug = false;
	}

	void checkSerial() {
		if(Serial.available())
		{
			char c = Serial.read();
			switch(c) {
			case 'd':
				Serial.println("Disabling debug messages.");
				disableDebug();
				break;					
			case 's':
				disableDebug();
				Serial.println("Enabling Scale Debug");
				processor.enableDebugAutoscale = true;
				break;
			case 'f':
				disableDebug();
				Serial.println("Enabling FFT Debug");
				processor.enableDebugFFT = true;
				break;
			case 'r':
				disableDebug();
				Serial.println("Enabling Render Debug");
				renderer.enableDebug = true;
			case 'b':
				printBins();
				break;
#ifdef __MKL26Z64__
			case 'c':
				Serial.println("Calibrating ADC (You should have the input silent)");
				processor.calibrateADC(true);
				break;
#endif
			}
		}
	}
	DisplayBin bins[DISPLAY_BINS];
/*void updatePattern() {
static unsigned long lastPop = 0;
const unsigned long popTime = 2500;
unsigned long currentTime = millis();
for(int i = 0; i < BOXES; i++) {
if(controller.isRendered(i)) {
controller.setColor(i, tr(EMPTY_COLOR), true);
}
}
if((currentTime-lastPop) > popTime) {
int box = random(0, BOXES);
Serial.print("pop: ");
Serial.println(box);
controller.setColor(box, tr(LEDFxUtilities::randomColor(currentMode.saturation, 255)), true);
lastPop = currentTime;
}
controller.render();
}
*/
};
#endif
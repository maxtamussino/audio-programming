/*
 * assignment2_drums
 * ECS7012 Music and Audio Programming
 *
 * Second assignment, to create a sequencer-based
 * drum machine which plays sampled drum sounds in loops.
 *
 * This code runs on the Bela embedded audio platform (bela.io).
 *
 * Andrew McPherson, Becky Ste wart and Victor Zappi
 * 2015-2020
 */


#include <Bela.h>
#include <cmath>
#include "drums.h"

#include "Button.h"
#include "Potentiometer.h"
#include "Led.h"
#include "Accelerometer.h"

#include "Filter.h"


/* Variables which are given to you: */

/* Drum samples are pre-loaded in these buffers. Length of each
 * buffer is given in gDrumSampleBufferLengths.
 */
extern float *gDrumSampleBuffers[NUMBER_OF_DRUMS];
extern int gDrumSampleBufferLengths[NUMBER_OF_DRUMS];

int gIsPlaying = 0;			/* Whether we should play or not. Implement this in Step 4b. */

/* Read pointer into the current drum sample buffer.
 *
 * TODO (step 3): you will replace this with two arrays, one
 * holding each read pointer, the other saying which buffer
 * each read pointer corresponds to.
 */
const unsigned int kNumConcurrentSamples = 16;
int gReadPointers[kNumConcurrentSamples];
int gDrumBufferForReadPointer[kNumConcurrentSamples];

/* Patterns indicate which drum(s) should play on which beat.
 * Each element of gPatterns is an array, whose length is given
 * by gPatternLengths.
 */
extern int *gPatterns[NUMBER_OF_PATTERNS];
extern int gPatternLengths[NUMBER_OF_PATTERNS];

/* These variables indicate which pattern we're playing, and
 * where within the pattern we currently are. Used in Step 4c.
 */
int gCurrentPattern = 0;
int gCurrentIndexInPattern = 0;

/* This variable holds the interval between events in **milliseconds**
 * To use it (Step 4a), you will need to work out how many samples
 * it corresponds to.
 */
int gEventIntervalMilliseconds = 250;
int gEventIntervalCounter = 0;

/* This variable indicates whether samples should be triggered or
 * not. It is used in Step 4b, and should be set in gpio.cpp.
 */
extern int gIsPlaying;

/* This indicates whether we should play the samples backwards.
 */
int gPlaysBackwards = 0;

/* For bonus step only: these variables help implement a fill
 * (temporary pattern) which is triggered by tapping the board.
 */
int gShouldPlayFill = 0;
int gPreviousPattern = 0;

/* TODO: Declare any further global variables you need here */
Button gButton0(0); // Digital 0
Button gButton1(1); // Digital 1
LED gLed(2);        // Digital 2
Potentiometer gPotentiometer(0);       // Analog  0
Accelerometer gAccelerometer(1,2,3,3); // Analog  1 (x)
									   //         2 (y)
									   //         3 (z)
									   // Digital 3 (sleep)


// setup() is called once before the audio rendering starts.
// Use it to perform any initialisation and allocation which is dependent
// on the period size or sample rate.
//
// userData holds an opaque pointer to a data structure that was passed
// in from the call to initAudio().
//
// Return true on success; returning false halts the program.

bool setup(BelaContext *context, void *userData)
{
	// Set up buttons with 50ms debounce interval
	gButton0.setup(context, 50);
	gButton1.setup(context, 50);
	
	// Set up LED, potentiometer and accelerometer
	gLed.setup(context);
	gPotentiometer.setup(context);
	gAccelerometer.setup(context);
	
	// Initialise all slots inactive
	for (int i = 0; i < kNumConcurrentSamples; i++) {
		gDrumBufferForReadPointer[i] = -1;
	}
	
	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.

void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		// Initialise output
		float out = 0;
		
		// Read inputs and react
    	gButton0.process(context, n);
    	gButton1.process(context, n);
    	gLed.process(context, n);
    	gPotentiometer.process(context, n);
    	gAccelerometer.process(context, n);
    	
    	// Start/stop playing when button0 is pressed
    	if (gButton0.pressed_now()) {
    		if (gIsPlaying) {
    			gIsPlaying = 0;
    		} else {
    			gIsPlaying = 1;
    			gEventIntervalCounter = 0;
    		}
    	}
    	
    	// Calibrate accelerometer when button1 is pressed
    	if (gButton1.pressed_now()) gAccelerometer.calibrate();
    	
    	// Determine speed from potentiometer (output mapped to 50-1000ms and converted to samples)
    	int nextEventIntervalSamples = (int)(gPotentiometer.get_value(50, 1000) * context->digitalSampleRate / 1000);
    	
    	// Check for taps on the accelerometer
    	if (gAccelerometer.tap_detected_now() && !gShouldPlayFill){
    		gShouldPlayFill = 1;
    		gPreviousPattern = gCurrentPattern;
    		gCurrentPattern = FILL_PATTERN;
    		gCurrentIndexInPattern = 0;
    	}
    	
    	// Change sample if accelerometer is turned
    	if (gAccelerometer.new_state_now()) {
    		Accelerometer::status_e acc_state = gAccelerometer.get_status();
    		gPlaysBackwards = 0;
    		
    		// Choose pattern depending on orientation
    		switch (acc_state) {
    			case Accelerometer::over:
    				gPlaysBackwards = 1;
    				break;
    			case Accelerometer::flat:
    				gCurrentPattern = 0;
    				break;
    			case Accelerometer::left:
    				gCurrentPattern = 1;
    				break;
    			case Accelerometer::right:
    				gCurrentPattern = 2;
    				break;
    			case Accelerometer::front:
    				gCurrentPattern = 3;
    				break;
    			case Accelerometer::back:
    				gCurrentPattern = 4;
    				break;
    			case Accelerometer::intermediate:
    				// Nothing happens here
    				break;
    		}
    		gCurrentIndexInPattern %= gPatternLengths[gCurrentPattern];
    	}
    	
    	// If currently playing, count towards next event
    	if (gIsPlaying) {
			if (gEventIntervalCounter == 0) {
				startNextEvent();
				gLed.flash(context, n, 2); // Flash LED for 2ms
				gEventIntervalCounter = nextEventIntervalSamples;
			}
			gEventIntervalCounter--;
    	}
		
		// Play active samples
		for (int i = 0; i < kNumConcurrentSamples; i++) {
			// Check if buffer is empty
			if (gDrumBufferForReadPointer[i] == -1) continue;
			
			// Check if end is reached
			bool reached_end;
			if (!gPlaysBackwards) {
				// Forwards end check
				reached_end = gReadPointers[i] >= gDrumSampleBufferLengths[gDrumBufferForReadPointer[i]];
			} else {
				// Backwards end check
				reached_end = gReadPointers[i] < 0;
			}
			if (reached_end) {
				// Deactivate this read pointer
				gDrumBufferForReadPointer[i] = -1;
				continue;
			}
			
			// Get output
			out += gDrumSampleBuffers[gDrumBufferForReadPointer[i]][gReadPointers[i]];
			
			// In/decrease read pointer
			if (gPlaysBackwards) gReadPointers[i]--; else gReadPointers[i]++;
		}
		
		// Rescale output to avoid clipping
		out *= 0.5;
    	
        // Write the output to every audio channel
    	for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
    		audioWrite(context, n, channel, out);
    	}
    }
}

/* Start playing a particular drum sound given by drumIndex. */
void startPlayingDrum(int drumIndex) {
	for (int i = 0; i < kNumConcurrentSamples; i++) {
		if (gDrumBufferForReadPointer[i] == -1) {
			gDrumBufferForReadPointer[i] = drumIndex;
			if (gPlaysBackwards) {
				gReadPointers[i] = gDrumSampleBufferLengths[drumIndex] - 1;
			} else {
				gReadPointers[i] = 0;
			}
			return;
		}
	}
}

/* Start playing the next event in the pattern */
void startNextEvent() {
	// Get event and play its drums
	int event = gPatterns[gCurrentPattern][gCurrentIndexInPattern];
	for (int i = 0; i < NUMBER_OF_DRUMS; i++) {
		if (eventContainsDrum(event, i)) startPlayingDrum(i);
	}
	
	// If accelerometer has entered new state during fill pattern,
	// gShouldPlayFill must be reset
	if (gCurrentPattern != FILL_PATTERN && gShouldPlayFill) {
		gShouldPlayFill = 0;
	}
	
	// Increase index in pattern and reset at the end
	if (gCurrentIndexInPattern == gPatternLengths[gCurrentPattern]) {
		gCurrentIndexInPattern = 0;
		// If it was the fill pattern, reset to previous pattern
		if (gCurrentPattern == FILL_PATTERN) {
			gCurrentPattern = gPreviousPattern;
			gShouldPlayFill = 0;
		}
	}
	gCurrentIndexInPattern++;
}

/* Returns whether the given event contains the given drum sound */
int eventContainsDrum(int event, int drum) {
	if(event & (1 << drum))
		return 1;
	return 0;
}

// cleanup_render() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in initialise_render().

void cleanup(BelaContext *context, void *userData)
{
	
}

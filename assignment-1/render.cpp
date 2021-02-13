// Queen Mary University of London
// ECS7012 - Music and Audio Programming
// Spring 2021
//
// Assignment 1: Synth Filter
// This project contains template code for implementing a resonant filter with
// parameters adjustable in the Bela GUI, and waveform visible in the scope

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>
#include <libraries/Scope/Scope.h>
#include <libraries/math_neon/math_neon.h>
#include <cmath>
#include "Wavetable.h"
#include <iostream>

#define BODE_ACTIVATE true
#define BODE_NUMPOINTS 150
#define BODE_START 100
#define BODE_FACTOR 1.03
#define BODE_FRAMESPERFREQ 200

// Browser-based GUI to adjust parameters
Gui gGui;
GuiController gGuiController;

// Browser-based oscilloscope to visualise signal
Scope gScope;

// Oscillator objects
Wavetable gSineOscillator, gSawtoothOscillator;

// Filter coefficients
float gFilterB0 = 0.0;
float gFilterB1 = 0.0;
float gFilterA1 = 0.0;

// Filter state
float gLastY = 0.0;
float gLastX = 0.0;

// Bode plot generation
float gBodeFrequencies[BODE_NUMPOINTS];
float gBodeGains[BODE_NUMPOINTS];
unsigned int gBodeFreqIndex = 0;
unsigned int gBodeFrameCounter = 0;
bool gBodeActive = BODE_ACTIVATE;


// Calculate filter coefficients given specifications
// frequencyHz -- filter frequency in Hertz (needs to be converted to discrete time frequency)
// resonance -- normalised parameter 0-1 which is related to filter Q
void calculate_coefficients(float sampleRate, float frequencyHz, float resonance)
{
	// Calculate powers of omega_c
	float omega_c = 2 * M_PI * frequencyHz / sampleRate;
	float omega_c2 = omega_c * omega_c;
	float omega_c3 = omega_c2 * omega_c;
	float omega_c4 = omega_c3 * omega_c;
	
	// Polynomial model for g
	float g = 0.9892 * omega_c - 0.4342 * omega_c2 + 0.1381 * omega_c3 - 0.0202 * omega_c4;
	
	// Filter coefficients
	gFilterB0 = g * 1.0 / 1.3;
	gFilterB1 = g * 0.3 / 1.3;
	gFilterA1 = g - 1.0;
}


bool setup(BelaContext *context, void *userData)
{
	std::vector<float> wavetable;
	const unsigned int wavetableSize = 1024;
		
	// Populate a buffer with the first 64 harmonics of a sawtooth wave
	wavetable.resize(wavetableSize);
	
	// Generate a sawtooth wavetable (a ramp from -1 to 1)
	for(unsigned int n = 0; n < wavetableSize; n++) {
		wavetable[n] = -1.0 + 2.0 * (float)n / (float)(wavetableSize - 1);
	}
	
	// Initialise the sawtooth wavetable, passing the sample rate and the buffer
	gSawtoothOscillator.setup(context->audioSampleRate, wavetable);

	// Recalculate the wavetable for a sine
	for(unsigned int n = 0; n < wavetableSize; n++) {
		wavetable[n] = sin(2.0 * M_PI * (float)n / (float)wavetableSize);
	}	
	
	// Initialise the sine oscillator
	gSineOscillator.setup(context->audioSampleRate, wavetable);

	// Set up the GUI
	gGui.setup(context->projectName);
	gGuiController.setup(&gGui, "Oscillator and Filter Controls");	
	
	// Arguments: name, default value, minimum, maximum, increment
	// Create sliders for oscillator and filter settings
	gGuiController.addSlider("Oscillator Frequency", 100, 40, 8000, 0);
	gGuiController.addSlider("Oscillator Amplitude", 0.5, 0, 2.0, 0);
	
	// Set up the scope
	gScope.setup(2, context->audioSampleRate);
	
	// Set up bode frequencies
	gBodeFrequencies[0] = BODE_START;
	gBodeGains[0] = 0;
	for (unsigned int i = 1; i < BODE_NUMPOINTS; i++) {
		gBodeFrequencies[i] = gBodeFrequencies[i - 1] * BODE_FACTOR;
		gBodeGains[i] = 0;
	}

	return true;
}

void render(BelaContext *context, void *userData)
{
	// Read the slider values
	float oscFrequency = gGuiController.getSliderValue(0);
	float oscAmplitude = gGuiController.getSliderValue(1);	
	
	// Bode plot frequency selection
	if (gBodeActive) {
		// Override frequency and amplitude
		oscFrequency = gBodeFrequencies[gBodeFreqIndex];
		oscAmplitude = 0.5;
		
		// Check if maximum frame fer frequency is reached
		gBodeFrameCounter++;
		if (gBodeFrameCounter == BODE_FRAMESPERFREQ) {
			gBodeFrameCounter = 0;
			gBodeFreqIndex++;
			
			// Check if finished
			if (gBodeFreqIndex == BODE_NUMPOINTS) {
				// Stop bode
				gBodeActive = false;
			}
		}
	}

	// Set the oscillator frequency
	gSineOscillator.setFrequency(oscFrequency);
	gSawtoothOscillator.setFrequency(oscFrequency);

	// Calculate new filter coefficients
	calculate_coefficients(context->audioSampleRate, 4000.0, 0.0);
	
    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	// Uncomment one line or the other to choose sine or sawtooth oscillator
    	// (or, if you like, add a GUI or hardware control to switch on the fly)
		float in = oscAmplitude * gSineOscillator.process();
		// float in = oscAmplitude * gSawtoothOscillator.process();
            
        // Apply the filter to the input signal
		float out = gFilterB0 * in + gFilterB1 * gLastX - gFilterA1 * gLastY;
 
        // Save filter state
		gLastX = in;
		gLastY = out;
		
		// Save bode gain
		if (gBodeActive) {
			float gain = out * 2;
			if (gain < 0) {
				gain = - gain;
			}
			if (gain > gBodeGains[gBodeFreqIndex]) {
				gBodeGains[gBodeFreqIndex] = gain;
			}
		}
            
        // Write the output to every audio channel
    	for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
    		audioWrite(context, n, channel, out);
    	}
    	
    	gScope.log(in, out);
    }
}

void cleanup(BelaContext *context, void *userData)
{
	// Print result (for matlab)
	std::cout << "frequencies = [";
	for (unsigned int i = 0; i < BODE_NUMPOINTS; i++) {
		std::cout << gBodeFrequencies[i] << ", ";
	}
	std::cout << "]\ngains = [";
	for (unsigned int i = 0; i < BODE_NUMPOINTS; i++) {
		std::cout << gBodeGains[i] << ", ";
	}
	std::cout << "]\n";
}

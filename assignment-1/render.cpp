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
#include <iostream>

#include "Wavetable.h"
#include "FirstOrderFilterIIR.h"

#define BODE_ACTIVATE true
#define BODE_NUMPOINTS 150
#define BODE_START 100
#define BODE_FACTOR 1.03
#define BODE_RENDERSPERFREQ 200

// Browser-based GUI to adjust parameters
Gui gGui;
GuiController gGuiController;

// Browser-based oscilloscope to visualise signal
Scope gScope;

// Oscillator objects
Wavetable gSineOscillator, gSawtoothOscillator;

// Filters
FirstOrderFilterIIR filters[4];

// Feedback path
float gGres = 0.75;
float gLastOutput = 0.0;

// Bode plot generation
float gBodeFrequencies[BODE_NUMPOINTS];
float gBodeGains[BODE_NUMPOINTS];
unsigned int gBodeFreqIndex = 0;
unsigned int gBodeFrameCounter = 0;
bool gBodeActive = BODE_ACTIVATE;


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
	gGuiController.addSlider("Oscillator Frequency", 500, 40, 8000, 0);
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
		oscAmplitude = 0.1;
		
		// Check if maximum frame fer frequency is reached
		gBodeFrameCounter++;
		if (gBodeFrameCounter == BODE_RENDERSPERFREQ) {
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
	for (unsigned int i = 0; i < 4; i++) {
		filters[i].calculate_coefficients(context->audioSampleRate, 1000.0, 0.0);
	}
	
    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	// Uncomment one line or the other to choose sine or sawtooth oscillator
    	// (or, if you like, add a GUI or hardware control to switch on the fly)
		float in = oscAmplitude * gSineOscillator.process();
		// float in = oscAmplitude * gSawtoothOscillator.process();
        
        // Feedback path
        float out = in - 4 * gGres * (gLastOutput - 0.5 * in);
        
        // Apply nonlinearity
		out = tanhf_neon(out);
		
		// Apply the filters
		for (unsigned int i = 0; i < 4; i++) {
			out = filters[i].process(out);
		}
		
		gLastOutput = out;
		
		// Save bode gain
		if (gBodeActive) {
			// Normalise out to fixed input amplitude (0.5)
			float gain = out * 10;
			
			// Absolute value
			if (gain < 0) {
				gain = - gain;
			}
			
			// Search for amplitude -> save maximum value
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
	// Print bode result (for matlab)
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

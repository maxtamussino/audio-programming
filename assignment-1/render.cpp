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

// Control the creation of data for a bode diagram
#define BODE_ACTIVATE true
#define BODE_NUMPOINTS 150
#define BODE_START 100
#define BODE_FACTOR 1.03
#define BODE_RENDERSPERFREQ 200

// Oscillator selection
#define OSC_SINE 0

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
float gGres = 0.0;
float gLastOutput = 0.0;

// Bode plot generation
bool gBodeActive = BODE_ACTIVATE;
#if BODE_ACTIVATE
float gBodeFrequencies[BODE_NUMPOINTS];
float gBodeGains[BODE_NUMPOINTS];
unsigned int gBodeFreqIndex = 0;
unsigned int gBodeRenderCounter = 0;
#endif


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
	gGuiController.addSlider("Oscillator Frequency", 100, 40, 8000, 1);
	gGuiController.addSlider("Oscillator Amplitude", 0.3, 0, 2.0, 0.1);
	gGuiController.addSlider("Cutoff frequency", 1000, 100, 5000, 1);
	gGuiController.addSlider("Resonance", 0.9, 0, 1, 0.01);
	
	// Set up the scope
	gScope.setup(2, context->audioSampleRate);
	
	// Set up bode frequencies
	#if BODE_ACTIVATE
	gBodeFrequencies[0] = BODE_START;
	gBodeGains[0] = 0;
	for (unsigned int i = 1; i < BODE_NUMPOINTS; i++) {
		gBodeFrequencies[i] = gBodeFrequencies[i - 1] * BODE_FACTOR;
		gBodeGains[i] = 0;
	}
	#endif

	return true;
}

// Calculate filter coefficients given specifications
// frequencyHz -- filter frequency in Hertz (needs to be converted to discrete time frequency)
// resonance -- normalised parameter 0-1 which is related to filter Q
void calculate_coefficients(float sampleRate, float frequencyHz, float resonance) {
	// Calculate powers of omega_c
	float omega_c = 2 * M_PI * frequencyHz / sampleRate;
	float omega_c2 = omega_c * omega_c;
	float omega_c3 = omega_c2 * omega_c;
	float omega_c4 = omega_c3 * omega_c;
	
	// Polynomial model for g
	float g = 0.9892 * omega_c - 0.4342 * omega_c2 + 0.1381 * omega_c3 - 0.0202 * omega_c4;
	
	// Polynomial model for G_res
	gGres = resonance * (1.0029 + 0.0526 * omega_c - 0.0926 * omega_c2 + 0.0218 * omega_c3);
	
	// Filter coefficients
	float coeffB0 = g * 1.0 / 1.3;
	float coeffB1 = g * 0.3 / 1.3;
	float coeffA1 = g - 1.0;
	
	// Set new coefficients for all filters
	for (unsigned int i = 0; i < 4; i++) {
		filters[i].set_coefficients(coeffB0, coeffB1, coeffA1);
	}
}

void render(BelaContext *context, void *userData)
{
	// Read the slider values
	float oscFrequency = gGuiController.getSliderValue(0);
	float oscAmplitude = gGuiController.getSliderValue(1);
	float cutoffFrequency = gGuiController.getSliderValue(2);
	float resonance = gGuiController.getSliderValue(3);
	
	// Bode plot frequency selection
	#if BODE_ACTIVATE
	if (gBodeActive) {
		// Override frequency and amplitude
		// Amplitude low to avoid nonlinearity
		oscFrequency = gBodeFrequencies[gBodeFreqIndex];
		oscAmplitude = 0.1;
		
		// Check if maximum render call per frequency is reached
		gBodeRenderCounter++;
		if (gBodeRenderCounter == BODE_RENDERSPERFREQ) {
			gBodeRenderCounter = 0;
			
			// New frequency
			gBodeFreqIndex++;
			
			// Check if finished
			if (gBodeFreqIndex == BODE_NUMPOINTS) {
				
				// Stop bode
				gBodeActive = false;
			}
		}
	}
	#endif

	// Set the oscillator frequency
	gSineOscillator.setFrequency(oscFrequency);
	gSawtoothOscillator.setFrequency(oscFrequency);

	// Calculate new filter coefficients
	calculate_coefficients(context->audioSampleRate, cutoffFrequency, resonance);
	
    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	
    	// Choose sine or sawtooth oscillator
    	float in = oscAmplitude;
    	if (gBodeActive || OSC_SINE) {
			in *= gSineOscillator.process();
		} else {
			in *= gSawtoothOscillator.process();
		}
        
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
		#if BODE_ACTIVATE
		if (gBodeActive) {
			// Normalise out to fixed input amplitude (0.1)
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
		#endif
            
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
	#if BODE_ACTIVATE
	std::cout << "frequencies = [" << gBodeFrequencies[0];
	for (unsigned int i = 1; i < BODE_NUMPOINTS; i++) {
		std::cout << ", " << gBodeFrequencies[i];
	}
	std::cout << "]\ngains = [" << gBodeGains[0];
	for (unsigned int i = 1; i < BODE_NUMPOINTS; i++) {
		std::cout << ", " << gBodeGains[i];
	}
	std::cout << "];\n";
	#endif
}

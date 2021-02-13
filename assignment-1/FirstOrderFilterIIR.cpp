/***** FirstOrderFilterIIR.cpp *****/

#include "FirstOrderFilterIIR.h"
#include <cmath>

FirstOrderFilterIIR::FirstOrderFilterIIR() {
	lastX_ = 0.0;
	lastY_ = 0.0;
}

// Calculate filter coefficients given specifications
// frequencyHz -- filter frequency in Hertz (needs to be converted to discrete time frequency)
// resonance -- normalised parameter 0-1 which is related to filter Q
void FirstOrderFilterIIR::calculate_coefficients(float sampleRate, float frequencyHz, float resonance) {
	// Calculate powers of omega_c
	float omega_c = 2 * M_PI * frequencyHz / sampleRate;
	float omega_c2 = omega_c * omega_c;
	float omega_c3 = omega_c2 * omega_c;
	float omega_c4 = omega_c3 * omega_c;
	
	// Polynomial model for g
	float g = 0.9892 * omega_c - 0.4342 * omega_c2 + 0.1381 * omega_c3 - 0.0202 * omega_c4;
	
	// Filter coefficients
	coeffB0_ = g * 1.0 / 1.3;
	coeffB1_ = g * 0.3 / 1.3;
	coeffA1_ = g - 1.0;
}

float FirstOrderFilterIIR::process(float input) {
	// Apply the filter to the input signal
	float output = coeffB0_ * input + coeffB1_ * lastX_ - coeffA1_ * lastY_;

    // Save filter state
	lastX_ = input;
	lastY_ = output;
	
	return output;
}
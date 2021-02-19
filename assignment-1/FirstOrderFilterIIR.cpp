/***** FirstOrderFilterIIR.cpp *****/
/* Implementation of a first-order infinite impulse response filter
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 1, Max Tamussino
 */

#include "FirstOrderFilterIIR.h"
#include <cmath>

FirstOrderFilterIIR::FirstOrderFilterIIR() {
	// Zero state at beginning
	lastX_ = 0.0;
	lastY_ = 0.0;
}

void FirstOrderFilterIIR::set_coefficients(float coeffB0, float coeffB1, float coeffA1) {
	// Filter coefficients
	coeffB0_ = coeffB0;
	coeffB1_ = coeffB1;
	coeffA1_ = coeffA1;
}

float FirstOrderFilterIIR::process(float input) {
	// Apply the filter to the input signal
	float output = coeffB0_ * input + coeffB1_ * lastX_ - coeffA1_ * lastY_;

    // Save filter state
	lastX_ = input;
	lastY_ = output;
	
	return output;
}
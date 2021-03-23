/***** Filter.cpp *****/
/* Implementation of a first-order IIR lowpass filter
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 1, Max Tamussino
 */

#include "Filter.h"
#include <cmath>

Filter::Filter(float alpha) {
	// Filter coefficients (LOWPASS)
	coeff_b0 = 1 - alpha;
	coeff_b1 = 0.0;
	coeff_a1 = -alpha;
}

float Filter::process(float input) {
	// Apply the filter to the input signal
	float output = coeff_b0 * input + coeff_b1 * last_input - coeff_a1 * last_output;

    // Save filter state
	last_input = input;
	last_output = output;
	
	return output;
}
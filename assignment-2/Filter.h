/***** Filter.h *****/
/* Class implementation of a first-order IIR lowpass filter
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#pragma once
#include <Bela.h>

class Filter {
public:
	// Constructor
	Filter(float alpha);
	
	// To be called once for each sample
	float process(float input);
	
	// Destructor
	~Filter() {}
	
private:
	// Filter State
	float last_output = 0.0;
	float last_input = 0.0;
	
	// Coefficients
	float coeff_b0, coeff_b1;
	float coeff_a1;
};
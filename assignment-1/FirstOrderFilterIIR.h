/***** FirstOrderFilterIIR.h *****/
/* Implementation of a first-order infinite impulse response filter
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 1, Max Tamussino
 */

#pragma once

class FirstOrderFilterIIR {
public:
	// Constructor
	FirstOrderFilterIIR();
	
	// Sets new filter coefficients
	void set_coefficients(float coeffB0, float coeffB1, float coeffA1);
	
	// To be called once for each sample
	float process(float input);
	
	// Destructor
	~FirstOrderFilterIIR() {}
	
private:
	// Coefficients
	float coeffB0_;
	float coeffB1_;
	float coeffA1_;
	
	// State
	float lastX_;
	float lastY_;
};
/***** potentiometer.cpp *****/
/* Class implementation of a potentiometer
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#include "potentiometer.h"
#include <Bela.h>
#include <cmath>

// Standard constructor
Potentiometer::Potentiometer(int analog_pin) {
	// Set pin
	pin = analog_pin;
}

// To be called during setup
void Potentiometer::setup(BelaContext *context, float input_max_) {
	// Check analog and audio frames
	audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	
	// Save maximum input for output mapping
	input_max = input_max_;
	
	// Finish
	setup_done = true;
}

// To be called once per frame
void Potentiometer::process(BelaContext *context, int frame) {
	if (!setup_done) return;
	
	if (frame % audioFramesPerAnalogFrame == 0) {
		value = analogRead(context, frame / audioFramesPerAnalogFrame, pin);
	}
}

float Potentiometer::get_value(float lower, float upper) {
	return map(value, 0, input_max, lower, upper);
}
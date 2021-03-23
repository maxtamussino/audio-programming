/***** Potentiometer.cpp *****/
/* Class implementation of a potentiometer
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#include "Potentiometer.h"
#include <Bela.h>
#include <cmath>

// Standard constructor
Potentiometer::Potentiometer(int analog_pin) {
	// Set pin
	pin = analog_pin;
}

// To be called during setup
void Potentiometer::setup(BelaContext *context) {
	// Check analog and audio frames
	audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	
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
	// Maximum possible input is 3.3V, maps 0-3.3V between lower and upper
	return map(value, 0, 3.3/4.096, lower, upper);
}
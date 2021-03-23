/***** Potentiometer.h *****/
/* Class implementation of a potentiometer
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#pragma once
#include <Bela.h>

class Potentiometer {
public:
	// Constructor
	Potentiometer(int analog_pin);
	
	// Setup
	void setup(BelaContext *context);
	
	// To be called once for each frame
	void process(BelaContext *context, int frame);
	
	// Get value
	float get_value(float lower, float upper);
	
	// Destructor
	~Potentiometer() {}
	
private:
	// Info
	int pin;
	bool setup_done = false;
	float value;
	
	// Frame handling
	int audioFramesPerAnalogFrame;
};

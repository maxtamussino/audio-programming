/***** accelerometer.h *****/
/* Class implementation of an accelerometer
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#pragma once
#include <Bela.h>

class Accelerometer {
public:
	// Constructor
	Accelerometer(int analog_in_pin_x, int analog_in_pin_y, int analog_in_pin_z, int digital_pin_sleep);
	
	// Setup
	void setup(BelaContext *context, float exit_thresh, float enter_thresh);
	
	// To be called once for each frame
	void process(BelaContext *context, int frame);
	
	// Calibration
	void calibrate(BelaContext *context, int frame);
	
	// Status enum
	enum status_e { flat, over, up, down, left, right, intermediate };
	
	// Destructor
	~Accelerometer() {}
	
private:
	// Info
	int pins[3], pin_sleep;
	bool setup_done = false;
	
	// Voltage to g mapping
	float input_0g, input_1g;
	
	// State
	status_e state;
	float input[3];
	float accelerations[3];
	
	// Thresholding
	float exit_threshold, enter_threshold;
	
	// Analog frame handling
	int audioFramesPerAnalogFrame;
	
	// Utility
	void set_new_state(Accelerometer::status_e new_state);
	void read_accelerations(BelaContext *context, int frame);
};
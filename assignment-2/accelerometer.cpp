/***** accelerometer.cpp *****/
/* Class implementation of an accelerometer
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#include "accelerometer.h"
#include <Bela.h>
#include <cmath>

// Standard constructor
Accelerometer::Accelerometer(int analog_in_pin_x, int analog_in_pin_y, int analog_in_pin_z, int digital_pin_sleep) {
	// Set pins
	pins[0] = analog_in_pin_x;
	pins[1] = analog_in_pin_y;
	pins[2] = analog_in_pin_z;
	pin_sleep = digital_pin_sleep;
}

// To be called during setup
void Accelerometer::setup(BelaContext *context, float exit_thresh, float enter_thresh) {
	// Configure sleep pin
	pinMode(context, 0, pin_sleep, OUTPUT);
	digitalWrite(context, 0, pin_sleep, HIGH);
	
	// Save parameters
	exit_threshold = exit_thresh;
	enter_threshold = enter_thresh;
	
	input_0g = 0.4;
	input_1g = 0.54;
	
	// Check analog and audio frames
	audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	
	// Initialise state
	state = intermediate;
	
	// Finish
	setup_done = true;
}

// To be called once per frame
void Accelerometer::process(BelaContext *context, int frame) {
	if (!setup_done) return;
	
	read_accelerations(context, frame);
	
	//TODO FILTER
	
	float total_acceleration = sqrt(pow(accelerations[0],2) + pow(accelerations[1],2) + pow(accelerations[2],2));
	if (total_acceleration > 1.1) {
		return;
	}
	//TODO ignore shaking
	
	switch (state) {
		case flat:
			if (accelerations[2] < exit_threshold) set_new_state(intermediate);
			break;
		case over:
			if (accelerations[2] > -exit_threshold) set_new_state(intermediate);
			break;
		case up:
			if (accelerations[0] > -exit_threshold) set_new_state(intermediate);
			break;
		case down:
			if (accelerations[0] < exit_threshold) set_new_state(intermediate);
			break;
		case left:
			if (accelerations[1] < exit_threshold) set_new_state(intermediate);
			break;
		case right:
			if (accelerations[1] > -exit_threshold) set_new_state(intermediate);
			break;
		case intermediate:
			if (accelerations[2] > enter_threshold) {
				set_new_state(flat);
			} else if (accelerations[2] < -enter_threshold) {
				set_new_state(over);
			} else if (accelerations[0] > enter_threshold) {
				set_new_state(down);
			} else if (accelerations[0] < -enter_threshold) {
				set_new_state(up);
			} else if (accelerations[1] > enter_threshold) {
				set_new_state(left);
			} else if (accelerations[1] < -enter_threshold) {
				set_new_state(right);
			}
			break;
	}
}

void Accelerometer::set_new_state(Accelerometer::status_e new_state) {
	state = new_state;
	rt_printf("[%f, %f, %f] New state: ", accelerations[0], accelerations[1], accelerations[2]);
	switch (new_state) {
		case flat:
			rt_printf("FLAT");
			break;
		case over:
			rt_printf("OVER");
			break;
		case up:
			rt_printf("UP");
			break;
		case down:
			rt_printf("DOWN");
			break;
		case left:
			rt_printf("LEFT");
			break;
		case right:
			rt_printf("RIGHT");
			break;
		case intermediate:
			rt_printf("INTERMEDIATE");
			break;
	}
	rt_printf("\n");
}

void Accelerometer::read_accelerations(BelaContext *context, int frame) {
	if (frame % audioFramesPerAnalogFrame == 0) {
		for (int i = 0; i < 3; i++) {
			input[i] = analogRead(context, frame / audioFramesPerAnalogFrame, pins[i]);
			accelerations[i] = (input[i] - input_0g) / (input_1g - input_0g);
		}
	}
}

void Accelerometer::calibrate(BelaContext *context, int frame) {
	input_0g = (input[0] + input[1]) / 2;
	input_1g = input[2];
	rt_printf("Assuming flat: [x=%f, y=%f, z=%f] --> 0g at %f, 1g at %f\n", input[0], input[1], input[2], input_0g, input_1g);
}

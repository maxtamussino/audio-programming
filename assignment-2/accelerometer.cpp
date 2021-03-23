/***** Accelerometer.cpp *****/
/* Class implementation of an accelerometer
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#include "Accelerometer.h"
#include <Bela.h>
#include <cmath>
#include <libraries/Scope/Scope.h>

#include "Filter.h"

// Standard constructor
Accelerometer::Accelerometer(int analog_in_pin_x, int analog_in_pin_y, int analog_in_pin_z, int digital_pin_sleep) {
	// Set pins
	pins[0] = analog_in_pin_x;
	pins[1] = analog_in_pin_y;
	pins[2] = analog_in_pin_z;
	pin_sleep = digital_pin_sleep;
	
	// Create filters
	for (int i = 0; i < 3; i++) {
		lowpass_firststage.push_back(Filter(0.995));
		lowpass_secondstage.push_back(Filter(0.995));
	}
}

// To be called during setup
void Accelerometer::setup(BelaContext *context) {
	// Configure sleep pin
	pinMode(context, 0, pin_sleep, OUTPUT);
	digitalWrite(context, 0, pin_sleep, HIGH);
	
	// Accelerometer values according to datasheet (for 1.5g sensitivity)
	for (int i = 0; i < 3; i++) {
		input_0g[i] = 1.65;
		input_1g[i] = input_0g[i] + 0.8;
	}
	
	// Check analog and audio frames
	audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	
	// Initialise state
	state = intermediate;
	
	// Debug scope
	scope.setup(3, context->audioSampleRate);
	
	// Finish
	setup_done = true;
}

// To be called once per frame
void Accelerometer::process(BelaContext *context, int frame) {
	if (!setup_done) return;
	state_new = false;
	
	if (frame % audioFramesPerAnalogFrame == 0) {
		for (int i = 0; i < 3; i++) {
			// Scale input to get value in volt
			input[i] = 4.096 * analogRead(context, frame / audioFramesPerAnalogFrame, pins[i]);
			
			// Rescale according to calibration to get value in g
			accelerations_raw[i] = (input[i] - input_0g[i]) / (input_1g[i] - input_0g[i]);
			
			// Filter the input
			accelerations_smooth[i] = lowpass_firststage[i].process(accelerations_raw[i]);
			
			// Has to be downsampled to realise filters with low cutoff frequencies
			if (downsample_counter == 0) {
				accelerations_filtered[i] = lowpass_secondstage[i].process(accelerations_smooth[i]);
			}
		}
		
		// Only new state possible if new LP output available
		if (downsample_counter == 0) {
			// Distinguish between constant 1g gravity and shock acceleration, which can be >1g
			float total_acceleration = sqrt(pow(accelerations_filtered[0],2) + pow(accelerations_filtered[1],2) + pow(accelerations_filtered[2],2));
			
			tap_detected = false;
			if (total_acceleration < 1.05) {
				calculate_new_state();
			} else if (total_acceleration > 1.1) {
				tap_detected = true;
				//rt_printf("Tap detected: %f\n", total_acceleration);
			}
			downsample_counter = downsample_rate;
		}
		
		downsample_counter--;
		
		//scope.log(accelerations_raw[2], accelerations_smooth[2], accelerations_filtered[2]);
		scope.log(accelerations_filtered[0], accelerations_filtered[1], accelerations_filtered[2]);
	}
}

// Calculates new state
void Accelerometer::calculate_new_state() {
	switch (state) {
		case flat:
			if (accelerations_filtered[2] < exit_threshold) set_new_state(intermediate);
			break;
		case over:
			if (accelerations_filtered[2] > -exit_threshold) set_new_state(intermediate);
			break;
		case left:
			if (accelerations_filtered[0] > -exit_threshold) set_new_state(intermediate);
			break;
		case right:
			if (accelerations_filtered[0] < exit_threshold) set_new_state(intermediate);
			break;
		case front:
			if (accelerations_filtered[1] > -exit_threshold) set_new_state(intermediate);
			break;
		case back:
			if (accelerations_filtered[1] < exit_threshold) set_new_state(intermediate);
			break;
		case intermediate:
			if (accelerations_filtered[2] > enter_threshold) {
				set_new_state(flat);
			} else if (accelerations_filtered[2] < -enter_threshold) {
				set_new_state(over);
			} else if (accelerations_filtered[0] < -enter_threshold) {
				set_new_state(left);
			} else if (accelerations_filtered[0] > enter_threshold) {
				set_new_state(right);
			} else if (accelerations_filtered[1] < -enter_threshold) {
				set_new_state(front);
			} else if (accelerations_filtered[1] > enter_threshold) {
				set_new_state(back);
			}
			break;
	}
}

// Sets new state
void Accelerometer::set_new_state(Accelerometer::status_e new_state) {
	state = new_state;
	state_new = true;
	
	/*
	rt_printf("New state: ");
	switch (new_state) {
		case flat:
			rt_printf("FLAT");
			break;
		case over:
			rt_printf("OVER");
			break;
		case front:
			rt_printf("FRONT");
			break;
		case back:
			rt_printf("BACK");
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
	*/
}

// Accelerometer must be brought in a position where one of the
// axis has its MAXIMUM value (NOT MINIMUM!) before calling
// this function (voltage for 1g is measured and saved)
void Accelerometer::calibrate() {
	// Determine axis to calibrate
	int axis = 0;
	float highest_input = 0.0;
	for (int i = 0; i < 3; i++) {
		if (input[i] > highest_input) {
			axis = i;
			highest_input = input[i];
		}
	}
	
	// Determine other axis
	int other_axis_1 = (axis + 1) % 3;
	int other_axis_2 = (axis + 2) % 3;
	
	// Calibrate
	input_0g[other_axis_1] = input[other_axis_1];
	input_0g[other_axis_2] = input[other_axis_2];
	input_1g[axis] = input[axis];
	/*
	rt_printf("Calibrated axis %d: [x=%f, y=%f, z=%f]", axis, input[0], input[1], input[2]);
	rt_printf("--> 1g of %d at %f V\n", axis, input_1g[axis]);
	rt_printf("--> 0g of %d at %f V\n", other_axis_1, input_0g[other_axis_1]);
	rt_printf("--> 0g of %d at %f V\n", other_axis_2, input_0g[other_axis_2]);
	*/
}
